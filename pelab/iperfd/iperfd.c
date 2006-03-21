/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * iperfd - tiny replacement for iperf server
 *
 * TODO: Accept the same command-line arguments as iperf (though only where
 *         relevant)
 *       Accept 'suffixes' like iperf (K,M)
 *       Use non-blocking accept() to prevent blocking in some (hopefully
 *         rare) cases.
 *       Use non-blocking IO for logging messages      
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdarg.h>
#include <netdb.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/select.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <netinet/tcp.h>

/*
 * We do huge read()s so that if the machine is busy, and we may have gotten
 * multiple packets, we can hopefully fetch them all in a single read
 */
const int max_readsize = 1024 * 1024;

/*
 * Some defaults
 */
const size_t def_readsize = 8192;
const uint16_t def_port = 5001;
const int def_mss = 0;
const int def_windowsize = 0;
const bool def_daemonize = false;
const int backlog = 10;

void croak(const char *fmt, ... );
void croak_perror(const char *str);

/*
 * Command-line options we support, from iperf
 *   accept
 * -l,  --len       #[KM]    length of buffer to read or write (default 8 KB)
 * -p, --port      #        server port to listen on/connect to
 * -w, --window    #[KM]    TCP window size (socket buffer size)
 *  -B, --bind      <host>   bind to <host>, an interface or multicast  address
 *  -C, --compatibility      for use with older versions does not sent extra msgs
 *  -M, --mss       #        set TCP maximum segment size (MTU - 40 bytes)
 *  -N, --nodelay            set TCP no delay, disabling Nagle's Algorithm
 *  -D, --daemon             run the server as a daemon
*/
#define LONG_OPTIONS()
const struct option long_options[] =
{
{"len",        required_argument, NULL, 'l'},
{"port",       required_argument, NULL, 'p'},
{"window",     required_argument, NULL, 'w'},
{"bind",       required_argument, NULL, 'B'},
{"compatibility",    no_argument, NULL, 'C'},
{"mss",        required_argument, NULL, 'M'},
{"nodelay",          no_argument, NULL, 'N'},
{"daemon",           no_argument, NULL, 'D'},
{0, 0, 0, 0}
};

