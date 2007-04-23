/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2004, 2006, 2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "decls.h"
#include "ssl.h"
#ifndef STANDALONE
#include "log.h"
#include "config.h"
#else
#define error		printf
#endif

/* Passed in from the makefile */
#ifndef ETCDIR
#define ETCDIR			"/usr/testbed/etc"
#endif
#define EMULAB_CERTFILE		"emulab.pem"
#define SERVER_CERTFILE		"server.pem"
#define CLIENT_CERTFILE		"client.pem"

#ifdef linux
#define EAUTH	EPERM
#endif

/*
 * This is used by tmcd to determine if the connection is ssl or not.
 */
int	isssl;

/*
 * Client side; optional use of SSL.
 */
int	nousessl;

/*
 * On the client, we search a couple of dirs for the pem file.
 */
static char	*clientcertdirs[] = {
	"/etc/testbed",
	"/etc/emulab",
	"/etc/rc.d/testbed",
	"/usr/local/etc/testbed",
	"/usr/local/etc/emulab",
	ETCDIR,
	0
};

/* Keep all this stuff private. */
static SSL		*ssl;
static SSL_CTX		*ctx;
static int		client = 0;
static char		nosslbuf[MAXTMCDPACKET];
static int		nosslbuflen, nosslbufidx;
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

	/*
	 * No session caching! Useless and eats up memory.
	 */
	SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_OFF);

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

	/*
	 * No session caching! Useless and eats up memory.
	 */
	SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_OFF);

	return 0;
}

/*
 * Handshake a new server connection. This creates a new SSL, which
 * is static and local to the process. 
 */
int
tmcd_sslaccept(int sock, struct sockaddr *addr, socklen_t *addrlen, int ms)
{
	int	newsock, cc, err;

	if ((newsock = accept(sock, addr, addrlen)) < 0)
		return -1;

	/*
	 * Client side; optional use of SSL.
	 */
	if (nousessl)
		return newsock;

	/*
	 * Set timeout values to keep us from hanging due to a
	 * malfunctioning or malicious client.
	 */
	if (ms > 0) {
		struct timeval tv;

		tv.tv_sec = ms / 1000;
		tv.tv_usec = (ms % 1000) * 1000;
		if (setsockopt(newsock, SOL_SOCKET, SO_RCVTIMEO,
			       &tv, sizeof(tv)) < 0) {
			errorc("setting SO_RCVTIMEO");
		}
	}

	/*
	 * Read the first bit. It indicates whether we need to SSL
	 * handshake or not. Clear the buffer to avoid confusing
	 * the last connection with this new connection. 
	 */
	bzero(nosslbuf, strlen(SPEAKSSL));
	      
	if ((cc = read(newsock, nosslbuf, strlen(SPEAKSSL))) <= 0) {
		if (cc < 0) {
			err = errno;
			if (err == EWOULDBLOCK)
				error("sslaccept: timeout reading request");
			else
				error("sslaccept: error reading request");
		} else
			err = EIO;
		goto badauth;
	}
	
	if (strncmp(nosslbuf, SPEAKSSL, cc)) {
		/*
		 * No ssl. Need to return this data on the next read.
		 * See below. 
		 */
		isssl = 0;
		nosslbuflen = cc;
		nosslbufidx = 0;
		return newsock;
	}
	isssl = 1;
	nosslbuflen = 0;

	if (! (ssl = SSL_new(ctx))) {
		tmcd_sslerror();
		err = EIO;
		goto badauth;
	}
	if (! SSL_set_fd(ssl, newsock)) {
		tmcd_sslerror();
		err = EIO;
		goto badauth;
	}
	if (SSL_accept(ssl) <= 0) {
		tmcd_sslerror();
		err = EAUTH;
		goto badauth;
	}
	
	return newsock;
 badauth:
	error("sslaccept: error speaking to %s\n",
	      inet_ntoa(((struct sockaddr_in *)addr)->sin_addr));
	tmcd_sslclose(newsock);
	errno = err;
	return -1;
}

/*
 * Handshake a new client connection. This creates a new SSL, which
 * is static and local to the process. 
 */
int
tmcd_sslconnect(int sock, const struct sockaddr *name, socklen_t namelen)
{
	char		*cp = SPEAKSSL;
	int		cc;
	X509		*peer = NULL;
	char		cname[256];
	struct hostent	*he;
	struct in_addr  ipaddr, cnameip;
	
	if (connect(sock, name, namelen) < 0)
		return -1;

	/*
	 * Client side; optional use of SSL.
	 */
	if (nousessl)
		return 0;

	/*
	 * Send our special tag which says we speak SSL.
	 */
	if ((cc = write(sock, cp, strlen(cp))) != strlen(cp)) {
		if (cc >= 0) {
			error("sslconnect: short write\n");
			errno = EIO;
		}
		return -1;
	}
	
	if (! (ssl = SSL_new(ctx))) {
		tmcd_sslerror();
		errno = EIO;
		return -1;
	}
	if (! SSL_set_fd(ssl, sock)) {
		tmcd_sslerror();
		errno = EIO;
		return -1;
	}
	if (SSL_connect(ssl) <= 0) {
		tmcd_sslerror();
		goto badauth;
	}

	/*
	 * Do the verification dance.
	 */
	if (SSL_get_verify_result(ssl) != X509_V_OK) {
		tmcd_sslprint("Certificate did not verify!\n");
		goto badauth;
	}
	
	if (! (peer = SSL_get_peer_certificate(ssl))) {
		tmcd_sslprint("No certificate was presented by the peer!\n");
		goto badauth;
	}

	/*
	 * Grab the common name from the cert.
	 */
	X509_NAME_get_text_by_NID(X509_get_subject_name(peer),
				  NID_commonName, cname, sizeof(cname));
	X509_free(peer);

	/*
	 * On the client, the common name must map to the same
	 * host we just connected to. This should be enough of
	 * a check?
	 */
	ipaddr = ((struct sockaddr_in *)name)->sin_addr;
	
	if (!(he = gethostbyname(cname))) {
		error("Could not map %s: %s\n", cname, hstrerror(h_errno));
		goto badauth;
	}
	memcpy((char *)&cnameip, he->h_addr, he->h_length);

	if (ipaddr.s_addr != cnameip.s_addr) {
		char buf[BUFSIZ];
		
		strcpy(buf, inet_ntoa(ipaddr));
		
		error("Certificate mismatch: %s mapped to %s instead of %s\n",
		      cname, buf, inet_ntoa(cnameip));
		goto badauth;
	}
	isssl = 1;
	return 0;

 badauth:
	errno = EAUTH;
	return -1;
}

