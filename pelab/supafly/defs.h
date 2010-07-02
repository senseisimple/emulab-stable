/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef __DEFS_H__
#define __DEFS_H__

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>

/** sane systems honor this as 32 bits regardless */
/**
 * each sent packet group (allowing for app/ip frag) must prefix a new
 * unit with the length of crap being sent.  This might be useful...
 */
typedef uint32_t slen_t;

#define MIDDLEMAN_SEND_CLIENT_PORT 6898
#define MIDDLEMAN_RECV_CLIENT_PORT 6342

#define MIDDLEMAN_MAX_CLIENTS 32

#define MAX_NAME_LEN 255

/* currently, just setting same as freebsd 6.x to get "same" behavior... */
#ifndef UDPCTL_MAXDGRAM
#define UDPCTL_MAXDGRAM 9216
#endif

/** 
 * This struct doubles as a tx block hdr and a stats collector.
 */
typedef struct block_hdr {
    uint32_t msg_id;
    uint32_t block_id;
    uint32_t frag_id;
    
    /* NOTE: times are when the first bytes were sent with write/sendto! */
    /** sender time */
    uint32_t tv_send_send_s;
    uint32_t tv_send_send_us;
    /** middleman times */
    uint32_t tv_mid_recv_s;
    uint32_t tv_mid_recv_us;
    uint32_t tv_mid_send_s;
    uint32_t tv_mid_send_us;
    /**
     * receiver time (only used in stats; just here cause it doesn't make
     * any difference to have in the packet, cause the packet is filled with
     * deadbeef anyway).
     */
    uint32_t tv_recv_recv_s;
    uint32_t tv_recv_recv_us;
} block_hdr_t;

int marshall_block_hdr_send(char *buf,
			    uint32_t msg_id,uint32_t block_id,uint32_t frag_id,
			    struct timeval *tv_send_send);
int marshall_block_hdr_mid( char *buf,
			    struct timeval *tv_mid_recv,
			    struct timeval *tv_mid_send);
block_hdr_t *unmarshall_block_hdr(char *buf,block_hdr_t *fill_hdr);

#endif /* __DEFS_H__ */
