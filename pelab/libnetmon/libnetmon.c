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
    fprintf(stderr,"*** ERROR\n    libnetmon:");
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
        monitorFDs = (bool*)NULL;
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

        /*
        real_socket = (socket_proto_t*)dlsym(RTLD_NEXT,"socket");
        if (!real_socket) {
            croak("Unable to get the address of socket(): %s\n",
                    dlerror());
        }

        real_close = (close_proto_t*)dlsym(RTLD_NEXT,"close");
        if (!real_close) {
            croak("Unable to get the address of close(): %s\n",
                    dlerror());
        }

        real_connect = (connect_proto_t*)dlsym(RTLD_NEXT,"connect");
        if (!real_connect) {
            croak("Unable to get the address of connect(): %s\n",
                    dlerror());
        }

        real_write = (write_proto_t*)dlsym(RTLD_NEXT,"write");
        if (!real_write) {
            croak("Unable to get the address of write(): %s\n",
                    dlerror());
        }

        real_send = (send_proto_t*)dlsym(RTLD_NEXT,"send");
        if (!real_send) {
            croak("Unable to get the address of send(): %s\n",
                    dlerror());
        }
        */

        intialized = true;
    } else {
        /* DEBUG(printf("Skipping intialization\n")); */
    }
}

void allocFDspace() {
    bool *allocRV;
    unsigned int newFDSize;

    DEBUG(printf("Allocating space for %i FDs\n",newFDSize));

    /*
     * Pick a new size, and use realloc() to grown our current allocation
     */
    newFDSize = fdSize + FD_ALLOC_SIZE;
    allocRV = realloc(monitorFDs, newFDSize * sizeof(bool));
    if (!allocRV) {
        croak("Unable to malloc space for monitorFDs array\n");
    }
    monitorFDs = allocRV;

    /*
     * Set newly-allocated entries to 0
     */
    bzero(monitorFDs + fdSize, (newFDSize - fdSize) * sizeof(bool));

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
        return monitorFDs[whichFD];
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
    /* fprintf(stderr,"Sending packet on fd %i, length %i\n",fd,len); */
    /*
     * XXX - At some point, we may want to use something more precise than
     * gettimeofday()
     */
    if (gettimeofday(&time,NULL)) {
        croak("Error in gettimeofday()");
    }
    fprintf(stderr,"%lu.%08lu [%i, %i]\n",time.tv_sec, time.tv_usec, fd,len);
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

    while (sockfd >= fdSize) {
        allocFDspace();
    }
    if (rv == 0) {
        DEBUG(printf("Watching FD %i\n",sockfd));
        monitorFDs[sockfd] = true;
    } else {
        /*
         * There are actually some cases when connect() can 'fail', but we
         * still want to watch the FD
         */
        if ((errno == EISCONN) ||     /* Socket is already connected */
            (errno == EINPROGRESS) || /* Non blocking socket */
            (errno == EINTR) ||       /* Connect will happen in background */
            (errno == EALREADY)) {    /* In progress in background */
            /*
             * TODO: In the case of the 'errors' that mean the socket is
             * connecting in the background, we really should make sure that
             * it actually connects - but this could be tricky. The caller is
             * supposed to select() on the FD to find out when it's ready, but
             * if they don't, and just write to it, we won't find out. So, for
             * now, just assume that the connect() will succeed.
             */
            DEBUG(printf("Watching FD %i (error)\n",sockfd));
            monitorFDs[sockfd] = true;
        } else {
            /*
             * Do this in case they called connect() to reconnect a previously
             * connected socket. If it fails, stop watching the FD
             */
            monitorFDs[sockfd] = false;
        }
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

    if ((!rv) && (d <= fdSize) && (monitorFDs[d])) {
        DEBUG(printf("No longer watching FD %i\n",d));
        monitorFDs[d] = false;
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
