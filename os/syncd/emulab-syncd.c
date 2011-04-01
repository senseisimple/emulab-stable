/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2006 University of Utah and the Flux Group.
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

#ifndef NO_EMULAB
#include "log.h"
#include "tmcc.h"
#else
void info(const char * fmt, ...)
{
  va_list args;
  char	buf[BUFSIZ];

  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  fprintf(stderr, "INFO: %s\n", buf);

}
void warning(const char * fmt, ...)
{
  va_list args;
  char	buf[BUFSIZ];

  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  fprintf(stderr, "WARNING: %s\n", buf);
}
void error(const char * fmt, ...)
{
  va_list args;
  char	buf[BUFSIZ];

  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  fprintf(stderr, "ERROR: %s\n", buf);
}
void fatal(const char * fmt, ...)
{
  va_list args;
  char	buf[BUFSIZ];

  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  fprintf(stderr, "FATAL: %s\n", buf);
  exit(-1);
}
void pfatal(const char *fmt, ...)
{
	va_list args;
	char	buf[BUFSIZ];

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	fatal("%s : %s", buf, strerror(errno));
}
void pwarning(const char *fmt, ...)
{
	va_list args;
	char	buf[BUFSIZ];

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	warning("%s : %s", buf, strerror(errno));
}
void errorc(const char *fmt, ...)
{
	va_list args;
	char	buf[BUFSIZ];

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	error("%s : %s", buf, strerror(errno));
}
#endif

int		debug   = 0;
int		verbose = 0;
static int	portnum = 0;
static void     print_barriers(void);
static void     clear_barriers(void);
static int      readall(int socket, void *buffer, size_t len);
static int      writeall(int socket, void *buffer, size_t len);
static int      handle_request(int, struct sockaddr_in *, barrier_req_t *,int);
static int	makesockets(int *portnum, int *udpsockp, int *tcpsockp);
static int	maxtcpsocket(fd_set *fs, unsigned int current_max);
static void     remove_tcpclient(int sock);
static void     release_client(int sock, struct in_addr ip, int istcp,
			       int result);
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
	int			error;
	struct in_addr		ipaddr;		/* Debugging */
	struct barrier_ctrl	*next;		/* Linked list */
	struct {
		int		count;
		int		sock;
		int		istcp;
		int		error;
		struct in_addr	ipaddr;		/* Debugging */
	} waiters[MAXWAITERS];
};
typedef struct barrier_ctrl barrier_ctrl_t;
static barrier_ctrl_t *allbarriers;

static int gotsiginfo;
static int gotsighup;

/*
 * Map TCP sockets onto the barrier they are waiting on so we can detect
 * closes and restore the counter.
 */
struct {
	barrier_ctrl_t *barrier;
} tcpsockets[FD_SETSIZE];

static char *usagestr =
 "usage: emulab-syncd [-d] [-p #]\n"
 " -h              Display this message\n"
 " -V              Print version information and exit\n"
 " -d              Do not daemonize.\n"
 " -v              Turn on verbose logging\n"
 " -l logfile      Specify log file instead of syslog\n"
 " -p portnum      Specify a port number to listen on\n"
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

#if defined(SIGINFO)
static void
handle_siginfo(int sig)
{
	gotsiginfo = 1;
}
#endif

static void
handle_sighup(int sig)
{
	gotsighup = 1;
}

