/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006-2007 University of Utah and the Flux Group.
 *
 * libnetmon, a library for monitoring network traffic sent by a process. See
 * README for instructions.
 */
#include "libnetmon.h"

/*
 * So that we can find out the name of the process that we're instrumenting
 */
extern int argc;
extern char** argv;

/*
 * Die with a standard Emulab-type error message format. In the future, I might
 * try to modify this to simply unlink our wrapper functions so that the app
 * can continue to run.
 */
static void croak(const char *format, ...) {
    va_list ap;
    fprintf(stderr,"*** ERROR\n    libnetmon: ");
    va_start(ap, format);
    vfprintf(stderr,format, ap);
    va_end(ap);
    fflush(stderr);
    abort();
}

/*
 * Set up our data structures, and go find the real versions of the functions
 * we're going to wrap.
 */
void lnm_init() {

    static bool initialized = false;
    char *sockpath;
    char *ctrl_sockpath;
    char *filepath;
    char *std_netmond;

    if (initialized == false) {
        DEBUG(printf("Initializing\n"));

        /*
         * Set up the array we use to track which FDs we're tracking
         */
        monitorFDs = (fdRecord*)NULL;
        fdSize = 0;
        allocFDspace();

        /*
         * By default, monitor TCP sockets only
         */
        monitor_tcp = true;
        monitor_udp = false;

        /*
         * Figure out which version of the output format we're supposed to use
         */
        char *outversion_s;
        outversion_s = getenv("LIBNETMON_OUTPUTVERSION");
        if (outversion_s) {
            if (!sscanf(outversion_s,"%u",&output_version) == 1) {
                croak("Bad output version: %s\n",outversion_s);
            }
        } else {
            output_version = 1;
        }
        DEBUG(printf("Using output version %i\n",output_version));

        /*
         * Find out if we're supposed to monitor UDP sockets. Set to a non-zero
         * int to monitor
         */
        char *monitor_udp_s;
        monitor_udp_s = getenv("LIBNETMON_MONITORUDP");
        if (monitor_udp_s) {
            int flag;
            if (!sscanf(outversion_s,"%i",&flag) == 1) {
                croak("Bad value for LIBNETMON_MONITORUDP: %s\n",
                        monitor_udp_s);
            }
            if (flag) {
                monitor_udp = true;
            } else {
                monitor_udp = false;
            }
        }

        /*
         * Get our PID
         * TODO: We need to handle fork() and its variants - our children could
         * get different PIDs
         */
        pid = getpid();

        /*
         * Default to reporting on everything - the individual reporting
         * options will get turned on later if report_all is true
         */
        report_io = report_sockopt = report_connect = report_init = false;
        report_all = true;
        char *reports_s;
        reports_s = getenv("LIBNETMON_REPORTS");
        if (reports_s) {
            lnm_parse_reportopt(reports_s);
        }

        /*
         * Find the real versions of the library functions we're going to wrap
         */
#define FIND_REAL_FUN(FUN) \
          real_##FUN = (FUN##_proto_t*)dlsym(RTLD_NEXT,#FUN); \
          if (!real_##FUN) \
              croak("Unable to get the address of " #FUN "(): %s\n", \
                    dlerror()); \

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
        FIND_REAL_FUN(sendto);
        FIND_REAL_FUN(sendmsg);

        /*
         * Connect to netmond if we've been asked to
         */

        /*
         * If this is set at all, use the standard data and control sockets for
         * netmond - it's a PITA to set them by hand
         */
        std_netmond = getenv("LIBNETMON_NETMOND");
        if (std_netmond) {
            sockpath = SOCKPATH;
            ctrl_sockpath = CONTROLSOCK;
        } else {
            sockpath = getenv("LIBNETMON_SOCKPATH");
            ctrl_sockpath = getenv("LIBNETMON_CONTROL_SOCKPATH");
        }

        filepath = getenv("LIBNETMON_OUTPUTFILE");
        if (sockpath) {
            int sockfd, contimo;
            struct sockaddr_un servaddr;

            DEBUG(printf("Opening socket at path %s\n",sockpath));

            sockfd = real_socket(AF_LOCAL, SOCK_STREAM, 0);
            if (!sockfd) {
                croak("Unable to create socket\n");
            }

            servaddr.sun_family = AF_LOCAL;
            strcpy(servaddr.sun_path,sockpath);

            char *contimo_s = getenv("LIBNETMON_CONNECTTIMO");
            if (contimo_s && sscanf(contimo_s,"%i", &contimo) == 1)
                    printf("libnetmon: Setting connection timeout to %i seconds\n",
                           contimo);
            else
                    contimo = 1;

            while (1) {
                    if (real_connect(sockfd, (struct sockaddr*) &servaddr,
                                     sizeof(servaddr)) == 0) {
                            break;
                    }
                    if (contimo && --contimo == 0) {
                            croak("Unable to connect to netmond socket\n");
                    }
                    printf("libnetmon: Failed to connect, trying again...\n");
                    sleep(1);
            }

            outstream = fdopen(sockfd,"w");
            if (!outstream) {
                croak("fdopen() failed on socket\n");
            }

            DEBUG(printf("Done opening socket\n"));

        } else if (filepath) {
            /*
             * Also allow output to a seperate file, so that we don't get
             * mixed in with the program's own stdout
             */
            outstream = fopen(filepath,"w+");
            if (!outstream) {
                croak("Unable to open output file\n");
            }
        } else {
            outstream = stdout;
        }

        /*
         * Connect to the netmond's control socket if we've been asked to
         */
        if (ctrl_sockpath) {
            struct sockaddr_un servaddr;

            DEBUG(printf("Opening control socket at path %s\n",ctrl_sockpath));

            controlfd = real_socket(AF_LOCAL, SOCK_STREAM, 0);
            if (!controlfd) {
                croak("Unable to create socket\n");
            }

            servaddr.sun_family = AF_LOCAL;
            strcpy(servaddr.sun_path,ctrl_sockpath);

            if (real_connect(controlfd, (struct sockaddr*) &servaddr,
                             sizeof(servaddr))) {
                croak("Unable to connect to netmond control socket\n");
            }

            /*
             * Set non-blocking, so we can quickly test for presence of
             * packets on the control socket.
             *
             * Note: Another possibility would be to use O_ASYNC on this
             * file descriptor, so that we get a signal when data is
             * available. But, this could interact poorly with apps that
             * use this signal themselves, so this is probably not a
             * good idea.
             */
            if (fcntl(controlfd, F_SETFL, O_NONBLOCK)) {
                croak("Unable to set control socket nonblocking\n");
            }

            /*
             * Ask the server for the parameters we're supposed to use
             */
            control_query();
            lnm_control_wait();
            lnm_control();

            DEBUG(printf("Done opening control socket\n"));

        } else {
            controlfd = -1;
        }

        /*
         * If we got all the way through that with report_all set, turn on the
         * others now
         */
        if (report_all) {
            report_io = report_sockopt = report_connect = report_init = true;
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

        /*
         * Get our command line. I really don't like going into the /proc
         * filesystem to get it, but it beats mucking around in the stack.
         */
        char cmdfile[1024];
        char *cmdline;
        char cmdbuf[4096];
        snprintf(cmdfile,1024,"/proc/%i/cmdline",pid);
        DEBUG(printf("Opening %s\n",cmdfile));
        int cmdfd = open(cmdfile,O_RDONLY);
        /*
         * If we couldn't open it, report why in lieu of the command line
         */
        if (cmdfd < 0) {
            cmdline = strerror(errno);
        } else {
            /*
             * Read as much of the command line as we can, and null terminate
             * it
             */
            int bytesread = real_read(cmdfd,cmdbuf,sizeof(cmdbuf) - 1);
            real_close(cmdfd);
            cmdbuf[bytesread] = '\0';

            /*
             * But wait, there's more! What we got out of /proc is
             * null-separated, so we have to change it into a tidy
             * space-separated string.
             */
            for (int i = 0; i < bytesread - 1; i++) {
                if (cmdbuf[i] == '\0') {
                    cmdbuf[i] = ' ';
                }
            }

            cmdline = cmdbuf;
        }

        /*
         * XXX: Should escape 's, so as not to confuse the monitor
         */
        printlog(LOG_INIT,NO_FD,"'%s'",cmdline);

        initialized = true;
        DEBUG(printf("Done initializing\n"));
    } else {
        /* DEBUG(printf("Skipping intialization\n")); */
    }
}

/*
 * Check for control messages
 */
void lnm_control() {
    ssize_t readrv;
    generic_m msg;

    if (controlfd < 0) {
        return;
    }

    /*
     * Socket is non-blocking
     *
     * NOTE: If read() is too slow, we might want some mechanism to only
     * check this FD every once in a while.
     */
    while ((readrv = real_read(controlfd, &msg, CONTROL_MESSAGE_SIZE))) {
        if (readrv == CONTROL_MESSAGE_SIZE) {
            /*
             * Got a whole message, process it
             */
            process_control_packet(&msg);
        } else if ((readrv < 0) && (errno == EAGAIN)) {
            /*
             * Normal case - no data ready for us
             */
            break;
        } else {
            // For now, croak. We can probably do something better
            croak("Error reading on control socket\n");
        }
    }

    return;
}

/*
 * Wait for a control message, then process it
 */
void lnm_control_wait() {
    fd_set fds;
    int selectrv;
    struct timeval tv;

    if (controlfd < 0) {
        return;
    }

    FD_ZERO(&fds);
    FD_SET(controlfd,&fds);

    tv.tv_sec = 10;
    tv.tv_usec = 0;

    DEBUG(printf("Waiting for a control message\n"));
    selectrv = select(controlfd + 1, &fds, NULL, NULL, &tv);
    if (selectrv == 0) {
        croak("Timed out waiting for a control message\n");
    } else if (selectrv < 0) {
        croak("Bad return value from select() in lnm_control_wait()\n");
    }

    DEBUG(printf("Done waiting for a control message\n"));

    lnm_control();

    return;
}

/*
 * 'Name' a file descriptor by getting a unique identifier for it - port
 * numbers, IP addresses, etc.
 */
void nameFD(int fd, const struct sockaddr *localname,
        const struct sockaddr *remotename) {
    struct sockaddr_in *remoteaddr, *localaddr;
    union {
        struct sockaddr sa;
        char data[128];
    } sockname;
    socklen_t namelen;

    if (remotename == NULL) {
        int gpn_rv;
        namelen = sizeof(sockname.data);
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
     * Keep some information about the socket, so that we can print it out
     * later
     */
    monitorFDs[fd].remote_port = ntohs(remoteaddr->sin_port);
    monitorFDs[fd].remote_hostname = remoteaddr->sin_addr;

    /*
     * Get the local port number
     */
    int gsn_rv;
    if (localname == NULL) {
        namelen = sizeof(sockname.data);
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

}

/*
 * Start monitoring a new file descriptor
 */
void startFD(int fd) {
    unsigned int socktype, typesize;
    int sndsize, rcvsize;

    if (monitorFD_p(fd)) {
        printf("Warning: Tried to start monitoring an FD already in use!\n");
        stopFD(fd);
    }

    /*
     * Make sure it's an IP connection
     * XXX : Make sure the pointer is valid!
     */
    /*
     * XXX : Fix this!
    if (remotename->sa_family != AF_INET) {
        DEBUG(printf("Ignoring a non-INET socket\n"));
        return;
    }
    */
    /*
     * Check to make sure it's a type of socket we're supposed to be monitoring
     */
    typesize = sizeof(unsigned int);
    if (getsockopt(fd,SOL_SOCKET,SO_TYPE,&socktype,&typesize) != 0) {
        croak("Unable to get socket type: %s\n",strerror(errno));
    }
    if (!((monitor_tcp && socktype == SOCK_STREAM) ||
          (monitor_udp && socktype == SOCK_DGRAM))) {
        DEBUG(printf("Ignoring a type socket we're not monitoring\n"));
        return;
    }

    allocFDspaceFor(fd);

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

    /*
     * Find out the socket buffer sizes
     */
    sndsize = getNewSockbuf(fd,SO_SNDBUF);
    rcvsize = getNewSockbuf(fd,SO_RCVBUF);

    if (forced_bufsize && (sndsize > forced_bufsize)) {
        printf("Warning: Tried to force SO_SNBUF to %i but got %i\n",
                forced_bufsize, sndsize);
    }

    if (forced_bufsize && (rcvsize > forced_bufsize)) {
        printf("Warning: Tried to force SO_RCVBUF to %i but got %i\n",
                forced_bufsize, rcvsize);
    }

    /*
     * For version 3 and up, port numbers are not used for socket identifiers,
     * so we can report a New event now. For earlier output versions, this gets
     * reported in informConnect()
     */
    if (output_version >= 3) {
        printlog(LOG_NEW,fd,(socktype == SOCK_STREAM)?"TCP":"UDP");
    }

    monitorFDs[fd].monitoring = true;
    monitorFDs[fd].connected = false;
    monitorFDs[fd].socktype = socktype;

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
    printlog(LOG_CLOSED,fd);

    monitorFDs[fd].monitoring = false;
/* // XXX: If changed to dynamic memory, remove memory leak here.
    // XXX: Possible memory leak?
    if (monitorFDs[fd].remote_hostname != NULL) {
        monitorFDs[fd].remote_hostname = NULL;
    }
*/
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
    printlog(LOG_EXIT,NO_FD);
}

void printlog(logmsg_t type, int fd, ...) {
    /*
     * A set of variables that turns on and off specific parts of the output
     * string
     */
    bool print_name = true;
    bool print_id = true;
    bool id_is_pid = false;
    bool print_timestamp = true;
    bool print_value = true;

    /*
     * If this is set to false, we bypass logging completely.
     */
    bool print = true;

    DEBUG(printf("printlog(%i,%i,...) called\n",type,fd));

    /*
     * Decide what to print based on the type of the log message and the output
     * version
     */
    switch (type) {
        case LOG_NEW:
            // Old versions didn't print a timestamp or socket type
            if (output_version < 3) { print_timestamp = print_value = false; }
            break;
        case LOG_REMOTEIP:
            // This message only showed up in version 3
            if (output_version < 3) { print = false; }
            break;
        case LOG_REMOTEPORT:
            // This message only showed up in version 3
            if (output_version < 3) { print = false; }
            break;
        case LOG_LOCALPORT:
            // This message only showed up in version 1
            if (output_version < 1) { print = false; }
            // Old version didn't include a timestamp
            if (output_version < 3) { print_timestamp = false; }
            // Only report if we're also reporting connection info
            if (!report_connect) { print = false; }
            break;
        case LOG_TCP_NODELAY:
            // Old version didn't include a timestamp
            if (output_version < 3) { print_timestamp = false; }
            // This message only showed up in version 2
            if (output_version < 2) { print = false; }
            // Allow global turning on/off of this message type
            if (!report_sockopt) { print = false; }
            break;
        case LOG_TCP_MAXSEG:
            // Old version didn't include a timestamp
            if (output_version < 3) { print_timestamp = false; }
            // This message only showed up in version 2
            if (output_version < 2) { print = false; }
            // Allow global turning on/off of this message type
            if (!report_sockopt) { print = false; }
            break;
        case LOG_SO_RCVBUF:
            // Old version didn't include a timestamp
            if (output_version < 3) { print_timestamp = false; }
            // This message only showed up in version 2
            if (output_version < 2) { print = false; }
            // Allow global turning on/off of this message type
            if (!report_sockopt) { print = false; }
            break;
        case LOG_SO_SNDBUF:
            // Old version didn't include a timestamp
            if (output_version < 3) { print_timestamp = false; }
            // This message only showed up in version 2
            if (output_version < 2) { print = false; }
            // Allow global turning on/off of this message type
            if (!report_sockopt) { print = false; }
            break;
        case LOG_CONNECTED:
            // No Value
            print_value = false;
            // This message only showed up in version 2
            if (output_version < 2) { print = false; }
            // Allow global turning on/off of this message type
            if (!report_connect) { print = false; }
            break;
        case LOG_ACCEPTED:
            // No Value
            print_value = false;
            // This message only showed up in version 2
            if (output_version < 2) { print = false; }
            // Allow global turning on/off of this message type
            if (!report_connect) { print = false; }
            break;
        case LOG_SEND:
            // This should be handled by calling log_packet()
            croak("LOG_SEND passed to printlog()");
            break;
        case LOG_SENDTO:
            // This should be handled by calling log_packet()
            croak("LOG_SENDTO passed to printlog()");
            break;
        case LOG_CLOSED:
            // No Value
            print_value = false;
            // Old version didn't include a timestamp
            if (output_version < 3) { print_timestamp = false; }
            // This message only showed up in version 2
            if (output_version < 2) { print = false; }
            // Allow global turning on/off of this message type
            if (!report_connect) { print = false; }
            break;
        case LOG_INIT:
            if (output_version < 3) { print = false; }
            if (!report_init) { print = false; }
            id_is_pid = true;
            break;
        case LOG_EXIT:
            if (output_version < 3) { print = false; }
            if (!report_init) { print = false; }
            id_is_pid = true;
            print_value = false;
            break;
        case LOG_SENDMSG:
            // In version 3, we don't actually log anything about the contents
            // of the call - we just want to make sure it isn't sneaking past
            // us. Techinically, it doesn't have to be UDP, but we'll lump it
            // in for now
            if (output_version < 3) { print = false; }
            if (!monitor_udp) { print = false; }
            print_value = false;
        default:
            croak("Invalid type (%i) passed to printlog()\n",type);
    }

    /*
     * If we've decided not to print anything at all, bail now
     */
    if (!print) {
        return;
    }

    /*
     * Print out the name of the command
     */
    if (print_name) {
        char *logname = log_type_names[type];
        fprintf(outstream,"%s: ",logname);
    }

    /*
     * Print out the ID of the socket the action is for. Some actions are not
     * associated with a specific socket, so we use the pid as the identifier
     */
    if (print_id) {
        if (id_is_pid) {
            fprintf(outstream,"%i ",pid);
        } else {
            fprintID(outstream,fd);
            fprintf(outstream," ");
        }
    }

    /*
     * Print out a timestamp
     */
    if (print_timestamp) {
        fprintTime(outstream);
        fprintf(outstream," ");
    }

    va_list ap;
    va_start(ap,fd);
    if (print_value) {
        /*
         * First variadic argument is the format string, which must be passed
         * specially to vfprintf;
         */
        char *fmt = va_arg(ap,char*);
        vfprintf(outstream,fmt,ap);
    }
    va_end(ap);

    fprintf(outstream,"\n");
}

/*
 * Print the unique identifier for a connection to the given filestream
 */
void fprintID(FILE *f, int fd) {

    if (fd == NO_FD) {
        croak("NO_FD passed to fprintID");
    } else if (fd < 0) {
        croak("Negative pid (%i) passed to fprintID");
    }

    if (output_version <= 2) {
        /*
         * Note, we've switched from local_port to FD for the first field - this is
         * so that we can report on a connection before connect() finishes
         */
        fprintf(f,"%i:%s:%i", fd,
                inet_ntoa(monitorFDs[fd].remote_hostname),
                monitorFDs[fd].remote_port);
    } else if (output_version == 3) {
        fprintf(f,"%i:%i",pid,fd);
    } else {
        croak("Improper output version");
    }

}

/*
 * Print out the current time in standard format
 */
void fprintTime(FILE *f) {
    struct timeval time;
    /*
     * XXX - At some point, we may want to use something more precise than
     * gettimeofday()
     */
    if (gettimeofday(&time,NULL)) {
        croak("Error in gettimeofday()\n");
    }
    fprintf(outstream,"%lu.%08lu",time.tv_sec, time.tv_usec);
}


/*
 * Handle a control message from netmond
 */
void process_control_packet(generic_m *m) {
    max_socket_m *maxmsg;
    out_ver_m *vermsg;
    reports_m *reportmsg;
    monitorudp_m *monitorudpmsg;

    DEBUG(printf("Processing control packet\n"));

    switch (m->type) {
        case CM_MAXSOCKSIZE:
            /*
             * The server told us what the socket buffer sizes should be
             */
            maxmsg = (max_socket_m *)m;

            if (maxmsg->force == 0) {
                forced_bufsize = 0;
            } else {
                forced_bufsize = maxmsg->force_size;
            }

            if (maxmsg->limit == 0) {
                max_bufsize = 0;
            } else {
                max_bufsize = maxmsg->limit_size;
            }

            DEBUG(printf("Set forced_bufsize = %i, max_bufsize = %i\n",
                        forced_bufsize, max_bufsize));
            break;
        case CM_OUTPUTVER:
            /*
             * The server told us which output version to use
             */
            vermsg = (out_ver_m *)m;
            output_version = vermsg->version;

            DEBUG(printf("Set output version to %i\n", output_version));

            break;
        case CM_REPORTS:
            /*
             * The server told us which things it wants us to report
             */
            reportmsg = (reports_m *)m;

            /*
             * Just for the heck of it, null out the last character to prevent
             * any dumb string functions from going off the end
             */
            reportmsg->reports[CONTROL_MESSAGE_PAYLOAD_SIZE - 1] = '\0';

            DEBUG(printf("Setting reports to %s\n", reportmsg->reports));

            lnm_parse_reportopt(reportmsg->reports);

            break;
        case CM_MONITORDUP:
            /*
             * The server is telling us whether it wants us to monitor UDP
             * sockets
             */
            monitorudpmsg = (monitorudp_m*)m;

            if (monitorudpmsg->enable) {
                DEBUG(printf("Enabling UDP monitoring\n"));
                monitor_udp = true;
            } else {
                DEBUG(printf("Disabling UDP monitoring\n"));
                monitor_udp = false;
            }

            break;
        default:
            croak("Got an unexpected control message type: %i\n",
                   (void *)m->type);
    }
}

/*
 * Send out a query to the control socket
 */
void control_query() {
    generic_m msg;
    query_m *qmsg;

    if (!controlfd) {
        croak("control_query() called without control socket\n");
    }

    qmsg = (query_m *)&msg;
    qmsg->type = CM_QUERY;

    if ((real_write(controlfd,&msg,CONTROL_MESSAGE_SIZE) !=
                CONTROL_MESSAGE_SIZE)) {
        croak("Error writing control query\n");
    }

    return;

}

/*
 * Parse a list of reports, setting the appropriate flags
 */
static void lnm_parse_reportopt(char *s) {
    char *p = s;

    /*
     * Turn 'all' off, since if they ask for specific ones, we only
     * give them those - but, we might set it back to true if they
     * say 'all'...
     */
    report_all = false;

    while (p != NULL && *p != '\0') {

        /*
         * Find out how many chars before the next option or end of
         * string, whichever comes first
         */
        int numchars;
        char* nextcomma = strchr(p,',');
        if (nextcomma == NULL) {
            numchars = strlen(p);
        } else {
            numchars = nextcomma - p;
        }

        /*
         * Not-so-fancy parsing
         */
        if (!strncmp(p,"all",numchars)) {
            report_all = true;
        } else if (!strncmp(p,"io",numchars)) {
            report_io = true;
        } else if (!strncmp(p,"connect",numchars)) {
            report_connect = true;
        } else if (!strncmp(p,"sockopt",numchars)) {
            report_sockopt = true;
        } else if (!strncmp(p,"init",numchars)) {
            report_init = true;
        } else {
            croak("Bad report: %s\n",p);
        }

        if (nextcomma) {
            p = nextcomma + 1;
        } else {
            p = NULL;
        }
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

void allocFDspaceFor(int fd) {
    while (fd >= fdSize) {
        allocFDspace();
    }
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

bool connectedFD_p(int whichFD) {
    if (whichFD >= fdSize) {
        return false;
    } else {
        return monitorFDs[whichFD].connected;
    }
}

/*
 * Let the user know that a packet has been sent. This function is, for now,
 * seperate from printlog() because it prints out very different messages for
 * early versions of the output format
 */
void log_packet(int fd, size_t len, const struct sockaddr *srvaddr) {
    if (!report_io) {
        return;
    }
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
                    time.tv_usec, inet_ntoa(monitorFDs[fd].remote_hostname),
                    monitorFDs[fd].remote_port, len);
            break;
        case 2:
            fprintf(outstream,"%lu.%06lu > ", time.tv_sec, time.tv_usec);
            fprintID(outstream,fd);
            fprintf(outstream," (%i)\n", len);
            break;
        case 3:
            /*
             * We handle send() and sendto() differently - specificially, we
             * assume sendto() calls are UDP, and thus we need to log the dest
             * IP, plus local and remote ports. (Note: It is legal to call
             * sendto() on a TCP socket, but you must give a null srvaddr,
             * which will result in us handling as a send())
             */
            if (monitorFDs[fd].socktype == SOCK_STREAM) {
                // send()
                fprintf(outstream,"Send: ");
                fprintID(outstream,fd);
                fprintf(outstream," %lu.%06lu %i\n", time.tv_sec, time.tv_usec,
                        len);
            } else {
                int local_port = monitorFDs[fd].local_port;
                char *remote_ip;
                int remote_port;
                if (srvaddr != NULL) {
                    const struct sockaddr_in *inaddr =
                        (const struct sockaddr_in*)srvaddr ;
                    /*
                     * XXX - We should cache this so we don;t have to
                     * re-compute it so many times.
                     * XXX - This is probably not thread-safe
                     */
                    remote_ip = inet_ntoa(inaddr->sin_addr);
                    remote_port = ntohs(inaddr->sin_port);
                } else {
                    if (! connectedFD_p(fd)) {
                        croak("Attempted to call sendto() on an unconnected "
                               "socket without a srvaddr");
                    }
                    remote_ip = inet_ntoa(monitorFDs[fd].remote_hostname);
                    remote_port = monitorFDs[fd].remote_port;
                }
                fprintf(outstream,"SendTo: ");
                fprintID(outstream,fd);
                fprintf(outstream," %lu.%06lu %i:%s:%i:%i\n",
                        time.tv_sec, time.tv_usec,
                        local_port, remote_ip, remote_port,
                        len);
            }
            break;
        default:
            croak("Bad output version: %i\n", (void *)output_version);
    }
    fflush(outstream);
}

/*
 * Inform the user that the nodelay flag has been changed
 */
void informNodelay(int fd) {
    if (monitorFDs[fd].socktype == SOCK_STREAM) {
        printlog(LOG_TCP_NODELAY, fd, "%i",monitorFDs[fd].tcp_nodelay);
    }
}

void informMaxseg(int fd) {
    if (monitorFDs[fd].socktype == SOCK_STREAM) {
        printlog(LOG_TCP_MAXSEG, fd, "%i",monitorFDs[fd].tcp_maxseg);
    }
}

void informBufsize(int fd, int which) {
    /* TODO: Handle bad which parameter */
    printlog((which == SO_SNDBUF) ? LOG_SO_SNDBUF : LOG_SO_RCVBUF,
             fd, "%i",
             (which == SO_SNDBUF) ? monitorFDs[fd].sndbuf :
                                    monitorFDs[fd].rcvbuf);

}

void informConnect(int fd, inform_which_t which) {
    /*
     * Let the monitor know about it - note: if it's a UDP socket, we've
     * already reported on it in startFD. Note that, for version 3, we
     * report the existence of the socket earler, in startFD
     */
    if ((output_version < 3) && monitorFDs[fd].socktype == SOCK_STREAM) {
        printlog(LOG_NEW, fd,
                 (monitorFDs[fd].socktype == SOCK_STREAM)?"TCP":"UDP");
    }

    /*
     * New versions of the output no longer include port numbers in the
     * identifier. So, report those now. Note that we only do this for TCP
     * sockets - UDP sockets will get this information reported with each
     * sendto()
     */
    if (monitorFDs[fd].socktype == SOCK_STREAM){
        printlog(LOG_REMOTEIP,fd,"%s",inet_ntoa(monitorFDs[fd].remote_hostname));
        printlog(LOG_REMOTEPORT,fd,"%i",monitorFDs[fd].remote_port);
    }

    /*
     * Some things we report on for every connection
     */
    informNodelay(fd);
    informMaxseg(fd);
    informBufsize(fd, SO_RCVBUF);
    informBufsize(fd, SO_SNDBUF);

    if (which == INFORM_CONNECT) {
        printlog(LOG_CONNECTED,fd);
    } else {
        printlog(LOG_ACCEPTED,fd);
    }
}

int getNewSockbuf(int fd, int which) {
    socklen_t newsize;
    socklen_t optsize;
    optsize = sizeof(newsize);
    if (getsockopt(fd,SOL_SOCKET,which,&newsize,&optsize)) {
        croak("Unable to get socket buffer size");
        /* Make GCC happy - won't get called */
        return 0;
    } else {
        if (which == SO_SNDBUF) {
            monitorFDs[fd].sndbuf = newsize;
        } else {
            monitorFDs[fd].rcvbuf = newsize;
        }

        return newsize;
    }
}

/*
 * Library function wrappers
 */
int socket(int domain, int type, int protocol) {
    int returnedFD;
    lnm_init();
    DEBUG(printf("socket() called\n"));
    returnedFD = real_socket(domain, type, protocol);
    if (returnedFD > 0) {
        startFD(returnedFD);
    }

    return returnedFD;
}

/*
 * We're only going to bother to monitor FDs after they have succeeded in
 * connecting to some host.
 *
 * TODO: Allow for some filters:
 *      Only certain hosts? (eg. not loopback)
 */
int connect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen) {

    int rv;
    lnm_init();
    lnm_control();

    DEBUG(printf("connect() called on %i\n",sockfd));

    /*
     * So, this is a bit messy, but we gotta do it this way. We report on the
     * socket _before_ calling the real connect. This help keep the stub
     * from getting too far behind - otherwise, it can't start until the real
     * app has finished the three way handshake and is well on its way to
     * sending data. If connect() fails, we'll report a socket close below.
     */

    /*
     * Find out some things about the address we're trying to connect to. We
     * decided in socket() if we're going to monitor this connection or not
     * TODO: We really should verify that serv_addr is a legal pointer
     */
    if (monitorFD_p(sockfd)) {
        nameFD(sockfd,NULL,serv_addr);
        informConnect(sockfd,INFORM_CONNECT);
    }

    rv = real_connect(sockfd, serv_addr, addrlen);

    if (!monitorFD_p(sockfd)) {
        /*
         * Just pass the result back
         */
        return rv;
    }

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
        monitorFDs[sockfd].connected = true;

        /*
         * Get the local port number so that we can monitor it
         */
        struct sockaddr_in localaddr;
        socklen_t namelen = sizeof(localaddr);
        if (getsockname(sockfd, (struct sockaddr*)&localaddr,&namelen) != 0) {
            croak("Unable to get local socket name: %s\n", strerror(errno));
        }
        int local_port = ntohs(localaddr.sin_port);
        monitorFDs[sockfd].local_port = local_port;

        /*
         * Report on the local port number
         */
        if (monitorFDs[sockfd].socktype == SOCK_STREAM) {
            printlog(LOG_LOCALPORT,sockfd,"%i",local_port);
        }
    } else {
        /*
         * Do this in case the connection really did fail
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
    lnm_control();

    DEBUG(printf("accept() called on %i\n",s));

    rv = real_accept(s, addr, addrlen);

    if (!monitorFD_p(s)) {
        return rv;
    }

    if (rv > 0) {
        /*
         * Got a new client! Start it up, name it, and report on its local port
         */
        startFD(rv);
        nameFD(rv,NULL,addr);
        informConnect(rv,INFORM_ACCEPT);
        // XXX Accessors
        monitorFDs[rv].connected = 1;
        printlog(LOG_LOCALPORT,rv,"%i",monitorFDs[rv].local_port);
    }

    return rv;

}

/*
 * When the user closes a socket, we can stop monitoring it
 */
int close(int d) {
    int rv;

    lnm_init();
    lnm_control();

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
 * writev
 * others?
 */
ssize_t send(int s, const void *msg, size_t len, int flags) {
    ssize_t rv;

    lnm_init();
    lnm_control();

    /*
     * Wait until _after_ the packet is sent to log it, since the call might
     * block, and we basically want to report when the kernel acked receipt of
     * the packet
     */
    /*
     * TODO: There are a LOT of error cases, flags, etc, that we should handle.
     */
    rv = real_send(s,msg,len,flags);

    if ((rv > 0) && monitorFD_p(s)) {
        log_packet(s,rv,NULL);
    }

    return rv;

}

ssize_t write(int fd, const void *buf, size_t count) {
    ssize_t rv;

    lnm_init();
    lnm_control();

    /*
     * Wait until _after_ the packet is sent to log it, since the call might
     * block, and we basically want to report when the kernel acked receipt of
     * the packet
     */
    rv = real_write(fd,buf,count);

    if ((rv > 0) && monitorFD_p(fd)) {
        log_packet(fd,rv,NULL);
    }

    return rv;

}

/*
 * Wrap the sendto() function to capture writes to UDP sockets
 */
ssize_t sendto(int fd, const void *buf, size_t count, int flags,
               const struct sockaddr *serv_addr, socklen_t addrlen) {
    ssize_t rv;

    lnm_init();
    lnm_control();

    rv = real_sendto(fd, buf, count, flags, serv_addr, addrlen);

    if ((rv > 0) && monitorFD_p(fd)) {
        /*
         * If this socket is UDP and not connected, we need to make sure that we
         * have the local port number, so that we can include it in the sendto()
         * reports
         */
        if (monitorFDs[fd].socktype == SOCK_DGRAM && !connectedFD_p(fd) &&
            monitorFDs[fd].local_port == 0) {
            // TODO: This overlaps a bit with nameFD - there is probably some
            // refactoring that should be done.
            struct sockaddr_in *localaddr;
            union {
                struct sockaddr sa;
                char data[128];
            } sockname;
            socklen_t namelen;
            namelen = sizeof(sockname.data);
            int gsn_rv = getsockname(fd,(struct sockaddr *)sockname.data,&namelen);
            if (gsn_rv != 0) {
                croak("Unable to get local socket name: %s\n", strerror(errno));
            }
            localaddr = (struct sockaddr_in *) &(sockname.sa);
            monitorFDs[fd].local_port = ntohs(localaddr->sin_port);
        }

        /*
         * Report on the actual packet
         */
        log_packet(fd,rv,serv_addr);
    }

    return rv;

}

/*
 * Wrap the sendmsg() function 
 */
ssize_t sendmsg(int s, const struct msghdr *msg, int flags) {
    ssize_t rv;

    lnm_init();
    lnm_control();

    /*
     * Wait until _after_ the packet is sent to log it, since the call might
     * block, and we basically want to report when the kernel acked receipt of
     * the packet
     */
    rv = real_sendmsg(s,msg,flags);

    if ((rv > 0) && monitorFD_p(s)) {
        printlog(LOG_SENDMSG,s);
    }

    return rv;

}

int setsockopt (int s, int level, int optname, const void *optval,
                 socklen_t optlen) {
    int rv;

    lnm_init();
    lnm_control();

    DEBUG(printf("setsockopt called (%i,%i)\n",level,optname));

    /*
     * If we're supposed to monitor this FD, there are some things that can
     * make us ignore or cap this call.
     */
    if (monitorFD_p(s)) {
        /*
         * Note, we do this on all sockets, not just those we are currently
         * monitoring, since it's likely they'll call setsockopt() before
         * connect()
         */
        if ((level == SOL_SOCKET) && ((optname == SO_SNDBUF) ||
                                      (optname == SO_RCVBUF))) {
            if (forced_bufsize) {
                /*
                 * I believe this is the right thing to do - return success but
                 * don't do anything - I think that this is what you normally get
                 * when you, say, pick a socket buffer size that is too big.
                 */
                printf("Warning: Ignored attempt to change SO_SNDBUF or "
                       "SO_RCVBUF\n");
                return 0;
            } else if (max_bufsize && (*((int *)optval) > max_bufsize)) {
                printf("Warning: Capped attempt to change SO_SNDBUF or "
                       "SO_RCVBUF\n");
                *((int *)optval) = max_bufsize;
            }

        }
    }

    /*
     * Actually call the real thing
     */
    rv = real_setsockopt(s,level,optname,optval,optlen);

    /*
     * If the call succeeded, we have to record some more information about it
     */
    if (rv == 0 && monitorFD_p(s)) {
        if ((level == SOL_SOCKET) && ((optname == SO_SNDBUF) ||
                                      (optname == SO_RCVBUF))) {
            /*
             * We have to get the socket buffer size the kernel chose: it might
             * not be exactly what the user asked for
             */
            getNewSockbuf(s,optname);
            if (connectedFD_p(s)) {
                informBufsize(s,optname);
            }
        }

        /*
         * There are some TCP options we have to watch for
         */
        if (level == IPPROTO_TCP) {
            if (optname == TCP_NODELAY) {
                monitorFDs[s].tcp_nodelay = *((int *)optval);

                if (connectedFD_p(s)) {
                     /* If connected, inform user of this call */
                    informNodelay(s);
                }
            }
            if (optname == TCP_MAXSEG) {
                monitorFDs[s].tcp_maxseg = *((int *)optval);

                if (connectedFD_p(s)) {
                    /* If connected, inform user of this call */
                    informMaxseg(s);
                }
            }
        }
    }

    /*
     * Finally, give back the returned value
     */
    return rv;
}

/*
 * The usual way to find 'eof' on a socket is to look for a small read
 * read. Since some programs might not be well-behaved in the sense that they
 * may not close() the socket, we wrap read() too
 */
ssize_t read(int d, void *buf, size_t nbytes) {
    ssize_t rv;

    lnm_init();
    lnm_control();

    DEBUG(printf("read() called\n"));

    rv = real_read(d,buf,nbytes);

    if ((rv == 0) && monitorFD_p(d)) {
        DEBUG(printf("Detected a closed socket with a zero-length read()\n"));
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
    lnm_control();

    rv = real_recv(s,buf,len,flags);

    DEBUG(printf("recv() returned %i\n",rv));

    if ((rv == 0) && monitorFD_p(s)) {
        DEBUG(printf("Detected a closed socket with a zero-length recv()\n"));
        stopFD(s);
    }


    return rv;
}

/*
 * See comment for read()
 */
ssize_t recvmsg(int s, struct msghdr *msg, int flags) {
    ssize_t rv;

    lnm_init();
    lnm_control();

    rv = real_recvmsg(s,msg,flags);

    if ((rv == 0) && monitorFD_p(s)) {
        DEBUG(printf("Detected a closed socket with zero-length recvmsg()\n"));
        stopFD(s);
    }

    return rv;
}
