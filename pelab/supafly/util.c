/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "util.h"
#include "defs.h"

void efatal(char *msg) {
    fprintf(stdout,"%s: %s\n",msg,strerror(errno));
    exit(-1);
}

void fatal(char *msg) {
    fprintf(stdout,"%s\n",msg);
    exit(-2);
}

void ewarn(char *msg) {
    fprintf(stdout,"WARNING: %s: %s\n",msg,strerror(errno));
}

void warn(char *msg) {
    fprintf(stdout,"WARNING: %s\n",msg);
}

int fill_sockaddr(char *hostname,int port,struct sockaddr_in *new_sa) {
    struct hostent *host_ptr;

    if (port < 1) {
	return -2;
    }

    new_sa->sin_family = AF_INET;
    new_sa->sin_port = htons((short) port);
 
    if (hostname != NULL && strlen(hostname) > 0) {
	host_ptr = gethostbyname(hostname);
    
	if (!host_ptr) {
	    return -3;
	}
	
	memcpy((char *) &new_sa->sin_addr.s_addr,
	       (char *) host_ptr->h_addr_list[0],
	       host_ptr->h_length);
    }
    else {
	new_sa->sin_addr.s_addr = INADDR_ANY;
    }

    return 0;
}

/** initializes the whole header; unspecified times are zeroed. */
int marshall_block_hdr_send(char *buf,
			    uint32_t msg_id,uint32_t block_id,uint32_t frag_id,
			    struct timeval *tv_send_send) {
    int offset = 0;
    uint32_t tmp = 0;

    tmp = htonl(msg_id);
    memcpy((uint32_t *)&(buf[offset]),&tmp,sizeof(uint32_t));
    offset += sizeof(uint32_t);
    tmp = htonl(block_id);
    memcpy((uint32_t *)&(buf[offset]),&tmp,sizeof(uint32_t));
    offset += sizeof(uint32_t);
    tmp = htonl(frag_id);
    memcpy((uint32_t *)&(buf[offset]),&tmp,sizeof(uint32_t));
    offset += sizeof(uint32_t);
    
    /** heh, we're only just past 1 billion seconds ;) */
    tmp = htonl((uint32_t)tv_send_send->tv_sec);
    memcpy((uint32_t *)&(buf[offset]),&tmp,sizeof(uint32_t));
    offset += sizeof(uint32_t);
    tmp = htonl((uint32_t)tv_send_send->tv_usec);
    memcpy((uint32_t *)&(buf[offset]),&tmp,sizeof(uint32_t));
    offset += sizeof(uint32_t);

    tmp = sizeof(uint32_t) * 6;
    memset(&buf[offset],0,tmp);

    return 1;
}

/** sets only the mid timestamps. */
int marshall_block_hdr_mid(char *buf,
			   struct timeval *tv_mid_recv,
			   struct timeval *tv_mid_send) {
    int offset = 0;
    uint32_t tmp = 0;

    offset = sizeof(uint32_t) * 5;

    if (tv_mid_recv) {
	tmp = htonl(tv_mid_recv->tv_sec);
	memcpy((uint32_t *)&(buf[offset]),&tmp,sizeof(uint32_t));
	offset += sizeof(uint32_t);
	tmp = htonl(tv_mid_recv->tv_usec);
	memcpy((uint32_t *)&(buf[offset]),&tmp,sizeof(uint32_t));
	offset += sizeof(uint32_t);
    }
    else {
	offset += sizeof(uint32_t) * 2;
    }
    if (tv_mid_send) {
	tmp = htonl(tv_mid_send->tv_sec);
	memcpy((uint32_t *)&(buf[offset]),&tmp,sizeof(uint32_t));
	offset += sizeof(uint32_t);
	tmp = htonl(tv_mid_send->tv_usec);
	memcpy((uint32_t *)&(buf[offset]),&tmp,sizeof(uint32_t));
	offset += sizeof(uint32_t);
    }
    else {
	offset += sizeof(uint32_t) * 2;
    }

    return 1;

}

block_hdr_t *unmarshall_block_hdr(char *buf,block_hdr_t *hdr) {
    int offset = 0;
    uint32_t tmp = 0;

    hdr->msg_id = ntohl(*((uint32_t *)&buf[offset]));
    offset += sizeof(uint32_t);
    hdr->block_id = ntohl(*((uint32_t *)&buf[offset]));
    offset += sizeof(uint32_t);
    hdr->frag_id = ntohl(*((uint32_t *)&buf[offset]));
    offset += sizeof(uint32_t);
    
    hdr->tv_send_send_s = ntohl(*((uint32_t *)&buf[offset]));
    offset += sizeof(uint32_t);
    hdr->tv_send_send_us = ntohl(*((uint32_t *)&buf[offset]));
    offset += sizeof(uint32_t);
    hdr->tv_mid_recv_s = ntohl(*((uint32_t *)&buf[offset]));
    offset += sizeof(uint32_t);
    hdr->tv_mid_recv_us = ntohl(*((uint32_t *)&buf[offset]));
    offset += sizeof(uint32_t);
    hdr->tv_mid_send_s = ntohl(*((uint32_t *)&buf[offset]));
    offset += sizeof(uint32_t);
    hdr->tv_mid_send_us = ntohl(*((uint32_t *)&buf[offset]));
    offset += sizeof(uint32_t);
    hdr->tv_recv_recv_s = ntohl(*((uint32_t *)&buf[offset]));
    offset += sizeof(uint32_t);
    hdr->tv_recv_recv_us = ntohl(*((uint32_t *)&buf[offset]));
    offset += sizeof(uint32_t);
    
    return hdr;
}
