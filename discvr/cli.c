/* 
 * Copyright (c) 2000 The University of Utah and the Flux Group.
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation.
 *
 * THE UNIVERSITY OF UTAH ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  THE UNIVERSITY OF UTAH DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * ---------------------------
 *
 * Filename: cli.c
 *   -- Author: Kristin Wright <kwright@cs.utah.edu> 
 *
 * ---------------------------
 *
 * $Id: cli.c,v 1.7 2001-08-02 21:21:05 ikumar Exp $
 */

#include "discvr.h"
#include "packet.h"
#include "util.h"

extern u_char *mac_list[MAX_NODES];
extern int num_nodes;


u_char *
find_nodeID(void)
{
        int                     i;
	struct sockaddr         *sa;
	char                    *ptr;
	u_char                  *myNodeIDtmp = 0; 
  	struct ifi_info         *ifi, *ifihead;

	/* 
	 * Get interface info for all inet4 interfaces 
	 * and don't return aliases. 
	 */
	for (ifihead = ifi = get_ifi_info(AF_INET, 0); 
		 ifi != NULL; ifi = ifi->ifi_next) {

		printf("%s: <", ifi->ifi_name);
		if (ifi->ifi_flags & IFF_UP)		printf("UP ");
		if (ifi->ifi_flags & IFF_BROADCAST)	printf("BCAST ");
		if (ifi->ifi_flags & IFF_MULTICAST)	printf("MCAST ");
		if (ifi->ifi_flags & IFF_LOOPBACK)	printf("LOOP ");
		if (ifi->ifi_flags & IFF_POINTOPOINT)	printf("P2P ");
		printf(">\n");

		if ( (i = ifi->ifi_hlen) > 0) {
			ptr = ifi->ifi_haddr;
			do {
				printf("%s%x", (i == ifi->ifi_hlen) ? "  " : ":", *ptr++);
			} while (--i > 0);
			
		}

		/* 
		 * We update myNodeIDtmp in block separate from above 
		 * since the above is just a debug clause and may be
		 * compiled out eventually. -lkw
		 */
		 
		if ( ifi->ifi_hlen > 0) {
		        myNodeIDtmp = max_haddr(myNodeIDtmp, ifi->ifi_haddr);
		}

		if ( (sa = ifi->ifi_addr) != NULL)
			printf("  IP addr: %s\n", sock_ntop(sa, sa->sa_len));
		if ( (sa = ifi->ifi_brdaddr) != NULL)
			printf("  broadcast addr: %s\n", sock_ntop(sa, sa->sa_len));
		if ( (sa = ifi->ifi_dstaddr) != NULL)
			printf("  destination addr: %s\n", sock_ntop(sa, sa->sa_len));
	}

        fprintf(stderr, "My node id:");
	print_nodeID(myNodeIDtmp);

	return myNodeIDtmp;
}                            

void
make_inquiry(topd_inqid_t *tip, u_int16_t ttl, u_int16_t factor) 
{
        struct timeval tv;
	u_char         *nid;

	/* First goes the the time of day... */
	if (gettimeofday(&tv, NULL) == -1) {
	        perror("Unable to get time-of-day.");
		exit(1);
	}

	tip->tdi_tv.tv_sec  = htonl(tv.tv_sec);
	tip->tdi_tv.tv_usec = htonl(tv.tv_usec);

	/* ...then the ttl and factor... */
	tip->tdi_ttl     = htons(ttl);
	tip->tdi_factor  = htons(factor);

	/* ...and now our nodeID */
	nid = find_nodeID();
	memcpy((void *)tip->tdi_nodeID, nid, ETHADDRSIZ);
	bzero(tip->tdi_p_nodeIF,ETHADDRSIZ);
}

void
cli(int sockfd, const struct sockaddr *pservaddr, socklen_t servlen, 
    u_int16_t ttl, u_int16_t factor)
{
        u_int32_t         n;
	char              recvline[MAXLINE + 1];
	topd_inqid_t      ti;
	
	make_inquiry(&ti, ttl, factor);

	printf("sending query to server:\n");
	sendto(sockfd, &ti, TOPD_INQ_SIZ, 0, pservaddr, servlen);
	print_tdinq((char *)&ti);
	n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL);
	fflush(stdin);
	printf("Receiving in client:==>\n");
	print_tdreply(recvline, n);
	gen_nam_file(recvline, n,"td1.nam");
	printf("Done!\n");
}

/*
 * Note that the TTL is a function of the network diameter that
 * we're interested in. The factor parameter is a function of the
 * network topology and performance.
 */ 
int
main(int argc, char **argv)
{
	int sockfd;
	struct sockaddr_in servaddr;

	if (argc != 4) {
		fprintf(stderr, "usage: cli <Server IPaddress> <TTL> <factor>\n");
		exit(1);
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	//printf("calling client\n");
	cli(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr), atoi(argv[2]), atoi(argv[3]));

	exit(0);
}
