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
    exit(1);
}

/*
 * Set up our data structures, and go find the real versions of the functions
 * we're going to wrap.
 */
void lnm_init() {

    static bool intialized = false;

    if (intialized == false) {
        DEBUG(printf("Initializing\n"));

        /*
         * Set up the array we use to track which FDs we're tracking
         */
        monitorFDs = (fdRecord*)NULL;
        fdSize = 0;
        allocFDspace();

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

        intialized = true;
    } else {
        /* DEBUG(printf("Skipping intialization\n")); */
    }
}

void startFD(int fd, const struct sockaddr * addr) {

    struct sockaddr_in *inaddr;
    unsigned int socktype, typesize;

    /*
     * Make sure it's an IP connection
     * XXX : Make sure the pointer is valid!
     */
    if (addr->sa_family != AF_INET) {
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

    inaddr = (struct sockaddr_in*)addr;

    /*
     * Give oursleves enough space in the array to record information about
     * this connection
     */
    while (fd >= fdSize) {
        allocFDspace();
    }

    DEBUG(printf("Watching FD %i\n",fd));
    monitorFDs[fd].monitoring = true;

    /*
     * Keep some information about the socket, so that we can print it out
     * later
     */
    monitorFDs[fd].remote_port = ntohs(inaddr->sin_port);
    /* XXX Error checking */
    monitorFDs[fd].remote_hostname = inet_ntoa(inaddr->sin_addr);
}

void stopFD(int fd) {
    if (!monitorFD_p(fd)) {
        return;
    }
    DEBUG(printf("No longer watching FD %i\n",fd));
    monitorFDs[fd].monitoring = false;
    if (monitorFDs[fd].remote_hostname != NULL) {
        monitorFDs[fd].remote_hostname = NULL;
    }
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
        croak("Error in gettimeofday()");
    }
    /*
    fprintf(stderr,"%lu.%08lu [%i, %i]\n",time.tv_sec, time.tv_usec, fd,len);
    */
    fprintf(stdout,"%lu.%06lu > %s.%i (%i)\n",time.tv_sec, time.tv_usec,
            monitorFDs[fd].remote_hostname, monitorFDs[fd].remote_port, len);
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
        startFD(sockfd,serv_addr);
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
 * When the user closes a socket, we can stop monitoring it
 */
int close(int d) {
    int rv;

    lnm_init();

    rv = real_close(d);

    if (!rv && monitorFD_p(d)) {
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
 * write
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
