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
 * $Id: serv_listen.c,v 1.8 2001-07-19 19:55:57 ikumar Exp $
 */

#include "discvr.h"
#include "packet.h"
#include "util.h"

enum {DUP, DUP_NBOR, NEW};

/*
 * Is this used anymore?
 */
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
u_char parent_nodeIF[ETHADDRSIZ];
u_char receivingIF[ETHADDRSIZ];

/*
 * Return 0 if the inquiry is not from us, 1 if it is.
 */
int
from_us(struct in_pktinfo *pktinfo, struct ifi_info *ifihead) 
{
        struct ifi_info *ifi;
        for( ifi = ifihead; ifi != NULL; ifi = ifi->ifi_next) {
	        if ( bcmp(&ifi->ifi_addr->sa_data, &pktinfo->ipi_addr.s_addr,
			  sizeof(pktinfo->ipi_addr.s_addr)) == 0 ) {
	                return 1;
		}
	}
	return 0;
}

int is_my_packet(struct sockaddr *pcliaddr, struct ifi_info *ifihead) 
{
	char *name1,*name2,*name_tmp;
	struct ifi_info *ifi;
        for( ifi = ifihead; ifi != NULL; ifi = ifi->ifi_next) {
		name_tmp=inet_ntoa(((struct sockaddr_in *)ifi->ifi_addr)->sin_addr);
		if((name1 = (char*)malloc(sizeof(char)*strlen(name_tmp)))==NULL)
			perror("is_my_packet: problem in malloc");
		strcpy(name1,name_tmp);
		name_tmp=inet_ntoa(((struct sockaddr_in *)pcliaddr)->sin_addr);
		if((name2 = (char*)malloc(sizeof(char)*strlen(name_tmp)))==NULL)
			perror("is_my_packet: problem in malloc");
		strcpy(name2,name_tmp);
		//printf("name1:\"%s\" name2:\"%s\"\n",name1,name2);
		if(strcmp(name1,name2)==0)
	                return 1;
	}
	return 0;
}

void
get_recvIFADDR(char *name,struct ifi_info * ifihead)
{
	struct ifi_info * ifi=NULL;

	for (ifi = ifihead; ifi != NULL; ifi = ifi->ifi_next) 
	{
		if(strcmp(name,ifi->ifi_name)==0) 
			//Will do this once know the conversion from MAC address to hostname
			//-ik
			//memcpy(&receivingIF,&(ifi->ifi_haddr),ETHADDRSIZ);	
			memcpy(&receivingIF,&myNodeID,ETHADDRSIZ);	
	}
}


/*
 * Return 0 if the two inquiries are identical, non-zero if not.
 */
int 
inqid_cmp(struct topd_inqid *tid1, struct topd_inqid *tid2)
{
        //return( bcmp((void *)tid1, (void*)tid2, sizeof(struct topd_inqid)));
	if((tid1->tdi_tv.tv_sec==tid2->tdi_tv.tv_sec) && (tid1->tdi_tv.tv_usec==tid2->tdi_tv.tv_usec)
		 && (strcmp(tid1->tdi_nodeID,tid2->tdi_nodeID)==0))
		return 1;
	return 0;
}


void
print_ifi_info(struct ifi_info *ifihead)
{
        struct ifi_info *ifi;
	struct sockaddr *sa;

	for (ifihead = ifi = get_ifi_info(AF_INET, 0); 
		 ifi != NULL; ifi = ifi->ifi_next) {
		printf("%s: <", ifi->ifi_name);
		if (ifi->ifi_flags & IFF_UP)			printf("UP ");
		if (ifi->ifi_flags & IFF_BROADCAST)		printf("BCAST ");
		if (ifi->ifi_flags & IFF_MULTICAST)		printf("MCAST ");
		if (ifi->ifi_flags & IFF_LOOPBACK)		printf("LOOP ");
		if (ifi->ifi_flags & IFF_POINTOPOINT)   printf("P2P ");
		printf(">\n");
		print_nodeID(ifi->ifi_haddr);

		if ( (sa = ifi->ifi_addr) != NULL)
			printf("  IP addr: %s\n", sock_ntop(sa, sa->sa_len));
		if ( (sa = ifi->ifi_brdaddr) != NULL)
			printf("  broadcast addr: %s\n", sock_ntop(sa, sa->sa_len));
		if ( (sa = ifi->ifi_dstaddr) != NULL)
			printf("  destination addr: %s\n", sock_ntop(sa, sa->sa_len));
	}
}	


