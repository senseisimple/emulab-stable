/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004, 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file mtp_dump.c
 *
 * Utility that will connect to a server and dump out any MTP packets it
 * receives.
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <float.h>

#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>

#include "mtp.h"

static int looping = 1;

#define min(x, y) ((x) < (y)) ? (x) : (y)
#define max(x, y) ((x) > (y)) ? (x) : (y)

/**
 * Signal handler for SIGINT, SIGQUIT, and SIGTERM that sets looping to zero.
 *
 * @param signal The actual signal received.
 */
static void sigquit(int signal)
{
    assert((signal == SIGINT) || (signal == SIGQUIT) || (signal == SIGTERM));

    looping = 0;
}

static int mygethostbyname(struct sockaddr_in *host_addr, char *host)
{
    struct hostent *host_ent;
    int retval = 0;

    assert(host_addr != NULL);
    assert(host != NULL);
    assert(strlen(host) > 0);

    if( (host_ent = gethostbyname(host)) != NULL ) {
	memcpy((char *)&host_addr->sin_addr.s_addr,
	       host_ent->h_addr,
	       host_ent->h_length);
	retval = 1;
    }
    else {
	retval = inet_aton(host, &host_addr->sin_addr);
    }
    return( retval );
}

static void usage(void)
{
    fprintf(stderr, "Usage: mtp_dump <host> <port> [<max-packets>]\n");
}

int main(int argc, char *argv[])
{
    int port = 0, max_packets = 0, retval = EXIT_FAILURE;
    struct sockaddr_in sin;
    
    signal(SIGQUIT, sigquit);
    signal(SIGTERM, sigquit);
    signal(SIGINT, sigquit);
    
    memset(&sin, 0, sizeof(sin));
    
    if (argc < 3) {
	fprintf(stderr, "error: not enough arguments\n");
	usage();
    }
    else if (sscanf(argv[2], "%d", &port) != 1) {
	fprintf(stderr, "error: port is not a number\n");
	usage();
    }
    else if ((argc > 3) && sscanf(argv[3], "%d", &max_packets) != 1) {
	fprintf(stderr, "error: max packets argument is not a number\n");
	usage();
    }
    else if (mygethostbyname(&sin, argv[1]) == 0) {
	fprintf(stderr, "error: unknown host %s\n", argv[1]);
    }
    else {
	mtp_handle_t mh;
	int fd;
	
#ifndef linux
	sin.sin_len = sizeof(sin);
#endif
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	    perror("socket");
	    retval = EXIT_FAILURE;
	}
	else if (connect(fd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
	    perror("connect");
	}
	else if ((mh = mtp_create_handle(fd)) == NULL) {
	    fprintf(stderr, "error: mtp_init_handle\n");
	}
	else {
	    float minx = FLT_MAX, miny = FLT_MAX;
	    float maxx = FLT_MIN, maxy = FLT_MIN;
	    struct mtp_packet mp;
	    int lpc = 0;
	    
	    while (looping && ((max_packets == 0) || (lpc < max_packets)) &&
		   (mtp_receive_packet(mh, &mp) == MTP_PP_SUCCESS)) {
		if (mp.data.opcode == MTP_UPDATE_POSITION) {
		    struct mtp_update_position *mup;

		    mup = &mp.data.mtp_payload_u.update_position;
		    minx = min(minx, mup->position.x);
		    maxx = max(maxx, mup->position.x);
		    miny = min(miny, mup->position.y);
		    maxy = max(maxy, mup->position.y);
		}
		
		mtp_print_packet(stdout, &mp);

		mtp_free_packet(&mp);

		lpc += 1;
	    }
	    
	    close(fd);
	    fd = -1;

	    mtp_delete_handle(mh);
	    mh = NULL;

	    printf(" minx = %.2f; miny = %.2f\n"
		   " maxx = %.2f; maxy = %.2f (width = %.2f, height = %.2f\n",
		   minx, miny,
		   maxx, maxy,
		   maxx - minx, maxy - miny);
	}

	retval = EXIT_SUCCESS;
    }
    
    return retval;
}
