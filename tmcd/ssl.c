/*
 * SSL support for tmcd/tmcc. 
 *
 * Questions:
 *     1. Should the CA have an encrypted key? If we do not distribute the
 *        key with the the cert (to the remote nodes), does it really matter?
 *        Only the cert is needed to operate.
 *     2. Should the client/server keys be encrypted? Doing so requires a
 *        password to use them, and that would obviously be a pain.
 *     3. Whats a challenge password? Do we need to worry about those?
 *     4. How do we verify a certificate? 
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "decls.h"
#include "ssl.h"
#include "log.h"
#include "config.h"

/* Passed in from the makefile */
#ifndef ETCDIR
#define ETCDIR			"/usr/testbed/etc"
#endif
#define EMULAB_CERTFILE		"emulab.pem"
#define SERVER_CERTFILE		"server.pem"
#define CLIENT_CERTFILE		"client.pem"

/*
 * On the client, we search a couple of dirs for the pem file.
 */
static char	*clientcertdirs[] = {
	"/etc/testbed",
	"/etc/rc.d/testbed",
	"/usr/local/etc/testbed",
	ETCDIR,
	0
};

/* Keep all this stuff private. */
static SSL		*ssl;
static SSL_CTX		*ctx;
static int		client = 0;
static void		tmcd_sslerror();
static void		tmcd_sslprint(const char *fmt, ...);

/*
 * Init our SSL context. This is done once, in the parent.
 */
int
tmcd_server_sslinit(void)
{
	char	buf[BUFSIZ];
	
	client = 0;
	SSL_library_init();
	SSL_load_error_strings();
	
	if (! (ctx = SSL_CTX_new(SSLv23_method()))) {
		tmcd_sslerror();
		return 1;
	}

	sprintf(buf, "%s/%s", ETCDIR, SERVER_CERTFILE);
	
	/*
	 * Load our server key and certificate and then check it.
	 */
	if (! SSL_CTX_use_certificate_file(ctx, buf, SSL_FILETYPE_PEM)) {
		tmcd_sslerror();
		return 1;
	}
	
	if (! SSL_CTX_use_PrivateKey_file(ctx, buf, SSL_FILETYPE_PEM)) {
		tmcd_sslerror();
		return 1;
	}
	
	if (SSL_CTX_check_private_key(ctx) != 1) {
		tmcd_sslerror();
		return 1;
	}

	/*
	 * Load our CA so that we can request client authentication.
	 * The CA list is sent to the client.
	 */
	sprintf(buf, "%s/%s", ETCDIR, EMULAB_CERTFILE);
	
	if (! SSL_CTX_load_verify_locations(ctx, buf, NULL)) {
		tmcd_sslerror();
		return 1;
	}

	/*
	 * Make it so the client must provide authentication.
	 */
	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER |
			   SSL_VERIFY_FAIL_IF_NO_PEER_CERT, 0);

	return 0;
}

int
tmcd_client_sslinit(void)
{
	char	buf[BUFSIZ], **cp;
	
	client = 1;
	SSL_library_init();
	SSL_load_error_strings();
	
	if (! (ctx = SSL_CTX_new(SSLv23_method()))) {
		tmcd_sslerror();
		return 1;
	}

	/*
	 * Search for the certificate.
	 */
	cp = clientcertdirs;
	while (*cp) {
		sprintf(buf, "%s/%s", *cp, CLIENT_CERTFILE);

		if (access(buf, R_OK) == 0)
			break;
		cp++;
	}
	if (! *cp) {
		error("Could not find a client certificate!\n");
		return 1;
	}

	/*
	 * Load our client certificate.
	 */
	if (! SSL_CTX_use_certificate_file(ctx, buf, SSL_FILETYPE_PEM)) {
		tmcd_sslerror();
		return 1;
	}
	
	if (! SSL_CTX_use_PrivateKey_file(ctx, buf, SSL_FILETYPE_PEM)) {
		tmcd_sslerror();
		return 1;
	}
	
	if (SSL_CTX_check_private_key(ctx) != 1) {
		tmcd_sslerror();
		return 1;
	}

	/*
	 * Load our CA so that we can do authentication.
	 */
	sprintf(buf, "%s/%s", *cp, EMULAB_CERTFILE);
	
	if (! SSL_CTX_load_verify_locations(ctx, buf, NULL)) {
		tmcd_sslerror();
		return 1;
	}

	/*
	 * Make it so the server must provide authentication.
	 */
	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER |
			   SSL_VERIFY_FAIL_IF_NO_PEER_CERT, 0);

	return 0;
}

