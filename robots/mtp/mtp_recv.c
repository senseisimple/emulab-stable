/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004, 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file mtp_recv.c
 *
 * Utility that listens on a given port, accepts a connection, prints out any
 * MTP packets received on that connection until it closes, and then waits for
 * another connection.
 */

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>

#include "mtp.h"

/**
 * Signal handler for SIGINT, SIGQUIT, and SIGTERM that just calls exit.
 *
 * @param signal The actual signal received.
 */
static void sigquit(int signal)
{
    assert((signal == SIGINT) || (signal == SIGQUIT) || (signal == SIGTERM));

    exit(0);
}

int main(int argc, char *argv[])
{
    int fd, port = 0, retval = EXIT_SUCCESS;

    signal(SIGQUIT, sigquit);
    signal(SIGTERM, sigquit);
    signal(SIGINT, sigquit);

    if ((argc == 2) && sscanf(argv[1], "%d", &port) != 1) {
	fprintf(stderr, "error: port argument is not a number: %s\n", argv[1]);
	retval = EXIT_FAILURE;
    }
    else if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	perror("socket");
	retval = EXIT_FAILURE;
    }
    else {
	struct sockaddr_in sin;
	int on_off = 1;
	
	memset(&sin, 0, sizeof(sin));
#ifndef linux
	sin.sin_len = sizeof(sin);
#endif
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = INADDR_ANY;

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on_off, sizeof(int));
	
	if (bind(fd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
	    perror("bind");
	    retval = EXIT_FAILURE;
	}
	else if (listen(fd, 5) == -1) {
	    perror("listen");
	    retval = EXIT_FAILURE;
	}
	else {
	    struct sockaddr_in sin_peer;
	    socklen_t slen;
	    int fd_peer;

	    slen = sizeof(sin_peer);
	    
	    getsockname(fd, (struct sockaddr *)&sin, &slen);
	    printf("Listening for mtp packets on port: %d\n",
		   ntohs(sin.sin_port));
	    
	    while ((fd_peer = accept(fd,
				     (struct sockaddr *)&sin_peer,
				     &slen)) != -1) {
		struct mtp_packet mp;
		mtp_handle_t mh;

		if ((mh = mtp_create_handle(fd_peer)) == NULL) {
		    fprintf(stderr, "error: mtp_init_handle\n");
		    retval = EXIT_FAILURE;
		    break;
		}
		else {
		    while (mtp_receive_packet(mh, &mp) == MTP_PP_SUCCESS) {
			mtp_print_packet(stdout, &mp);
			
			mtp_free_packet(&mp);
		    }

		    mtp_delete_handle(mh);
		    mh = NULL;
		    
		    close(fd_peer);
		    fd_peer = -1;
		}
		
		slen = sizeof(sin_peer);
	    }
	    
	    perror("accept");
	}
	
	close(fd);
	fd = -1;
    }
    
    return retval;
}