/*
 * Verify the certificate of the client.
 */
int
tmcd_sslverify_client(char *nodeid, char *class, char *type, int islocal)
{
	X509		*peer = NULL;
	char		cname[256], unitname[256];

	if (SSL_get_verify_result(ssl) != X509_V_OK) {
		error("sslverify: Certificate did not verify!\n");
		return -1;
	}
	
	if (! (peer = SSL_get_peer_certificate(ssl))) {
		error("sslverify: No certificate presented!\n");
		return -1;
	}

	/*
	 * Grab stuff from the cert.
	 */
	X509_NAME_get_text_by_NID(X509_get_subject_name(peer),
				  NID_organizationalUnitName,
				  unitname, sizeof(unitname));

	X509_NAME_get_text_by_NID(X509_get_subject_name(peer),
				  NID_commonName,
				  cname, sizeof(cname));
	X509_free(peer);
	
	/*
	 * On the server, things are a bit more difficult since
	 * we share a common cert locally and a per group cert remotely.
	 *
	 * Make sure common name matches.
	 */
	if (strcmp(cname, BOSSNODE)) {
		error("sslverify: commonname mismatch: %s!=%s\n",
		      cname, BOSSNODE);
		return -1;
	}

	/*
	 * If the node is remote, then the unitname must match the type.
	 * Simply a convention. 
	 */
#if 1
	if (!islocal &&
	    strcmp(unitname, "pcwa") && strcmp(unitname, "pcron") &&
	    strcmp(unitname, "pcplab")) {
		error("sslverify: unitname mismatch: %s!=pcwa|pcron|pcplab\n",
		      unitname, type);
		return -1;
	}
#else
	if (!islocal && strcmp(unitname, type)) {
		error("sslverify: unitname mismatch: %s!=%s\n",
		      unitname, type);
		return -1;
	}
#endif
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
	if (isssl)
		cc = SSL_write(ssl, buf, nbytes);
	else
		cc = write(sock, buf, nbytes);

	if (cc <= 0) {
		if (cc < 0 && isssl) {
			tmcd_sslerror();
		}
		return cc;
	}
	return cc;
}

/*
 * Read stuff in. The nosslbuf stuff is overly general; the caller behaves
 * in a much more constrained manner.
 */
int
tmcd_sslread(int sock, void *buf, size_t nbytes)
{
	int	cc = 0, nosslcount = 0;

	/*
	 * This cruft only happens on the server.
	 */
	if (nosslbuflen) {
		nosslcount = (nosslbuflen > nbytes ? nbytes : nosslbuflen);

		memcpy(buf, &nosslbuf[nosslbufidx], nosslcount);
		nosslbuflen -= nosslcount;
		nosslbufidx += nosslcount;

		if (nosslcount == nbytes)
			return nosslcount;

		/*
		 * The request has to be presented to the caller as a single
		 * message. Since we read just enough to look for the
		 * SPEAKSSL tag above, see if there is more, and get it.
		 */
		if (ioctl(sock, FIONREAD, &cc) < 0)
			return -1;

		if (cc == 0)
			return nosslcount;

		nbytes -= nosslcount;
		buf     = (void *) (((char *) buf) + nosslcount);
	}

	errno = 0;
	if (isssl)
		cc = SSL_read(ssl, buf, nbytes);
	else
		cc = read(sock, buf, nbytes);
	
	if (cc <= 0) {
		if (cc < 0 && isssl) {
			tmcd_sslerror();
		}
		return cc;
	}
	return cc + nosslcount;
}

/*
 * Terminate the SSL part of a connection. Also close the sock.
 */
int
tmcd_sslclose(int sock)
{
	int	error;

	if (ssl) {
		if (! (error = SSL_shutdown(ssl))) {
			shutdown(sock, SHUT_WR);
			error = SSL_shutdown(ssl);
		}
		if (error < 0) {
			tmcd_sslprint("SSL_shutdown: ");
			tmcd_sslerror();
		}
		SSL_free(ssl);
		ssl = NULL;
		ERR_clear_error();
	}
	nosslbuflen = 0;
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
		fprintf(stderr, "SSL Error: %s\n", buf);
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
	}
	else
		error("%s", buf);
}
