/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef TBNEXTHOP
#define TBNEXTHOP

#include <netinet/in.h>

uint16_t
get_nexthop_if(struct in_addr addr);

#endif
