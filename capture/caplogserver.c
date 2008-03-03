/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2008 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <sys/param.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <syslog.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>
#include <paths.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <setjmp.h>
#include <netdb.h>
#ifndef __linux__
#include <rpc/rpc.h>
#endif
#ifdef WITHSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif /* WITHSSL */
#include "config.h"
#include "capdecls.h"

#define geterr(e)	strerror(e)

void		sigterm(int);
void		cleanup(void);
void		looper(void);
void		usage(void);
void		warning(char *format, ...);
void		die(char *format, ...);
void		dolog(int level, char *format, ...);
int		clientconnect(void);
int		Write(void *thing, void *data, int size);
int		Read(void *thing, void *data, int size);

#ifndef LOG_TESTBED
#define LOG_TESTBED	LOG_USER
#endif

/*
 *  Configurable things.
 */
#define PIDNAME		"%s/%s.logpid"
#define RUNNAME		"%s/%s.run"
#define ACLNAME		"%s/%s.acl"
#define BUFSIZE		4096
#define DEFAULT_CERTFILE PREFIX"/etc/capture.pem"
#define DEFAULT_CLIENT_CERTFILE PREFIX"/etc/client.pem"
#define DEFAULT_CAFILE	PREFIX"/etc/emulab.pem"

char		*Progname;
char		*Pidname;
char		*Runname;
char		*Aclname;
char		*Machine;
int		debug = 0;
sigset_t	actionsigmask;
sigset_t	allsigmask;
int		sockfd, portnum = LOGGERPORT;
char		ourhostname[MAXHOSTNAMELEN];

#ifdef  WITHSSL
SSL_CTX		*ctx;
int		initializedSSL = 0;
int		usingSSL = 0;
const char	*certfile = NULL;
const char	*cafile = NULL;

int
initializessl(void)
{
	static int initializedSSL = 0;
	
	if (initializedSSL)
		return 0;
	
	SSL_load_error_strings();
	SSL_library_init();
	
	ctx = SSL_CTX_new( SSLv23_method() );
	if (ctx == NULL) {
		dolog( LOG_NOTICE, "Failed to create context.");
		return 1;
	}
	
#ifndef PREFIX
#define PREFIX
#endif
	if (!certfile) { certfile = DEFAULT_CERTFILE; }
		
	if (SSL_CTX_use_certificate_file(ctx, certfile,
					 SSL_FILETYPE_PEM ) <= 0) {
	    dolog(LOG_NOTICE, 
		  "Could not load %s as certificate file.", certfile);
	    return 1;
	}
		
	if (SSL_CTX_use_PrivateKey_file(ctx, certfile,
					SSL_FILETYPE_PEM ) <= 0) {
	    dolog(LOG_NOTICE, "Could not load %s as key file.", certfile );
	    return 1;
	}
	initializedSSL = 1;
	return 0;
}

int
sslverify(SSL *ssl, char *requiredunit)
{
	X509		*peer = NULL;
	char		cname[256], unitname[256];
	
	assert(ssl != NULL);
	assert(requiredunit != NULL);

	if (SSL_get_verify_result(ssl) != X509_V_OK) {
		dolog(LOG_NOTICE,
		      "sslverify: Certificate did not verify!\n");
		return -1;
	}
	
	if (! (peer = SSL_get_peer_certificate(ssl))) {
		dolog(LOG_NOTICE, "sslverify: No certificate presented!\n");
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
		dolog(LOG_NOTICE,
		      "sslverify: commonname mismatch: %s!=%s\n",
		      cname, BOSSNODE);
		return -1;
	}

	/*
	 * If the node is remote, then the unitname must match the type.
	 * Simply a convention. 
	 */
	if (strcmp(unitname, requiredunit)) {
		dolog(LOG_NOTICE,
		      "sslverify: unitname mismatch: %s!=Capture Server\n",
		      unitname);
		return -1;
	}
	
	return 0;
}
#endif /* WITHSSL */ 

