/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2008 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/fcntl.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <paths.h>
#include <setjmp.h>
#include <pwd.h>
#include <grp.h>
#include "decls.h"
#include "config.h"
#include "ssl.h"
#include "log.h"
#include "tbdefs.h"
#include "bootwhat.h"
#include "bootinfo.h"
#include "libtmcd.h"

#define RUNASUSER	"nobody"
#define RUNASGROUP	"nobody"

/* socket read/write timeouts in ms */
#define READTIMO	3000
#define WRITETIMO	3000

#define TESTMODE

int		debug = 0;
static int	verbose = 0;
static int	insecure = 0;
static int	byteswritten = 0;
static struct in_addr myipaddr;
static void	tcpserver(int sock, int portnum);
static void	udpserver(int sock, int portnum);
static int      handle_request(int, struct sockaddr_in *, char *, int);
static int	makesockets(int portnum, int *udpsockp, int *tcpsockp);
int		client_writeback(int sock, void *buf, int len, int tcp);
void		client_writeback_done(int sock, struct sockaddr_in *client);

/* socket timeouts */
static int	readtimo = READTIMO;
static int	writetimo = WRITETIMO;

/* thread support */
#define MAXCHILDREN	20
#define MINCHILDREN	8
static int	numchildren;
static int	maxchildren       = 13;
static int      num_udpservers    = 3;
static int      num_altudpservers = 1;
static int      num_alttcpservers = 1;
static int	mypid;
static volatile int killme;



char *usagestr = 
 "usage: tmcd [-d] [-p #]\n"
 " -d              Turn on debugging. Multiple -d options increase output\n"
 " -p portnum	   Specify a port number to listen on\n"
 " -c num	   Specify number of servers (must be %d <= x <= %d)\n"
 " -v              More verbose logging\n"
 " -i ipaddr       Sets the boss IP addr to return (for multi-homed servers)\n"
 "\n";

void
usage()
{
	fprintf(stderr, usagestr, MINCHILDREN, MAXCHILDREN);
	exit(1);
}

static void
cleanup()
{
	signal(SIGHUP, SIG_IGN);
	killme = 1;
	killpg(0, SIGHUP);
}

static void
setverbose(int sig)
{
	signal(sig, SIG_IGN);
	
	if (sig == SIGUSR1)
		verbose = 1;
	else
		verbose = 0;
	info("verbose logging turned %s\n", verbose ? "on" : "off");

	/* Just the parent sends this */
	if (numchildren)
		killpg(0, sig);
	signal(sig, setverbose);
}

