/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef TBNEXTHOP
#define TBNEXTHOP

#include <netinet/in.h>
#include <sys/types.h>

uint16_t
get_nexthop_if(struct in_addr addr);

int
ipfw_addfwd(struct in_addr nexthop, struct in_addr src, struct in_addr dst,
	    struct in_addr dstmask, bool out);
  
#endif
