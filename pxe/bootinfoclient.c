/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <netdb.h>
#include <err.h>
#include <errno.h>
#include "bootwhat.h"
#include "bootinfo.h"
#include "config.h"

int		debug = 0;
static char	*progname;

void
usage()
{
	fprintf(stderr,
		"Usage: %s <options> [-d] [-k hostkey] [-s bossnode]\n"
		"options:\n"
		"-d         - Turn on debugging\n"
		"-k         - Specify a widearea hostkey to be used instead of IP for identification\n"
		"-s         - Boss Node\n",
		progname);
	exit(-1);
}

int
main(int argc, char **argv)
{
	int			sock, c, cc, pollmode = 0;
	char			*bossnode = BOSSNODE;
	struct sockaddr_in	name, target;
	boot_info_t		boot_info, boot_reply;
	boot_what_t	       *boot_whatp = (boot_what_t *) &boot_reply.data;
	extern char		build_info[];
	struct hostent	       *he;
	struct timeval		timeout;
	char* 			hostkey = NULL;

	progname = argv[0];

	while ((c = getopt(argc, argv, "dhvk:s:")) != -1) {
		switch (c) {
		case 'd':
			debug++;
			break;
		case 's':
			bossnode = optarg;
			break;
		case 'k':
			hostkey = optarg;
			break;
		case 'v':
		    	fprintf(stderr, "%s\n", build_info);
			exit(0);
			break;
		case 'h':
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc)
		usage();

	if (debug)
		printf("%s\n", build_info);

	if((hostkey != NULL) && ((strlen(hostkey) > HOSTKEY_LENGTH) || (strlen(hostkey) == 0))) {
		warn("provided hostkey [%s] is not valid (too big or small)", hostkey);
		exit(1);
	}


	/* Make sure we can map target */
	if ((he = gethostbyname(bossnode)) == NULL) {
		warn("gethostbyname(%s)", bossnode);
		exit(1);
	}

	bzero(&target, sizeof(target));
	memcpy((char *)&target.sin_addr, he->h_addr, he->h_length);
	target.sin_family = AF_INET;
	target.sin_port   = htons((u_short) BOOTWHAT_DSTPORT);

	/* Create socket */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		warn("opening datagram socket");
		exit(1);
	}

	/*
	 * We use a socket level timeout instead of polling for data.
	 */
	timeout.tv_sec  = 5;
	timeout.tv_usec = 0;
	
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
		       &timeout, sizeof(timeout)) < 0) {
		warn("setsockopt(SOL_SOCKET, SO_RCVTIMEO)");
		exit(1);
	}

	/* Bind our side of it */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons((u_short) BOOTWHAT_SRCPORT);
	if (bind(sock, (struct sockaddr *) &name, sizeof(name))) {
		warn("binding datagram socket");
		exit(1);
	}
	
	/*
	 * Create a bootinfo request packet and send it.
	 */
	bzero(&boot_info, sizeof(boot_info));
	boot_info.version = BIVERSION_CURRENT;
	boot_info.opcode  = BIOPCODE_BOOTWHAT_REQUEST;
	if(hostkey != NULL) {
		strcpy(boot_info.data, hostkey);
		boot_info.opcode = BIOPCODE_BOOTWHAT_KEYED_REQUEST;
	}

	while (1) {
		struct sockaddr_in	client;
		int			length;
		
		cc = sendto(sock, &boot_info, sizeof(boot_info), 0,
			    (struct sockaddr *)&target, sizeof(target));
		if (cc != sizeof(boot_info)) {
			if (cc < 0) {
				warn("Writing to socket");
				goto again;
			}
			fprintf(stderr, "short write (%d != %d)\n",
				cc, sizeof(boot_info));
			goto again;
		}
	poll:
		length = sizeof(client);
		bzero(&client, sizeof(client));
		cc = recvfrom(sock, &boot_reply, sizeof(boot_reply), 0,
			      (struct sockaddr *)&client, &length);
		if (cc < 0) {
			if (errno != EWOULDBLOCK)
				warn("Reading from socket");
			if (pollmode)
				goto poll;
			goto again;
		}
		pollmode = 0;

		switch (boot_whatp->type) {
		case BIBOOTWHAT_TYPE_PART:
			printf("partition:%d", boot_whatp->what.partition);
			if (boot_whatp->cmdline[0])
				printf(" %s", boot_whatp->cmdline);
			printf("\n");
			goto done;
			break;
		case BIBOOTWHAT_TYPE_WAIT:
			if (debug)
				printf("wait: now polling\n");
			pollmode = 1;
			goto poll;
			break;
		case BIBOOTWHAT_TYPE_REBOOT:
			printf("reboot\n");
			goto done;
			break;
		case BIBOOTWHAT_TYPE_AUTO:
			if (debug)
				printf("query: will query again\n");
			goto again;
			break;
		case BIBOOTWHAT_TYPE_MFS:
			printf("mfs:%s\n", boot_whatp->what.mfs);
			goto done;
			break;
		}
	again:
		if (debug)
			fprintf(stderr, "Trying again in 5 seconds\n");
		sleep(5);
	}
 done:
	close(sock);
	exit(0);
}
