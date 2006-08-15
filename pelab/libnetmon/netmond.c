/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 *
 * netmond, a 'server' for libnetmon - simply repeat what a process being
 * monitored with libnetmon tell us on a unix-domian socket
 */

/*
 * Get unitstd.h on Linux to declare getopt()
 */
#ifdef __linux__
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>

#include "netmon.h"

extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;
extern int optreset;

typedef enum { DATA = 0, CONTROL = 1} socktype;
const unsigned int MAX_SOCKS = 256;

/*
 * Output version clients should use
 */
unsigned int output_version = 2;

/*
 * Limit the maximum socket size
 */
unsigned int max_socksize = 0;

/*
 * Force a specific socket buffer size
 */
unsigned int forced_socksize = 0;

/*
 * Only enable a specific set of reports
 */
char *reports = NULL;

/*
 * Give a client a report on what settings it should use.
 *
 * Returns non-zero if it gets an error - probably means the client should
 * be disconnected
 */
unsigned int write_status(int fd) {
    generic_m message;
    max_socket_m *sockrep;
    out_ver_m *verrep;
    reports_m *reprep;

    /*
     * Report on the maximum socket size
     */
    sockrep = (max_socket_m *)&(message);
    sockrep->type = CM_MAXSOCKSIZE;

    if (max_socksize) {
        sockrep->limit = 1;
    } else {
        sockrep->limit = 0;
    }
    sockrep->limit_size = max_socksize;

    if (forced_socksize) {
        sockrep->force = 1;
    } else {
        sockrep->force = 0;
    }
    sockrep->force_size = forced_socksize;

    if (write(fd,&message,CONTROL_MESSAGE_SIZE) != CONTROL_MESSAGE_SIZE) {
        /* Client probably disconnected */
        return 1;
    }

    /*
     *  Report on the output version in use
     */
    verrep = (out_ver_m *)&(message);
    verrep-> type = CM_OUTPUTVER;
    verrep->version = output_version;

    if (write(fd,&message,CONTROL_MESSAGE_SIZE) != CONTROL_MESSAGE_SIZE) {
        /* Client probably disconnected */
        return 1;
    }

    /*
     *  Report on which reports we want
     */
    if (reports != NULL) {
        reprep = (reports_m *)&(message);
        reprep-> type = CM_REPORTS;
        strcpy(reprep->reports,reports);
        reprep->reports[strlen(reports)] = '\0';

        if (write(fd,&message,CONTROL_MESSAGE_SIZE) != CONTROL_MESSAGE_SIZE) {
            /* Client probably disconnected */
            return 1;
        }
    }

    return 0;
}

void usage() {
    fprintf(stderr,"Usage: netmond [-v version] [-l size] [-f size]\n");
    fprintf(stderr,"    -v version      Specify output version (default is 2)\n");
    fprintf(stderr,"    -l size         Maximum socket buffer size\n");
    fprintf(stderr,"    -f size         Force a socket buffer size\n");
    fprintf(stderr,"    -r reports      Enable listed reports\n");
    exit(-1);
}