int
main(int argc, char **argv)
{
	int			tcpsock, udpsock, i, ch;
	int			alttcpsock, altudpsock;
	int			status, pid;
	int			portnum = TBSERVER_PORT;
	FILE			*fp;
	char			buf[BUFSIZ];
	struct hostent		*he;
	extern char		build_info[];
	int			server_counts[4]; /* udp,tcp,altudp,alttcp */
	struct {
		int	pid;
		int	which;
	} servers[MAXCHILDREN];

	while ((ch = getopt(argc, argv, "dp:c:Xvi:")) != -1)
		switch(ch) {
		case 'p':
			portnum = atoi(optarg);
			break;
		case 'd':
			debug++;
			break;
		case 'c':
			maxchildren = atoi(optarg);
			break;
		case 'X':
			insecure = 1;
			break;
		case 'v':
			verbose++;
			break;
		case 'i':
			if (inet_aton(optarg, &myipaddr) == 0) {
				fprintf(stderr, "invalid IP address %s\n",
					optarg);
				usage();
			}
			break;
		case 'h':
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc)
		usage();
	if (maxchildren < MINCHILDREN || maxchildren > MAXCHILDREN)
		usage();

#ifdef  WITHSSL
	if (tmcd_server_sslinit()) {
		error("SSL init failed!\n");
		exit(1);
	}
#endif
	if (debug) 
		loginit(0, 0);
	else {
		/* Become a daemon */
		daemon(0, 0);
		loginit(1, "tmcd");
	}
	info("daemon starting (version %d)\n", CURRENT_VERSION);
	info("%s\n", build_info);

	/*
	 * Grab our IP for security check below.
	 */
	if (myipaddr.s_addr == 0) {
#ifdef	LBS
		strcpy(buf, BOSSNODE);
#else
		if (gethostname(buf, sizeof(buf)) < 0)
			pfatal("getting hostname");
#endif
		if ((he = gethostbyname(buf)) == NULL) {
			error("Could not get IP (%s) - %s\n",
			      buf, hstrerror(h_errno));
			exit(1);
		}
		memcpy((char *)&myipaddr, he->h_addr, he->h_length);
	}

	/*
	 * If we were given a port on the command line, don't open the 
	 * alternate ports
	 */
	if (portnum != TBSERVER_PORT) {
	    if (makesockets(portnum, &udpsock, &tcpsock) < 0) {
		error("Could not make sockets!");
		exit(1);
	    }
	    num_alttcpservers = num_altudpservers = 0;
	} else {
	    if (makesockets(portnum, &udpsock, &tcpsock) < 0 ||
		makesockets(TBSERVER_PORT2, &altudpsock, &alttcpsock) < 0) {
		    error("Could not make sockets!");
		    exit(1);
	    }
	}

	signal(SIGTERM, cleanup);
	signal(SIGINT, cleanup);
	signal(SIGHUP, cleanup);
	signal(SIGUSR1, setverbose);
	signal(SIGUSR2, setverbose);

	/*
	 * Stash the pid away.
	 */
	mypid = getpid();
	sprintf(buf, "%s/tmcd.pid", _PATH_VARRUN);
	fp = fopen(buf, "w");
	if (fp != NULL) {
		fprintf(fp, "%d\n", mypid);
		(void) fclose(fp);
	}

	/*
	 * Change to non-root user!
	 */
	if (geteuid() == 0) {
		struct passwd	*pw;
		uid_t		uid;
		gid_t		gid;

		/*
		 * Must be a valid user of course.
		 */
		if ((pw = getpwnam(RUNASUSER)) == NULL) {
			error("invalid user: %s", RUNASUSER);
			exit(1);
		}
		uid = pw->pw_uid;
		gid = pw->pw_gid;

		if (setgroups(1, &gid)) {
			errorc("setgroups");
			exit(1);
		}
		if (setgid(gid)) {
			errorc("setgid");
			exit(1);
		}
		if (setuid(uid)) {
			errorc("setuid");
			exit(1);
		}
		info("Flipped to user/group %d/%d\n", uid, gid);
	}

	/*
	 * Now fork a set of children to handle requests. We keep the
	 * pool at a set level. There are 4 types of servers, each getting
	 * a different number of servers. We do it this cause otherwise
	 * we have to deal with the select storm problem; a bunch of processes
	 * select on the same set of file descriptors, and all get woken up
	 * when something comes in, then all read from the socket but only
	 * one gets it and the others go back to sleep. There are various ways
	 * to deal with this problem, but all of them are a lot more code!
	 */
	server_counts[0] = num_udpservers;
	server_counts[1] = num_altudpservers;
	server_counts[2] = num_alttcpservers;
	server_counts[3] = maxchildren -
		(num_udpservers + num_altudpservers + num_altudpservers);
	bzero(servers, sizeof(servers));
	
	while (1) {
		while (!killme && numchildren < maxchildren) {
			int which = 3;
			
			/*
			 * Find which kind of server is short one.
			 */
			for (i = 0; i < 4; i++) {
				if (server_counts[i]) {
					which = i;
					break;
				}
			}
			
			if ((pid = fork()) < 0) {
				errorc("forking server");
				goto done;
			}
			if (pid) {
				server_counts[which]--;
				/*
				 * Find free slot
				 */
				for (i = 0; i < maxchildren; i++) {
					if (!servers[i].pid)
						break;
				}
				servers[i].pid   = pid;
				servers[i].which = which;
				numchildren++;
				continue;
			}
			/* Poor way of knowing parent/child */
			numchildren = 0;
			mypid = getpid();
			
			/* Child does useful work! Never Returns! */
			signal(SIGTERM, SIG_DFL);
			signal(SIGINT, SIG_DFL);
			signal(SIGHUP, SIG_DFL);
			
			switch (which) {
			case 0: udpserver(udpsock, portnum);
				break;
			case 1: udpserver(altudpsock, TBSERVER_PORT2);
				break;
			case 2: tcpserver(alttcpsock, TBSERVER_PORT2);
				break;
			case 3: tcpserver(tcpsock, portnum);
				break;
			}
			exit(-1);
		}

		/*
		 * Parent waits.
		 */
		pid = waitpid(-1, &status, 0);
		if (pid < 0) {
			errorc("waitpid failed");
			continue;
		} 
		if (WIFSIGNALED(status)) {
			error("server %d exited with signal %d!\n",
			      pid, WTERMSIG(status));
		}
		else if (WIFEXITED(status)) {
			error("server %d exited with status %d!\n",
			      pid, WEXITSTATUS(status));	  
		}
		numchildren--;

		/*
		 * Figure out which and what kind of server it was that died.
		 */
		for (i = 0; i < maxchildren; i++) {
			if (servers[i].pid == pid) {
				servers[i].pid = 0;
				server_counts[servers[i].which]++;
				break;
			}
		}
		if (killme && !numchildren)
			break;
	}
 done:
	CLOSE(tcpsock);
	close(udpsock);
	info("daemon terminating\n");
	exit(0);
}