int
main(int argc, char **argv)
{
	int			tcpsock, udpsock, i, ch;
	unsigned int		maxfd;
	FILE			*fp;
	char			buf[BUFSIZ];
#ifndef NO_EMULAB
	extern char		build_info[];
#endif
	fd_set			fds, sfds;
	char			*logfile = (char *) NULL;
	struct sigaction	sa;

	while ((ch = getopt(argc, argv, "hVdp:vl:")) != -1)
		switch(ch) {
		case 'l':
			if (strlen(optarg) > 0) {
				logfile = optarg;
			}
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
		case 'd':
			debug++;
		case 'v':
			verbose++;
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
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc) {
		fprintf(stderr,
			"Error: Unrecognized command line arguments: %s ...\n",
			argv[0]);
		usage();
	}

#ifndef NO_EMULAB
	if (debug) 
		loginit(0, logfile);
	else {
		if (logfile)
			loginit(0, logfile);
		else
			loginit(1, "syncd");
	}
#endif
	info("daemon starting (version %d)\n", CURRENT_VERSION);
#ifndef NO_EMULAB
	info("%s\n", build_info);
#endif

	/*
	 * Create TCP/UDP server.
	 */
	if (makesockets(&portnum, &udpsock, &tcpsock) < 0) {
		error("Could not make sockets!");
		exit(1);
	}
	/*
	 * Register with tmcc
	 */
#ifndef NO_EMULAB
	if (PortRegister(SERVER_SERVNAME, portnum) < 0) {
		error("Could not register service with tmcd!");
		exit(1);
	}
#endif
	
	/* Now become a daemon */
	if (!debug)
		daemon(0, 1);

	signal(SIGUSR1, setverbose);
	signal(SIGUSR2, setverbose);
	sa.sa_handler = handle_sighup;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0; // XXX No SA_RESTART, we want the select to break...
	sigaction(SIGHUP, &sa, NULL);
#if defined(SIGINFO)
	sa.sa_handler = handle_siginfo;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINFO, &sa, NULL);
#endif
	
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
	tcpsockets[tcpsock].barrier = (barrier_ctrl_t *)-1; /* sentinel */
	FD_SET(udpsock, &sfds);
	tcpsockets[udpsock].barrier = (barrier_ctrl_t *)-1; /* sentinel */
	maxfd = tcpsock;
	if (udpsock > tcpsock)
		maxfd = udpsock;

	while (1) {
		struct sockaddr_in	client;
		int			length, cc, newsock, j;
		barrier_req_t		barrier_req;
		
		if (gotsiginfo) {
			print_barriers();
			gotsiginfo = 0;
		}
		
		if (gotsighup) {
			clear_barriers();
			maxfd = maxtcpsocket(&sfds, maxfd);
			gotsighup = 0;
		}
		
		fds = sfds;
		errno = 0;
		i = select(maxfd + 1, &fds, NULL, NULL, NULL);
		if (i < 0) {
			if (errno == EINTR) {
				if (!gotsiginfo && !gotsighup) {
					warning("select interrupted, "
						"continuing");
				}
				continue;
			}
			fatal("select: %s", strerror(errno));
		}
		
		if (i == 0)
			continue;

		if (FD_ISSET(tcpsock, &fds)) {
			FD_CLR(tcpsock, &fds);
			i -= 1;
			length  = sizeof(client);
			newsock = accept(tcpsock,
					 (struct sockaddr *)&client, &length);
			if (newsock < 0) {
				errorc("accepting TCP connection");
				continue;
			}

			if ((cc = readall(newsock, &barrier_req,
					  sizeof(barrier_req))) <= 0) {
				if (cc < 0)
					errorc("Reading TCP request");
				error("TCP connection aborted\n");
				close(newsock);
				continue;
			}
			if (handle_request(newsock,
					   &client,
					   &barrier_req,
					   1) != 0) {
				/* There was an error while processing. */
				close(newsock);
			}
			else if (tcpsockets[newsock].barrier != NULL) {
				/*
				 * The barrier was not crossed, add a 
				 * check for readability so we can detect 
				 * a closed connection.
 				 */
				FD_SET(newsock, &sfds);
				fcntl(newsock, F_SETFL, O_NONBLOCK);
				if (newsock > maxfd)
					maxfd = newsock;
			}
			else {
				/*
				 * The barrier was crossed and the socket 
				 * closed, update maxfd.
				 */
				maxfd = maxtcpsocket(&sfds, maxfd);
			}
		}
		if (FD_ISSET(udpsock, &fds)) {
			FD_CLR(udpsock, &fds);
			i -= 1;
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

		/* Check other active FDs for garbage/closed connections. */
		for (j = 0; i > 0; j++) {
			char scratch[1024];
			
			if (FD_ISSET(j, &fds)) {
				i -= 1;
				while ((cc = read(j,
						  scratch,
						  sizeof(scratch))) > 0) {
					/* Nothing to do... */
				}
				if( cc == 0 ) {
					/* Connection closed */
					FD_CLR(j, &sfds);
					close(j);
					remove_tcpclient(j);
					if( j == maxfd ) {
						maxfd = maxtcpsocket(&sfds,
								     maxfd);
					}
				}
			}
		}
	}
	close(tcpsock);
	close(udpsock);
	info("daemon terminating\n");
	exit(0);
}

