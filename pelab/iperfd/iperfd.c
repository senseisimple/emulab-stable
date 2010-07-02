/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * iperfd - tiny replacement for iperf server
 *
 * TODO:
 *       Use non-blocking accept() to prevent blocking in some (hopefully
 *         rare) cases.
 *       Use non-blocking IO for logging messages      
 *       Check return value of close(), ie. EINTR
 *       Should we block (or ignore) some signals?
 */


#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <stdarg.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <netinet/tcp.h>
#include <sys/time.h>

/*
 * These cause Linux to use BSD names in some structures, turn on some more
 * stuff in unistd.h, etc.
 */
#ifdef __linux__
#define __PREFER_BSD
#define __USE_BSD
#define __FAVOR_BSD
#endif

#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

/*
 * We do huge read()s so that if the machine is busy, and we may have gotten
 * multiple packets, we can hopefully fetch them all in a single read
 */
const int max_readsize = 1024 * 1024;

/*
 * How many errors before we give up?
 */
const int max_errors = 10;

/*
 * How long (in seconds) do we wait to clear the error count if we haven't had
 * any more errors?
 */
const int error_lifetime = 60;

/*
 * These are global so that error-handling functions can use them easily
 */
int recent_error_count = 0;
struct timeval last_error;

/*
 * Some defaults
 */
const size_t def_readsize = 8192;
const uint16_t def_port = 5001;
const int def_windowsize = 0;
const bool def_daemonize = false;
const bool def_singleclient = false;
const int backlog = 10;

/*
 * Internal helper functions
 */
void croak(const char *fmt, ... );
void croak_perror(const char *str);
bool parsenum(char *numstr, int *out);
void fprinttimestamp(FILE *file);
void fprintaddr(FILE *file, struct sockaddr_in *addr);
void error();

/*
 * Command-line options we support, from iperf
 *   accept
 * -l,  --len       #[KM]    length of buffer to read or write (default 8 KB)
 * -p, --port      #        server port to listen on/connect to
 * -w, --window    #[KM]    TCP window size (socket buffer size)
 * -B, --bind      <host>   bind to <host>, an interface or multicast  address
 * -C, --compatibility      for use with older versions does not sent extra msgs
 * -M, --mss       #        set TCP maximum segment size (MTU - 40 bytes)
 * -N, --nodelay            set TCP no delay, disabling Nagle's Algorithm
 * -D, --daemon             run the server as a daemon
*/
#define LONG_OPTIONS()
const struct option long_options[] =
{
{"single",           no_argument, NULL, '1'},
{"len",        required_argument, NULL, 'l'},
{"port",       required_argument, NULL, 'p'},
{"window",     required_argument, NULL, 'w'},
{"bind",       required_argument, NULL, 'B'},
{"compatibility",    no_argument, NULL, 'C'},
{"mss",        required_argument, NULL, 'M'},
{"nodelay",          no_argument, NULL, 'N'},
{"daemon",           no_argument, NULL, 'D'},
{"server",           no_argument, NULL, 's'},
{"client",           no_argument, NULL, 'c'},
{0, 0, 0, 0}
};