/*
 * Create sockets on specified port.
 */
static int
makesockets(int portnum, int *udpsockp, int *tcpsockp)
{
	struct sockaddr_in	name;
	int			length, i, udpsock, tcpsock;

	/*
	 * Setup TCP socket for incoming connections.
	 */

	/* Create socket from which to read. */
	tcpsock = socket(AF_INET, SOCK_STREAM, 0);
	if (tcpsock < 0) {
		pfatal("opening stream socket");
	}

	i = 1;
	if (setsockopt(tcpsock, SOL_SOCKET, SO_REUSEADDR,
		       (char *)&i, sizeof(i)) < 0)
		pwarning("setsockopt(SO_REUSEADDR)");;
	
	/* Create name. */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons((u_short) portnum);
	if (bind(tcpsock, (struct sockaddr *) &name, sizeof(name))) {
		pfatal("binding stream socket");
	}
	/* Find assigned port value and print it out. */
	length = sizeof(name);
	if (getsockname(tcpsock, (struct sockaddr *) &name, &length)) {
		pfatal("getsockname");
	}
	if (listen(tcpsock, 128) < 0) {
		pfatal("listen");
	}
	info("listening on TCP port %d\n", ntohs(name.sin_port));
	
	/*
	 * Setup UDP socket
	 */

	/* Create socket from which to read. */
	udpsock = socket(AF_INET, SOCK_DGRAM, 0);
	if (udpsock < 0) {
		pfatal("opening dgram socket");
	}

	i = 1;
	if (setsockopt(udpsock, SOL_SOCKET, SO_REUSEADDR,
		       (char *)&i, sizeof(i)) < 0)
		pwarning("setsockopt(SO_REUSEADDR)");;

	i = 128 * 1024;
	if (setsockopt(udpsock, SOL_SOCKET, SO_RCVBUF, &i, sizeof(i)) < 0)
		pwarning("setsockopt(SO_RCVBUF)");
	
	/* Create name. */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons((u_short) portnum);
	if (bind(udpsock, (struct sockaddr *) &name, sizeof(name))) {
		pfatal("binding dgram socket");
	}

	/* Find assigned port value and print it out. */
	length = sizeof(name);
	if (getsockname(udpsock, (struct sockaddr *) &name, &length)) {
		pfatal("getsockname");
	}
	info("listening on UDP port %d\n", ntohs(name.sin_port));

	*tcpsockp = tcpsock;
	*udpsockp = udpsock;
	return 0;
}

/*
 * Listen for UDP requests. This is not a secure channel, and so this should
 * eventually be killed off.
 */
