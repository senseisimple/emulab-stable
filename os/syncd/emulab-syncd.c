/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/fcntl.h>
#include <paths.h>
#include "decls.h"
#include "log.h"

int		debug   = 0;
int		verbose = 0;
static int	portnum = SERVER_PORTNUM;
static int      handle_request(int, struct sockaddr_in *, barrier_req_t *,int);
static int	makesockets(int portnum, int *udpsockp, int *tcpsockp);
static void     release_client(int sock, struct in_addr ip, int istcp);
int		client_writeback(int sock, void *buf, int len, int tcp);
void		client_writeback_done(int sock, struct sockaddr_in *client);

/*
 * This is the barrier structure, one per barrier. For each barrier, a list
 * of waiters, with the socket they are waiting on. Fixed for now, make
 * dynamic later.
 */
#define MAXWAITERS	4096

struct barrier_ctrl {
	char			*name;
	int			count;
	int			index;
	struct in_addr		ipaddr;		/* Debugging */
	struct barrier_ctrl	*next;		/* Linked list */
	struct {
		int		sock;
		int		istcp;
		struct in_addr	ipaddr;		/* Debugging */
	} waiters[MAXWAITERS];
};
typedef struct barrier_ctrl barrier_ctrl_t;
barrier_ctrl_t *allbarriers;

char *usagestr =
 "usage: tmcd [-d] [-p #]\n"
 " -d              Do not daemonize.\n"
 " -v              Turn on verbose logging\n"
 " -l              Specify log file instead of syslog\n"
 " -p portnum	   Specify a port number to listen on\n"
 "\n";

void
usage()
{
	fprintf(stderr, usagestr);
	exit(1);
}

static void
setverbose(int sig)
{
	signal(sig, SIG_IGN);
	
	if (sig == SIGUSR1)
		verbose = 1;
	else
		verbose = 0;
}