/*
 * Pretty print the list of barriers and their waiters.
 */
static void
print_barriers(void)
{
	barrier_ctrl_t		*barrierp;

	info("BEGIN Barrier listing:\n");
	for (barrierp = allbarriers;
	     barrierp != NULL;
	     barrierp = barrierp->next) {
		int lpc;

		info("  name:%s\twaiters:%d\tcount:%d\n",
		     barrierp->name,
		     barrierp->index,
		     barrierp->count);
		for (lpc = 0; lpc < barrierp->index; lpc++) {
			info("    %s error:%d (%s)\n",
			     inet_ntoa(barrierp->waiters[lpc].ipaddr),
			     barrierp->waiters[lpc].error,
			     barrierp->waiters[lpc].istcp ? "tcp" : "udp");
		}
	}
	info("END Barrier listing\n");
}

/*
 * Clear all barriers and release their waiters with an error code.
 */
static void
clear_barriers(void)
{
	barrier_ctrl_t		*barrierp, *barriernextp;

	info("Clearing all barriers\n");
	
	for (barrierp = allbarriers;
	     barrierp != NULL;
	     barrierp = barriernextp) {
		int i;

		barriernextp = barrierp->next;
		barrierp->next = NULL;
		for (i = 0; i < barrierp->index; i++) {
			release_client(barrierp->waiters[i].sock,
				       barrierp->waiters[i].ipaddr,
				       barrierp->waiters[i].istcp,
				       SERVER_ERROR_SIGHUP);
		}
		free(barrierp);
		barrierp = NULL;
	}

	allbarriers = NULL;
}

/*
 * Read all of the requested bytes from a socket.
 */
static int
readall(int socket, void *buffer, size_t len)
{
	int			offset = 0, error = 0, retval = 0;

	while( (offset < len) && !error )
	{
		int	rc;

		rc = read(socket, &((char *)buffer)[offset], len - offset);
		if (rc < 0) {
			if (errno != EINTR) {
				error = 1;
				retval = -1;
			}
		}
		else if (rc == 0) {
			/* Connection was closed. */
			len = 0;
			retval = 0;
		}
		else {
			offset += rc;
			retval += rc;
		}
	}
	return retval;
}

/*
 * Write all of the bytes to a socket.
 */
static int
writeall(int socket, void *buffer, size_t len)
{
	int			offset = 0, error = 0, retval = 0;

	while( (offset < len) && !error )
	{
		int	rc;

		rc = write(socket, &((char *)buffer)[offset], len - offset);
		if (rc < 0) {
			if (errno != EINTR) {
				error = 1;
				retval = -1;
			}
		}
		else if (rc == 0) {
			/* Connection was closed. */
			len = 0;
			retval = 0;
		}
		else {
			offset += rc;
			retval += rc;
		}
	}
	return retval;
}

/*
 * Create sockets on specified port.
 */
static int
makesockets(int *portnum, int *udpsockp, int *tcpsockp)
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

	/* Start with default portnumber, and then retry if taken. */
	if (*portnum == 0) {
		name.sin_port = htons((u_short) SERVER_PORTNUM);
		if (! bind(tcpsock, (struct sockaddr *) &name, sizeof(name))) {
			/* Got the default port */
			goto bound;
		}
		if (errno != EADDRINUSE)
			pfatal("binding stream socket");
		/* Retry below with portnum=0 */
	}
	name.sin_port = htons((u_short) *portnum);
	if (bind(tcpsock, (struct sockaddr *) &name, sizeof(name))) {
		pfatal("binding stream socket");
	}
 bound:
	/* Find assigned port value and print it out. */
	length = sizeof(name);
	if (getsockname(tcpsock, (struct sockaddr *) &name, &length)) {
		pfatal("getsockname");
	}
	if (listen(tcpsock, 128) < 0) {
		pfatal("listen");
	}
	*portnum = ntohs(name.sin_port);

	info("listening on TCP port %d\n", *portnum);
	
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
	name.sin_port = htons((u_short) *portnum);
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
 * Find the maximum TCP socket descriptor number.
 */
