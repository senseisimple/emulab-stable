/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 *
 * libnetmon, a library for monitoring network traffic sent by a process. See
 * README for instructions.
 */
#include "libnetmon.h"

/*
 * Die with a standard Emulab-type error message format. In the future, I might
 * try to modify this to simply unlink our wrapper functions so that the app
 * can continue to run.
 */
void croak(char *format, ...) {
    va_list ap;
    fprintf(stderr,"*** ERROR\n    libnetmon: ");
    vfprintf(stderr,format, ap);
    va_end(ap);
    fflush(stderr);
    exit(1);
}

/*
 * Set up our data structures, and go find the real versions of the functions
 * we're going to wrap.
 */
void lnm_init() {

    static bool intialized = false;
    char *sockpath;

    if (intialized == false) {
        DEBUG(printf("Initializing\n"));

        /*
         * Set up the array we use to track which FDs we're tracking
         */
        monitorFDs = (fdRecord*)NULL;
        fdSize = 0;
        allocFDspace();

        /*
         * Figure out which version of the output format we're supposed to use
         */
        char *outversion_s;
        outversion_s = getenv("LIBNETMON_OUTPUTVERSION");
        if (outversion_s) {
            if (!sscanf(outversion_s,"%i",&output_version) == 1) {
                croak("Bad output version: %s\n",outversion_s);
            }
        } else {
            output_version = 1;
        }
        DEBUG(printf("Using output version %i\n",output_version));

#define FIND_REAL_FUN(FUN) \
          real_##FUN = (FUN##_proto_t*)dlsym(RTLD_NEXT,#FUN); \
          if (!real_##FUN) { \
              croak("Unable to get the address of " #FUN "(): %s\n", \
                    dlerror()); \
          }

        /*
         * Find the real versions of the library functions we're going to wrap
         */
        FIND_REAL_FUN(socket);
        FIND_REAL_FUN(close);
        FIND_REAL_FUN(connect);
        FIND_REAL_FUN(write);
        FIND_REAL_FUN(send);
        FIND_REAL_FUN(setsockopt);
        FIND_REAL_FUN(read);
        FIND_REAL_FUN(recv);
        FIND_REAL_FUN(recvmsg);
        FIND_REAL_FUN(accept);

        /*
         * Connect to netmod if we've been asked to
         */
        sockpath = getenv("LIBNETMON_SOCKPATH");
        if (sockpath) {
            int sockfd;
            struct sockaddr_un servaddr;

            DEBUG(printf("Opening socket at path %s\n",sockpath));
            
            sockfd = real_socket(AF_LOCAL, SOCK_STREAM, 0);
            if (!sockfd) {
                croak("Unable to create socket\n");
            }

            servaddr.sun_family = AF_LOCAL;
            strcpy(servaddr.sun_path,sockpath);

            if (real_connect(sockfd, (struct sockaddr*) &servaddr,
                             sizeof(servaddr))) {
                croak("Unable to connect to netmond socket\n");
            }

            outstream = fdopen(sockfd,"w");
            if (!outstream) {
                croak("fdopen() failed on socket\n");
            }

            DEBUG(printf("Done opening socket\n"));

        } else {
            outstream = stdout;
        }

        /*
         * Check to see if we're supposed to force a specific socket buffer
         * size
         */
        char *bufsize_s;
        if ((bufsize_s = getenv("LIBNETMON_SOCKBUFSIZE"))) {
            if (sscanf(bufsize_s,"%i",&forced_bufsize) == 1) {
                printf("libnetmon: Forcing socket buffer size %i\n",
                        forced_bufsize);
            } else {
                croak("Bad sockbufsize: %s\n",bufsize_s);
            }
        } else {
            forced_bufsize = 0;
        }

        /*
         * Run a function to clean up state when the program exits
         */
        if (atexit(&stopWatchingAll) != 0) {
            croak("Unable to register atexit() function\n");
        }

        intialized = true;
    } else {
        /* DEBUG(printf("Skipping intialization\n")); */
    }
}

/*
 * Start monitoring a new file descriptor
 */