int main(int argc, char **argv) {

    int sockfd;
    int controlsockfd;
    struct sockaddr_un servaddr;
    struct sockaddr_un cservaddr;
    int max_clientfd;
    fd_set real_fdset, returned_fdset;
    int i;
    socktype sock_types[MAX_SOCKS];
    char ch;

    /*
     * Process command-line args
     */
    while ((ch = getopt(argc, argv, "f:l:v:r:")) != -1) {
        switch(ch) {
            case 'f':
                if (sscanf(optarg,"%i",&forced_socksize) != 1) {
                    usage();
                }
                break;
            case 'l':
                if (sscanf(optarg,"%i",&max_socksize) != 1) {
                    usage();
                }
                break;
            case 'v':
                if (sscanf(optarg,"%i",&output_version) != 1) {
                    usage();
                }
                break;
            case 'r':
                reports = (char*)malloc(strlen(optarg) + 1);
                strcpy(reports,optarg);
                break;
            default:
                usage();
        }
    }

    argc -= optind;
    argv += optind;

    if (argc != 0) {
        usage();
    }

    /*
     * Make ourselves a socket
     */
    sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);

    /*
     * Bind to a local socket
     */
    unlink(SOCKPATH);
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, SOCKPATH);
    
    if (bind(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr))) {
        perror("Failed to bind() socket\n");
        return 1;
    }

    if (listen(sockfd, 10)) {
        perror("Failed to listen() on socket\n");
        return 1;
    }

    if (chmod(SOCKPATH,0666)) {
        perror("Failed to chmod() socket\n");
        return 1;
    }

    /*
     * Also bind to the control socket
     */
    controlsockfd = socket(AF_LOCAL, SOCK_STREAM, 0);

    unlink(CONTROLSOCK);
    cservaddr.sun_family = AF_LOCAL;
    strcpy(cservaddr.sun_path, CONTROLSOCK);
    
    if (bind(controlsockfd, (struct sockaddr*) &cservaddr, sizeof(cservaddr))) {
        perror("Failed to bind() control socket\n");
        return 1;
    }

    if (listen(controlsockfd, 10)) {
        perror("Failed to listen() on control socket\n");
        return 1;
    }

    if (chmod(CONTROLSOCK,0666)) {
        perror("Failed to chmod() control socket\n");
        return 1;
    }

    /*
     * We have to do this because netmod may be run as root, but we want to
     * make sure any user can use it.
     */

    FD_ZERO(&real_fdset);
    FD_SET(sockfd,&real_fdset);
    FD_SET(controlsockfd,&real_fdset);
    max_clientfd = controlsockfd;

    while (1) {
        /*
         * Make a blocking call to select() to wait for a client to connect or 
         * send us data
         */
        /* fprintf(stderr,"Waiting for clients\n"); */
        bcopy(&real_fdset,&returned_fdset,sizeof(fd_set));

        if (select(max_clientfd + 1,&returned_fdset,NULL,NULL,NULL) <= 0) {
            /* 
             * Just repeat in case of failure
             */
            continue;
        }

        /*
         * Let's see if we got any new clients on our listen socket
         */
        if (FD_ISSET(sockfd,&returned_fdset)) {
            struct sockaddr_un clientaddr;
            socklen_t clientlen;
            int clientfd;
            clientlen = sizeof(clientaddr);
            clientfd = accept(sockfd, (struct sockaddr*)&clientaddr, &clientlen);

            if (clientfd) {
//                fprintf(stderr,"A new client connected\n");

                FD_SET(clientfd,&real_fdset);
                sock_types[clientfd] = DATA;

                if (clientfd > max_clientfd) {
                    /* Note, max_clientfd never goes down, but that shouldn't
                       be too much of a problem */
                    max_clientfd = clientfd;
                }
            }

        }

        /*
         * Check for new clients on the control socket
         */
        if (FD_ISSET(controlsockfd,&returned_fdset)) {
            struct sockaddr_un clientaddr;
            socklen_t clientlen;
            int clientfd;
            clientlen = sizeof(clientaddr);
            clientfd = accept(controlsockfd, (struct sockaddr*)&clientaddr,
                    &clientlen);

            if (clientfd) {
//                fprintf(stderr,"A new client connected to the control socket\n");

                FD_SET(clientfd,&real_fdset);
                sock_types[clientfd] = CONTROL;
                if (clientfd > max_clientfd) {
                    /* Note, max_clientfd never goes down, but that shouldn't
                       be too much of a problem */
                    max_clientfd = clientfd;
                }

                /*
                 * Send the client a report on what it's supposed to be
                 * doing
                 */
                //write_status(clientfd);
            }

        }

        /*
         * Now, check to see if any clients have sent us any data
         */
        for (i = controlsockfd + 1; i <= max_clientfd; i++) {
            if (FD_ISSET(i,&returned_fdset)) {
                if (sock_types[i] == DATA) {
                    /*
                     * Simple data socket
                     */
                    char *buf[1024];
                    size_t bufsize = 1024;
                    int read_bytes;

                    read_bytes = read(i,buf,bufsize);

                    if (read_bytes >= 1) {
                        /*
                         * Okay, this client had data for us, let's copy that back
                         * out to stdout
                         */
                        write(1,buf,read_bytes);
                    } else {
                        /*
                         * If we get back a 0 length read, or an error, boot the
                         * client
                         */
                        printf("A client disconnected\n");
                        close(i);
                        FD_CLR(i,&real_fdset);
                    }
                } else if (sock_types[i] == CONTROL) {
                    /*
                     * Control socket
                     */
                    generic_m mesg;
                    int read_bytes;

                    read_bytes = read(i,&mesg,CONTROL_MESSAGE_SIZE);
                    if (read_bytes >= 1) {
                        if (read_bytes != CONTROL_MESSAGE_SIZE) {
                            printf("Bad control message size!\n");
                        } else {
                            /*
                             * Only handle query messages for now
                             */
                            if (mesg.type == CM_QUERY) {
                                write_status(i);
                            } else {
                                printf("Bad control type: %i!\n",mesg.type);
                            }
                        }
                    } else {
                        printf("A control client disconnected\n");
                        close(i);
                        FD_CLR(i,&real_fdset);
                    }
                }
            }
        }
    }

    return 0;
}