static int
maxtcpsocket(fd_set *fs, unsigned int current_max)
{
	int		lpc, retval = 0;

	for (lpc = 0; lpc <= current_max; lpc++) {
		if (tcpsockets[lpc].barrier != NULL)
			retval = lpc;
		else
			FD_CLR(lpc, fs);
	}

	return retval;
}

/*
 * Remove a barrier from the global list.
 */
static void
remove_barrier(barrier_ctrl_t *barrierp)
{
	barrier_ctrl_t	   *tmp;
	
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
}

/*
 * Handle a barrier request.
 */
static int
handle_request(int sock, struct sockaddr_in *client,
	       barrier_req_t *barrier_reqp, int istcp)
{
	barrier_ctrl_t	   *barrierp = allbarriers;
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

		barrier_reqp->name[sizeof(barrier_reqp->name) - 1] = '\0';
		barrierp->name = strdup(barrier_reqp->name);
		if (!barrierp->name)
			fatal("Out of memory!");
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
			
			release_client(sock, client->sin_addr, istcp, 0);
			goto check;
		}
	}
	else if (barrier_reqp->request == BARRIER_WAIT) {
		barrier_reqp->count = -1;
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
	barrierp->waiters[barrierp->index].count  = barrier_reqp->count;
	barrierp->waiters[barrierp->index].sock   = sock;
	barrierp->waiters[barrierp->index].istcp  = istcp;
	barrierp->waiters[barrierp->index].error  = barrier_reqp->error;
	barrierp->waiters[barrierp->index].ipaddr.s_addr =
		client->sin_addr.s_addr;
	barrierp->index++;
	if (barrier_reqp->error > barrierp->error)
		barrierp->error = barrier_reqp->error;
	if (istcp)
		tcpsockets[sock].barrier = barrierp;

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
			       barrierp->waiters[i].istcp,
			       barrierp->error);
	}

	/*
	 * And free the barrier structure from the list.
	 */
	remove_barrier(barrierp);
	
	return 0;
}

/*
 * Remove a TCP client from the waiter list without trying to respond.
 */
static void
remove_tcpclient(int sock)
{
	barrier_ctrl_t		*barrierp;
	int			packing = 0, i;
	
	barrierp = tcpsockets[sock].barrier;
	tcpsockets[sock].barrier = NULL;
	for (i = 0; i < barrierp->index; i++ ) {
		if (barrierp->waiters[i].sock == sock) {
			info("%s: removing %s\n",
			     barrierp->name,
			     inet_ntoa(barrierp->waiters[i].ipaddr));
			packing = 1;
			barrierp->count -= barrierp->waiters[i].count;
		}
		if (packing) {
			barrierp->waiters[i] = barrierp->waiters[i + 1];
		}
	}
	barrierp->index -= 1;
	if (barrierp->index == 0) {
		remove_barrier(barrierp);
		barrierp = NULL;
	}
}

/*
 * Release a single client from a barrier at a time.
 */
static void
release_client(int sock, struct in_addr ipaddr, int istcp, int result)
{
	int	err;
	
	if (istcp) {
		if ((err = writeall(sock, &result, sizeof(result))) <= 0) {
			if (err < 0) {
				errorc("writing to TCP client");
			}
			error("write to TCP client aborted");
		}
		tcpsockets[sock].barrier = NULL;
		close(sock);
	}
	else {
		struct sockaddr_in	client;
		
		client.sin_addr    = ipaddr;
		client.sin_family  = AF_INET;
		client.sin_port    = htons(portnum);

		while( ((err = sendto(sock, &result, sizeof(result), 0,
				      (struct sockaddr *)&client,
				      sizeof(client))) < 0) &&
		       (errno == EINTR) )
		{
			// Nothing to do...
		}
		if (err < 0)
			errorc("writing to UDP client");
	}
}
