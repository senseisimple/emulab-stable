/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file networkInterface.h
 *
 * Header file for network interface convenience functions.
 *
 * NOTE: Most of this was taken from the JanosVM.
 */

#ifndef NETWORK_INTERFACE_H
#define NETWORK_INTERFACE_H

#include <sys/types.h>
#include <sys/socket.h>

/**
 * Format a link layer socket address into a string.
 *
 * <p>Example output: 00:02:b3:65:b6:46
 *
 * @param dest The destination for the formatted address string.
 * @param sa The socket address to convert.
 * @return dest
 */
char *niFormatMACAddress(char *dest, struct sockaddr *sa);

#endif
