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
 * $Id: forw_requests.c,v 1.10 2001-08-02 21:21:05 ikumar Exp $
 */

#include <math.h>

#include "discvr.h"
#include "packet.h"
#include "util.h"

/*
 * Send a request on to each interface 
 */
char recvbuf[BUFSIZ];
extern u_char myNodeID[ETHADDRSIZ];
extern u_char parent_nodeIF[ETHADDRSIZ];

static double start = 0.0;

double
tod(void)
{
        double s;
	struct timeval tv;

	gettimeofday(&tv, 0);
	s = tv.tv_sec;
	s += (1e-6 * tv.tv_usec);
	return (s - start);
}


struct ifi_info *
get_ifi_struct(int sock, struct ifi_info * ifihead)
{
	struct ifi_info * ifi=NULL;

	for (ifi = ifihead; ifi != NULL; ifi = ifi->ifi_next) 
	{
		if(ifi->sock == sock) return ifi;	
	}
	return NULL;
}

/*
void
addMyID(char* mesg, int size)
{
	struct topd_nbor *p;
	p = (struct topd_nbor *) (mesg + sizeof(topd_inqid_t));

	memcpy(p->tdnbor_dnode,myNodeID,ETHADDRSIZ);
}
*/


int *
forward_request(struct ifi_info *ifihead, const struct in_pktinfo *pktinfo, 
		 const char *mesg, int mesglen, int *ptrSockNum) 
{
	int                     s, n;
    const int               on = 1;
	char                    ifname[IFNAMSIZ];
	struct sockaddr_in      sin,tempAddr;
	struct ifi_info         *ifi;
	int 			*sock_list=NULL, *temp_sock_list=NULL;
	struct topd_inqid *temp_mesg;
	int t_int=0,i=0;

	bzero(&tempAddr, sizeof(tempAddr));
	tempAddr.sin_family      = AF_INET;
	tempAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	tempAddr.sin_port        = htons(0);
	
	temp_mesg = (struct topd_inqid *)mesg;
	t_int = ntohs(temp_mesg->tdi_ttl);
    t_int=t_int-1;
	temp_mesg->tdi_ttl = htons(t_int);

	(*ptrSockNum) = 0;
	for (ifi = ifihead; ifi != NULL; ifi = ifi->ifi_next) 
	{
		// Skip all the interfaces which are not useful
		if( (ifi->ifi_flags & !IFF_UP) || (ifi->ifi_flags & IFF_LOOPBACK) ||
		    (strcmp(ifi->ifi_name, if_indextoname(pktinfo->ipi_ifindex, ifname)) == 0) ||
			/*(strcmp(ifi->ifi_name,"fxp4")==0)*/
			(strncmp(inet_ntoa(((struct sockaddr_in *)(ifi->ifi_addr))->sin_addr),"155.101.132",11)==0)
			)

		{
			continue;
		}

		memcpy(&(temp_mesg->tdi_p_nodeIF),&(ifi->ifi_haddr),IFHADDRSIZ);
		//memcpy(&(temp_mesg->tdi_p_nodeIF),&myNodeID,IFHADDRSIZ);

		printf("Forwarding the query to interface: \"%s\"\n",ifi->ifi_name);
		temp_sock_list = sock_list;
		sock_list = (int *)malloc(sizeof(int)*((*ptrSockNum)+1));
		for(i=0;i<(*ptrSockNum);i++)
		{
			//printf("copying: %d\n",temp_sock_list[i]);
			sock_list[i] = temp_sock_list[i];
		}
		free(temp_sock_list);
	    sock_list[(*ptrSockNum)] = s = socket(AF_INET, SOCK_DGRAM, 0);
		(*ptrSockNum)++;
	    if (s == -1) 
		{
			perror("Unable to get socket");
			exit(1);
		}
		if(bind(s, (struct sockaddr *) &tempAddr, sizeof(tempAddr))<0)
	    {
        	perror("Problem in bind call");
       	} 
		ifi->sock = s;
		if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) != 0) 
		{
	        perror("setsockopt died.");
			// Do something intelligent. -lkw 
		} 
		bzero(&sin, sizeof(sin));
		sin.sin_len = htons(sizeof sin);
		sin.sin_family = PF_INET;
		sin.sin_port = htons(SERV_PORT);
		sin.sin_addr = ((struct sockaddr_in *)(ifi->ifi_brdaddr))->sin_addr;
        printf("The dest. address: %s\n", inet_ntoa(sin.sin_addr));
		printf("Forwarding the enquiry: ");
		print_tdinq(mesg);
		n = sendto(s, mesg, mesglen, 0, 
			   (struct sockaddr *)&sin, sizeof(struct sockaddr_in));
		if (n != mesglen) 
		{
	        perror("Didn't send all of packet");
			exit(1);
		} 
	}

	return sock_list;

	//************************************************************************/
}
