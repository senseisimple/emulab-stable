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
 * Filename: forw_requests.c
 *   -- Author: Kristin Wright <kwright@cs.utah.edu> 
 *
 * ---------------------------
 *
 * $Id: forw_requests.c,v 1.3 2000-07-13 18:52:51 kwright Exp $
 */

#include "discvr.h"
#include "packet.h"
#include "util.h"

/*
 * Send a request on to each interface 
 */
char recvbuf[BUFSIZ];

void
forward_request(struct ifi_info *ifi, const struct in_pktinfo *pktinfo, 
		 const char *mesg, int mesglen) 
{
	int                     s, n;
	fd_set                  rset;
        const int               on = 1;
	char                    ifname[IFNAMSIZ];
	struct topd_inqid       *tdi;
	struct topd_nborlist    *save;
	struct sockaddr_in      sin;
	struct ifi_info         *ifihead;
	struct timeval          tv;
	
	for (ifihead = ifi = get_ifi_info(AF_INET, 0); 
	     ifi != NULL; ifi = ifi->ifi_next) {
	  
	        /* 
		 * Don't send a message if the interface is down,
		 * a loopback interface, or it's the receiving
		 * interface.
		 * 
		 * Add check for control net. -lkw
		 */
	        if (ifi->ifi_flags & !IFF_UP || 
		    ifi->ifi_flags & IFF_LOOPBACK ||
		    strcmp(ifi->ifi_name, if_indextoname(pktinfo->ipi_ifindex, ifname)) == 0) {
		  ifi->ifi_myflags |= MY_IFF_RECVIF; /* may be unnec. -lkw */
		        continue;
		}
		
	        s = socket(AF_INET, SOCK_DGRAM, 0);
	        if (s == -1) {
		        perror("Unable to get socket");
			exit(1);
		}
		if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) != 0) {
		        perror("setsockopt died.");
			/* Do something intelligent. -lkw */
		} 

		bzero(&sin, sizeof(sin));
		sin.sin_len = htons(sizeof sin);
		sin.sin_family = PF_INET;
		sin.sin_port = htons(SERV_PORT);
		/* sin.sin_addr.s_addr = inet_addr(BROADCAST_IP); */
		/* 
		 * struct sockaddr_in sin 
		 *   -> struct in_addr sin_addr
		 *      -> u_int32_t s_addr
		 *
		 * struct sockaddr *ifi_brdaddr
		 *  -> ifi_brdaddr->sa_data
		 */
		sin.sin_addr.s_addr = inet_addr(ifi->ifi_brdaddr->sa_data); 

		/* 
		 * though sendto() returns the correct number of bytes, 
		 * tcpdump is showing two packets for this one line. -lkw 
		 * 
		 *   60 eureka:testbed/discvr> tcpdump port 9877
		 *   tcpdump: listening on fxp0
		 *   15:04:59.357514 eureka.cs.utah.edu.1192 > 155.99.212.255.9877: udp 15
		 *   15:04:59.357528 eureka.cs.utah.edu.1192 > 155.99.212.255.9877: udp 15
		 *   15:20:02.075858 kamas.cs.utah.edu.9877 > eureka.cs.utah.edu.1214: udp 15
		 */
		n = sendto(s, mesg, mesglen, 0, 
			   (struct sockaddr *)&sin, sizeof(struct sockaddr_in));
		fprintf(stderr, "sent: ");
		print_tdinq(mesg);

		if (n != mesglen) {
		        perror("Didn't send all of packet");
			exit(1);
		} 

		/*
		 * Can't wait forever because
		 * there may be either no nodes on this interface or 
		 * the nodes that are there may not be responding. 
		 * Our wait delay is equal to the factor times the ttl. 
		 *
		 * The packet size *might* exceed our buffer size. 
		 * Put in some mechanism to check. -lkw
		 */

		tdi = (struct topd_inqid *)mesg;
		tv.tv_sec  = ntohs(tdi->tdi_ttl) * ntohs(tdi->tdi_factor);
		fprintf(stderr, "tv.tv_sec: %d", tv.tv_sec);
		tv.tv_usec = 0;
		FD_ZERO(&rset);
		FD_SET(s, &rset);

		select(s + 1, &rset, NULL, NULL, &tv);
		
		if (!FD_ISSET(s, &rset)) {
		        /* socket is not readable */
		        fprintf(stderr, "Socket not readable before timeout.\n");
                        continue; 
		}

		while (	(n = recvfrom(s, recvbuf, BUFSIZ, 0, NULL, NULL)) > 0 ) {	

		        /* 
			 * It's possible that one neighbor responded very quickly
			 * and other neighbors (perhaps because they have many more
			 * neighbors whose messages they had to parse) could respond
			 * slowly. To cover this case, we should continue to try to 
			 * read from the socket until our delay is up. 
			 */
		        /* 
			 * Be sure to malloc enough space for all of the mesg. 
			 * See note above about possible datagram truncation. -lkw
			 */
		        save = ifi->ifi_nbors;
			ifi->ifi_nbors = (struct topd_nborlist *)malloc(sizeof(struct topd_nborlist)); 
			if ( ifi->ifi_nbors == NULL ) {
		                perror("Not enough memory for neighbor list.");
				exit(1);
			}
			/* skip inquiry ID */
			ifi->ifi_nbors->tdnbl_nbors = (u_char *)malloc(n);
			if ( ifi->ifi_nbors->tdnbl_nbors == NULL ) {
		                perror("Not enough memory for neighbor list.");
				exit(1);
			}
			memcpy((void *)ifi->ifi_nbors->tdnbl_nbors, recvbuf + sizeof(topd_inqid_t), n);
			ifi->ifi_nbors->tdnbl_n = n / sizeof(struct topd_nbor);
			ifi->ifi_nbors->tdnbl_next = save;
		}
	}
}
