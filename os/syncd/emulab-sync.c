/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <setjmp.h>
#include "decls.h"
#include "config.h"

#ifndef SYNCSERVER
#define SYNCSERVER	"/var/emulab/boot/syncserver"
#endif

static int	debug = 0;

/* Forward decls */
static int	doudp(barrier_req_t *, struct in_addr, int);
static int	dotcp(barrier_req_t *, struct in_addr, int);

/* For connect timeouts. */
static jmp_buf	deadline;
static int	deadsyncdflag;
static void	deadsyncd(int sig);

char *usagestr = 
 "usage: emulab-sync [options]\n"
 " -n <name>       Optional barrier name; must be less than 64 bytes long\n"
 " -d              Turn on debugging\n"
 " -s server       Specify a sync server to connect to\n"
 " -p portnum      Specify a port number to connect to\n"
 " -i count        Initialize named barrier to count waiters\n"
 " -a              Master returns immediately when initializing barrier\n"
 " -u              Use UDP instead of TCP\n"
 "\n";

void
usage()
{
	fprintf(stderr, usagestr);
	exit(1);
}

int
main(int argc, char **argv)
{
	int			n, ch;
	struct hostent		*he;
	FILE			*fp;
	struct in_addr		serverip;
	int			portnum = SERVER_PORTNUM;
	int			useudp  = 0;
	char			*server = NULL;
	int			initme  = 0;
	int			nowait  = 0;
	char			*barrier= DEFAULT_BARRIER;
	barrier_req_t		barrier_req;
	
	while ((ch = getopt(argc, argv, "ds:p:ui:n:a")) != -1)
		switch(ch) {
		case 'a':
			nowait++;
			break;
		case 'd':
			debug++;
			break;
		case 'p':
			portnum = atoi(optarg);
			break;
		case 'i':
			initme = atoi(optarg);
			break;
		case 'n':
			barrier = optarg;
			break;
		case 's':
			server = optarg;
			break;
		case 'u':
			useudp = 1;
			break;
		default:
			usage();
		}

	argv += optind;
	argc -= optind;

	if (argc)
		usage();

	if (strlen(barrier) >= sizeof(barrier_req.name))
		usage();

	/*
	 * Look for the syncserver access file.
	 */
	if (!server &&
	    (access(SYNCSERVER, R_OK) == 0) &&
	    ((fp = fopen(SYNCSERVER, "r")) != NULL)) {
		char	buf[BUFSIZ], *bp;
		
		if (fgets(buf, sizeof(buf), fp)) {
			if ((bp = strchr(buf, '\n')))
				*bp = (char) NULL;

			/*
			 * Look for port spec
			 */
			if ((bp = strchr(buf, ':'))) {
				*bp++   = (char) NULL;
				portnum = atoi(bp);
			}
			server = strdup(buf);
		}
		fclose(fp);
	}
	if (!server) {
		fprintf(stderr, "Must provide name of the syncserver!\n\n");
		usage();
	}
	
	/*
	 * Map server to IP.
	 */
	he = gethostbyname(server);
	if (he)
		memcpy((char *)&serverip, he->h_addr, he->h_length);
	else {
		fprintf(stderr, "gethostbyname(%s) failed\n", server); 
		exit(1);
	}

	/*
	 * Build up the request structure to send to the server.
	 */
	bzero(&barrier_req, sizeof(barrier_req));
	strcpy(barrier_req.name, barrier);
	if (initme) {
		barrier_req.request = BARRIER_INIT;
		barrier_req.count   = initme;
		if (nowait)
			barrier_req.flags = BARRIER_INIT_NOWAIT;
	}
	else
		barrier_req.request = BARRIER_WAIT;
	
	if (useudp)
		n = doudp(&barrier_req, serverip, portnum);
	else
		n = dotcp(&barrier_req, serverip, portnum);
	exit(n);
}

/*
 * TCP version, which uses ssl if compiled in.
 */
static int
dotcp(barrier_req_t *barrier_reqp, struct in_addr serverip, int portnum)
{
	int			sock, n, cc;
	struct sockaddr_in	name;
	char			*bp, buf[BUFSIZ];
	
	while (1) {
		/* Create socket from which to read. */
		sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock < 0) {
			perror("creating stream socket:");
			return -1;
		}

		/* Create name. */
		name.sin_family = AF_INET;
		name.sin_addr   = serverip;
		name.sin_port   = htons(portnum);

		/* For alarm. */
		deadsyncdflag = 0;
		signal(SIGALRM, deadsyncd);
		if (setjmp(deadline)) {
			alarm(0);
			signal(SIGALRM, SIG_DFL);
			goto refused;
		}
		alarm(5);

		cc = connect(sock, (struct sockaddr *) &name, sizeof(name));
		/*
		 * If we get the alarm now, no problem. We will close the
		 * connection and try again after a short wait. The server
		 * will recongnize this for what is, not recording any state
		 * until we actually send it the request below (write to sock).
		 */
		alarm(0);
		signal(SIGALRM, SIG_DFL);

		/* Success */
		if (cc == 0)
			break;

		if (errno != ECONNREFUSED && errno != ETIMEDOUT) {
			perror("connecting stream socket");
			close(sock);
			return -1;
		}
	refused:
		close(sock);
		if (debug) 
			fprintf(stderr, "Connection refused. Waiting ...\n");
		sleep(5);
	}

	n = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &n, sizeof(n)) < 0) {
		perror("setsockopt SO_KEEPALIVE");
		goto bad;
	}

	/*
	 * Write the command to the socket and wait for the response.
	 */
	bp = (char *) barrier_reqp;
	n  = sizeof(*barrier_reqp);
	while (n) {
		if ((cc = write(sock, bp, n)) <= 0) {
			if (cc < 0) {
				perror("Writing to socket:");
				goto bad;
			}
			fprintf(stderr, "write aborted");
			goto bad;
		}
		bp += cc;
		n  -= cc;
	}

	while (1) {
		if ((cc = read(sock, buf, sizeof(buf) - 1)) <= 0) {
			if (cc < 0) {
				perror("Reading from socket:");
				goto bad;
			}
			break;
		}
	}
	close(sock);
	return 0;
 bad:
	close(sock);
	return -1;
}

/*
 * Not very robust ...
 */
static int
doudp(barrier_req_t *barrier_reqp, struct in_addr serverip, int portnum)
{
	int			sock, length, n, cc;
	struct sockaddr_in	name, client;
	char			buf[BUFSIZ];

	/* Create socket from which to read. */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("creating dgram socket:");
		return -1;
	}

	/* Create name. */
	name.sin_family = AF_INET;
	name.sin_addr   = serverip;
	name.sin_port   = htons(portnum);

	/*
	 * Write the command to the socket and wait for the response
	 */
	n  = sizeof(*barrier_reqp);
	cc = sendto(sock, barrier_reqp,
		    n, 0, (struct sockaddr *)&name, sizeof(name));
	if (cc != n) {
		if (cc < 0) {
			perror("Writing to socket:");
			return -1;
		}
		fprintf(stderr, "short write (%d != %d)\n", cc, n);
		return -1;
	}

	cc = recvfrom(sock, buf, sizeof(buf) - 1, 0,
		      (struct sockaddr *)&client, &length);

	if (cc < 0) {
		perror("Reading from socket:");
		return -1;
	}
	close(sock);
	return 0;
}

void
deadsyncd(int sig)
{
	deadsyncdflag = 1;
	longjmp(deadline, 1);
}