int
main(int argc, char **argv)
{
	char strbuf[MAXPATHLEN];
	int op, i;
	extern int optind;
	extern char *optarg;
	struct sockaddr_in name;
	struct sigaction sa;
	sigset_t actionsigmask;
	
	if ((Progname = rindex(argv[0], '/')))
		Progname++;
	else
		Progname = *argv;

	while ((op = getopt(argc, argv, "dv:c:p:")) != EOF) {
		switch (op) {
		case 'd':
			debug++;
			break;
		case 'p':
			portnum = atoi(optarg);
			break;
#ifdef  WITHSSL
		case 'c':
		        certfile = optarg;
		        break;
		case 'v':
			cafile = optarg;
			break;
#endif
		}
	}
	argc -= optind;
	argv += optind;

	if (!debug)
		(void)daemon(0, 0);

	openlog(Progname, LOG_PID, LOG_TESTBED);
	dolog(LOG_NOTICE, "starting");

	sigemptyset(&actionsigmask);
	sigaddset(&actionsigmask, SIGINT);
	sigaddset(&actionsigmask, SIGTERM);
	memset(&sa, 0, sizeof sa);
	sa.sa_handler = sigterm;
	sa.sa_mask = actionsigmask;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	/*
	 * Create and bind our socket.
	 */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		die("socket(): opening stream socket: %s", geterr(errno));

	i = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
		       (char *)&i, sizeof(i)) < 0)
		die("setsockopt(): SO_REUSEADDR: %s", geterr(errno));
	
	/* Create wildcard name. */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons((u_short) portnum);
	if (bind(sockfd, (struct sockaddr *) &name, sizeof(name)))
		die("bind(): binding stream socket: %s", geterr(errno));

	/* Find assigned port value and print it out. */
	i = sizeof(name);
	if (getsockname(sockfd, (struct sockaddr *)&name, &i))
		die("getsockname(): %s", geterr(errno));
	portnum = ntohs(name.sin_port);

	if (listen(sockfd, 1) < 0)
		die("listen(): %s", geterr(errno));

	if (gethostname(ourhostname, sizeof(ourhostname)) < 0)
		die("gethostname(): %s", geterr(errno));

	if (!getuid()) {
		FILE	*fp;
	    
		sprintf(strbuf, "%s/caplogserver.pid", _PATH_VARRUN);
		fp = fopen(strbuf, "w");
		if (fp != NULL) {
			fprintf(fp, "%d\n", getpid());
			(void) fclose(fp);
			Pidname = strdup(strbuf);
		}
	}
	dolog(LOG_NOTICE, "Ready! Listening on TCP port %d", portnum);
	looper();
	cleanup();
	exit(0);
}

void
looper(void)
{
	static fd_set	sfds;
	static int	fdcount;
	fd_set		fds;
	int		i;
	struct timeval  timeout;
	int		child_pid, status;

	FD_ZERO(&sfds);
	fdcount = sockfd;
	FD_SET(sockfd, &sfds);
	fdcount++;

	for (;;) {
		fds = sfds;
		timeout.tv_usec = 0;
		timeout.tv_sec  = 10;

		/*
		 * Do not let children stack up.
		 */
		while ((child_pid = waitpid(-1, &status, WNOHANG)) > 0) {
			dolog(LOG_NOTICE, "child %d has exited", child_pid);
		}

		i = select(fdcount, &fds, NULL, NULL, &timeout);
		if (i < 0) {
			if (errno == EINTR) {
				warning("input select interrupt, continuing");
				continue;
			}
			cleanup();
			die("%s: select: ", geterr(errno));
		}
		if (i == 0) {
			continue;
		}
		if (FD_ISSET(sockfd, &fds)) {
			(void) clientconnect();
		}
	}
}

