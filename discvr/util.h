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
 * Filename: util.h
 *   -- Author: Kristin Wright <kwright@cs.utah.edu> 
 *
 * ---------------------------
 *
 * $Id: util.h,v 1.6 2001-08-04 22:58:30 ikumar Exp $
 */

void 
print_haddr(u_char *haddr, u_short hlen);

void 
println_haddr(u_char *haddr, u_short hlen);

u_char*
max_haddr(u_char *ha1, u_char *ha2);

void
print_tdinq(const char *mesg);

void
print_tdreply(const char *mesg, size_t nbytes); 

void
print_tdnbrlist(struct topd_nborlist * list);

void
print_tdpairs(const char *mesg, size_t nbytes);

void
print_tdifinbrs(struct ifi_info *ifihead);

void
get_mac_addr(u_char *haddr, u_short hlen, char* addr);




