/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file networkInterface.c
 *
 * Implemenation of the network interface convenience functions.
 *
 * NOTE: Most of this was taken from the JanosVM.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <net/if.h>
#include <ifaddrs.h>

#if defined(__FreeBSD__)
#include <net/if_dl.h>
#include <net/if_arp.h>
#include <net/if_types.h>
#endif

#include "networkInterface.h"

char *niFormatMACAddress(char *dest, struct sockaddr *sa)
{
    char *retval = dest;
    int lpc, hlen;
    char *haddr;
    
#if defined(AF_LINK)
    /* FreeBSD link layer address. */
    struct sockaddr_dl *sdl;
    
    sdl = (struct sockaddr_dl *)sa;
    hlen = sdl->sdl_alen;
    haddr = sdl->sdl_data + sdl->sdl_nlen;
#else
    hlen = 0;
#endif
    for( lpc = 0; lpc < hlen; lpc++ )
    {
	sprintf(dest,
		"%s%02x",
		(lpc > 0) ? ":" : "",
		haddr[lpc] & 0xff);
	dest += strlen(dest);
    }
    return( retval );
}