int main(int argc, char **argv) {
    printf("iperfd starting\n");

    
    /*
     * Handle command-line arguments
     */
    int readsize = def_readsize;
    int port = def_port;
    int mss = def_mss;
    int windowsize = def_windowsize;
    bool daemonize = def_daemonize;
    while (1) {
        char ch;
        int option_index;
        ch = getopt_long(argc,argv,"hl:p:w:B:CM:ND",long_options,
                &option_index);
        if (ch == -1) {
            /* End of options */
            break;
        }
        switch(ch) {
            case 'l':
                if (sscanf(optarg,"%i",&readsize) != 1) {
                    croak("Bad read length parameter\n");
                }
                break;
            case 'p':
                if (sscanf(optarg,"%i",&port) != 1) {
                    croak("Bad port parameter\n");
                }
                break;
            case 'C':
                /* No-op */
                break;
            case 'w':
                if (sscanf(optarg,"%i",&windowsize) != 1) {
                    croak("Bad window size parameter\n");
                }
                break;
            case 'M':
                if (sscanf(optarg,"%i",&mss) != 1) {
                    croak("Bad MSS parameter\n");
                }
                break;
            case 'D':
                daemonize = true;
                break;
            case 'N': /* I don't think this one really matters for a server */
            case 'B': /* Not likely to actually be used */
                croak("Option %c is not yet supported\n",ch);
                break;
            default:
                croak("Unknown option %c\n",ch);
        }
    }

    /*
     * Get the protoent for tcp - sure, it's usually the same everywhere, but
     * let's do it the right way
     */
    struct protoent *tcpent = getprotobyname("tcp");
    if (tcpent == NULL) {
	croak_perror("Unable to get protocol entry for tcp");
    }

    /*
     * Set up the socket we're going to listen() for connections on
     */
    int listenfd = socket(PF_INET, SOCK_STREAM,tcpent->p_proto);
    if (listenfd < 0) {
	croak_perror("Unable to create socket");
    }

    /*
     * Bind to the correct port
     */
    struct sockaddr_in inaddr;
    inaddr.sin_len = sizeof(inaddr);
    inaddr.sin_family = htons(PF_INET);
    inaddr.sin_port = htons(port);
    inaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenfd, (struct sockaddr*)&inaddr, sizeof(inaddr)) < 0) {
	croak_perror("Unable to bind to port");
    }

    /*
     * Listen for clients
     */
    if (listen(listenfd, backlog) < 0) {
	croak_perror("Unable to listen() on socket");
    }

    /*
     * Daemonize if we're supposed to
     */
    if (daemonize) {
        if (daemon(0,0) == -1) {
            croak_perror("Unable to daemonize");
        }
    }

    /*
     * The main listen/read loop
     */
    fd_set master_fdset, tmp_fdset;
    FD_ZERO(&master_fdset);
    FD_SET(listenfd,&master_fdset);
    int maxfd = listenfd;
    unsigned char buffer[readsize];
    while (1) {
	bcopy(&master_fdset, &tmp_fdset, sizeof(master_fdset));
	if (select(maxfd + 1, &tmp_fdset, NULL, NULL, NULL) < 0) {
	    if (errno == EINTR) {
		/* We just got interrupted, try again */
		continue;
	    } else {
		croak_perror("Error from select()");
	    }
	}

	/*
	 * Handle the listen socket first
	 */
	if (FD_ISSET(listenfd, &tmp_fdset)) {
	    struct sockaddr_in clientaddr;
	    socklen_t clientlen;
	    int newclient = accept(listenfd, (struct sockaddr *)&clientaddr,
		    &clientlen);
	    /*
             * TODO: More interesting logging
             */
	    if (newclient >= FD_SETSIZE) {
		fprintf(stderr,"Too many clients, dropping one\n");
                close(newclient);
	    } else {

                printf("Got a new client\n");
                /*
                 * TODO: Drop client, not croak, on failures here
                 */
                if (mss != 0) {
                    if (setsockopt(newclient,tcpent->p_proto,TCPOPT_MAXSEG,
                                &mss,sizeof(mss)) == -1) {
                        croak_perror("Unable to set MSS for client");
                    }
                }
                if (windowsize != 0) {
                    if (setsockopt(newclient,SOL_SOCKET,SO_SNDBUF,
                                &windowsize,sizeof(windowsize)) == -1) {
                        croak_perror("Unable to set send buffer size");
                    }
                    if (setsockopt(newclient,SOL_SOCKET,SO_SNDBUF,
                                &windowsize,sizeof(windowsize)) == -1) {
                        croak_perror("Unable to set send buffer size");
                    }
                }
                FD_SET(newclient,&master_fdset);
                if (newclient > maxfd) {
                    maxfd = newclient;
                }
            }
	}

	/*
	 * Looks for clients to read from
	 */
	int i;
	for (i = 0; i <= maxfd; i++) {
	    if (i == listenfd) { continue; }
	    if (FD_ISSET(i,&tmp_fdset)) {
		/*
		 * TODO: Do non-blocking I/O and more than one read?
		 * TODO: Do more reads? (fewer select() calls)
		 * TODO: Handle SIGPIPE?
		 */
                ssize_t readbytes = read(i,buffer,readsize);
                if (readbytes == 0) {
                    printf("Client disconnected\n");
                    FD_CLR(i,&master_fdset);
                    while (!FD_ISSET(maxfd,&master_fdset)) {
                        maxfd--;
                    }
                }
	    }
	}
    }

    /*
     * Cleanup code - will only be reached if something in the top loop calls
     * break - which nothing does yet
     */
    int i;
    for (i = 0; i <= maxfd; i++) {
	if (FD_ISSET(i,&master_fdset)) {
	    if (close(i) < 0) {
		perror("Closing socket");
	    }
	}
    }

    return 1;
}

void croak(const char *fmt, ... ) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr,"*** Error: ");
    vfprintf(stderr,fmt,ap);
    va_end(ap);
    exit(1);
}

void croak_perror(const char *str) {
    fprintf(stderr,"*** Error: ");
    perror(str);
    exit(1);
}