int main(int argc, char **argv) {

    /*
     * Print out some hopefully useful debugging information
     */
    fprinttimestamp(stdout);
    printf(" iperfd starting, called as: ");
    for (int i = 0; i < argc; i++) {
        printf(argv[i]);
        if (i != argc) {
            printf(" ");
        }
    }
    printf("\n");
    fprinttimestamp(stdout);
    printf(" Maximum number of clients: %i \n", FD_SETSIZE);

    
    /*
     * Handle command-line arguments
     */
    int readsize = def_readsize;
    int port = def_port;
    int windowsize = def_windowsize;
    bool daemonize = def_daemonize;
    bool singleclient = def_singleclient;
    while (1) {
        char ch;
        int option_index;
        ch = getopt_long(argc,argv,"hl:p:w:B:CM:NDsc1",long_options,
                &option_index);
        if (ch == -1) {
            /* End of options */
            break;
        }
        switch(ch) {
            case 'l':
                if (!parsenum(optarg,&readsize)) {
                    croak("Bad read length parameter\n");
                } else if (readsize > max_readsize) {
                    croak("Read length parameter too big\n");
                }
                break;
            case 'p':
                if (!parsenum(optarg,&port)) {
                    croak("Bad port parameter\n");
                }
                break;
            case 'C':
                /* No-op */
                break;
            case 's':
                /* No-op */
                break;
            case 'c':
                croak("iperfd cannot be used as a client");
                break;
            case 'w':
                if (!parsenum(optarg,&windowsize)) {
                    croak("Bad window size parameter\n");
                }
                break;
            case 'D':
                daemonize = true;
                break;
            case '1':
                singleclient = true;
                break;
            case 'M': /* Just not implemented yet */
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
#ifndef __linux__
    inaddr.sin_len = sizeof(inaddr);
#endif
    inaddr.sin_family = htons(PF_INET);
    inaddr.sin_port = htons(port);
    inaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenfd, (struct sockaddr*)&inaddr, sizeof(inaddr)) < 0) {
	croak_perror("Unable to bind to port");
    }

    /*
     * Set the listen socket to nonblocking to deal with some obscure DOS-type
     * problems with clients dropping or being kicked in between select() and
     * accept()
     */
    int oldflags = fcntl(listenfd,F_GETFL);
    if (fcntl(listenfd, F_SETFL, oldflags | O_NONBLOCK)) {
        croak_perror("Unable to set listen socket to nonblocking\n");
    }

    /*
     * Listen for clients
     */
    if (listen(listenfd, backlog) < 0) {
	croak_perror("Unable to listen() on socket");
    }

    /*
     * We'll use this to hold the address of clients that connect, so
     * that we can report on which one has disconnected
     */
    struct sockaddr_in clients[FD_SETSIZE];

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
    int client_count = 0;

    /*
     * According to the manpage, this can't fail in this circumstance
     */
    gettimeofday(&last_error,NULL);
    recent_error_count = 0;

    while (recent_error_count < max_errors) {

        /*
         * Reset the error count if enough time has passed
         */
	bcopy(&master_fdset, &tmp_fdset, sizeof(master_fdset));
	if (select(maxfd + 1, &tmp_fdset, NULL, NULL, NULL) < 0) {
	    if (errno == EINTR) {
		/* We just got interrupted, try again */
		continue;
	    } else {
                /* Log this error, sleep for a while, and try again. Yes,
                 * sleeping could fubar bandwidth for current tests, but I
                 * don't want this process to go nuts and do syscalls in a
                 * tight loop
                 */
                perror("select() failed");
                error();
                sleep(1);
	    }
	}

	/*
	 * Handle the listen socket first
	 */
	if (FD_ISSET(listenfd, &tmp_fdset)) {

            /*
             * Accept any new clients
             */
	    struct sockaddr_in clientaddr;
	    socklen_t clientlen = sizeof(clientaddr);
	    int newclient = accept(listenfd, (struct sockaddr *)&clientaddr,
		    &clientlen);
	    if ((singleclient && (client_count > 0)) ||
                (newclient >= FD_SETSIZE)) {
                /*
                 * We can only handle so many clients...
                 */
                fprinttimestamp(stdout);
		printf(" Too many clients, dropping ");
                fprintaddr(stdout,&clientaddr);
                printf(" [%i]\n", newclient);
                close(newclient);
	    } else if (newclient == -1) {
                if (errno == EINTR        || errno == EWOULDBLOCK ||
                    errno == ECONNABORTED || errno == EPROTO) {
                    /*
                     * These are non-fatal, they client somehow got
                     * disconnected before we could accept() it, just ignore
                     * this accept() and go on with life
                     */
                } else {
                    /*
                     * Something more serious has happened, log it
                     */
                    perror("Error from accept()");
                    error();
                }
            } else {

                fprinttimestamp(stdout);
                printf(" New client: ");
                fprintaddr(stdout,&clientaddr);
                printf(" [%i]\n",newclient);

                /*
                 * Try to set the socket buffer size
                 */
                bool sockopterr = false;
                if (windowsize != 0) {
                    if (setsockopt(newclient,SOL_SOCKET,SO_SNDBUF,
                                &windowsize,sizeof(windowsize)) == -1) {
                        perror("Unable to set send buffer size");
                        sockopterr = true;
                    }
                    if (setsockopt(newclient,SOL_SOCKET,SO_RCVBUF,
                                &windowsize,sizeof(windowsize)) == -1) {
                        perror("Unable to set recieve buffer size");
                        sockopterr = true;
                    }
                }

                if (sockopterr) {
                    fprintf(stderr,"Dropping client due to sockopt error\n");
                    close(newclient);
                    error();
                    continue;
                } else {
                    client_count++;
                    FD_SET(newclient,&master_fdset);
                    if (newclient > maxfd) {
                        maxfd = newclient;
                    }
                    bcopy(&clientaddr, clients + newclient, sizeof(clientaddr));
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
		 * TODO: Do we need to handle signals, like SIGPIPE?
                 * TODO: Can we flush the FD without actually reading data?
		 */
                ssize_t readbytes = read(i,buffer,readsize);
                bool close_connection = false;
                if (readbytes == 0) {
                    fprinttimestamp(stdout);
                    printf(" Client disconnected: ");
                    fprintaddr(stdout,clients + i);
                    printf(" [%i]\n",i);

                    close_connection = true;
                } else if (readbytes == -1) {
                    if (errno == EINTR) {
                        /*
                         * Transient error, just try again later
                         */
                        continue;
                    } else {
                        /*
                         * Could be a serious error - log and drop the client
                         */
                        perror("read() failed");
                        fprintf(stderr,"Client was ");
                        fprintaddr(stderr, clients + i);
                        fprintf(stderr,"\n");

                        error();
                        
                        close_connection = true;
                    }
                }

                /*
                 * Close the client if it disconnected normally or had any
                 * errors
                 */
                if (close_connection) {
                    close(i);
                    client_count--;
                    FD_CLR(i,&master_fdset);
                    while (!FD_ISSET(maxfd,&master_fdset)) {
                        maxfd--;
                    }
                }
	    }
	}
    }

    /*
     * Cleanup code - will only be reached if something in the above loop
     * exits due to too many errors
     */
    fprintf(stderr,"Too many errors, exiting\n");
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

/*
 * Parse a numeric parameter iperf-style
 */
bool parsenum(char *str, int *out) {
    int len = strlen(str);
    int multiplier = 1;
    if (str[len-1] == 'k') {
        multiplier = 1000;
        str[len-1] = '\0';
    } else if (str[len-1] == 'm') {
        multiplier = 1000 * 1000;
        str[len-1] = '\0';
    } else if (str[len-1] == 'g') {
        multiplier = 1000 * 1000 * 1000;
        str[len-1] = '\0';
    } else if (str[len-1] == 'K') {
        multiplier = 1024;
        str[len-1] = '\0';
    } else if (str[len-1] == 'M') {
        multiplier = 1024 * 1024;
        str[len-1] = '\0';
    } else if (str[len-1] == 'G') {
        multiplier = 1024 * 1024 * 1024;
        str[len-1] = '\0';
    } 
    if (sscanf(str,"%i",out) == 1) {
        *out = *out * multiplier;
        //printf("Returning %i\n",*out);
        return true;
    } else {
        return false;
    }
}

/*
 * Print the 'seconds' part of the current timestamp
 */
void fprinttimestamp(FILE *file) {
    struct timeval time;
    if (gettimeofday(&time,NULL) != 0) {
        croak_perror("Unable to get time or day!");
    }
    fprintf(file,"%lu",time.tv_sec);
}

/*
 * Print out an internet address in dotted decimal notation, plus port
 * number
 */
void fprintaddr(FILE *file, struct sockaddr_in *addr) {
    fprintf(file,"%i.%i.%i.%i:%i",
            htonl(addr->sin_addr.s_addr) >> 24 & 0xff,
            htonl(addr->sin_addr.s_addr) >> 16 & 0xff,
            htonl(addr->sin_addr.s_addr) >> 8 & 0xff,
            htonl(addr->sin_addr.s_addr) >> 0 & 0xff,
            htons(addr->sin_port));
}

void error() {
    /*
     * What time is it, Mr. Fox?
     */
    struct timeval now;
    gettimeofday(&now,NULL);

    /*
     * If enough time has passed since the last error, reset the error count.
     * The reason that I do this here is that I don't want to be calling it
     * every time through the main loop, which will get run all the time when
     * we have a client. And, it only really matters when we get a new error.
     */
    if (now.tv_sec > (last_error.tv_sec + error_lifetime)) {
        recent_error_count = 0;
    }

    recent_error_count++;
    bcopy(&now,&last_error,sizeof(now));

    return;
}