int
clientconnect(void)
{
	struct sockaddr_in	client;
	struct timeval		timeout;
	int			i, cc, length = sizeof(client);
	int			clientfd = -1, logfd = -1, pid;
	logger_t		logreq;
	capret_t		capret = CAPOK;
	char			strbuf[BUFSIZ];
	static fd_set		sfds;
	static int		fdcount;
	fd_set			fds;
#ifdef WITHSSL
	int			ret;
	SSL			*ssl = NULL;
#endif
	void			*output;

	clientfd = accept(sockfd, (struct sockaddr *)&client, &length);
	if (clientfd < 0) {
		dolog(LOG_NOTICE, "accept()ing new client: %s", geterr(errno));
		return 1;
	}
	pid = fork();
	if (pid)
	    return pid;

	signal(SIGTERM, SIG_DFL);
	signal(SIGINT,  SIG_DFL);
	dolog(LOG_NOTICE, "%s connecting", inet_ntoa(client.sin_addr));
	
	/*
	 * Read the first part to verify the key. We must get the
	 * proper bits or this is not a valid tip connection.
	 */
	if ((cc = read(clientfd, &logreq, sizeof(logreq))) <= 0) {
		close(clientfd);
		dolog(LOG_NOTICE, "%s connecting, error reading log request",
		      inet_ntoa(client.sin_addr));
		goto done;
	}
	output = (void *) clientfd;

#ifdef WITHSSL
	if (cc == sizeof(logreq) &&
	    !strncmp(logreq.secretkey.key, "USESSL", 6)) {
	    if (initializessl() != 0)
		goto done;

	    ssl = SSL_new(ctx);
	    if (!ssl) {
		dolog(LOG_NOTICE, "SSL_new failed");
		goto done;
	    }	    
	    if (SSL_set_fd(ssl, clientfd) <= 0) {
		dolog(LOG_NOTICE, "SSL_set_fd failed");
		goto done;
	    }
	    if (SSL_accept(ssl) <= 0) {
		dolog(LOG_NOTICE, "%s connecting, SSL_accept error.",
		      inet_ntoa(client.sin_addr));
		ERR_print_errors_fp(stderr);
		goto done;
	    }
	    if (SSL_read(ssl, (void *)&logreq, sizeof(logreq)) <= 0) {
		dolog(LOG_NOTICE, "%s connecting, error reading logger req.",
		      inet_ntoa(client.sin_addr));
		goto done;
	    }
	    usingSSL = 1;
	    output   = (void *) ssl;
	}
#endif /* WITHSSL */
	/* Verify size of the request is sane */
	if (cc == sizeof(logreq) &&
	    logreq.secretkey.keylen == strlen(logreq.secretkey.key)) {
	    /*
	     * Need to grab the machine name out of the request
	     * and get its key from disk. Be careful not to let
	     * any metachars in the name when forming the path.
	     */
	    char *bp = logreq.node_id;
	    while (*bp) {
		if (!(isalnum(*bp) || *bp == '_' || *bp == '-')) {
		    capret = CAPERROR;
		    dolog(LOG_NOTICE, "%s connecting, bad chars in node_id",
			  inet_ntoa(client.sin_addr));
		    goto done;
		}
		bp++;
	    }
	    Machine = strdup(logreq.node_id);
	    (void) snprintf(strbuf, sizeof(strbuf), ACLNAME, ACLPATH, Machine);
	    Aclname = strdup(strbuf);
	    (void) snprintf(strbuf, sizeof(strbuf), RUNNAME, LOGPATH, Machine);
	    Runname = strdup(strbuf);
	}
	else {
	    capret = CAPERROR;
	}
	
	/*
	 * Open file and write contents to the end.
	 */
	if (capret == CAPOK && (logfd = open(Runname, O_RDONLY)) < 0) {
		warning("open('%s'): %s", Runname, geterr(errno));
		capret = CAPERROR;
	}
	
	/*
	 * Return status to client.
	 */
	if (Write(output, (void *)&capret, sizeof(capret)) <= 0) {
		dolog(LOG_NOTICE, "%s connecting, error writing status",
		      inet_ntoa(client.sin_addr));
		goto done;
	}
	if (capret != CAPOK)
	    goto done;

	/*
	 * Now just loop reading from file and sending it back.
	 * Need to use select since we are waiting for stuff to
	 * get added to the end of the file.
	 */
	FD_ZERO(&sfds);
	FD_SET(logfd, &sfds);
	FD_SET(clientfd, &sfds);
	fdcount = logfd + 1;
	
	for (;;) {
		fds = sfds;
		timeout.tv_usec = 0;
		timeout.tv_sec  = 10;

		i = select(fdcount, &fds, NULL, NULL, &timeout);
		if (i < 0) {
			if (errno == EINTR) {
				warning("input select interrupt, continuing");
				continue;
			}
			warning("%s: select: ", geterr(errno));
			break;
		}
		if (i == 0) {
			continue;
		}
		if (FD_ISSET(clientfd, &fds)) {
		    /*
		     * There should not be anything to read. If so, its
		     * an error so bail. Typically, it means the client
		     * has actually gone away.
		     */
		    break;
		}
		if (FD_ISSET(logfd, &fds)) {
		    char *bp = strbuf;
		    int   cc = read(logfd, strbuf, sizeof(strbuf));
		    if (cc == 0)
			continue;
		    if (cc < 0) {
			warning("%s: read: ", geterr(errno));
			goto done;
		    }
		    while (cc) {
			int wc = Write(output, bp, cc);

			if (wc < 0) {
			    warning("%s: write: ", geterr(errno));
			    goto done;
			}
			cc -= wc;
			bp += wc;
		    }			
		}
	}
	exit(0);
 done:
	exit(1);
}

