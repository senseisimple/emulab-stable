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
 * $Id: util.c,v 1.7 2001-08-02 21:21:05 ikumar Exp $
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
		        printf("%s%x", (i == hlen) ? "  " : ":", *ptr++);
		} while (--i > 0);
	}
	printf("\n");
}

void
get_mac_addr(u_char *haddr, u_short hlen, char* addr)
{
    int     i;
	u_char    *ptr;
	char temp_str[10];
	int len=0;
	
	if ((i = hlen) > 0) {
		ptr = haddr;
		do {
		        //sprintf(temp_str,"%s%x", (i == hlen) ? "  " : ":", *ptr++);
		        sprintf(temp_str,"%1x%1x", (0xf0 & *ptr)>>4, (0x0f & *ptr));
				ptr++;
				strncpy(addr+len,temp_str,strlen(temp_str));
				len+=strlen(temp_str);
		} while (--i > 0);
	}
	sprintf(addr+len,"\0");
}


void 
print_haddr(u_char *haddr, u_short hlen)
{
        int     i;
	u_char    *ptr;
	
	if ((i = hlen) > 0) {
		ptr = haddr;
		do {
  		        printf("%s%x", (i == hlen) ? "" : ":", *ptr++);
		} while (--i > 0);
	}
}

/* 
 * Print a td reply packet. They are of the form
 * 
 * [Inquiry ID ]
 * [TTL, Factor]
 * [Path, Dest ] 
 * [Path, Dest ] 
 * [...]
 *
 * Inquiry IDs consist of a node ID and a 
 * timestamp. 
 * 
 * The TTL and Factor are both unsigned 16-bit
 * numbers derived from user parameters.
 * 
 * Paths and destinations consist
 * of <node ID, MAC address> pairs. Node IDs 
 * themselves are MAC addresses.
 */
void
print_tdreply(const char *mesg, size_t nbytes)
{
	struct topd_nbor *p; 
	
	print_tdinq(mesg);
	p = (struct topd_nbor *) (mesg + sizeof(topd_inqid_t));

	while( (char *)p < mesg + nbytes ) {

		printf("ROUTE\t\t\t\tDEST\n");
		printf("[");
		print_haddr(p->tdnbor_pnode, ETHADDRSIZ);
		printf("-");
		print_haddr(p->tdnbor_pif, ETHADDRSIZ);
		printf("] ");
		printf("[");	
		print_haddr(p->tdnbor_dnode, ETHADDRSIZ);
		printf("-");
		print_haddr(p->tdnbor_dif, ETHADDRSIZ);
		printf("]\n\n");

		p++;
	}
}

void
print_tdpairs(const char *mesg, size_t nbytes)
{
        struct topd_nbor *p;

        p = (struct topd_nbor *)mesg;

        while( (char *)p < mesg + nbytes ) {

                printf( "ROUTE\t\t\t\tDEST\n");
                printf( "[");
                print_haddr(p->tdnbor_pnode, ETHADDRSIZ);
                printf( "-");
                print_haddr(p->tdnbor_pif, ETHADDRSIZ);
                printf( "] ");
                printf("[");
                print_haddr(p->tdnbor_dnode, ETHADDRSIZ);
                printf("-");
                print_haddr(p->tdnbor_dif, ETHADDRSIZ);
                printf("]\n\n");

                p++;
        }
}

void
print_tdnbrlist(struct topd_nborlist * list)
{
	printf("Printing the neighbor list:\n");
	while(list!=NULL)
	{
		print_tdpairs(list->tdnbl_nbors,list->tdnbl_n * sizeof(struct topd_nbor));
		list = list->tdnbl_next;
	}
	printf("---\n");
}

void
print_tdifinbrs(struct ifi_info *ifihead)
{
	struct ifi_info      *ifi;
	ifi = ifihead;
	for(; ifi != NULL; ifi = ifi->ifi_next) {
	
	if (ifi->ifi_flags & !IFF_UP || 
		    ifi->ifi_flags & IFF_LOOPBACK || (strcmp(ifi->ifi_name,"fxp4")==0)) {
		        continue;
		}
	printf("The neighbor's list of interface \"%s\"==>\n",ifi->ifi_name);
	print_tdnbrlist(ifi->ifi_nbors);
	printf("------\n");

	}
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

	printf("\nINQ:%u.%u TTL:%d FACTOR:%d NODE:",
		ntohl(tip->tdi_tv.tv_sec), 
		ntohl(tip->tdi_tv.tv_usec),
		ntohs(tip->tdi_ttl), 
		ntohs(tip->tdi_factor)); 
	print_nodeID(tip->tdi_nodeID);
	printf(" PARENT i/f:");
	print_nodeID(tip->tdi_p_nodeIF);
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