void startFD(int fd, const struct sockaddr *localname,
        const struct sockaddr *remotename) {

    struct sockaddr_in *remoteaddr, *localaddr;
    unsigned int socktype, typesize;
    union {
        struct sockaddr sa;
        char data[128];
    } sockname;
    socklen_t namelen;

    if (monitorFD_p(fd)) {
        printf("Warning: Tried to start monitoring an FD already in use!\n");
        stopFD(fd);
    }

    if (remotename == NULL) {
        int gpn_rv;
        gpn_rv = getpeername(fd,(struct sockaddr *)sockname.data,&namelen);
        if (gpn_rv != 0) {
            croak("Unable to get remote socket name: %s\n", strerror(errno));
        }
        /* Assume it's the right address family, since we checked that above */
        remotename = (struct sockaddr*)&(sockname.sa);
        remoteaddr = (struct sockaddr_in *) &(sockname.sa);
    } else {
        remoteaddr = (struct sockaddr_in*)remotename;
    }

    /*
     * Make sure it's an IP connection
     * XXX : Make sure the pointer is valid!
     */
    if (remotename->sa_family != AF_INET) {
        DEBUG(printf("Ignoring a non-INET socket\n"));
        return;
    }
    /*
     * Check to make sure it's a TCP socket
     */
    typesize = sizeof(unsigned int);
    if (getsockopt(fd,SOL_SOCKET,SO_TYPE,&socktype,&typesize) != 0) {
        croak("Unable to get socket type: %s\n",strerror(errno));
    }
    if (socktype != SOCK_STREAM) {
        DEBUG(printf("Ignoring a non-TCP socket\n"));
        return;
    }

    /*
     * Give oursleves enough space in the array to record information about
     * this connection
     */
    while (fd >= fdSize) {
        allocFDspace();
    }

    /*
     * Keep some information about the socket, so that we can print it out
     * later
     */
    monitorFDs[fd].remote_port = ntohs(remoteaddr->sin_port);
    /* XXX Error checking */
    monitorFDs[fd].remote_hostname = inet_ntoa(remoteaddr->sin_addr);

    /*
     * Get the local port number
     */
    int gsn_rv;
    if (localname == NULL) {
        gsn_rv = getsockname(fd,(struct sockaddr *)sockname.data,&namelen);
        if (gsn_rv != 0) {
            croak("Unable to get local socket name: %s\n", strerror(errno));
        }
        /* Assume it's the right address family, since we checked that above */
        localaddr = (struct sockaddr_in *) &(sockname.sa);
    } else {
        localaddr = (struct sockaddr_in *) localname;
    }
    monitorFDs[fd].local_port = ntohs(localaddr->sin_port);

    /*
     * We may have been asked to force the socket buffer size
     */
    if (forced_bufsize) {
        int sso_rv;
        sso_rv = real_setsockopt(fd,SOL_SOCKET,SO_SNDBUF,
                &forced_bufsize, sizeof(forced_bufsize));
        if (sso_rv == -1) {
            croak("Unable to force out buffer size: %s\n",strerror(errno));
        }
        sso_rv = real_setsockopt(fd,SOL_SOCKET,SO_RCVBUF,
                &forced_bufsize, sizeof(forced_bufsize));
        if (sso_rv == -1) {
            croak("Unable to force in buffer size: %s\n",strerror(errno));
        }
    }

    monitorFDs[fd].monitoring = true;

    /*
     * Let the monitor know about it
     */
    if (output_version == 2) {
        fprintf(outstream,"New: ");
        fprintID(outstream,fd);
        printf("\n");
    }

    DEBUG(printf("Watching FD %i\n",fd));

}

/*
 * Stop watching an FD
 */
void stopFD(int fd) {
    if (!monitorFD_p(fd)) {
        return;
    }

    DEBUG(printf("No longer watching FD %i\n",fd));

    /*
     * Let the monitor know we're done with it
     */
    if (output_version == 2) {
        fprintf(outstream,"Closed: ");
        fprintID(outstream,fd);
        fprintf(outstream,"\n");
    }

    monitorFDs[fd].monitoring = false;
    if (monitorFDs[fd].remote_hostname != NULL) {
        monitorFDs[fd].remote_hostname = NULL;
    }
}

/*
 * Stop watching all FDs - for use when the program exits
 */
void stopWatchingAll() {
    int i;
    for (i = 0; i < fdSize; i++) {
        if (monitorFD_p(i)) {
            stopFD(i);
        }
    }
}

/*
 * Print the unique identifier for a connection to the given filestream
 */
