/* 
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000 The University of Utah and the Flux Group.
 * All rights reserved.
 *
 * ---------------------------
 *
 * Filename: util.h
 *   -- Author: Kristin Wright <kwright@cs.utah.edu> 
 *
 * ---------------------------
 *
 * $Id: util.h,v 1.7 2004-06-17 18:17:01 mike Exp $
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




