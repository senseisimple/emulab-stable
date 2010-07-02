/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005, 2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "config.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <rpc/rpc.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <assert.h>
#include <stdarg.h>
#include <sys/time.h>
#include <grp.h>
#include "capdecls.h"

static int debug = 0;

static char *usagestr = 
 "usage: capquery [-d] [-s server] [-p #] name\n"
 " -d		   Turn on debugging.\n"
 " -s server	   Specify a name to connect to.\n"
 " -p portnum	   Specify a port number to connect to.\n"
 "\n";

void
usage()
{
	fprintf(stderr, usagestr);
	exit(1);
}

static int
mygethostbyname(struct sockaddr_in *host_addr, char *host, int port)
{
    struct hostent *host_ent;
    int retval = 0;
    
    assert(host_addr != NULL);
    assert(host != NULL);
    assert(strlen(host) > 0);
    
    memset(host_addr, 0, sizeof(struct sockaddr_in));
#ifndef linux
    host_addr->sin_len = sizeof(struct sockaddr_in);
#endif
    host_addr->sin_family = AF_INET;
    host_addr->sin_port = htons(port);
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

int
main(int argc, char **argv)
{
	int			sock, ch, rc, retval = EXIT_FAILURE;
	int			port = SERVERPORT;
	char		       *server = BOSSNODE;
	struct sockaddr_in	sin;
	whoami_t		whoami;

	while ((ch = getopt(argc, argv, "ds:p:")) != -1)
		switch(ch) {
		case 's':
			server = optarg;
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'd':
			debug++;
			break;
		case 'h':
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();
	
	if (strlen(argv[0]) >= sizeof(whoami.name))
		fprintf(stderr, "Name too long: %s\n", argv[0]);
	
	if (getuid() != 0)
		fprintf(stderr, "Must be run as root\n");

	memset(&whoami, 0, sizeof(whoami));
	strcpy(whoami.name, argv[0]);
	whoami.portnum = -1;
	
	if (!mygethostbyname(&sin, server, port))
		fprintf(stderr, "Bad server name: %s\n", server);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		perror("socket");
	else if (bindresvport(sock, NULL) < 0)
		perror("bindresvport");
	else if (connect(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		perror("connect");
	else if (write(sock, &whoami, sizeof(whoami)) != sizeof(whoami))
		perror("write");
	else if ((rc = read(sock, &port, sizeof(port))) != sizeof(port))
		perror("read");
	else {
		printf("%d\n", port);
		retval = EXIT_SUCCESS;
	}

	close(sock);

	return retval;
}