void fprintID(FILE *f, int fd) {

    fprintf(f,"%i:%s:%i", monitorFDs[fd].local_port,
            monitorFDs[fd].remote_hostname,
            monitorFDs[fd].remote_port);

}

void allocFDspace() {
    fdRecord *allocRV;
    unsigned int newFDSize;

    /*
     * Pick a new size, and use realloc() to grown our current allocation
     */
    newFDSize = fdSize + FD_ALLOC_SIZE;
    DEBUG(printf("Allocating space for %i FDs\n",newFDSize));

    allocRV = realloc(monitorFDs, newFDSize * sizeof(fdRecord));
    if (!allocRV) {
        croak("Unable to malloc space for monitorFDs array\n");
    }
    monitorFDs = allocRV;

    /*
     * Set newly-allocated entries to 0
     */
    bzero(monitorFDs + fdSize, (newFDSize - fdSize) * sizeof(fdRecord));

    fdSize = newFDSize;

    return;
}

bool monitorFD_p(int whichFD) {
    /*
     * If this FD is greater than the size of our fd array, then we're
     * definitely not monitoring it.
     */
    if (whichFD >= fdSize) {
        return false;
    } else {
        return monitorFDs[whichFD].monitoring;
    }
}

/*
 * Let the user know that a packet has been sent.
 *
 * TODO: Log someplace other than stdout - possibly IPC to the monitor process,
 * or to a socket.
 */
void log_packet(int fd, size_t len) {
    struct timeval time;
    /*
     * XXX - At some point, we may want to use something more precise than
     * gettimeofday()
     */
    if (gettimeofday(&time,NULL)) {
        croak("Error in gettimeofday()\n");
    }
    switch (output_version) {
        case 0:
            fprintf(outstream,"%lu.%08lu [%i, %i]\n",time.tv_sec, time.tv_usec,
                    fd,len);
            break;
        case 1:
            fprintf(outstream,"%lu.%06lu > %s.%i (%i)\n",time.tv_sec,
                    time.tv_usec, monitorFDs[fd].remote_hostname,
                    monitorFDs[fd].remote_port, len);
            break;
        case 2:
            fprintf(outstream,"%lu.%06lu > ", time.tv_sec, time.tv_usec);
            fprintID(outstream,fd);
            fprintf(outstream," (%i)\n", len);
            break;
        default:
            croak("Bad output version: %i\n",output_version);
    }
    fflush(outstream);
}

/*
 * Library function wrappers
 */

/*
 * Commented out, since we're actually deciding which FDs to monitor based on
 * success of the connect() function now.
 */
/*
int socket(int domain, int type, int protocol) {
    int returnedFD;
    lmn_init();
    DEBUG(printf("socket() called\n"));
    returnedFD = real_socket(domain, type, protocol);
    if (returnedFD > 0) {
        while (returnedFD > fdSize) {
            allocFDspace();
        }
        DEBUG(printf("Watching FD %i\n",returnedFD));
        monitorFDs[returnedFD] = true;
    }

    return returnedFD;
}
*/

/*
 * We're only going to bother to monitor FDs after they have succeeded in
 * connecting to some host.
 *
 * TODO: Allow for some filters:
 *      Only TCP connections
 *      Only certain hosts? (eg. not loopback)
 */
int connect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen) {

    int rv;
    lnm_init();
    DEBUG(printf("connect() called on %i\n",sockfd));

    rv = real_connect(sockfd, serv_addr, addrlen);

    /*
     * There are actually some cases when connect() can 'fail', but we
     * still want to watch the FD
     */
    if ((rv == 0) ||
           ((errno == EISCONN) ||     /* Socket is already connected */
            (errno == EINPROGRESS) || /* Non blocking socket */
            (errno == EINTR) ||       /* Connect will happen in background */
            (errno == EALREADY))) {    /* In progress in background */
        /*
         * TODO: In the case of the 'errors' that mean the socket is
         * connecting in the background, we really should make sure that
         * it actually connects - but this could be tricky. The caller is
         * supposed to select() on the FD to find out when it's ready, but
         * if they don't, and just write to it, we won't find out. So, for
         * now, just assume that the connect() will succeed.
         */

        /*
         * Find out some things about the address we've connected to
         * Note: The kernel already verified for us that the pointer is okay
         */
        startFD(sockfd,NULL,serv_addr);

    } else {
        /*
         * Do this in case they called connect() to reconnect a previously
         * connected socket. If it fails, stop watching the FD
         */
        stopFD(sockfd);
    }

    return rv;
}

