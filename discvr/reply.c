/* 
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000 The University of Utah and the Flux Group.
 * All rights reserved.
 *
 * ---------------------------
 *
 * Filename: reply.c
 *   -- Author: Kristin Wright <kwright@cs.utah.edu> 
 *
 * ---------------------------
 *
 * $Id: reply.c,v 1.11 2004-06-17 18:17:01 mike Exp $
 */


#include "discvr.h"
#include "packet.h"
#include "util.h"

extern u_char myNodeID[ETHADDRSIZ];
//extern u_char parent_nodeIF[ETHADDRSIZ];
extern topd_inqid_t inqid_current;
//extern u_char receivingIF[ETHADDRSIZ];
/*
 * Concatenate all the individual interfaces' messages into
 * one long neighbor list.
 */
u_int32_t
compose_reply(struct ifi_info *ifi, char *mesg, const int mesglen, int sendnbors,
			  u_char senderIF[], u_char recverIF[]) 
{
        struct topd_nborlist *nborl;
	struct ifi_info      *ifihead;
        char                 *nw;
	char                 *nid;
	struct topd_nbor     *nbor;

	nw = (u_char *) mesg;
	
	/* Add on the INQ ID we're responding to. */
	memcpy(mesg, &inqid_current, sizeof(topd_inqid_t));
	nw += sizeof(topd_inqid_t);

	nid=nw;
	nw += ETHADDRSIZ << 2;
	if ((char *)nw > mesg + mesglen ) { 
		        fprintf(stderr, "ran out of room and you didn't do anything
						reasonable.1\n");
			return 0;
		}
	memcpy(nid, myNodeID, ETHADDRSIZ);
    nid += ETHADDRSIZ;
	bzero(nid, ETHADDRSIZ);
    nid += ETHADDRSIZ;
    memcpy(nid,senderIF,ETHADDRSIZ);
    nid += ETHADDRSIZ;
    memcpy(nid,recverIF,ETHADDRSIZ);

	for (ifihead = ifi; ifi != NULL; ifi = ifi->ifi_next) {
	        nborl  = ifi->ifi_nbors;

	        /* Add check for control net -LKW */
	        if (ifi->ifi_flags & !IFF_UP || 
		    ifi->ifi_flags & IFF_LOOPBACK || (strcmp(ifi->ifi_name,"fxp4")==0)) {
		        continue;
		}

		/* We report this interface.
		 * That is, we put down the pair
		 * [ <myNodeID, ifi->haddr>, <0,0> ]
		 * for the [path, nbor] pair.
		 */
		/*
		nid = nw;
		nw += ETHADDRSIZ << 2; // we're writing 4 nodeids
		if ((char *)nw > mesg + mesglen ) { 
		        fprintf(stderr, "ran out of room and you didn't do anything
						reasonable.1\n");
			return 0;
		}
		//printf("My parent's address is:");
		//print_nodeID(parent_nodeIF);
		memcpy(nid, myNodeID, ETHADDRSIZ);
		nid += ETHADDRSIZ;
		memcpy(nid, ifi->ifi_haddr, ETHADDRSIZ);
		nid += ETHADDRSIZ;
		memcpy(nid,senderIF,ETHADDRSIZ);
		nid += ETHADDRSIZ;
		memcpy(nid,recverIF,ETHADDRSIZ);
		//bzero(nid, ETHADDRSIZ);
		*/
		if ( sendnbors != 0 ) {
		        while ( nborl != NULL ) {
		              nbor = (struct topd_nbor *)nw;
		              nw += nborl->tdnbl_n * sizeof(struct topd_nbor);
			      if ((char *)nw > mesg + mesglen ) {
			              fprintf(stderr, "ran out of room and you didn't do
								  anything reasonable.2\n");
				      return 0;
			      }
			      memcpy(nbor, nborl->tdnbl_nbors, nborl->tdnbl_n * sizeof(struct topd_nbor));
			      nborl = nborl->tdnbl_next;
			} 
		}
	}

	return (nw - mesg);
}
