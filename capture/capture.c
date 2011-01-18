/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2008 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Testbed note:  This code has developed over the last several
 * years in RCS.  This is an import of the current version of
 * capture from the /usr/src/utah RCS repository into the testbed,
 * with some new hacks to port it to Linux.
 *
 * - dga, 10/10/2000
 */

/*
 * A LITTLE hack to record output from a tty device to a file, and still
 * have it available to tip using a pty/tty pair.
 */

#define SAFEMODE
	
#include <sys/param.h>

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <syslog.h>
#include <termios.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>
#include <paths.h>

#include <sys/param.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/termios.h>
#ifdef USESOCKETS
#include <sys/types.h>
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
#endif /* USESOCKETS */
#include "capdecls.h"

#define geterr(e)	strerror(e)

void quit(int);
void reinit(int);
void newrun(int);
void terminate(int);
void cleanup(void);
void capture(void);

void usage(void);
void warning(char *format, ...);
void die(char *format, ...);
void dolog(int level, char *format, ...);

int val2speed(int val);
void rawmode(char *devname, int speed);
int netmode();
void writepid(void);
void createkey(void);
int handshake(void);
#ifdef USESOCKETS
int clientconnect(void);
#endif
int handleupload(void);

#ifdef __linux__
#define _POSIX_VDISABLE '\0'
#define revoke(tty)	(0)
#endif

#ifndef LOG_TESTBED
#define LOG_TESTBED	LOG_USER
#endif

/*
 *  Configurable things.
 */
#define PIDNAME		"%s/%s.pid"
#define LOGNAME		"%s/%s.log"
#define RUNNAME		"%s/%s.run"
#define TTYNAME		"%s/%s"
#define PTYNAME		"%s/%s-pty"
#define ACLNAME		"%s/%s.acl"
#define DEVNAME		"%s/%s"
#define BUFSIZE		4096
#define DROP_THRESH	(32*1024)
#define MAX_UPLOAD_SIZE	(32 * 1024 * 1024)
#define DEFAULT_CERTFILE PREFIX"/etc/capture.pem"
#define DEFAULT_CLIENT_CERTFILE PREFIX"/etc/client.pem"
#define DEFAULT_CAFILE	PREFIX"/etc/emulab.pem"

char 	*Progname;
char 	*Pidname;
char	*Logname;
char	*Runname;
char	*Ttyname;
char	*Ptyname;
char	*Devname;
char	*Machine;
int	logfd = -1, runfd, devfd = -1, ptyfd = -1;
int	hwflow = 0, speed = B9600, debug = 0, runfile = 0, standalone = 0;
int	stampinterval = -1;
sigset_t actionsigmask;
sigset_t allsigmask;
int	 powermon = 0;
#ifndef  USESOCKETS
#define relay_snd 0
#define relay_rcv 0
#define remotemode 0
#else
char		  *Bossnode = BOSSNODE;
struct sockaddr_in Bossaddr;
char		  *Aclname;
int		   serverport = SERVERPORT;
int		   sockfd, tipactive, portnum, relay_snd, relay_rcv;
int		   remotemode;
int		   upportnum = -1, upfd = -1, upfilefd = -1;
char		   uptmpnam[64];
size_t		   upfilesize = 0;
struct sockaddr_in tipclient;
struct sockaddr_in relayclient;
struct in_addr	   relayaddr;
secretkey_t	   secretkey;
char		   ourhostname[MAXHOSTNAMELEN];
int		   needshake;
gid_t		   tipgid;
uid_t		   tipuid;
char		  *uploadCommand;

#ifdef  WITHSSL

SSL_CTX * ctx;
SSL * sslCon;
SSL * sslRelay;
SSL * sslUpload;

int initializedSSL = 0;

