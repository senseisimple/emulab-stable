/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <netinet/in.h>

/**
 * ugh, this is all very catch-all.
 *
 * ANYTHING that I might in more than one entity goes here.
 */

void efatal(char *msg);
void fatal(char *msg);
void ewarn(char *msg);
void warn(char *msg);
int fill_sockaddr(char *hostname,int port,struct sockaddr_in *new_sa);

#endif
