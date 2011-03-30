/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2004, 2006 University of Utah and the Flux Group.
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

#ifndef NO_EMULAB
#include "config.h"
#include "tmcc.h"
#endif

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
 " -h              Display this message\n"
 " -V              Print version information and exit\n"
 " -n name         Optional barrier name; must be less than 64 bytes long\n"
 " -d              Turn on debugging\n"
 " -s server       Specify a sync server to connect to\n"
 " -p portnum      Specify a port number to connect to\n"
 " -i count        Increase the barrier value by 'count' waiters\n"
 " -a              Master returns immediately when initializing barrier\n"
 " -u              Use UDP instead of TCP\n"
 " -e errornum     Signal an error (must be zero or >= 32)\n"
 " -m              Test whether or not this is the 'master' node\n"
 "\n";

void
usage()
{
	fprintf(stderr, "%s", usagestr);
	exit(64);
}

int
main(int argc, char **argv)
{
	int			n, ch, errval = 0;
	struct hostent		*he;
	unsigned char		**hal;
	FILE			*fp;
	struct in_addr		serverip;
	int			portnum = 0;
	int			useudp  = 0;
	char			*server = NULL;
	int			initme  = 0;
	int			nowait  = 0;
	char			*barrier= DEFAULT_BARRIER;
	barrier_req_t		barrier_req;
#ifndef NO_EMULAB
	extern char		build_info[];
#endif
	int			mastercheck = 0;
	
	while ((ch = getopt(argc, argv, "hVds:p:ui:n:ae:m")) != -1)
		switch(ch) {
		case 'a':
			nowait++;
			break;
		case 'd':
			debug++;
			break;
		case 'p':
			if (sscanf(optarg, "%d", &portnum) == 0) {
				fprintf(stderr,
					"Error: -p value is not a number: "
					"%s\n",
					optarg);
				usage();
			}
			else if ((portnum <= 0) || (portnum >= 65536)) {
				fprintf(stderr,
					"Error: -p value is not between "
					"0 and 65536: %d\n",
					portnum);
				usage();
			}
			break;
		case 'i':
			if (sscanf(optarg, "%d", &initme) == 0) {
				fprintf(stderr,
					"Error: -i value is not a number: "
					"%s\n",
					optarg);
				usage();
			}
			else if (initme <= 0) {
				fprintf(stderr,
					"Error: -i value must be greater "
					"than zero: %s\n",
					optarg);
				usage();
			}
			break;
		case 'n':
			if (strlen(optarg) == 0) {
				fprintf(stderr, "Error: -n value is empty\n");
				usage();
			}
			else if (strlen(optarg) >= 64) {
				fprintf(stderr,
					"Error: -n value is too long: %s\n",
					optarg);
				usage();
			}
			else {
				barrier = optarg;
			}
			break;
		case 's':
			if (strlen(optarg) == 0) {
				fprintf(stderr, "Error: -s value is empty\n");
				usage();
			}
			else {
				server = optarg;
			}
			break;
		case 'u':
			useudp = 1;
			break;
		case 'e':
			if (sscanf(optarg, "%d", &errval) == 0) {
				fprintf(stderr,
					"Error: -e value is not a number: "
					"%s\n",
					optarg);
				usage();
			}
			else if ((errval < 0) ||
				 (errval >= SERVER_ERROR_BASE)) {
				fprintf(stderr,
					"Error: -e value must be zero or "
					"less than %d\n",
					SERVER_ERROR_BASE);
				usage();
			}
			break;
		case 'V':
#ifndef NO_EMULAB
			fprintf(stderr, "%s\n", build_info);
#else
                        fprintf(stderr, "Built in standalone mode\n");
#endif
			exit(0);
			break;
		case 'h':
			fprintf(stderr, "%s", usagestr);
			exit(0);
			break;
		case 'm':
			mastercheck = 1;
			break;
		default:
			usage();
		}

	if (nowait && (initme == 0)) {
		fprintf(stderr,	"Error: -a requires the -i option\n");
		usage();
	}
	
	argv += optind;
	argc -= optind;

	if (argc) {
		fprintf(stderr,
			"Error: Unrecognized command line arguments: %s ...\n",
			argv[0]);
		usage();
	}

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
		fprintf(stderr,
			"Error: Could not deduce the name of the syncserver!\n"
			"You may be running in an older experiment, you\n"
			"will have to manually start the emulab-syncd and\n"
			"use the -s option, or, you can recreate your\n"
			"experiment.\n\n");
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
		exit(68);
	}
	if (debug) {
		printf( "server name %s, addr(s)", server);
		for (hal = (unsigned char **)he->h_addr_list; *hal; hal++)
			printf(" %u.%u.%u.%u\n", 
			       (*hal)[0], (*hal)[1],(*hal)[2], (*hal)[3]);
	}

	if (mastercheck) {
		/* Compare server IP with the current host IP addresses.
		 * (Windows nodes all have the physical node name.)
		 */
		char nodename[BUFSIZ];
		gethostname(nodename, sizeof(nodename));
		he = gethostbyname(nodename); /* Internal retval storage. */
		if (!he) {
			fprintf(stderr, "gethostbyname(%s) failed\n", nodename); 
			exit(68);
		}
		if (debug) {
			printf( "node name %s, addr(s)", nodename);
			for (hal = (unsigned char **)he->h_addr_list; *hal; hal++)
				printf(" %u.%u.%u.%u\n",
				       (*hal)[0], (*hal)[1],(*hal)[2], (*hal)[3]);
		}
		/* Host is multi-homed.  Compare to ALL of its addresses. */
		for (hal = (unsigned char **)he->h_addr_list; *hal; hal++)
			if (memcmp((char *)&serverip, *hal, he->h_length) == 0)
				exit(0);
		exit(1);
	}

	/*
	 * Lookup the port.
	 */
	if (portnum == 0) {
#ifndef NO_EMULAB
		char nodename[BUFSIZ];
		int  rc;

		while (1) {
			rc = PortLookup(SERVER_SERVNAME, nodename,
					sizeof(nodename), &portnum);

			/* Got the port; we do not care about the nodename */
			if (rc == 0 && portnum != 0) {
				if (debug) {
					printf("Sync Server at %s:%d\n",
					       nodename, portnum);
				}
				break;
			}

			if (rc == 0 && portnum == 0) {
				fprintf(stderr,
					"Could not lookup service with tmcd!");
				exit(1);
			}

			/* Delay */
			sleep(5);
		}
#else
                fprintf(stderr, "No port specified\n");
                exit(1);
#endif
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
	barrier_req.error = errval;
	
	if (useudp)
		n = doudp(&barrier_req, serverip, portnum);
	else
		n = dotcp(&barrier_req, serverip, portnum);
	
	if (n < 0)
		exit(76); /* Protocol failure */
	else
		exit(n); /* User defined error. */

	assert(0); /* Not reached. */
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

	bp = buf;
	n = 0;
	memset(buf, 0, sizeof(buf));
	while (1) {
		if ((cc = read(sock, bp, sizeof(buf) - n)) <= 0) {
			if (cc < 0) {
				perror("Reading from socket:");
				goto bad;
			}
			break;
		}
		else {
			bp += cc;
			n += cc;
		}
	}
	close(sock);

	if (n == 4) {
		return *((int *)buf);
	}
	else {
		return -1;
	}
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

	if (cc == 4) {
		return *((int *)buf);
	}
	else {
		return -1;
	}
}

void
deadsyncd(int sig)
{
	deadsyncdflag = 1;
	longjmp(deadline, 1);
}