/*
 * SIGTERM
 */
void
sigterm(int sig)
{
	cleanup();
	exit(0);
}

/*
 *  Display proper usage / error message and exit.
 */
void
usage(void)
{
	fprintf(stderr, "usage: %s [-d] [-p port] [-c certfile] [-v calist]\n",
		Progname);
	exit(1);
}

void
warning(char *format, ...)
{
	char msgbuf[BUFSIZE];
	va_list ap;

	va_start(ap, format);
	vsnprintf(msgbuf, BUFSIZE, format, ap);
	va_end(ap);
	dolog(LOG_WARNING, msgbuf);
}

void
die(char *format, ...)
{
	char msgbuf[BUFSIZE];
	va_list ap;

	va_start(ap, format);
	vsnprintf(msgbuf, BUFSIZE, format, ap);
	va_end(ap);
	dolog(LOG_ERR, msgbuf);
	exit(1);
}

void
dolog(int level, char *format, ...)
{
	char msgbuf[BUFSIZE];
	va_list ap;

	va_start(ap, format);
	vsnprintf(msgbuf, BUFSIZE, format, ap);
	va_end(ap);
	if (debug) {
		fprintf(stderr, "%s\n", msgbuf);
		fflush(stderr);
	}
	syslog(level, "%s\n", msgbuf);
}

void
cleanup(void)
{
	int	child_pid, status;

	dolog(LOG_NOTICE, "exiting");
	(void) killpg(0, SIGTERM);

	while ((child_pid = waitpid(-1, &status, WNOHANG)) > 0) {
	    dolog(LOG_NOTICE, "child %d has exited", child_pid);
	}

	if (Pidname)
	    (void) unlink(Pidname);
}

int
Write(void *thing, void * data, int size)
{
#ifdef	WITHSSL
    if (usingSSL)
	return SSL_write((SSL *)thing, data, size);
    else
#endif
	return write((int) thing, data, size);
}

int
Read(void *thing, void * data, int size)
{
#ifdef	WITHSSL
    if (usingSSL)
	return SSL_read((SSL *) thing, data, size);
    else
#endif
	return read((int) thing, data, size);
}