void
serv_listen(int sockfd, struct sockaddr *pcliaddr, socklen_t clilen)
{
	int			flags, class=NEW;
	const int		on = 1;
   	socklen_t		len;
	ssize_t			n;
	char			mesg[MAXLINE], str[INET6_ADDRSTRLEN], ifname[IFNAMSIZ];
	char                    *reply;
	u_char                  *myNodeIDtmp = 0;
	struct in_addr		in_zero;
	struct ifi_info         *ifi, *ifihead;
	struct in_pktinfo	pktinfo;
	struct topd_inqnode     *inqn, *inqhead = 0;
	int i=0,j=0;
	struct topd_inqid *temp_inqid;
	int t_int;

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
		fprintf(stderr,"setsockopt of IP_RECVIF");
		return;
		
	}

	bzero(&in_zero, sizeof(struct in_addr));	/* all 0 IPv4 address */

	/* 
	 * Find and set our node ID 
	 */

	//ik: get the list of interfaces using the function "get_ifi_info" into the
	//link list "ifi" and the max hardware address of all the i/fs is the Node ID
	for (ifihead = ifi = get_ifi_info(AF_INET, 0); 
		 ifi != NULL; ifi = ifi->ifi_next) {
		if ( ifi->ifi_hlen > 0) {
		        myNodeIDtmp = max_haddr(myNodeIDtmp, ifi->ifi_haddr);
		}
	}
	memcpy(&myNodeID, myNodeIDtmp, ETHADDRSIZ);
	printf("My node id:");
	print_nodeID(myNodeID);

	print_ifi_info(ifihead);
	free_ifi_info(ifihead);

	/*
	 * Wait for requests. When a request comes in, send the request
	 * on to each interface. 
	 */
	i=0;
	for ( ; ; ) {
		//printf("**********for==>%d\n",i++);
		len = clilen;
		flags = 0;
		n = recvfrom_flags(sockfd, mesg, MAXLINE, &flags,
						   pcliaddr, &len, &pktinfo);
		printf("%d-byte datagram from %s", n, sock_ntop(pcliaddr, len));
		if (memcmp(&pktinfo.ipi_addr, &in_zero, sizeof(in_zero)) != 0)
			// came to my "below" IP address... -ik
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

		//printf("The message received is: ==>\n%s\n====\n",*mesg);
		// -ik: just to check what was recevied:
		print_tdinq(mesg);

		/* 
		 * There are 3 possible classifications for this inquiry.
		 * Our response to the inquiry depends upon its classification:
		 * 
		 * - New inquiry -> forward and reply (NEW)
		 * - Duplicate inquiry
		 *       - From us -> ignore (DUP)
		 *       - From another -> reply with interface list. (DUP_NBOR)
		 * 
		 * We now classify each inquiry.
		 */ 
		ifihead = get_ifi_info(AF_INET, 0); 
		
		if(inqhead==NULL) class = NEW;
		j=0;
		for ( inqn = inqhead; inqn != NULL; inqn = inqn->inqn_next ) {
			//printf("inquiry loop ##### %d\n",j++);
			//printf("Comparing #### ==>\n\"");
			//print_tdinq((char*)inqn->inqn_inq);
			//printf("\"and\n\"");
			//print_tdinq(mesg);
			//printf("\"\n");
		          if (inqid_cmp(inqn->inqn_inq, (struct topd_inqid *)mesg)) {
		                  /* Duplicate inquiry */
		                  //if (from_us(&pktinfo, ifihead))... 
				  if(is_my_packet(pcliaddr, ifihead)) {
					  printf("Duplicate inquiry from me\n");
			                  class = DUP;
					  break;
				  } else {
					  printf("Duplicate inquiry from others\n");
				          class = DUP_NBOR;
					  break;
				  }
			  } else {
		                  /* New inquiry */
			          class = NEW;
			  }
		}

		if (class == DUP) continue;
		if (class == NEW)
			printf("Got a new inquiry\n");

		/* 
		 * Save the inquiry ID into inqid_current.
		 */
		get_recvIFADDR(if_indextoname(pktinfo.ipi_ifindex, ifname),ifihead);
		//printf(">>>>>>>The receiving interface is:");
		//print_nodeID(receivingIF);
		memcpy(&inqid_current, mesg, sizeof(topd_inqid_t));

		/* Send string to all interfaces. forward_request() will 
		 * stuff reply into ifi_info. 
		 */
 
		//ifihead = get_ifi_info(AF_INET, 0); 
		temp_inqid = (struct topd_inqid *)mesg;

		memcpy(&parent_nodeIF, &(temp_inqid->tdi_p_nodeIF), ETHADDRSIZ);
		//printf("Parents interface:");
		//print_nodeID(parent_nodeIF);
		
		t_int = ntohs(temp_inqid->tdi_ttl);
		if(t_int==0)
			printf("The TTL is zero so i will not forward!!\n");
		if ((class == NEW) && (t_int!=0)) {
		        struct topd_inqnode *save = inqhead;
		        //struct topd_inqid *temp;
			//int t_int;
		        inqhead = (struct topd_inqnode *)malloc(sizeof(struct topd_inqnode));
		        inqhead->inqn_inq = (struct topd_inqid *)malloc(sizeof(struct topd_inqid));
			if (inqhead == NULL || inqhead->inqn_inq == NULL ) {
			        fprintf(stderr, "Ran out of room while expanding inquiry list.\n");
				exit(1);
			}
			memcpy(inqhead->inqn_inq, mesg, sizeof(struct topd_inqid));
			inqhead->inqn_next = save;
				
			//temp = (struct topd_inqid *)mesg;
			//t_int = ntohs(temp->tdi_ttl);
			//t_int=t_int-2;
			//temp->tdi_ttl = htons(t_int);
			
		        ifihead=forward_request(ifihead, &pktinfo, mesg, n );
			//printf("printing after forward===>\n");
			//print_tdifinbrs(ifihead);

		}

		if ( (reply=(char *)malloc(BUFSIZ)) == NULL) {
		        fprintf(stderr, "Ran out of memory for reply mesg.\n");
			exit(1);
		}
		n = compose_reply(ifihead, reply, BUFSIZ, class == NEW);
		printf("replying: ");
		print_tdreply(reply, n);
		printf("done!\n");
		free_ifi_info(ifihead);
		
		printf("Sending response to: %s\n",sock_ntop(pcliaddr,sizeof(struct sockaddr)));
		/* send response back to original sender */
		sendto(sockfd, reply, n, 0, pcliaddr, sizeof(struct sockaddr));
	}
}
