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
 * Filename: util.c
 *   -- Author: Kristin Wright <kwright@cs.utah.edu> 
 *
 * ---------------------------
 *
 * $Id: util.c,v 1.1 2000-07-06 17:42:39 kwright Exp $
 */

#include "discvr.h"
#include "packet.h"
#include "util.h"

void 
println_haddr(u_char *haddr, u_short hlen)
{
        int     i;
	u_char    *ptr;
	
	if ((i = hlen) > 0) {
		ptr = haddr;
		do {
		        fprintf(stderr, "%s%x", (i == hlen) ? "  " : ":", *ptr++);
		} while (--i > 0);
	}
	fprintf(stderr, "\n");
}

void 
print_haddr(u_char *haddr, u_short hlen)
{
        int     i;
	u_char    *ptr;
	
	if ((i = hlen) > 0) {
		ptr = haddr;
		do {
		        fprintf(stderr, "%s%x", (i == hlen) ? "" : ":", *ptr++);
		} while (--i > 0);
	}
}

/* 
 * Print a td reply packet. They are of the form
 * 
 * [Inquiry ID],
 * [Path, Dest] 
 * [Path, Dest] 
 * [...]
 *
 * Inquiry IDs consist of a node ID and a 
 * timestamp. 
 * 
 * Paths and destinations consist
 * of <node ID, MAC address> pairs. Node IDs 
 * themselves are MAC addresses.
 */
void
print_tdreply(const char *mesg)
{
	const u_char *p, *p_node, *p_if, *d_node, *d_if;
	
	print_tdinq(mesg);

	p      = mesg + sizeof(topd_inqid_t);
        p_node = &p[0];
	p_if   = &p[ETHADDRSIZ];
	d_node = &p[ETHADDRSIZ << 1];
	d_if   = d_node + ETHADDRSIZ;

	fprintf(stderr, "ROUTE\t\t\t\tDEST\n");
	fprintf(stderr, "[");
	print_haddr(p_node, ETHADDRSIZ);
	fprintf(stderr, "-");
	print_haddr(p_if, ETHADDRSIZ);
	fprintf(stderr, "] ");
	fprintf(stderr, "[");	
	print_haddr(d_node, ETHADDRSIZ);
	fprintf(stderr, "-");
	print_haddr(d_if, ETHADDRSIZ);
	fprintf(stderr, "]\n\n");
}

/*
 * Print a td inquiry packet. They are of the form:
 * 
 * [Inquiry ID],
 * [Node ID or inquiring node]
 *
 * See description of Inquiry IDs and
 * Node IDs above.
 * 
 */
void
print_tdinq(const char *mesg)
{
        topd_inqid_t *tip = (topd_inqid_t *)mesg;

	fprintf(stderr, "\nINQ:%u.%u from node: ",
		ntohl(tip->tdi_tv.tv_sec), ntohl(tip->tdi_tv.tv_usec)); 
	print_nodeID(tip->tdi_nodeID);
}

u_char *max_haddr(u_char *ha1, u_char *ha2)
{
        u_char *t1 = ha1;
	u_char *t2 = ha2;

	if (ha1 == 0) {
	        return ha2;
	} else if (ha2 == 0) {
	        return ha1; 
	}

        while (1) {
	        if (*t1 > *t2) {
	                return ha1;
		} else if (*t1 < *t2) {
		        return ha2;
		} 
		t1++; t2++;
	}

        /* -lkw */
	fprintf(stderr, "should never get here because mac addresses are unique.\n"); 
	exit(1);
}
