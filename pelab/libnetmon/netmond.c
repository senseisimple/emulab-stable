/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 *
 * netmond, a 'server' for libnetmon - simply repeat what a process being
 * monitored with libnetmon tell us on a unix-domian socket
 */

#define SOCKPATH "/var/tmp/netmon.sock"

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>

int main() {

    int sockfd;
    struct sockaddr_un servaddr;
    int max_clientfd;
    fd_set real_fdset, returned_fdset;
    int i;

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
     * We have to do this because netmod may be run as root, but we want to
     * make sure any user can use it.
     */

    FD_ZERO(&real_fdset);
    FD_SET(sockfd,&real_fdset);
    max_clientfd = sockfd;

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
                fprintf(stderr,"A new client connected\n");

                FD_SET(clientfd,&real_fdset);
                if (clientfd > max_clientfd) {
                    /* Note, max_clientfd never goes down, but that shouldn't
                       be too much of a problem */
                    max_clientfd = clientfd;
                }
            }

        }

        /*
         * Now, check to see if any clients have sent us any data
         */
        for (i = sockfd; i <= max_clientfd; i++) {
            if (FD_ISSET(i,&returned_fdset)) {
                char *buf[1024];
                size_t bufsize = 1024;
                size_t read_bytes;

                read_bytes = read(i,buf,bufsize);

                if (read_bytes >= 1) {
                    /*
                     * Okay, this client had data for us, let's copy that back out
                     * to stdout
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
            }
        }
    }

    return 0;
}