static void
udpserver(int sock, int portnum)
{
	char			buf[MYBUFSIZE];
	struct sockaddr_in	client;
	int			length, cc;
	unsigned int		nreq = 0;
	
	info("udpserver starting: pid=%d sock=%d portnum=%d\n",
	     mypid, sock, portnum);

	/*
	 * Wait for udp connections.
	 */
	while (1) {
		setproctitle("UDP %d: %u done", portnum, nreq);
		length = sizeof(client);		
		cc = recvfrom(sock, buf, sizeof(buf) - 1,
			      0, (struct sockaddr *)&client, &length);
		if (cc <= 0) {
			if (cc < 0)
				errorc("Reading UDP request");
			error("UDP Connection aborted\n");
			continue;
		}
		buf[cc] = '\0';
		handle_request(sock, &client, buf, 0);
		nreq++;
	}
	exit(1);
}

int
tmcd_accept(int sock, struct sockaddr *addr, socklen_t *addrlen, int ms)
{
	int	newsock;

	if ((newsock = accept(sock, addr, addrlen)) < 0)
		return -1;

	/*
	 * Set timeout value to keep us from hanging due to a
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

	return newsock;
}

/*
 * Listen for TCP requests.
 */
static void
tcpserver(int sock, int portnum)
{
	char			buf[MAXTMCDPACKET];
	struct sockaddr_in	client;
	int			length, cc, newsock;
	unsigned int		nreq = 0;
	struct timeval		tv;
	
	info("tcpserver starting: pid=%d sock=%d portnum=%d\n",
	     mypid, sock, portnum);

	/*
	 * Wait for TCP connections.
	 */
	while (1) {
		setproctitle("TCP %d: %u done", portnum, nreq);
		length  = sizeof(client);
		newsock = ACCEPT(sock, (struct sockaddr *)&client, &length,
				 readtimo);
		if (newsock < 0) {
			errorc("accepting TCP connection");
			continue;
		}

		/*
		 * Set write timeout value to keep us from hanging due to a
		 * malfunctioning or malicious client.
		 * NOTE: ACCEPT function sets read timeout.
		 */
		tv.tv_sec = writetimo / 1000;
		tv.tv_usec = (writetimo % 1000) * 1000;
		if (setsockopt(newsock, SOL_SOCKET, SO_SNDTIMEO,
			       &tv, sizeof(tv)) < 0) {
			errorc("setting SO_SNDTIMEO");
			CLOSE(newsock);
			continue;
		}

		/*
		 * Read in the command request.
		 */
		if ((cc = READ(newsock, buf, sizeof(buf) - 1)) <= 0) {
			if (cc < 0) {
				if (errno == EWOULDBLOCK)
					errorc("Timeout reading TCP request");
				else
					errorc("Error reading TCP request");
			}
			error("TCP connection aborted\n");
			CLOSE(newsock);
			continue;
		}
		buf[cc] = '\0';
		handle_request(newsock, &client, buf, 1);
		CLOSE(newsock);
		nreq++;
	}
	exit(1);
}


/*
 * Hack to test timeouts and other anomolous situations for tmcc
 */
 /* XXX Ryan finish this */
#if 0
XML_COMMAND_PROTOTYPE(dotmcctest)
{
	char		buf[MYBUFSIZE];
	int		logit;

	logit = verbose;

	if (logit)
		info("TMCCTEST: %s\n", rdata);

	/*
	 * Always allow the test that doesn't tie up a server thread
	 */
	if (strncmp(rdata, "noreply", strlen("noreply")) == 0)
		return 0;

	/*
	 * The rest can tie up a server thread for non-trivial amounts of
	 * time, only allow if debugging.
	 */
	if (!debug) {
		strcpy(buf, "tmcctest disabled\n");
		goto doit;
	}

	/*
	 * Delay reply by the indicated amount of time
	 */
	if (strncmp(rdata, "delayreply", strlen("delayreply")) == 0) {
		int delay = 0;
		if (sscanf(rdata, "delayreply %d", &delay) == 1 &&
		    delay < 20) {
			sleep(delay);
			sprintf(buf, "replied after %d seconds\n", delay);
		} else {
			strcpy(buf, "bogus delay value\n");
		}
		goto doit;
	}

	if (tcp) {
		/*
		 * Reply in pieces
		 */
		if (strncmp(rdata, "splitreply", strlen("splitreply")) == 0) {
			memset(buf, '1', MYBUFSIZE/4);
			buf[MYBUFSIZE/4] = 0;
			client_writeback(sock, buf, strlen(buf), tcp);
			sleep(1);
			memset(buf, '2', MYBUFSIZE/4);
			buf[MYBUFSIZE/4] = 0;
			client_writeback(sock, buf, strlen(buf), tcp);
			sleep(2);
			memset(buf, '4', MYBUFSIZE/4);
			buf[MYBUFSIZE/4] = 0;
			client_writeback(sock, buf, strlen(buf), tcp);
			sleep(4);
			memset(buf, '0', MYBUFSIZE/4);
			buf[MYBUFSIZE/4-1] = '\n';
			buf[MYBUFSIZE/4] = 0;
			logit = 0;
		} else {
			strcpy(buf, "no such TCP test\n");
		}
	} else {
		strcpy(buf, "no such UDP test\n");
	}

 doit:
	client_writeback(sock, buf, strlen(buf), tcp);
	if (logit)
		info("%s", buf);
	return 0;
}
#endif

static int
handle_request(int sock, struct sockaddr_in *client, char *rdata, int istcp)
{
	struct sockaddr_in redirect_client;
	int		   redirect = 0, havekey = 0;
	char		   *cp, *bp;
	char		   privkey[TBDB_FLEN_PRIVKEY];
	char		   buf[MYBUFSIZE];
	int		   i = 0, err = 0;
	int		   version = DEFAULT_VERSION;
	tmcdreq_t	   tmcdreq, *reqp = &tmcdreq;
	tmcdresp_t	   *response = NULL;

	byteswritten = 0;
#ifdef	WITHSSL
	cp = (istcp ? (isssl ? "SSL" : "TCP") : "UDP");
#else
	cp = (istcp ? "TCP" : "UDP");
#endif
	setproctitle("%s: %s", inet_ntoa(client->sin_addr), cp);

	/*
	 * Init the req structure.
	 */
	bzero(reqp, sizeof(*reqp));

	/*
	 * Look for special tags. 
	 */
	bp = rdata;
	while ((bp = strsep(&rdata, " ")) != NULL) {
		/*
		 * Look for PRIVKEY. 
		 */
		if (sscanf(bp, "PRIVKEY=%64s", buf)) {
			for (i = 0; i < strlen(buf); i++){
				if (! isxdigit(buf[i])) {
					info("tmcd client provided invalid "
					     "characters in privkey");
					goto skipit;
				}
			}
			havekey = 1;
			strncpy(privkey, buf, sizeof(privkey));

			if (debug) {
				info("PRIVKEY %s\n", buf);
			}
			continue;
		}


		/*
		 * Look for VERSION. 
		 * Check for clients that are newer than the server
		 * and complain.
		 */
		if (sscanf(bp, "VERSION=%d", &i) == 1) {
			version = i;
			if (version > CURRENT_VERSION) {
				error("version skew on request from %s: "
				      "server=%d, request=%d, "
				      "old TMCD installed?\n",
				      inet_ntoa(client->sin_addr),
				      CURRENT_VERSION, version);
			}
			continue;
		}
		reqp->version = version;

		/*
		 * Look for REDIRECT, which is a proxy request for a
		 * client other than the one making the request. Good
		 * for testing. Might become a general tmcd redirect at
		 * some point, so that we can test new tmcds.
		 */
		if (sscanf(bp, "REDIRECT=%30s", buf)) {
			redirect_client = *client;
			redirect        = 1;
			inet_aton(buf, &client->sin_addr);

			info("REDIRECTED from %s to %s\n",
			     inet_ntoa(redirect_client.sin_addr), buf);

			continue;
		}
		
		/*
		 * Look for VNODE. This is used for virtual nodes.
		 * It indicates which of the virtual nodes (on the physical
		 * node) is talking to us. Currently no perm checking.
		 * Very temporary approach; should be done via a per-vnode
		 * cert or a key.
		 */
		if (sscanf(bp, "VNODEID=%30s", buf)) {
			for (i = 0; i < strlen(buf); i++){
				if (! (isalnum(buf[i]) ||
				       buf[i] == '_' || buf[i] == '-')) {
					info("tmcd client provided invalid "
					     "characters in vnodeid");
					goto skipit;
				}
			}
			reqp->isvnode = 1;
			strncpy(reqp->vnodeid, buf, sizeof(reqp->vnodeid));

			if (debug) {
				info("VNODEID %s\n", buf);
			}
			continue;
		}

		/*
		 * An empty token (two delimiters next to each other)
		 * is indicated by a null string. If nothing matched,
		 * and its not an empty token, it must be the actual
		 * command and arguments. Break out.
		 *
		 * Note that rdata will point to any text after the command.
		 *
		 */
		if (*bp) {
			break;
		}
	}

	reqp->verbose = verbose;
	reqp->debug = debug;

	tmcd_init(reqp, &myipaddr, NULL);

	/*
	 * Map the ip to a nodeid.
	 */
	if (havekey) {
		if ((err = iptonodeid(client->sin_addr, reqp, privkey))) {
			error("No such node with wanode_key [%s]\n", privkey);
			goto skipit;
		}
	}
	else {
		if ((err = iptonodeid(client->sin_addr, reqp, NULL))) {
			if (reqp->isvnode) {
				error("No such vnode %s associated with %s\n",
				      reqp->vnodeid, inet_ntoa(client->sin_addr));
			}
			else {
				error("No such node: %s\n",
				      inet_ntoa(client->sin_addr));
			}
			goto skipit;
	}
	/*
	 * Redirect geni sliver nodes to the tmcd of their origin.
	 * FIXME
	 */
	if (reqp->tmcd_redirect[0]) {
		char	buf[BUFSIZ];

		sprintf(buf, "REDIRECT=%s\n", reqp->tmcd_redirect);
		client_writeback(sock, buf, strlen(buf), istcp);
		goto skipit;
	}

	/*
	 * Redirect is allowed from the local host only!
	 * I use this for testing. See below where I test redirect
	 * if the verification fails. 
	 */
	if (!insecure && redirect &&
	    redirect_client.sin_addr.s_addr != myipaddr.s_addr) {
		char	buf1[32], buf2[32];
		
		strcpy(buf1, inet_ntoa(redirect_client.sin_addr));
		strcpy(buf2, inet_ntoa(client->sin_addr));

		if (verbose)
			info("%s INVALID REDIRECT: %s\n", buf1, buf2);
		goto skipit;
	}

	reqp->istcp = istcp;
#ifdef  WITHSSL
	/*
	 * We verify UDP requests below based on the particular request
	 */
	if (!reqp->istcp)
		goto execute;

	reqp->isssl = isssl;
	/*
	 * If the connection is not SSL, then it must be a local node.
	 */
	if (isssl) {
#if 0
		if (tmcd_sslverify_client(reqp->nodeid, reqp->pclass,
					  reqp->ptype,  reqp->islocal)) {
			error("%s: SSL verification failure\n", reqp->nodeid);
			if (! redirect)
				goto skipit;
		}
#endif
		/*
		 * LBS: I took this test out. This client verification has
		 * always been a pain, and offers very little since since
		 * the private key is not encrypted anyway. Besides, we
		 * do not return any sensitive data via tmcd, just a lot of
		 * goo that would not be of interest to anyone. I will
		 * kill this code at some point.
		 */
		if (0 && tmcd_sslverify_client(NULL, NULL,
					  reqp->ptype,  reqp->islocal)) {
			error("%s: SSL verification failure\n", reqp->nodeid);
			if (! redirect)
				goto skipit;
		}
	}
	else if (reqp->iscontrol) {
		error("%s: Control node connection without SSL!\n",
		      reqp->nodeid);
		if (!insecure)
			goto skipit;
	}
#else
	/*
	 * When not compiled for ssl, do not allow remote connections.
	 */
	if (!reqp->islocal) {
		error("%s: Remote node connection not allowed (Define SSL)!\n",
		      reqp->nodeid);
		if (!insecure)
			goto skipit;
	}
	if (reqp->iscontrol) {
		error("%s: Control node connection not allowed "
		      "(Define SSL)!\n", reqp->nodeid);
		if (!insecure)
			goto skipit;
	}
#endif
	/*
	 * Check for a redirect using the default DB. This allows
	 * for a simple redirect to a secondary DB for testing.
	 * Upon return, the dbname has been changed if redirected.
	 */
	if (checkdbredirect(reqp)) {
		/* Something went wrong */
		goto skipit;
	}

	/*
	 * Do private key check. (Non-plab) widearea nodes must report a
	 * private key. It comes over ssl of course. At present we skip
	 * this check for ron nodes. 
	 *
	 * LBS: I took this test out cause its silly. Will kill the code at
	 * some point.
	 */
	if (0 && !reqp->islocal && !(reqp->isplabdslice || reqp->isplabsvc)) {
		if (!havekey) {
			error("%s: No privkey sent!\n", reqp->nodeid);
			/*
			 * Skip. Okay, the problem is that the nodes out
			 * there are not reporting the key!
			goto skipit;
			 */
		}
		else if (checkprivkey(reqp, client->sin_addr, privkey)) {
			error("%s: privkey mismatch: %s!\n",
			      reqp->nodeid, privkey);
			goto skipit;
		}
	}
execute:
	response = tmcd_handle_request(reqp, sock, bp, rdata);

skipit:
	if (response) {
		client_writeback(sock, response->data, response->length, istcp);
		if (!istcp) 
			client_writeback_done(sock,
				      redirect ? &redirect_client : client);
   	}

#if 0
	if (byteswritten && (command_array[i].flags & F_MINLOG) == 0)
		info("%s: %s wrote %d bytes\n",
		     reqp->nodeid, bp,
		     byteswritten);
#endif


	tmcd_free_response(response);

#if 0
	if (cur_node == NULL) {
		info("%s: INVALID REQUEST\n", reqp->nodeid);
		goto skipit;
	}
#endif


	return 0;
}


/*
 * Lets hear it for global state...Yeah!
 */
static char udpbuf[8192];
static int udpfd = -1, udpix;

/*
 * Write back to client
 */
int
client_writeback(int sock, void *buf, int len, int tcp)
{
	int	cc;
	char	*bufp = (char *) buf;
	
	if (tcp) {
		while (len) {
			if ((cc = WRITE(sock, bufp, len)) <= 0) {
				if (cc < 0) {
					errorc("writing to TCP client");
					return -1;
				}
				error("write to TCP client aborted");
				return -1;
			}
			byteswritten += cc;
			len  -= cc;
			bufp += cc;
		}
	} else {
		if (udpfd != sock) {
			if (udpfd != -1)
				error("UDP reply in progress!?");
			udpfd = sock;
			udpix = 0;
		}
		if (udpix + len > sizeof(udpbuf)) {
			error("client data write truncated");
			len = sizeof(udpbuf) - udpix;
		}
		memcpy(&udpbuf[udpix], bufp, len);
		udpix += len;
	}
	return 0;
}

void
client_writeback_done(int sock, struct sockaddr_in *client)
{
	int err;

	/*
	 * XXX got an error before we wrote anything,
	 * still need to send a reply.
	 */
	if (udpfd == -1)
		udpfd = sock;

	if (sock != udpfd)
		error("UDP reply out of sync!");
	else if (udpix != 0) {
		err = sendto(udpfd, udpbuf, udpix, 0,
			     (struct sockaddr *)client, sizeof(*client));
		if (err < 0)
			errorc("writing to UDP client");
	}
	byteswritten = udpix;
	udpfd = -1;
	udpix = 0;
}