/*
 * Handshake a new server connection. This creates a new SSL, which
 * is static and local to the process. 
 */
int
tmcd_sslaccept(int sock, struct sockaddr *addr, socklen_t *addrlen)
{
	int	newsock;

	if ((newsock = accept(sock, addr, addrlen)) < 0)
		return -1;

	if (! (ssl = SSL_new(ctx))) {
		tmcd_sslerror();
		return -1;
	}
	if (! SSL_set_fd(ssl, newsock)) {
		tmcd_sslerror();
		return -1;
	}
	if (SSL_accept(ssl) <= 0) {
		tmcd_sslerror();
		return -1;
	}
	tmcd_sslverify(newsock, 0);
	
	return newsock;
}

/*
 * Handshake a new client connection. This creates a new SSL, which
 * is static and local to the process. 
 */
int
tmcd_sslconnect(int sock, const struct sockaddr *name, socklen_t namelen)
{
	if (connect(sock, name, namelen) < 0)
		return -1;
	
	if (! (ssl = SSL_new(ctx))) {
		tmcd_sslerror();
		return -1;
	}
	if (! SSL_set_fd(ssl, sock)) {
		tmcd_sslerror();
		return -1;
	}
	if (SSL_connect(ssl) <= 0) {
		tmcd_sslerror();
		return -1;
	}
	tmcd_sslverify(sock, 0);
	
	return 0;
}

/*
 * Verify the certificate of the peer.
 */
int
tmcd_sslverify(int sock, char *host)
{
	X509	*peer;
	char	*cp, buf[256];

	if (SSL_get_verify_result(ssl) != X509_V_OK) {
		tmcd_sslprint("Certificate did not verify!\n");
		return 1;
	}
	
	if (! (peer = SSL_get_peer_certificate(ssl))) {
		tmcd_sslprint("No certificate was presented by the peer!\n");
		return 1;
	}

	if ((cp = X509_NAME_oneline(X509_get_subject_name(peer), 0, 0))) {
		printf("Peer subject: %s\n", cp);
		free(cp);
	}

	if ((cp = X509_NAME_oneline(X509_get_issuer_name(peer), 0, 0))) {
		printf("Peer issuer: %s\n", cp);
		free(cp);
	}

	return 0;
}

/*
 * Write stuff out. According to docs, the write call will not
 * return until the entire amount is written. Not that it matters; the
 * caller is going to check and rewrite if short.
 */
int
tmcd_sslwrite(int sock, const void *buf, size_t nbytes)
{
	int	cc;

	errno = 0;
	if ((cc = SSL_write(ssl, buf, nbytes)) <= 0) {
		if (cc < 0) {
			tmcd_sslerror();
		}
		return cc;
	}
	return cc;
}

/*
 * Read stuff in.
 */
int
tmcd_sslread(int sock, void *buf, size_t nbytes)
{
	int	cc;

	errno = 0;
	if ((cc = SSL_read(ssl, buf, nbytes)) <= 0) {
		if (cc < 0) {
			tmcd_sslerror();
		}
		return cc;
	}
	return cc;
}

/*
 * Terminate the SSL part of a connection. Also close the sock.
 */
int
tmcd_sslclose(int sock)
{
	if (ssl) {
		SSL_shutdown(ssl);
		SSL_free(ssl);
		ssl = NULL;
	}
	close(sock);
	return 0;
}

/*
 * Log an SSL error
 */
static void
tmcd_sslerror()
{
	char	buf[BUFSIZ];
	
	ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));

	if (client)
		printf("SSL Error: %s\n", buf);
	else
		error("SSL Error: %s\n", buf);
}

/*
 * Temp. tmcd needs to switch to log interface.
 */
static void
tmcd_sslprint(const char *fmt, ...)
{
	char	buf[BUFSIZ];
	va_list args;

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	if (client) {
		fputs(buf, stderr);
		fputs("\n", stderr);
	}
	else
		error("%s\n", buf);
}
