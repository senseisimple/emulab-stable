/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 *
 * netmond, a 'server' for libnetmon - simply repeat what a process being
 * monitored with libnetmon tell us on a unix-domian socket
 *
 * TODO: Handle more than one client at a time
 */

#define SOCKPATH "/var/tmp/netmon.sock"

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main() {

    int sockfd;
    struct sockaddr_un servaddr;

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

    while (1) {
        fprintf(stderr,"Waiting for clients\n");

        /*
         * Do a blocking wait for a client to connect() to us
         */
        struct sockaddr_un clientaddr;
        socklen_t clientlen;
        int clientfd;
        clientlen = sizeof(clientaddr);
        clientfd = accept(sockfd, (struct sockaddr*)&clientaddr, &clientlen);

        if (clientfd) {
            char *buf[1024];
            size_t bufsize = 1024;
            size_t read_bytes;
            fprintf(stderr,"Got a client\n");

            /* 
             * As long as the client is connected, just take whatever it
             * tells us and spit it to stdout
             */
            while ((read_bytes = read(clientfd,buf,bufsize))) {
                write(1,buf,read_bytes);
            }

            close(clientfd);
        }
    }

    return 0;
}
