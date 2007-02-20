/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <stdio.h>

#include "defs.h"
#include "crypto.h"

/** 
 * globals
 */
char *optarg;
int optind,opterr,optopt;

/** -s <bytes>*/
int block_size = 4096;
/** -m <middleman hostname> */
char *middleman_host = "localhost";
/** -R <middleman hostport> */
short middleman_port = MIDDLEMAN_RECV_CLIENT_PORT;
int debug = 0;
int udp = 0;
int udp_ack = 0;

char *deadbeef = "deadbeef";

/** 
 * functions
 */
void usage(char *bin) {
    fprintf(stdout,
	    "USAGE: %s -smMUdu  (option defaults in parens)\n"
	    "  -s <block_size>  rx block size (%d)\n"
	    "  -m <hostname>    Middleman host to connect to (%s)\n"
	    "  -M <port>        Middle port to connect to (%d)\n"
	    "  -U               Use udp instead of tcp\n"
	    "  -d[d..d]         Enable various levels of debug output\n"
	    "  -a               Enable udp acks back to the sender via middleman\n"
	    "  -u               Print this msg\n",
	    bin,block_size,middleman_host,middleman_port
	    );
}

void parse_args(int argc,char **argv) {
    int c;
    char *ep = NULL;

    while ((c = getopt(argc,argv,"s:m:M:udUa")) != -1) {
	switch(c) {
	case 's':
	    block_size = (int)strtol(optarg,&ep,10);
	    if (ep == optarg) {
		usage(argv[0]);
		exit(-1);
	    }
	    break;
	case 'm':
	    middleman_host = optarg;
	    break;
	case 'M':
	    middleman_port = (short)strtol(optarg,&ep,10);
	    if (ep == optarg) {
		usage(argv[0]);
		exit(-1);
	    }
	    break;
	case 'u':
	    usage(argv[0]);
	    exit(0);
	case 'd':
	    ++debug;
	    break;
	case 'U':
	    udp = 1;
	    break;
	case 'a':
	    udp_ack = 1;
	    break;
	default:
	    break;
	}
    }

    /** arg check */
    if (block_size % 8 != 0) {
	fprintf(stderr,"block_size must be divisible by 8!\n");
	exit(-1);
    }

    return;

}


int main(int argc,char **argv) {
    char *buf,*outbuf;
    struct sockaddr_in recv_sa;
    int recv_sock;
    int i;
    int retval;
    int bytesRead;
    int blocksRead = 0;
    struct timeval t1;
    int block_count = 0;
    char *crbuf = "foo";
    struct sockaddr_in client_sin;
    int slen;
    block_hdr_t hdr;
    struct timeval tv;
    
    parse_args(argc,argv);

    /* initialize ourself... */
    //srand(time(NULL));

    /* grab some buf! */
    if ((buf = (char *)malloc(sizeof(char)*block_size)) == NULL) {
	efatal("no memory for data buf");
    }
    if ((outbuf = (char *)malloc(sizeof(char)*block_size)) == NULL) {
	efatal("no memory for output data buf");
    }
    
    /* fill with deadbeef */
    //for (i = 0; i < block_size; i += 8) {
    //memcpy(&buf[i],deadbeef,8);
    //}
    
    /* setup the sockaddrs */
    if ((retval = fill_sockaddr(middleman_host,
				middleman_port,
				&recv_sa)) != 0) {
	if (retval == -1) {
	    fatal("bad port");
	}
	else {
	    efatal("host lookup failed");
	}
    }

    /* startup recv client... */
    if ((recv_sock = socket(PF_INET,(udp)?SOCK_DGRAM:SOCK_STREAM,0)) == -1) {
	efatal("could not get recv socket");
    }

    if (!udp) {
	/* connect to the middleman if we can... */
	if (connect(recv_sock,
		    (struct sockaddr *)&recv_sa,
		    sizeof(struct sockaddr_in)) < 0) {
	    efatal("could not connect to middleman");
	}
    }
    else {
	/* just send the middleman a "register" msg */
	sendto(recv_sock,crbuf,sizeof(crbuf),0,
	       (struct sockaddr *)&recv_sa,sizeof(struct sockaddr_in));
    }

    if (debug > 1) {
	fprintf(stdout,"DEBUG: connected to %s:%d\n",
		inet_ntoa(recv_sa.sin_addr),ntohs(recv_sa.sin_port));
    }

    /**
     * read blocks forever, noting times at which a block was completely read.
     */

    if (debug > 1) {
	fprintf(stdout,"DEBUG: Receiving blocks...\n");
    }

    while (1) {
	bytesRead = 0;

	if (!udp) {
	    while (bytesRead < block_size) {
		retval = read(recv_sock,
			      &buf[(block_size - bytesRead)],
			      (block_size - bytesRead));
		
		if (retval < 0) {
		    if (errno == ECONNRESET) {
			/* middleman must've dumped out. */
			efatal("middleman appears to have disappeared");
		    }
		    else if (errno == EAGAIN) {
			;
		    }
		    else {
			ewarn("weird");
		    }
		}
		else if (retval == 0) {
		    /* middleman dumped out */
		    efatal("middleman appears to have disappeared");
		}
		else {
		    bytesRead += retval;
		}
	    }
	}
	else {
	    slen = sizeof(struct sockaddr_in);
	    retval = recvfrom(recv_sock,buf,block_size,0,
			      (struct sockaddr *)&client_sin,
			      (socklen_t *)&slen);

	    if (retval < 0) {
		ewarn("error while recvfrom middleman");
	    }
	    else {
		bytesRead = retval;
	    }
	}

	gettimeofday(&t1,NULL);

	/* unmarshall to get id numbers */
	unmarshall_block_hdr(buf,&hdr);
	
	fprintf(stdout,"TIME m%d b%d f%d %.4f\n",
		hdr.msg_id,hdr.block_id,hdr.frag_id,
		t1.tv_sec + t1.tv_usec / 1000000.0f);
	++block_count;
	
	fprintf(stdout,
		"INFO: read %d bytes at %.6f\n",
		bytesRead,
		t1.tv_sec + t1.tv_usec / 1000000.0f);

	fflush(stdout);

	/* if udp, send a best-effort ack that the middleman will forward. */
	if (udp && udp_ack) {
	    retval = sendto(recv_sock,buf,sizeof(block_hdr_t),0,
			    (struct sockaddr *)&client_sin,
			    sizeof(struct sockaddr_in));

	    gettimeofday(&tv,NULL);
	    
	    if (retval < 0) {
		ewarn("while sending udp ack to client");
	    }
	    else if (retval != sizeof(block_hdr_t)) {
		ewarn("only sent part of udp ack msg");
	    }
	    else if (retval == sizeof(block_hdr_t)) {
		fprintf(stdout,
			"ACKTIME: sent m%d b%d f%d at %.6f\n",
			hdr.msg_id,hdr.block_id,hdr.frag_id,
			tv.tv_sec+tv.tv_usec/1000000.0f);
		fflush(stdout);
	    }
	}

    }

    /* never get here... */
    exit(0);

}