/*
 * We will also watch for accept()ed connections
 */
int accept(int s, struct sockaddr * addr,
        socklen_t * addrlen) {

    int rv;
    lnm_init();
    DEBUG(printf("accept() called on %i\n",s));

    rv = real_accept(s, addr, addrlen);

    if (rv > 0) {
        /*
         * Got a new client!
         */
        startFD(rv,addr,NULL);
    }

    return rv;

}

/*
 * When the user closes a socket, we can stop monitoring it
 */
int close(int d) {
    int rv;

    lnm_init();

    rv = real_close(d);

    if (!rv && monitorFD_p(d)) {
        DEBUG(printf("Detected a closed socket with close()\n"));
        stopFD(d);
    }

    return rv;
}

/*
 * Wrap the send() function so that we can log messages sent to any of the
 * socket's we're monitoring.
 *
 * TODO: Need to write wrappers for other functions that can be used to send
 * data on a socket:
 * sendto
 * sendmsg
 * writev
 * others?
 */
ssize_t send(int s, const void *msg, size_t len, int flags) {
    ssize_t rv;

    lnm_init();

    /*
     * Wait until _after_ the packet is sent to log it, since the call might
     * block, and we basically want to report when the kernel acked receipt of
     * the packet
     */
    /*
     * TODO: There are a LOT of error cases, flags, etc, that we should handle.
     * For 
     */
    rv = real_send(s,msg,len,flags);

    if ((rv > 0) && monitorFD_p(s)) {
        log_packet(s,rv);
    }

    return rv;

}

ssize_t write(int fd, const void *buf, size_t count) {
    ssize_t rv;

    lnm_init();

    /*
     * Wait until _after_ the packet is sent to log it, since the call might
     * block, and we basically want to report when the kernel acked receipt of
     * the packet
     */
    rv = real_write(fd,buf,count);

    if ((rv > 0) && monitorFD_p(fd)) {
        log_packet(fd,rv);
    }

    return rv;

}

int setsockopt (int s, int level, int optname, const void *optval,
                 socklen_t optlen) {
    lnm_init();

    DEBUG(printf("setsockopt called (%i,%i)\n",level,optname));

    /*
     * Note, we do this on all sockets, not just those we are currently
     * monitoring, since it's likely they'll call setsockopt() before
     * connect()
     */
    if (forced_bufsize && (level == SOL_SOCKET) && ((optname == SO_SNDBUF) ||
                                  (optname == SO_RCVBUF))) {
        /*
         * I believe this is the right thing to do - return success but don't
         * do anything - I think that this is what you normally get when you,
         * say, pick a socket buffer size that is too big.
         */
        printf("Warning: Ignored attempt to change SO_SNDBUF or SO_RCVBUF\n");
        return 0;
    } else {
        return real_setsockopt(s,level,optname,optval,optlen);
    }
}

/*
 * The usual way to find 'eof' on a socket is to look for a zero-length
 * read. Since some programs might not be well-behaved in the sense that they
 * may not close() the socket, we wrap read() too
 */
ssize_t read(int d, void *buf, size_t nbytes) {
    ssize_t rv;

    lnm_init();

    rv = real_read(d,buf,nbytes);
    
    if ((rv == 0) && monitorFD_p(d)) {
        DEBUG(printf("Detected a closed socket with zero-length read()\n"));
        stopFD(d);
    }
    
    return rv;
}

/*
 * See comment for read()
 */
ssize_t recv(int s, void *buf, size_t len, int flags) {
    ssize_t rv;

    lnm_init();

    rv = real_recv(s,buf,len,flags);
    
    if ((rv == 0) && monitorFD_p(s)) {
        DEBUG(printf("Detected a closed socket with zero-length recv()\n"));
        stopFD(s);
    }
    
    return rv;
}

/*
 * See comment for recvmsg()
 */
ssize_t recvmsg(int s, struct msghdr *msg, int flags) {
    ssize_t rv;

    lnm_init();

    rv = real_recvmsg(s,msg,flags);
    
    if ((rv == 0) && monitorFD_p(s)) {
        DEBUG(printf("Detected a closed socket with zero-length recvmsg()\n"));
        stopFD(s);
    }
    
    return rv;
}