const char * certfile = NULL;
const char * cafile = NULL;

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
	
	if (relay_snd) {
		if (!cafile) { cafile = DEFAULT_CAFILE; }
		if (SSL_CTX_load_verify_locations(ctx, cafile, NULL) == 0) {
			die("cannot load verify locations");
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
		
		if (!certfile) { certfile = DEFAULT_CLIENT_CERTFILE; }
		if (SSL_CTX_use_certificate_file( ctx,
						  certfile,
						  SSL_FILETYPE_PEM ) <= 0) {
			dolog(LOG_NOTICE, 
			      "Could not load %s as certificate file.",
			      certfile );
			return 1;
		}
		
		if (SSL_CTX_use_PrivateKey_file( ctx,
						 certfile,
						 SSL_FILETYPE_PEM ) <= 0) {
			dolog(LOG_NOTICE, 
			      "Could not load %s as key file.",
			      certfile );
			return 1;
		}
	}
	else {
		if (!certfile) { certfile = DEFAULT_CERTFILE; }
		
		if (SSL_CTX_use_certificate_file( ctx,
						  certfile,
						  SSL_FILETYPE_PEM ) <= 0) {
			dolog(LOG_NOTICE, 
			      "Could not load %s as certificate file.",
			      certfile );
			return 1;
		}
		
		if (SSL_CTX_use_PrivateKey_file( ctx,
						 certfile,
						 SSL_FILETYPE_PEM ) <= 0) {
			dolog(LOG_NOTICE, 
			      "Could not load %s as key file.",
			      certfile );
			return 1;
		}
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
#endif /* USESOCKETS */

int
main(int argc, char **argv)
{
	char strbuf[MAXPATHLEN], *newstr();
	int op, i;
	struct sigaction sa;
	extern int optind;
	extern char *optarg;
#ifdef  USESOCKETS
	struct sockaddr_in name;
#endif

	if ((Progname = rindex(argv[0], '/')))
		Progname++;
	else
		Progname = *argv;

	while ((op = getopt(argc, argv, "rds:Hb:ip:c:T:aou:v:Pm")) != EOF)
		switch (op) {
#ifdef	USESOCKETS
#ifdef  WITHSSL
		case 'c':
		        certfile = optarg;
		        break;
#endif  /* WITHSSL */
		case 'b':
			Bossnode = optarg;
			break;

		case 'p':
			serverport = atoi(optarg);
			break;

		case 'i':
			standalone = 1;
			break;

		case 'm':
			remotemode = 1;
			break;
#endif /* USESOCKETS */
		case 'H':
			++hwflow;
			break;

		case 'd':
			debug++;
			break;

		case 'r':
			runfile++;
			break;

		case 's':
			if ((i = atoi(optarg)) == 0 ||
			    (speed = val2speed(i)) == 0)
				usage();
			break;
		case 'T':
			stampinterval = atoi(optarg);
			if (stampinterval < 0)
				usage();
			break;
		case 'P':
			powermon = 1;
			break;
#ifdef  WITHSSL
		case 'a':
			relay_snd = 1;
			break;
			
		case 'o':
			relay_rcv = 1;
			break;
			
		case 'u':
			uploadCommand = optarg;
			break;

		case 'v':
			cafile = optarg;
			break;
#endif
		}

	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();

	if (!debug)
		(void)daemon(0, 0);

	Machine = argv[0];

	(void) snprintf(strbuf, sizeof(strbuf), PIDNAME, LOGPATH, argv[0]);
	Pidname = newstr(strbuf);
	(void) snprintf(strbuf, sizeof(strbuf), LOGNAME, LOGPATH, argv[0]);
	Logname = newstr(strbuf);
	(void) snprintf(strbuf, sizeof(strbuf), RUNNAME, LOGPATH, argv[0]);
	Runname = newstr(strbuf);
	(void) snprintf(strbuf, sizeof(strbuf), TTYNAME, TIPPATH, argv[0]);
	Ttyname = newstr(strbuf);
	(void) snprintf(strbuf, sizeof(strbuf), PTYNAME, TIPPATH, argv[0]);
	Ptyname = newstr(strbuf);
	if (remotemode)
		strcpy(strbuf, argv[1]);
	else
		(void) snprintf(strbuf, sizeof(strbuf),
				DEVNAME, DEVPATH, argv[1]);
	Devname = newstr(strbuf);

	openlog(Progname, LOG_PID, LOG_TESTBED);
	dolog(LOG_NOTICE, "starting");

	/*
	 * We process the "action" signals sequentially, there are just
	 * too many interdependencies.  We block em while we shut down too.
	 */
	sigemptyset(&actionsigmask);
	sigaddset(&actionsigmask, SIGHUP);
	sigaddset(&actionsigmask, SIGUSR1);
	sigaddset(&actionsigmask, SIGUSR2);
	allsigmask = actionsigmask;
	sigaddset(&allsigmask, SIGINT);
	sigaddset(&allsigmask, SIGTERM);
	memset(&sa, 0, sizeof sa);
	sa.sa_handler = quit;
	sa.sa_mask = allsigmask;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	if (!relay_snd) {
		sa.sa_handler = reinit;
		sa.sa_mask = actionsigmask;
		sigaction(SIGHUP, &sa, NULL);
	}
	if (runfile) {
		sa.sa_handler = newrun;
		sigaction(SIGUSR1, &sa, NULL);
	}
	sa.sa_handler = terminate;
	sigaction(SIGUSR2, &sa, NULL);

#ifdef HAVE_SRANDOMDEV
	srandomdev();
#else
	srand(time(NULL));
#endif
	
	/*
	 * Open up run/log file, console tty, and controlling pty.
	 */
	if (runfile) {
		unlink(Runname);
		
		if ((runfd = open(Runname,O_WRONLY|O_CREAT|O_APPEND,0600)) < 0)
			die("%s: open: %s", Runname, geterr(errno));
		if (fchmod(runfd, 0640) < 0)
			die("%s: fchmod: %s", Runname, geterr(errno));
	}
#ifdef  USESOCKETS
	/*
	 * Verify the bossnode and stash the address info
	 */
	{
		struct hostent *he;

		he = gethostbyname(Bossnode);
		if (he == 0) {
			die("gethostbyname(%s): %s",
			    Bossnode, hstrerror(h_errno));
		}
		memcpy ((char *)&Bossaddr.sin_addr, he->h_addr, he->h_length);
		Bossaddr.sin_family = AF_INET;
		Bossaddr.sin_port   = htons(serverport);
	}

	(void) snprintf(strbuf, sizeof(strbuf), ACLNAME, ACLPATH, Machine);
	Aclname = newstr(strbuf);
	
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
	name.sin_port = 0;
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

	createkey();
	dolog(LOG_NOTICE, "Ready! Listening on TCP port %d", portnum);

	if (relay_snd) {
		struct sockaddr_in sin;
		struct hostent *he;
		secretkey_t key;
		char *port_idx;
		int port;

		if ((port_idx = strchr(argv[0], ':')) == NULL)
			die("%s: bad format, expecting 'host:port'", argv[0]);
		*port_idx = '\0';
		port_idx += 1;
		if (sscanf(port_idx, "%d", &port) != 1)
			die("%s: bad port number", port_idx);
		he = gethostbyname(argv[0]);
		if (he == 0) {
			die("gethostbyname(%s): %s",
			    argv[0], hstrerror(h_errno));
		}
		bzero(&sin, sizeof(sin));
		memcpy ((char *)&sin.sin_addr, he->h_addr, he->h_length);
		sin.sin_family = AF_INET;
		sin.sin_port = htons(port);

		if ((ptyfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
			die("socket(): %s", geterr(errno));
		if (connect(ptyfd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
			die("connect(): %s", geterr(errno));
		snprintf(key.key, sizeof(key.key), "RELAY %d", portnum);
		key.keylen = strlen(key.key);
		if (write(ptyfd, &key, sizeof(key)) != sizeof(key))
			die("write(): %s", geterr(errno));
#ifdef  WITHSSL
		initializessl();
		sslRelay = SSL_new(ctx);
		if (!sslRelay)
			die("SSL_new()");
		if (SSL_set_fd(sslRelay, ptyfd) <= 0)
			die("SSL_set_fd()");
		if (SSL_connect(sslRelay) <= 0)
			die("SSL_connect()");
		if (sslverify(sslRelay, "Capture Server"))
			die("SSL connection did not verify");
#endif
		if (fcntl(ptyfd, F_SETFL, O_NONBLOCK) < 0)
			die("fcntl(O_NONBLOCK): %s", geterr(errno));
		tipactive = 1;
	}

	if (relay_rcv) {
		struct hostent *he;

		he = gethostbyname(argv[1]);
		if (he == 0) {
			die("gethostbyname(%s): %s",
			    argv[1], hstrerror(h_errno));
		}
		memcpy ((char *)&relayaddr, he->h_addr, he->h_length);
	}
#else
	if ((ptyfd = open(Ptyname, O_RDWR)) < 0)
		die("%s: open: %s", Ptyname, geterr(errno));
#endif
	
	if (!relay_snd) {
		if ((logfd = open(Logname,O_WRONLY|O_CREAT|O_APPEND,0640)) < 0)
			die("%s: open: %s", Logname, geterr(errno));
		if (chmod(Logname, 0640) < 0)
			die("%s: chmod: %s", Logname, geterr(errno));
	}
	
	if (!relay_rcv) {
#ifdef  USESOCKETS
	    if (remotemode) {
		if (netmode() != 0)
		    die("Could not establish connection to %s\n", Devname);
	    }
	    else
#endif
		    rawmode(Devname, speed);
	}
	writepid();
	capture();
	cleanup();
	exit(0);
}

#ifdef TWOPROCESS
int	pid;

void
capture(void)
{
	int flags = FNDELAY;

	(void) fcntl(ptyfd, F_SETFL, &flags);

	if (pid = fork())
		in();
	else
		out();
}

/*
 * Loop reading from the console device, writing data to log file and
 * to the pty for tip to pick up.
 */
in(void)
{
	char buf[BUFSIZE];
	int cc;
	sigset_t omask;
	
	while (1) {
		if ((cc = read(devfd, buf, BUFSIZE)) < 0) {
			if ((errno == EWOULDBLOCK) || (errno == EINTR))
				continue;
			else
				die("%s: read: %s", Devname, geterr(errno));
		}
		sigprocmask(SIG_BLOCK, &actionsigmask, &omask);

		if (write(logfd, buf, cc) < 0)
			die("%s: write: %s", Logname, geterr(errno));

		if (runfile) {
			if (write(runfd, buf, cc) < 0)
				die("%s: write: %s", Runname, geterr(errno));
		}

		if (write(ptyfd, buf, cc) < 0) {
			if ((errno != EIO) && (errno != EWOULDBLOCK))
				die("%s: write: %s", Ptyname, geterr(errno));
		}
		sigprocmask(SIG_SETMASK, &omask, NULL);
	}
}

/*
 * Loop reading input from pty (tip), and send off to the console device.
 */
out(void)
{
	char buf[BUFSIZE];
	int cc;
	sigset_t omask;
	struct timeval timeout;

	timeout.tv_sec  = 0;
	timeout.tv_usec = 100000;
	
	while (1) {
		sigprocmask(SIG_BLOCK, &actionsigmask, &omask);
		if ((cc = read(ptyfd, buf, BUFSIZE)) < 0) {
			sigprocmask(SIG_SETMASK, &omask, NULL);
			if ((errno == EIO) || (errno == EWOULDBLOCK) ||
			    (errno == EINTR)) {
				select(0, 0, 0, 0, &timeout);
				continue;
			}
			else
				die("%s: read: %s", Ptyname, geterr(errno));
		}

		if (write(devfd, buf, cc) < 0)
			die("%s: write: %s", Devname, geterr(errno));
		
		sigprocmask(SIG_SETMASK, &omask, NULL);
	}
}
#else
static fd_set	sfds;
static int	fdcount;
void
capture(void)
{
	fd_set fds;
	int i, cc, lcc;
	sigset_t omask;
	char buf[BUFSIZE];
	struct timeval timeout;
#ifdef LOG_DROPS
	int drop_topty_chars = 0;
	int drop_todev_chars = 0;
#endif

	/*
	 * XXX for now we make both directions non-blocking.  This is a
	 * quick hack to achieve the goal that capture never block
	 * uninterruptably for long periods of time (use threads).
	 * This has the unfortunate side-effect that we may drop chars
	 * from the perspective of the user (use threads).  A more exotic
	 * solution would be to poll the readiness of output (use threads)
	 * as well as input and not read from one end unless we can write
	 * the other (use threads).
	 *
	 * I keep thinking (use threads) that there is a better way to do
	 * this (use threads).  Hmm...
	 */
	if ((devfd >= 0) && (fcntl(devfd, F_SETFL, O_NONBLOCK) < 0))
		die("%s: fcntl(O_NONBLOCK): %s", Devname, geterr(errno));
#ifndef USESOCKETS
	/*
	 * It gets better!
	 * In FreeBSD 4.0 and beyond, the fcntl fails because the slave
	 * side is not open even though the only effect of this call is
	 * to set the file description's FNONBLOCK flag (i.e. the pty and
	 * tty code do nothing additional).  Turns out that we could use
	 * ioctl instead of fcntl to set the flag.  The call will still
	 * fail, but the flag will be left set.  Rather than rely on that
	 * dubious behavior, I temporarily open the slave, do the fcntl
	 * and close the slave again.
	 */
#ifdef __FreeBSD__
	if ((i = open(Ttyname, O_RDONLY)) < 0)
		die("%s: open: %s", Ttyname, geterr(errno));
#endif
	if (fcntl(ptyfd, F_SETFL, O_NONBLOCK) < 0)
		die("%s: fcntl(O_NONBLOCK): %s", Ptyname, geterr(errno));
#ifdef __FreeBSD__
	close(i);
#endif
#endif /* USESOCKETS */

	FD_ZERO(&sfds);
	if (devfd >= 0)
		FD_SET(devfd, &sfds);
	fdcount = devfd;
#ifdef  USESOCKETS
	if (devfd < sockfd)
		fdcount = sockfd;
	FD_SET(sockfd, &sfds);
#endif	/* USESOCKETS */
	if (ptyfd >= 0) {
		if (devfd < ptyfd)
			fdcount = ptyfd;
		FD_SET(ptyfd, &sfds);
	}

	fdcount++;

	for (;;) {
#ifdef LOG_DROPS
		if (drop_topty_chars >= DROP_THRESH) {
			warning("%d dev -> pty chars dropped",
				drop_topty_chars);
			drop_topty_chars = 0;
		}
		if (drop_todev_chars >= DROP_THRESH) {
			warning("%d pty -> dev chars dropped",
				drop_todev_chars);
			drop_todev_chars = 0;
		}
#endif
		fds = sfds;
		timeout.tv_usec = 0;
		timeout.tv_sec  = 30;
#ifdef	USESOCKETS
		if (needshake) {
			timeout.tv_sec += (random() % 60);
		}
#endif
		i = select(fdcount, &fds, NULL, NULL, &timeout);
		if (i < 0) {
			if (errno == EINTR) {
				warning("input select interrupted, continuing");
				continue;
			}
			die("%s: select: %s", Devname, geterr(errno));
		}
		if (i == 0) {
#ifdef	USESOCKETS
			if (needshake) {
				(void) handshake();
				continue;
			}
#endif
			continue;
		}
#ifdef	USESOCKETS
		if (FD_ISSET(sockfd, &fds)) {
			(void) clientconnect();
		}
		if ((upfd >=0) && FD_ISSET(upfd, &fds)) {
			(void) handleupload();
		}
#endif	/* USESOCKETS */
		if ((devfd >= 0) && FD_ISSET(devfd, &fds)) {
			errno = 0;
#ifdef  WITHSSL
			if (relay_rcv) {
			  cc = SSL_read(sslRelay, buf, sizeof(buf));
			  if (cc <= 0) {
			    FD_CLR(devfd, &sfds);
			    devfd = -1;
			    bzero(&relayclient, sizeof(relayclient));
			    continue;
			  }
			}
			else
#endif
			  cc = read(devfd, buf, sizeof(buf));
			if (cc <= 0) {
#ifdef  USESOCKETS
				if (remotemode) {
					FD_CLR(devfd, &sfds);
					close(devfd);
					warning("remote socket closed;"
						"attempting to reconnect");
					while (netmode() != 0) {
					    usleep(5000000);
					}
					FD_SET(devfd, &sfds);
					continue;
				}
#endif
				if (cc < 0)
					die("%s: read: %s",
					    Devname, geterr(errno));
				if (cc == 0)
					die("%s: read: EOF", Devname);
			}
			errno = 0;

			sigprocmask(SIG_BLOCK, &actionsigmask, &omask);
#ifdef	USESOCKETS
			if (!tipactive)
				goto dropped;
#endif
			for (lcc = 0; lcc < cc; lcc += i) {
#ifdef  WITHSSL
				if (relay_snd) {
					i = SSL_write(sslRelay, &buf[lcc], cc-lcc);
				}
			        else if (sslCon != NULL) {
				        i = SSL_write(sslCon, &buf[lcc], cc-lcc);
					if (i < 0) { i = 0; } /* XXX Hack */
			        } else
#endif /* WITHSSL */ 
			        {
				        i = write(ptyfd, &buf[lcc], cc-lcc);
				}
				if (i < 0) {
					/*
					 * Either tip is blocked (^S) or
					 * not running (the latter should
					 * return EIO but doesn't due to a
					 * pty bug).  Note that we have
					 * dropped some chars.
					 */
					if (errno == EIO || errno == EAGAIN) {
#ifdef LOG_DROPS
						drop_topty_chars += (cc-lcc);
#endif
						goto dropped;
					}
					die("%s: write: %s",
					    Ptyname, geterr(errno));
				}
				if (i == 0) {
#ifdef	USESOCKETS
					sigprocmask(SIG_SETMASK, &omask, NULL);
					goto disconnected;
#else
					die("%s: write: zero-length", Ptyname);
#endif
				}
			}
dropped:
			if (stampinterval >= 0) {
				static time_t laststamp;
				struct timeval tv;
				char stampbuf[40], *cts;
				time_t now;

				gettimeofday(&tv, 0);
				now = tv.tv_sec;
				if (stampinterval == 0 ||
				    now > laststamp + stampinterval) {
					cts = ctime(&now);
					cts[24] = 0;
					snprintf(stampbuf, sizeof stampbuf,
						 "\nSTAMP{%s}\n", cts);
					write(logfd, stampbuf,
					      strlen(stampbuf));
				}
				laststamp = now;
			}
			if (logfd >= 0) {
				i = write(logfd, buf, cc);
				if (i < 0)
					die("%s: write: %s", Logname, geterr(errno));
				if (i != cc)
					die("%s: write: incomplete", Logname);
			}
			if (runfile) {
				i = write(runfd, buf, cc);
				if (i < 0)
					die("%s: write: %s",
					    Runname, geterr(errno));
				if (i != cc)
					die("%s: write: incomplete", Runname);
			}
			sigprocmask(SIG_SETMASK, &omask, NULL);

		}
		if ((ptyfd >= 0) && FD_ISSET(ptyfd, &fds)) {
			int lerrno;

			sigprocmask(SIG_BLOCK, &actionsigmask, &omask);
			errno = 0;
#ifdef WITHSSL
			if (relay_snd) {
				cc = SSL_read( sslRelay, buf, sizeof(buf) );
				if (cc < 0) { /* XXX hack */
					cc = 0;
					SSL_free(sslRelay);
					sslRelay = NULL;
					upportnum = -1;
				}
			}
			else if (sslCon != NULL) {
			        cc = SSL_read( sslCon, buf, sizeof(buf) );
				if (cc < 0) { /* XXX hack */
					cc = 0;
					SSL_free(sslCon);
					sslCon = NULL;
				}
			} else
#endif /* WITHSSL */ 
			{
			        cc = read(ptyfd, buf, sizeof(buf));
			}
			lerrno = errno;
			sigprocmask(SIG_SETMASK, &omask, NULL);
			if (cc < 0) {
				/* XXX commonly observed */
				if (lerrno == EIO || lerrno == EAGAIN)
					continue;
#ifdef	USESOCKETS
				if (lerrno == ECONNRESET || lerrno == ETIMEDOUT)
					goto disconnected;
				die("%s: socket read: %s",
				    Machine, geterr(lerrno));
#else
				die("%s: read: %s", Ptyname, geterr(lerrno));
#endif
			}
			if (cc == 0) {
#ifdef	USESOCKETS
			disconnected:
				/*
				 * Other end disconnected.
				 */
				if (relay_snd)
					die("relay receiver died");
				dolog(LOG_INFO, "%s disconnecting",
				      inet_ntoa(tipclient.sin_addr));
				FD_CLR(ptyfd, &sfds);
				close(ptyfd);
				tipactive = 0;
				continue;
#else
				/*
				 * Delay after reading 0 bytes from the pty.
				 * At least under FreeBSD, select on a
				 * disconnected pty (control half) always
				 * return ready and the subsequent read always
				 * returns 0.  To keep capture from eating up
				 * CPU constantly when no one is connected to
				 * the pty (i.e., most of the time) we delay
				 * after doing a zero length read.
				 *
				 * Note we keep tabs on the device so that we
				 * will wake up early if it goes active.
				 */
				timeout.tv_sec  = 1;
				timeout.tv_usec = 0;
				FD_ZERO(&fds);
				FD_SET(devfd, &fds);
				select(devfd+1, &fds, 0, 0, &timeout);
				continue;
#endif
			}
			errno = 0;

			sigprocmask(SIG_BLOCK, &actionsigmask, &omask);
			for (lcc = 0; lcc < cc; lcc += i) {
				if (relay_rcv) {
#ifdef USESOCKETS
#ifdef  WITHSSL
					if (sslRelay != NULL) {
						i = SSL_write(sslRelay,
							      &buf[lcc],
							      cc - lcc);
					} else
#endif
					{
						i = cc - lcc;
					}
#endif
				}
				else {
					i = write(devfd, &buf[lcc], cc-lcc);
				}
				if (i < 0) {
					/*
					 * Device backed up (or FUBARed)
					 * Note that we dropped some chars.
					 */
					if (errno == EAGAIN) {
#ifdef LOG_DROPS
						drop_todev_chars += (cc-lcc);
#endif
						goto dropped2;
					}
					die("%s: write: %s",
					    Devname, geterr(errno));
				}
				if (i == 0)
					die("%s: write: zero-length", Devname);
			}
dropped2:
			sigprocmask(SIG_SETMASK, &omask, NULL);
		}
	}
}
#endif

/*
 * SIGHUP means we want to close the old log file (because it has probably
 * been moved) and start a new version of it.
 */
void
reinit(int sig)
{
	/*
	 * We know that the any pending write to the log file completed
	 * because we blocked SIGHUP during the write.
	 */
	close(logfd);
	
	if ((logfd = open(Logname, O_WRONLY|O_CREAT|O_APPEND, 0640)) < 0)
		die("%s: open: %s", Logname, geterr(errno));
	if (chmod(Logname, 0640) < 0)
		die("%s: chmod: %s", Logname, geterr(errno));
	
	dolog(LOG_NOTICE, "new log started");

	if (runfile)
		newrun(sig);
}

/*
 * SIGUSR1 means we want to close the old run file and start a new version
 * of it. The run file is not rolled or saved, so we unlink it to make sure
 * that no one can hang onto an open fd.
 */
void
newrun(int sig)
{
	/*
	 * We know that the any pending write to the log file completed
	 * because we blocked SIGUSR1 during the write.
	 */
	close(runfd);
	unlink(Runname);

	if ((runfd = open(Runname, O_WRONLY|O_CREAT|O_APPEND, 0600)) < 0)
		die("%s: open: %s", Runname, geterr(errno));

#ifdef  USESOCKETS
	/*
	 * Set owner/group of the new run file. Avoid race in which a
	 * user can get the new file before the chmod, by creating 0600
	 * and doing the chmod below.
	 */
	if (fchown(runfd, tipuid, tipgid) < 0)
		die("%s: fchown: %s", Runname, geterr(errno));
#endif
	if (fchmod(runfd, 0640) < 0)
		die("%s: fchmod: %s", Runname, geterr(errno));
	
	dolog(LOG_NOTICE, "new run started");
}

/*
 * SIGUSR2 means we want to revoke the other side of the pty to close the
 * tip down gracefully.  We flush all input/output pending on the pty,
 * do a revoke on the tty and then close and reopen the pty just to make
 * sure everyone is gone.
 */
void
terminate(int sig)
{
#ifdef	USESOCKETS
	if (tipactive) {
		shutdown(ptyfd, SHUT_RDWR);
		close(ptyfd);
		FD_CLR(ptyfd, &sfds);
		ptyfd = 0;
		tipactive = 0;
		dolog(LOG_INFO, "%s revoked", inet_ntoa(tipclient.sin_addr));
	}
	else
		dolog(LOG_INFO, "revoked");

	tipuid = tipgid = 0;
	if (runfile)
		newrun(sig);

	/* Must be done *after* all the above stuff is done! */
	createkey();
#else
	int ofd = ptyfd;
	
	/*
	 * We know that the any pending access to the pty completed
	 * because we blocked SIGUSR2 during the operation.
	 */
	tcflush(ptyfd, TCIOFLUSH);
	if (revoke(Ttyname) < 0)
		dolog(LOG_WARNING, "could not revoke access to tty");
	close(ptyfd);
	
	if ((ptyfd = open(Ptyname, O_RDWR)) < 0)
		die("%s: open: %s", Ptyname, geterr(errno));

	/* XXX so we don't have to recompute the select mask */
	if (ptyfd != ofd) {
		dup2(ptyfd, ofd);
		close(ptyfd);
		ptyfd = ofd;
	}

#ifdef __FreeBSD__
	/* see explanation in capture() above */
	if ((ofd = open(Ttyname, O_RDONLY)) < 0)
		die("%s: open: %s", Ttyname, geterr(errno));
#endif
	if (fcntl(ptyfd, F_SETFL, O_NONBLOCK) < 0)
		die("%s: fcntl(O_NONBLOCK): %s", Ptyname, geterr(errno));
#ifdef __FreeBSD__
	close(ofd);
#endif
	
	dolog(LOG_NOTICE, "pty reset");
#endif	/* USESOCKETS */
}

/*
 *  Display proper usage / error message and exit.
 */
char *optstr =
#ifdef USESOCKETS
#ifdef WITHSSL
"[-c certfile] [-v calist] [-u uploadcmd] "
#endif
"[-b bossnode] [-p bossport] [-i] "
#endif
"-HdraoP [-s speed] [-T stampinterval]";
void
usage(void)
{
	fprintf(stderr, "usage: %s %s machine tty\n", Progname, optstr);
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
	quit(0);
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
		fprintf(stderr, "%s - %s\n", Machine, msgbuf);
		fflush(stderr);
	}
	syslog(level, "%s - %s\n", Machine, msgbuf);
}

void
quit(int sig)
{
	cleanup();
	exit(1);
}

void
cleanup(void)
{
	dolog(LOG_NOTICE, "exiting");
#ifdef TWOPROCESS
	if (pid)
		(void) kill(pid, SIGTERM);
#endif
	(void) unlink(Pidname);
}

char *
newstr(char *str)
{
	char *np;

	if ((np = malloc((unsigned) strlen(str) + 1)) == NULL)
		die("malloc: out of memory");

	return(strcpy(np, str));
}

/*
 * Open up PID file and write our pid into it.
 */
void
writepid(void)
{
	int fd;
	char buf[8];

	if (relay_snd)
		return;
	
	if ((fd = open(Pidname, O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0)
		die("%s: open: %s", Pidname, geterr(errno));

	if (chmod(Pidname, 0644) < 0)
		die("%s: chmod: %s", Pidname, geterr(errno));
	
	(void) snprintf(buf, sizeof(buf), "%d\n", getpid());
	
	if (write(fd, buf, strlen(buf)) < 0)
		die("%s: write: %s", Pidname, geterr(errno));
	
	(void) close(fd);
}

int
powermonmode(void)
{
	struct termios serial_opts;
	int old_tiocm, status;
	
	// get copy of other current serial port settings to restore later
	if(ioctl(devfd, TIOCMGET, &old_tiocm) == -1)
		return -1;

	// get current serial port settings (must modify existing settings)
	if(tcgetattr(devfd, &serial_opts) == -1)
		return -1;
	
	// clear out settings
	serial_opts.c_cflag = 0;
	serial_opts.c_iflag = 0;
	serial_opts.c_lflag = 0;
	serial_opts.c_oflag = 0;
	
	// set baud rate
	if(cfsetispeed(&serial_opts, speed) == -1)
		return -1;
	if(cfsetospeed(&serial_opts, speed) == -1)
		return -1;
	
	// no parity
	serial_opts.c_cflag &= ~PARENB;
	serial_opts.c_iflag &= ~INPCK;
	
	// apply settings and check for error
	// this is done because tcsetattr() would return success if *any*
	// settings were set correctly; this way, more error checking is done
	if(tcsetattr(devfd, TCSANOW, &serial_opts) == -1)
		return -1;

	serial_opts.c_cflag &= ~CSTOPB; // 1 stop bit
	serial_opts.c_cflag &= ~CSIZE;  // reset byte size
	serial_opts.c_cflag |= CS8;     // 8 bits
	
	// apply settings and check for error
	if(tcsetattr(devfd, TCSANOW, &serial_opts) == -1)
		return -1;
	
	// disable hardware flow control
	serial_opts.c_cflag &= ~CRTSCTS;
	
	// apply settings and check for error
	if(tcsetattr(devfd, TCSANOW, &serial_opts) == -1)
		return -1;
	
	// disable software flow control
	serial_opts.c_iflag &= ~(IXON | IXOFF | IXANY);
	
	// apply settings and check for error
	if(tcsetattr(devfd, TCSANOW, &serial_opts) == -1)
		return -1;
	
	// raw I/O
	serial_opts.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	serial_opts.c_oflag &= ~OPOST;
	
	// apply settings and check for error
	if(tcsetattr(devfd, TCSANOW, &serial_opts) == -1)
		return -1;
	
	// timeouts
	serial_opts.c_cc[VMIN] = 0;
	serial_opts.c_cc[VTIME] = 10;
	
	// apply settings and check for error
	if(tcsetattr(devfd, TCSANOW, &serial_opts) == -1)
		return -1;
	
	// misc. settings
	serial_opts.c_cflag |= HUPCL;
	serial_opts.c_cflag |= (CLOCAL | CREAD);

	// apply settings and check for error
	if(tcsetattr(devfd, TCSANOW, &serial_opts) == -1)
		return -1;
	
	status = old_tiocm; // get settings
	status |= TIOCM_DTR;  // turn on DTR and...
	status |= TIOCM_RTS;  // ...RTS lines to power up device
	// apply settings
	if(ioctl(devfd, TIOCMSET, &status) == -1)
		return -1;
	
	// wait for device to power up
	usleep(100000);
	
	tcflush(devfd, TCOFLUSH);
	return 0;
}

/*
 * Put the console line into raw mode.
 */
void
rawmode(char *devname, int speed)
{
	struct termios t;

	if ((devfd = open(devname, O_RDWR|O_NONBLOCK)) < 0)
		die("%s: open: %s", devname, geterr(errno));
	
	if (ioctl(devfd, TIOCEXCL, 0) < 0)
		warning("TIOCEXCL %s: %s", Devname, geterr(errno));
	if (tcgetattr(devfd, &t) < 0)
		die("%s: tcgetattr: %s", Devname, geterr(errno));
	(void) cfsetispeed(&t, speed);
	(void) cfsetospeed(&t, speed);
	cfmakeraw(&t);
	t.c_cflag |= CLOCAL;
	if (hwflow)
#ifdef __linux__
	        t.c_cflag |= CRTSCTS;
#else
		t.c_cflag |= CCTS_OFLOW | CRTS_IFLOW;
#endif
	t.c_cc[VSTART] = t.c_cc[VSTOP] = _POSIX_VDISABLE;
	if (tcsetattr(devfd, TCSAFLUSH, &t) < 0)
		die("%s: tcsetattr: %s", Devname, geterr(errno));

	if (powermon && powermonmode() < 0)
		die("%s: powermonmode: %s", Devname, geterr(errno));
	
}

/*
 * The console line is really a socket on some node:port.
 */
#ifdef  USESOCKETS
int
netmode()
{
	struct sockaddr_in	sin;
	struct hostent		*he;
	char			*bp;
	int			port;
	char			hostport[BUFSIZ];
	
	strcpy(hostport, Devname);
	if ((bp = strchr(hostport, ':')) == NULL)
		die("%s: bad format, expecting 'host:port'", hostport);
	*bp++ = '\0';
	if (sscanf(bp, "%d", &port) != 1)
		die("%s: bad port number", bp);
	he = gethostbyname(hostport);
	if (he == 0) {
		warning("gethostbyname(%s): %s", hostport, hstrerror(h_errno));
		return -1;
	}
	bzero(&sin, sizeof(sin));
	memcpy ((char *)&sin.sin_addr, he->h_addr, he->h_length);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	if ((devfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		warning("socket(): %s", geterr(errno));
		return -1;
	}
	if (connect(devfd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		warning("connect(): %s", geterr(errno));
		close(devfd);
		return -1;
	}
	if (fcntl(devfd, F_SETFL, O_NONBLOCK) < 0) {
		warning("%s: fcntl(O_NONBLOCK): %s", Devname, geterr(errno));
		close(devfd);
		return -1;
	}
	return 0;
}
#endif

/*
 * From kgdbtunnel
 */
static struct speeds {
	int speed;		/* symbolic speed */
	int val;		/* numeric value */
} speeds[] = {
#ifdef B50
	{ B50,	50 },
#endif
#ifdef B75
	{ B75,	75 },
#endif
#ifdef B110
	{ B110,	110 },
#endif
#ifdef B134
	{ B134,	134 },
#endif
#ifdef B150
	{ B150,	150 },
#endif
#ifdef B200
	{ B200,	200 },
#endif
#ifdef B300
	{ B300,	300 },
#endif
#ifdef B600
	{ B600,	600 },
#endif
#ifdef B1200
	{ B1200, 1200 },
#endif
#ifdef B1800
	{ B1800, 1800 },
#endif
#ifdef B2400
	{ B2400, 2400 },
#endif
#ifdef B4800
	{ B4800, 4800 },
#endif
#ifdef B7200
	{ B7200, 7200 },
#endif
#ifdef B9600
	{ B9600, 9600 },
#endif
#ifdef B14400
	{ B14400, 14400 },
#endif
#ifdef B19200
	{ B19200, 19200 },
#endif
#ifdef B38400
	{ B38400, 38400 },
#endif
#ifdef B28800
	{ B28800, 28800 },
#endif
#ifdef B57600
	{ B57600, 57600 },
#endif
#ifdef B76800
	{ B76800, 76800 },
#endif
#ifdef B115200
	{ B115200, 115200 },
#endif
#ifdef B230400
	{ B230400, 230400 },
#endif
};
#define NSPEEDS (sizeof(speeds) / sizeof(speeds[0]))

int
val2speed(int val)
{
	int n;
	struct speeds *sp;

	for (sp = speeds, n = NSPEEDS; n > 0; ++sp, --n)
		if (val == sp->val)
			return (sp->speed);

	return (0);
}

#ifdef USESOCKETS
int
clientconnect(void)
{
	struct sockaddr_in sin;
	int		cc, length = sizeof(sin);
	int		newfd;
	secretkey_t     key;
	capret_t	capret;
#ifdef WITHSSL
	int             dorelay = 0, doupload = 0, dooptions = 0;
	int             newspeed = 0;
	speed_t         newsymspeed;
	int             opterr = 0;
	int             ret;
	SSL	       *newssl;
	struct termios  serial_opts;
	char           *caddr;
#endif

	newfd = accept(sockfd, (struct sockaddr *)&sin, &length);
	if (newfd < 0) {
		dolog(LOG_NOTICE, "accept()ing new client: %s", geterr(errno));
		return 1;
	}

	/*
	 * Read the first part to verify the key. We must get the
	 * proper bits or this is not a valid tip connection.
	 */
	if ((cc = read(newfd, &key, sizeof(key))) <= 0) {
		close(newfd);
		dolog(LOG_NOTICE, "%s connecting, error reading key",
		      inet_ntoa(sin.sin_addr));
		return 1;
	}

#ifdef WITHSSL
	if (cc == sizeof(key) && 
	    (0 == strncmp( key.key, "USESSL", 6 ) ||
	     (dorelay = (0 == strncmp( key.key, "RELAY", 5 ))) ||
	     (dooptions = (0 == strncmp( key.key, "OPTIONS", 7 ))) ||
	     (doupload = (0 == strncmp( key.key, "UPLOAD", 6 ))))) {
	  /* 
	     dolog(LOG_NOTICE, "Client %s wants to use SSL",
		inet_ntoa(sin.sin_addr) );
	  */

	  initializessl();
	  /*
	  if ( write( newfd, "OKAY", 4 ) <= 0) {
	    dolog( LOG_NOTICE, "Failed to send OKAY to client." );
	    close( newfd );
	    return 1;
	  }
	  */

	  newssl = SSL_new( ctx );
	  if (!newssl) {
	    dolog(LOG_NOTICE, "SSL_new failed.");
	    close(newfd);
	    return 1;
	  }	    
	    
	  if ((ret = SSL_set_fd( newssl, newfd )) <= 0) {
	    dolog(LOG_NOTICE, "SSL_set_fd failed.");
	    close(newfd);
	    return 1;
	  }

	  dolog(LOG_NOTICE, "going to accept" );

	  if ((ret = SSL_accept( newssl )) <= 0) {
	    dolog(LOG_NOTICE, "%s connecting, SSL_accept error.",
		  inet_ntoa(sin.sin_addr));
	    ERR_print_errors_fp( stderr );
	    SSL_free(newssl);
	    close(newfd);
	    return 1;
	  }

	  if (doupload) {
	    strcpy(uptmpnam, _PATH_TMP "capture.upload.XXXXXX");
	    if (upfd >= 0 || !relay_snd || !uploadCommand) {
	      dolog(LOG_NOTICE, "%s upload already connected.",
		    inet_ntoa(sin.sin_addr));
	      SSL_free(newssl);
	      close(newfd);
	      return 1;
	    }
	    else if (sslverify(newssl, "Capture Server")) {
	      SSL_free(newssl);
	      close(newfd);
	      return 1;
	    }
	    else if ((upfilefd = mkstemp(uptmpnam)) < 0) {
	      dolog(LOG_NOTICE, "failed to create upload file");
	      printf(" %s\n", uptmpnam);
	      perror("mkstemp");
	      SSL_free(newssl);
	      close(newfd);
	      return 1;
	    }
	    else {
	      upfd = newfd;
	      upfilesize = 0;
	      FD_SET(upfd, &sfds);
	      if (upfd >= fdcount) {
		fdcount = upfd;
		fdcount += 1;
	      }
	      sslUpload = newssl;
	      if (fcntl(upfd, F_SETFL, O_NONBLOCK) < 0)
		die("fcntl(O_NONBLOCK): %s", geterr(errno));
	      return 0;
	    }
	  }
	  else if (dooptions) {
	      /*
	       * Just handle these quick inline -- then don't have to
	       * worry about multiple option changes cause they all
	       * happen "atomically" from the client point of view.
	       */

	      caddr = inet_ntoa(sin.sin_addr);
	      sscanf(key.key,"OPTIONS SPEED=%d",&newspeed);
	      newsymspeed = val2speed(newspeed);
	      dolog(LOG_NOTICE,"%s changing speed to %d.",
		    caddr,newspeed);
	      if (newspeed == 0) {
		  dolog(LOG_ERR,"%s invalid speed option %d.",
			caddr,newspeed);
		  opterr = 1;
	      }

	      if (opterr == 0 && tcgetattr(devfd,&serial_opts) == -1) {
		  dolog(LOG_ERR,"%s failed to get attrs before speed change: %s.",
			caddr,strerror(errno));
		  opterr = 1;
	      }

	      // XXX testing
	      //serial_opts.c_lflag |= ECHO | ECHONL;

	      if (opterr) {
	      }
	      else if (cfsetispeed(&serial_opts,newsymspeed) == -1) {
		  dolog(LOG_ERR,"%s cfsetispeed(%d) failed: %s.",
			caddr,newspeed,strerror(errno));
		  opterr = 1;
	      }
	      else if (cfsetospeed(&serial_opts,newsymspeed) == -1) {
		  dolog(LOG_ERR,"%s cfsetospeed(%d) failed: %s.",
			caddr,newspeed,strerror(errno));
		  opterr = 1;
	      }
	      else if (tcsetattr(devfd,TCSANOW,&serial_opts) == -1) {
		  dolog(LOG_ERR,"%s tcsetattr(%d) failed: %s.",
			caddr,newspeed,strerror(errno));
		  opterr = 1;
	      }

	      SSL_free(newssl);
	      shutdown(newfd, SHUT_RDWR);
	      close(newfd);
	      return opterr;
	  }
	  else if (dorelay) {
	    if (devfd >= 0) {
	      dolog(LOG_NOTICE, "%s relay already connected.",
		    inet_ntoa(sin.sin_addr));
	      SSL_free(newssl);
	      shutdown(newfd, SHUT_RDWR);
	      close(newfd);
	      return 1;
	    }
	    else if (memcmp(&relayaddr,
			    &sin.sin_addr,
			    sizeof(relayaddr)) != 0) {
	      dolog(LOG_NOTICE, "%s is not the relay host.",
		    inet_ntoa(sin.sin_addr));
	      SSL_free(newssl);
	      shutdown(newfd, SHUT_RDWR);
	      close(newfd);
	      return 1;
	    }
	    else {
	      relayclient = sin;
	      devfd = newfd;
	      sscanf(key.key, "RELAY %d", &upportnum);
	      FD_SET(devfd, &sfds);
	      if (devfd >= fdcount) {
		fdcount = devfd;
		fdcount += 1;
	      }
	      sslRelay = newssl;
	      if (fcntl(devfd, F_SETFL, O_NONBLOCK) < 0)
		die("fcntl(O_NONBLOCK): %s", geterr(errno));
	      createkey();
	      return 0;
	    }
	  }
	  else if (!tipactive) {
	    sslCon = newssl;
	    tipclient = sin;
	    ptyfd = newfd;
	    dolog(LOG_NOTICE, "going to read key" );
	    if ((cc = SSL_read(newssl, (void *)&key, sizeof(key))) <= 0) {
	      ret = cc;
	      close(newfd);
	      dolog(LOG_NOTICE, "%s connecting, error reading capturekey.",
		    inet_ntoa(sin.sin_addr));
	      /*
		{
		FILE * foo = fopen("/tmp/err.txt", "w");
		ERR_print_errors_fp( foo );
		fclose( foo );
		}
	      */
	      close(ptyfd);
	      return 1;
	    }
	  }

	  dolog(LOG_NOTICE, "got key" );
	}
	else if (!tipactive) {
		tipclient = sin;
		ptyfd = newfd;
	}
#endif /* WITHSSL */
	/*
	 * Is there a better way to do this? I suppose we
	 * could shut the main socket down, and recreate
	 * it later when the client disconnects, but that
	 * sounds horribly brutish!
	 */
	if (tipactive) {
		capret = CAPBUSY;
		if ((cc = write(newfd, &capret, sizeof(capret))) <= 0) {
			dolog(LOG_NOTICE, "%s refusing. error writing status",
			      inet_ntoa(tipclient.sin_addr));
		}
		dolog(LOG_NOTICE, "%s connecting, but tip is active",
		      inet_ntoa(tipclient.sin_addr));
		
		close(newfd);
		return 1;
	}
	/* Verify size of the key is sane */
	if (cc != sizeof(key) ||
	    key.keylen != strlen(key.key) ||
	    strncmp(secretkey.key, key.key, key.keylen)) {
		/*
		 * Tell the other side that their key is bad.
		 */
		capret = CAPNOPERM;
#ifdef WITHSSL
		if (sslCon != NULL) {
		    if ((cc = SSL_write(sslCon, (void *)&capret, sizeof(capret))) <= 0) {
		        dolog(LOG_NOTICE, "%s connecting, error perm status",
			      inet_ntoa(tipclient.sin_addr));
		    }
		} else
#endif /* WITHSSL */
		{
		    if ((cc = write(ptyfd, &capret, sizeof(capret))) <= 0) {
		        dolog(LOG_NOTICE, "%s connecting, error perm status",
			      inet_ntoa(tipclient.sin_addr));
		    }
		}
		close(ptyfd);
		dolog(LOG_NOTICE,
		      "%s connecting, secret key does not match",
		      inet_ntoa(tipclient.sin_addr));
		return 1;
	}
#ifndef SAFEMODE
	/* Do not spit this out into a public log file */
	dolog(LOG_INFO, "Key: %d: %s",
	      secretkey.keylen, secretkey.key);
#endif

	/*
	 * Tell the other side that all is okay.
	 */
	capret = CAPOK;
#ifdef WITHSSL
	if (sslCon != NULL) {
	    if ((cc = SSL_write(sslCon, (void *)&capret, sizeof(capret))) <= 0) {
		close(ptyfd);
		dolog(LOG_NOTICE, "%s connecting, error writing status",
		      inet_ntoa(tipclient.sin_addr));
		return 1;
	    }
	} else
#endif /* WITHSSL */
	{
	    if ((cc = write(ptyfd, &capret, sizeof(capret))) <= 0) {
		close(ptyfd);
		dolog(LOG_NOTICE, "%s connecting, error writing status",
		      inet_ntoa(tipclient.sin_addr));
		return 1;
	    }
	}
	
	/*
	 * See Mike comments (use threads) above.
	 */
	if (fcntl(ptyfd, F_SETFL, O_NONBLOCK) < 0)
		die("fcntl(O_NONBLOCK): %s", geterr(errno));

	FD_SET(ptyfd, &sfds);
	if (ptyfd >= fdcount) {
		fdcount = ptyfd;
		fdcount++;
	}
	tipactive = 1;

	dolog(LOG_INFO, "%s connecting", inet_ntoa(tipclient.sin_addr));
	return 0;
}

int
handleupload(void)
{
	int		drop = 0, rc, retval = 0;
	char		buffer[BUFSIZE];

#ifdef WITHSSL
	rc = SSL_read(sslUpload, buffer, sizeof(buffer));
#else
	/* XXX no clue if this is correct */
	rc = read(upfd, buffer, sizeof(buffer));
#endif
	if (rc < 0) {
		if ((errno != EINTR) && (errno != EAGAIN)) {
			drop = 1;
		}
	}
	else if ((upfilesize + rc) > MAX_UPLOAD_SIZE) {
		dolog(LOG_NOTICE, "upload too large");
		drop = 1;
	}
	else if (rc == 0) {
		snprintf(buffer, sizeof(buffer), uploadCommand, uptmpnam);
		dolog(LOG_NOTICE, "upload done");
		drop = 1;
		close(devfd);
		/* XXX run uisp */
		system(buffer);
		rawmode(Devname, speed);
	}
	else {
		write(upfilefd, buffer, rc);
		upfilesize += rc;
	}

	if (drop) {
#ifdef WITHSSL
		SSL_free(sslUpload);
		sslUpload = NULL;
#endif
		FD_CLR(upfd, &sfds);
		close(upfd);
		upfd = -1;
		close(upfilefd);
		upfilefd = -1;
		unlink(uptmpnam);
	}
	
	return retval;
}

/*
 * Generate our secret key and write out the file that local tip uses
 * to do a secure connect.
 */
void
createkey(void)
{
	int			cc, i, fd;
	unsigned char		buf[BUFSIZ];
	FILE		       *fp;

	if (relay_snd)
		return;

	/*
	 * Generate the key. Should probably generate a random
	 * number of random bits ...
	 */
	if ((fd = open("/dev/urandom", O_RDONLY)) < 0) {
		syslog(LOG_ERR, "opening /dev/urandom: %m");
		exit(1);
	}
	
	if ((cc = read(fd, buf, DEFAULTKEYLEN)) != DEFAULTKEYLEN) {
		if (cc < 0)
			syslog(LOG_ERR, "Reading random bits: %m");
		else
			syslog(LOG_ERR, "Reading random bits");
		exit(1);
	}
	close(fd);
	
	/*
	 * Convert into ascii text string since that is easier to
	 * deal with all around.
	 */
	secretkey.key[0] = 0;
	for (i = 0; i < DEFAULTKEYLEN; i++) {
		int len = strlen(secretkey.key);
		
		snprintf(&secretkey.key[len],
			 sizeof(secretkey.key) - len,
			 "%x", (unsigned int) buf[i]);
	}
	secretkey.keylen = strlen(secretkey.key);

#ifndef SAFEMODE
	/* Do not spit this out into a public log file */
	dolog(LOG_INFO, "NewKey: %d: %s", secretkey.keylen, secretkey.key);
#endif
	
	/*
	 * First write out the info locally so local tip can connect.
	 * This is still secure in that we rely on unix permission, which
	 * is how most of our security is based anyway.
	 */

	/*
	 * We want to control the mode bits when this file is created.
	 * Sure, could change the umask, but I hate that function.
	 */
	(void) unlink(Aclname);
	if ((fd = open(Aclname, O_WRONLY|O_CREAT|O_TRUNC, 0600)) < 0)
		die("%s: open: %s", Aclname, geterr(errno));

	/*
	 * Set owner/group of the new run file. Avoid race in which a
	 * user can get the new file before the chmod, by creating 0600
	 * and doing the chmod after.
	 */
	if (fchown(fd, tipuid, tipgid) < 0)
		die("%s: fchown: %s", Runname, geterr(errno));
	if (fchmod(fd, 0640) < 0)
		die("%s: fchmod: %s", Runname, geterr(errno));
	
	if ((fp = fdopen(fd, "w")) == NULL)
		die("fdopen(%s)", Aclname, geterr(errno));

	fprintf(fp, "host:   %s\n", ourhostname);
	fprintf(fp, "port:   %d\n", portnum);
	if (upportnum > 0) {
		fprintf(fp, "uphost: %s\n", inet_ntoa(relayaddr));
		fprintf(fp, "upport: %d\n", upportnum);
	}
	fprintf(fp, "keylen: %d\n", secretkey.keylen);
	fprintf(fp, "key:    %s\n", secretkey.key);
	fclose(fp);

	/*
	 * Send the info over.
	 */
	(void) handshake();
}

/*
 * Contact the boss node and tell it our local port number and the secret
 * key we are using.
 */
static	jmp_buf deadline;
static	int	deadbossflag;

void
deadboss(int sig)
{
	deadbossflag = 1;
	longjmp(deadline, 1);
}

/*
 * Tell the capserver our new secret key, and receive the setup info
 * back (owner/group of the tty/acl/run file). The handshake might be
 * delayed, so we continue to operate, and when we do handshake, set
 * the files properly.
 */
int
handshake(void)
{
	int			sock, cc, err = 0;
	whoami_t		whoami;
	tipowner_t		tipown;

	/*
	 * In standalone, do not contact the capserver.
	 */
	if (standalone || relay_snd)
		return err;

	/*
	 * Global. If we fail, we keep trying from the main loop. This
	 * allows local tip to operate while still trying to contact the
	 * server periodically so remote tip can connect.
	 */
	needshake = 1;

	/*
	 * Don't do this if a local tip session is active. Typically, it
	 * means something is wrong, and that it would be a bad idea to
	 * interrupt a tip session without a potentially blocking operation,
	 * as that would really annoy the user. When the tip goes inactive,
	 * we will try again normally. 
	 */
	if (tipactive)
	    return 0;

	/* Our whoami info. */
	strcpy(whoami.name, Machine);
	whoami.portnum = portnum;
	memcpy(&whoami.key, &secretkey, sizeof(secretkey));

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		die("socket(): %s", geterr(errno));
	}

	/*
	 * Bind to a reserved port so that capserver can verify integrity
	 * of the sender by looking at the port number. The actual port
	 * number does not matter.
	 */
	if (bindresvport(sock, NULL) < 0) {
		warning("Could not bind reserved port");
		close(sock);
		return -1;
	}

	/* For alarm. */
	deadbossflag = 0;
	signal(SIGALRM, deadboss);
	if (setjmp(deadline)) {
		alarm(0);
		signal(SIGALRM, SIG_DFL);
		warning("Timed out connecting to %s\n", Bossnode);
		close(sock);
		return -1;
	}
	alarm(5);

	if (connect(sock, (struct sockaddr *)&Bossaddr, sizeof(Bossaddr)) < 0){
		warning("connect(%s): %s", Bossnode, geterr(errno));
		err = -1;
		close(sock);
		goto done;
	}

	if ((cc = write(sock, &whoami, sizeof(whoami))) != sizeof(whoami)) {
		if (cc < 0)
			warning("write(%s): %s", Bossnode, geterr(errno));
		else
			warning("write(%s): Failed", Bossnode);
		err = -1;
		close(sock);
		goto done;
	}
	
	if ((cc = read(sock, &tipown, sizeof(tipown))) != sizeof(tipown)) {
		if (cc < 0)
			warning("read(%s): %s", Bossnode, geterr(errno));
		else
			warning("read(%s): Failed", Bossnode);
		err = -1;
		close(sock);
		goto done;
	}
	close(sock);

	/*
	 * Now that we have owner/group info, set the runfile and aclfile.
	 */
	tipuid = tipown.uid;
	tipgid = tipown.gid;
	if (runfile && chown(Runname, tipuid, tipgid) < 0)
		die("%s: chown: %s", Runname, geterr(errno));

	if (chown(Aclname, tipuid, tipgid) < 0)
		die("%s: chown: %s", Aclname, geterr(errno));

	dolog(LOG_INFO,
	      "Handshake complete. Owner %d, Group %d", tipuid, tipgid);
	needshake = 0;

 done:
	alarm(0);
	signal(SIGALRM, SIG_DFL);
	return err;
}
#endif