int
main(int argc, char **argv)
{
	int			tcpsock, udpsock, i, fdcount, ch;
	FILE			*fp;
	char			buf[BUFSIZ];
	extern char		build_info[];
	fd_set			fds, sfds;
	char			*logfile = (char *) NULL;

	while ((ch = getopt(argc, argv, "dp:vl:")) != -1)
		switch(ch) {
		case 'l':
			logfile = optarg;
			break;
		case 'p':
			portnum = atoi(optarg);
			break;
		case 'd':
			debug++;
		case 'v':
			verbose++;
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

	if (debug) 
		loginit(0, logfile);
	else {
		if (logfile)
			loginit(0, logfile);
		else
			loginit(1, "syncd");
	}
	info("daemon starting (version %d)\n", CURRENT_VERSION);
	info("%s\n", build_info);

	/*
	 * Create TCP/UDP server.
	 */
	if (makesockets(portnum, &udpsock, &tcpsock) < 0) {
		error("Could not make sockets!");
		exit(1);
	}
	/* Now become a daemon */
	if (!debug)
		daemon(0, 1);

	signal(SIGUSR1, setverbose);
	signal(SIGUSR2, setverbose);

	/*
	 * Stash the pid away.
	 */
	if (!geteuid()) {
		sprintf(buf, "%s/syncd.pid", _PATH_VARRUN);
		fp = fopen(buf, "w");
		if (fp != NULL) {
			fprintf(fp, "%d\n", getpid());
			(void) fclose(fp);
		}
	}

	/*
	 * Now sit and listen for connections.
	 */
	FD_ZERO(&sfds);
	FD_SET(tcpsock, &sfds);
	FD_SET(udpsock, &sfds);
	fdcount = tcpsock;
	if (udpsock > tcpsock)
		fdcount = udpsock;
	fdcount++;

	while (1) {
		struct sockaddr_in	client;
		int			length, cc, newsock;
		barrier_req_t		barrier_req;
		
		fds = sfds;
		errno = 0;
		i = select(fdcount, &fds, NULL, NULL, NULL);
		if (i < 0) {
			if (errno == EINTR) {
				warning("select interrupted, continuing");
				continue;
			}
			fatal("select: %s", strerror(errno));
		}
		if (i == 0)
			continue;

		if (FD_ISSET(tcpsock, &fds)) {
			length  = sizeof(client);
			newsock = accept(tcpsock,
					 (struct sockaddr *)&client, &length);
			if (newsock < 0) {
				errorc("accepting TCP connection");
				continue;
			}

			if ((cc = read(newsock, &barrier_req,
				       sizeof(barrier_req))) <= 0) {
				if (cc < 0)
					errorc("Reading TCP request");
				error("TCP connection aborted\n");
				close(newsock);
				continue;
			}
			handle_request(newsock, &client, &barrier_req, 1);
		}
		if (FD_ISSET(udpsock, &fds)) {
			length = sizeof(client);
			
			cc = recvfrom(udpsock, &barrier_req,
				      sizeof(barrier_req),
				      0, (struct sockaddr *)&client, &length);
			if (cc <= 0) {
				if (cc < 0)
					errorc("Reading UDP request");
				error("UDP Connection aborted\n");
				continue;
			}
			handle_request(udpsock, &client, &barrier_req, 0);
		}
	}
	close(tcpsock);
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

	i = SOCKBUFSIZE;
	if (setsockopt(udpsock, SOL_SOCKET, SO_SNDBUF, &i, sizeof(i)) < 0)
		pwarning("Could not increase send socket buffer size to %d",
			 SOCKBUFSIZE);
    
	i = SOCKBUFSIZE;
	if (setsockopt(udpsock, SOL_SOCKET, SO_RCVBUF, &i, sizeof(i)) < 0)
		pwarning("Could not increase recv socket buffer size to %d",
			 SOCKBUFSIZE);

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
 * Handle a barrier request.
 */
static int
handle_request(int sock, struct sockaddr_in *client,
	       barrier_req_t *barrier_reqp, int istcp)
{
	barrier_ctrl_t	   *barrierp = allbarriers;
	barrier_ctrl_t	   *tmp;
	int		   i;

	/*
	 * Find the control structure. Since a waiter can check in before
	 * the controller does, just create a new one. The count will go
	 * to -1 if its a waiter. It will all even out later!
	 */
	while (barrierp) {
		if (!strcmp(barrierp->name, barrier_reqp->name))
			break;
		barrierp = barrierp->next;
	}
	if (!barrierp) {
		barrierp = calloc(1, sizeof(barrier_ctrl_t));
		if (!barrierp)
			fatal("Out of memory!");

		barrierp->name = strdup(barrier_reqp->name);
		if (verbose)
			info("Barrier created: %s\n", barrierp->name);
		barrierp->next = allbarriers;
		allbarriers    = barrierp;
	}

	if (barrier_reqp->request == BARRIER_INIT) {
		barrierp->count  += barrier_reqp->count;
		barrierp->ipaddr.s_addr = client->sin_addr.s_addr;

		if (verbose)
			info("%s: Barrier initialized: %s %d (%d)\n",
			     inet_ntoa(client->sin_addr),
			     barrierp->name, barrier_reqp->count,
			     barrierp->count);

		if (barrier_reqp->flags & BARRIER_INIT_NOWAIT) {
			info("%s: Barrier Master not waiting: %s\n",
			     inet_ntoa(client->sin_addr),
			     barrierp->name);
			
			release_client(sock, client->sin_addr, istcp);
			goto check;
		}
	}
	else if (barrier_reqp->request == BARRIER_WAIT) {
		barrierp->count -= 1;

		if (verbose)
			info("%s: Barrier waiter: %s %d\n",
			     inet_ntoa(client->sin_addr),
			     barrierp->name, barrierp->count);
	}
	else {
		error("Ignoring invalid barrier request from %s",
		      inet_ntoa(client->sin_addr));
		return 1;
	}

	/*
	 * Record the waiter info for later wakeup, including the
	 * initializer, who is also woken up.
	 */
	barrierp->waiters[barrierp->index].sock   = sock;
	barrierp->waiters[barrierp->index].istcp  = istcp;
	barrierp->waiters[barrierp->index].ipaddr.s_addr =
		client->sin_addr.s_addr;
	barrierp->index++;

	/*
	 * If the count goes to zero, wake everyone up. We write back a
	 * value to make it "easy" for the waiter to notice an error,
	 * say if the server node goes down or the daemon dies.
	 */
 check:
	if (barrierp->count != 0)
		return 0;

	for (i = 0; i < barrierp->index; i++) {
		if (verbose)
			info("%s: releasing %s\n", barrierp->name,
			     inet_ntoa(barrierp->waiters[i].ipaddr));

		release_client(barrierp->waiters[i].sock,
			       barrierp->waiters[i].ipaddr,
			       barrierp->waiters[i].istcp);
	}

	/*
	 * And free the barrier structure from the list.
	 */
	if (barrierp == allbarriers) {
		allbarriers = allbarriers->next;
		free(barrierp);
	}
	else {
		tmp = allbarriers;
		while (tmp) {
			if (tmp->next == barrierp)
				break;
			tmp = tmp->next;
		}
		tmp->next = barrierp->next;
		free(barrierp);
	}
	return 0;
}

/*
 * Release a single client from a barrier at a time.
 */
static void
release_client(int sock, struct in_addr ipaddr, int istcp)
{
	int	err, foo = 1;
	
	if (istcp) {
		if ((err = write(sock, &foo, sizeof(foo))) <= 0) {
			if (err < 0) {
				errorc("writing to TCP client");
			}
			error("write to TCP client aborted");
		}
		close(sock);
	}
	else {
		struct sockaddr_in	client;
		
		client.sin_addr    = ipaddr;
		client.sin_family  = AF_INET;
		client.sin_port    = htons(portnum);

		err = sendto(sock, &foo, sizeof(foo), 0,
			     (struct sockaddr *)&client,
			     sizeof(client));
		if (err < 0)
			errorc("writing to UDP client");
	}
}












