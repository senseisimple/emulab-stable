/* 
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000 The University of Utah and the Flux Group.
 * All rights reserved.
 *
 * ---------------------------
 *
 * Filename: serv.c
 *   -- Author: Kristin Wright <kwright@cs.utah.edu> 
 *
 * ---------------------------
 *
 * $Id: serv.c,v 1.8 2004-06-17 18:17:01 mike Exp $
 */

#include "discvr.h"
#include "packet.h"

char hostname[MAX_HOSTNAME];
topd_inqid_t inqid_current;

int
main(int argc, char **argv)
{
	int			sockfd;
	struct sockaddr_in	servaddr, cliaddr;
	struct sockaddr 	name;
	int 			namelen=sizeof(name);	

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(SERV_PORT);

	if(bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0)
	{
		perror("bind");
	}
	//printf("The address I am listening on is: %s\n",inet_ntoa(servaddr.sin_addr));
	/*
	if(getsockname(sockfd, (struct sockaddr *)&name, &namelen)<0)
	{
		perror("getsockname\n");
	}
	printf("No I am listening on: \"%s\"\n",sock_ntop(&name,name.sa_len));
	*/
	serv_listen(sockfd, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
	
	/* We'll never get here */
	
	exit(0);
}
