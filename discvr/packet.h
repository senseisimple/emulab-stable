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
 * Filename: packet.h
 *   -- Author: Kristin Wright <kwright@cs.utah.edu> 
 *
 * ---------------------------
 *
 * $Id: packet.h,v 1.1 2000-07-06 17:42:37 kwright Exp $
 */

#ifndef _TOPD_PACKET_H_
#define _TOPD_PACKET_H_

#include "discvr.h"

/*
 * Node IDs are the highest MAC address on the machine.
 * The easiest but not the most portable thing to
 * do is assume a 6-byte hardware address. This isn't
 * completely ridiculous as most of this tool's 
 * target platforms use ethernet. 
 * 
 * typedef u_char topd_nodeID_t[ETHADDRSIZ]; 
 */

/* 
 * Inquiry packet has only a unique Inquiry ID
 * comprised of [timestamp, sender-nodeID] pair. Timestamps
 * are struct timevals returned by gettimeofday().
 */
typedef struct topd_inqid {
        struct timeval     tdi_tv;
        u_char             tdi_nodeID[ETHADDRSIZ];
} topd_inqid_t;

#define TOPD_INQ_SIZ ALIGN(sizeof(struct topd_inqid))

/*
 * Neighbor (reply) packets consist of 
 *
 *     [Neighbor ID, Route]
 *
 * pairs. Routes identify the interface through which
 * the node can reach the neighbor. These interfaces
 * are not necessarily local. Rather, they are 
 * interfaces belonging to the neighbor's parent.
 */
struct topd_nbor {
  u_char  tdnbor_route[ETHADDRSIZ];
  u_char  tdnbor_nbor[ETHADDRSIZ];
};

struct topd_nborlist {
  struct topd_nborlist *tdnbl_next;          /* next of these structures */
  u_int32_t            tdnbl_n;              /* number of neighbors in neighbor list */ 
  u_char               *tdnbl_nbors;         /* neighbor list */
};



#endif /* _TOPD_PACKET_H_ */
