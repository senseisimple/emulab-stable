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
 * Filename: reply.c
 *   -- Author: Kristin Wright <kwright@cs.utah.edu> 
 *
 * ---------------------------
 *
 * $Id: reply.c,v 1.2 2000-07-06 22:51:47 kwright Exp $
 */


#include "discvr.h"
#include "packet.h"
#include "util.h"

extern u_char myNodeID[ETHADDRSIZ];
extern topd_inqid_t inqid_current;

/*
 * Concatenate all the individual interfaces' messages into
 * one long neighbor list.
 */
u_int32_t
compose_reply(struct ifi_info *ifi, char *mesg, const int mesglen) 
{
	struct ifi_info    *ifihead;
        char               *nw;
	char               *nid;

	nw = (u_char *) mesg;
	
	/* Add on the INQ ID we're responding to. */
	memcpy(mesg, &inqid_current, sizeof(topd_inqid_t));
	nw += sizeof(topd_inqid_t);

	for (ifihead = ifi; ifi != NULL; ifi = ifi->ifi_next) {
	        /* Add check for control net -LKW */
	        if (ifi->ifi_flags & !IFF_UP || 
		    ifi->ifi_flags & IFF_LOOPBACK) {
		        continue;
		}

		if (ifi->ifi_nbors == NULL) {
		        /* 
			 * There are no neighbors on this interface. 
			 * We need only report this interface as a
			 * dead end.  That is, we put down the pair
			 * [ <myNodeID, ifi->haddr>, <0,0> ]
			 * for the [path, nbor] pair.
			 */
		        nid = nw;
			nw += ETHADDRSIZ << 2; /* we're writing 4 nodeids */
			if ((char *)nw > mesg + mesglen ) {
			        fprintf(stderr, "ran out of room and you didn't do anything reasonable.\n");
			        return 0;
			}
			memcpy(nid, myNodeID, ETHADDRSIZ);
			nid += ETHADDRSIZ;
			memcpy(nid, ifi->ifi_haddr, ETHADDRSIZ);
			fprintf(stderr, "\there:");
			print_haddr(ifi->ifi_haddr, ETHADDRSIZ);
			nid += ETHADDRSIZ;
			bzero(nid, ETHADDRSIZ << 1);
			continue;
		} else {
		        /* We have neighbors */
		}
	}

	return (nw - mesg);
}
