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
 * Filename: serv_listen.c
 *   -- Author: Kristin Wright <kwright@cs.utah.edu> 
 *
 * ---------------------------
 *
 * $Id: serv_listen.c,v 1.1 2000-07-06 17:42:38 kwright Exp $
 */

#include "discvr.h"
#include "packet.h"
#include "util.h"

extern topd_inqid_t inqid_current;

/*
 * Each td server has a unique nodeID which is
 * simply the lowest MAC address on the machine.
 * The MAC address isn't necessarily *up*.
 * 
 * This global points to the MAC address that serves
 * as the nodeID. 
 */
u_char myNodeID[ETHADDRSIZ];


void
serv_listen(int sockfd, struct sockaddr *pcliaddr, socklen_t clilen)
{
	int			flags;
	const int		on = 1;
   	socklen_t		len;
	ssize_t			n;
	char			mesg[MAXLINE], str[INET6_ADDRSTRLEN], ifname[IFNAMSIZ];
	char                    *reply;
	u_char                  *myNodeIDtmp = 0;
	struct sockaddr         *sa;
	struct in_addr		in_zero;
	struct ifi_info         *ifi, *ifihead;
	struct in_pktinfo	pktinfo;

	/* 
	 * Use this option to specify that the receiving interface
	 * be returned in the control message structure passed to
	 * sendto().
	 * 
	 * Note: We're assuming that we can set IP_RECVIF here, but
	 * it's not necessarily supported on all systems. It 
	 * is supported on FreeBSD. -lkw
	 */
	if (setsockopt(sockfd, IPPROTO_IP, IP_RECVIF, &on, sizeof(on)) < 0) {
		fprintf(stderr, "setsockopt of IP_RECVIF");
		return;
		
	}

	bzero(&in_zero, sizeof(struct in_addr));	/* all 0 IPv4 address */

	/* 
	 * Get interface info for all inet4 interfaces 
	 * and don't return aliases. 
	 */
	for (ifihead = ifi = get_ifi_info(AF_INET, 0); 
		 ifi != NULL; ifi = ifi->ifi_next) {
		printf("%s: <", ifi->ifi_name);
		if (ifi->ifi_flags & IFF_UP)			printf("UP ");
		if (ifi->ifi_flags & IFF_BROADCAST)		printf("BCAST ");
		if (ifi->ifi_flags & IFF_MULTICAST)		printf("MCAST ");
		if (ifi->ifi_flags & IFF_LOOPBACK)		printf("LOOP ");
		if (ifi->ifi_flags & IFF_POINTOPOINT)	printf("P2P ");
		printf(">\n");
		print_nodeID(ifi->ifi_haddr);

		/* 
		 * We update myNodeIDtmp in a block separate from above 
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

	memcpy(&myNodeID, myNodeIDtmp, ETHADDRSIZ);
        fprintf(stderr, "My node id:");
	print_nodeID(myNodeID);
	

	/*
	 * Wait for requests. When a request comes in, send the request
	 * on to each interface. 
	 */
	for ( ; ; ) {
		len = clilen;
		flags = 0;
		n = recvfrom_flags(sockfd, mesg, MAXLINE, &flags,
						   pcliaddr, &len, &pktinfo);
		printf("%d-byte datagram from %s", n, sock_ntop(pcliaddr, len));
		if (memcmp(&pktinfo.ipi_addr, &in_zero, sizeof(in_zero)) != 0)
			printf(", to %s", inet_ntop(AF_INET, &pktinfo.ipi_addr,
			        str, sizeof(str)));
		if (pktinfo.ipi_ifindex > 0)
			printf(", recv i/f = %s",
				   if_indextoname(pktinfo.ipi_ifindex, ifname));
		if (flags & MSG_TRUNC)	printf(" (datagram truncated)");
		if (flags & MSG_CTRUNC)	printf(" (control info truncated)");
#ifdef	MSG_BCAST
		if (flags & MSG_BCAST)	printf(" (broadcast)");
#endif
		printf("\n");

		/* 
		 * Save the inquiry ID into inqid_current.
		 */
		memcpy(&inqid_current, mesg, sizeof(topd_inqid_t));

		/* Send string to all interfaces. forward_request() will 
		 * stuff reply into ifi_info. 
		 */
	        reply = forward_request(ifihead, &pktinfo, mesg, n);
		fprintf(stderr, "replying: ");
		print_tdreply(reply);

		/* send response back to original sender */
		sendto(sockfd, reply, 40, 0, pcliaddr, sizeof(struct sockaddr));
	}

	free_ifi_info(ifihead);
}
