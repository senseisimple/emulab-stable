/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <stdio.h>
#include <fcntl.h>

#include "defs.h"
#include "crypto.h"

/** 
 * globals
 */
char *optarg;
int optind,opterr,optopt;

/** -s <bytes>*/
int block_size = 4096;
/** -c <num> */
int block_count = 1024;
/** -C <num> */
int msg_count = 1;
/** -p <microseconds> */
long block_pause_us = 1000;
/** -P <microseconds> */
long msg_pause_us = 5000000;
/** -m <middleman hostname> */
char *middleman_host = "localhost";
/** -M <middleman hostport> */
short middleman_port = MIDDLEMAN_SEND_CLIENT_PORT;
int debug = 0;

char *deadbeef = "deadbeef";

int ack_count = 0;

int unack_threshold = -1;

int udp = 0;
int udp_frag_size = 0;

/** 
 * functions
 */
void usage(char *bin) {
    fprintf(stdout,
	    "USAGE: %s -scCpPmMudba  (option defaults in parens)\n"
	    "  -s <block_size>  tx block size (%d)\n"
	    "  -c <block_count> tx block size (%d)\n"
	    "  -C <msg_count>   number of times to send block_count msgs (%d)\n"
	    "  -p <time>        Microseconds to pause between block sends (%d)\n"
	    "  -P <time>        Microseconds to pause between msg sends (%d)\n"
	    "  -m <hostname>    Middleman host to connect to (%s)\n"
	    "  -M <port>        Middle port to connect to (%d)\n"
	    "  -a <threshold>   Stop sending if gte threshold unack'd blocks "
	    "remain (if less than 0, \n"
	    "                     never wait) (%d)\n"
	    "                     (NOTE: cannot use with udp)\n"
	    "  -U               Use udp instead of tcp\n"
	    "  -F <frag_size>   Application fragment size for udp (i.e. if \n"
	    "                     -s 1024 and -F 512, fragment each block \n"
	    "                     into two chunks)\n"
	    "  -d[d..d]         Enable various levels of debug output\n"
	    "  -u               Print this msg\n",
	    bin,block_size,block_count,msg_count,block_pause_us,msg_pause_us,
	    middleman_host,middleman_port,unack_threshold
	    );
}

void parse_args(int argc,char **argv) {
    int c;
    char *ep = NULL;

    while ((c = getopt(argc,argv,"s:c:C:p:P:m:M:R:h:uda:bUF:")) != -1) {
	switch(c) {
	case 's':
	    block_size = (int)strtol(optarg,&ep,10);
	    if (ep == optarg) {
		usage(argv[0]);
		exit(-1);
	    }
	    if (block_size < sizeof(block_hdr_t)) {
		fprintf(stderr,
			"ERROR: block size must be at least %d bytes "
			"(sizeof msg hdr)!\n",
			sizeof(block_hdr_t));
		exit(-1);
	    }
	    break;
	case 'c':
	    block_count = (int)strtol(optarg,&ep,10);
	    if (ep == optarg) {
		usage(argv[0]);
		exit(-1);
	    }
	    break;
	case 'C':
	    msg_count = (int)strtol(optarg,&ep,10);
	    if (ep == optarg) {
		usage(argv[0]);
		exit(-1);
	    }
	    break;
	case 'p':
	    block_pause_us = (int)strtol(optarg,&ep,10);
	    if (ep == optarg) {
		usage(argv[0]);
		exit(-1);
	    }
	    break;
	case 'P':
	    msg_pause_us = (int)strtol(optarg,&ep,10);
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
	case 'a':
	    unack_threshold = (int)strtol(optarg,&ep,10);
	    if (ep == optarg) {
		usage(argv[0]);
		exit(-1);
	    }
	    break;
	case 'U':
	    udp = 1;
	    break;
	case 'F':
	    udp_frag_size = (int)strtol(optarg,&ep,10);
	    if (ep == optarg) {
		usage(argv[0]);
		exit(-1);
	    }
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

    if (unack_threshold > -1 && udp) {
	fprintf(stderr,
		"ERROR: cannot use an ack window with udp since this program\n"
		"does not retransmit unack'd msgs!\n");
	exit(-1);
    }

    if ((udp_frag_size > 0 && udp_frag_size < sizeof(block_hdr_t)) 
	|| block_size < sizeof(block_hdr_t)) {
	fprintf(stderr,
		"ERROR: requested block/frag size is less than hdr size\n"
		"of %d bytes\n!",
		sizeof(block_hdr_t));
	exit(-1);
    }

    return;
}


int main(int argc,char **argv) {
    char *buf;
    struct sockaddr_in send_sa;
    int send_sock;
    int remaining_block_count;
    int remaining_msg_count;
    int remaining_frag_count;
    int i;
    int retval;
    int bytesWritten;
    struct timeval tv;
    int ack_count;
    int frag_count;
    /** might be block_size or udp_frag_size */
    int udp_msg_size;
    int bsize;
    
    parse_args(argc,argv);

    /* initialize ourself... */
    srand(time(NULL));

    if (udp_frag_size) {
	frag_count = (block_size % udp_frag_size)?(block_size/udp_frag_size + 1):(block_size/udp_frag_size);
	udp_msg_size = udp_frag_size;
    }
    else {
	frag_count = 1;
	udp_msg_size = block_size;
    }
    bsize = udp_msg_size;

    /* grab some buf! */
    if ((buf = (char *)malloc(sizeof(char)*bsize)) == NULL) {
	efatal("no memory for data buf");
    }
    
    /* fill with deadbeef */
    for (i = 0; i < bsize; i += 8) {
	memcpy(&buf[i],deadbeef,8);
    }
    
    /* setup the sockaddrs */
    if ((retval = fill_sockaddr(middleman_host,
				middleman_port,
				&send_sa)) != 0) {
	if (retval == -1) {
	    fatal("bad port");
	}
	else {
	    efatal("host lookup failed");
	}
    }

    /* startup send client... */
    if ((send_sock = socket(PF_INET,(udp)?SOCK_DGRAM:SOCK_STREAM,0)) == -1) {
	efatal("could not get send socket");
    }

    if (!udp) {
	/* connect to the middleman if we can... */
	if (connect(send_sock,
		    (struct sockaddr *)&send_sa,
		    sizeof(struct sockaddr_in)) < 0) {
	    efatal("could not connect to middleman");
	}
    }

    /* set nonblocking so we can read acks at low priority */
    fcntl(send_sock,F_SETFL,O_NONBLOCK);
    
    /* heh, we don't do encryption!  that way, if the middleman
     * does a decrypt/encrypt with the same keys/iv, they should forward
     * deadbeef to the sfreceiver.  nasty, but whatever.  provides a little
     * testing.
     */
    remaining_msg_count = msg_count;
    ack_count = 0;
    while (remaining_msg_count > 0) {
	/* prepare to send a msg... */
	remaining_block_count = block_count;

	while (remaining_block_count > 0) {
	    /* send a block */
	    bytesWritten = 0;

	    remaining_frag_count = frag_count;
	    
	    if (udp) {
		while (remaining_frag_count) {
		    gettimeofday(&tv,NULL);
		    marshall_block_hdr_send(buf,
					    msg_count-remaining_msg_count,
					    block_count-remaining_block_count,
					    frag_count-remaining_frag_count,
					    &tv);
		    retval = sendto(send_sock,buf,udp_msg_size,0,
				    (struct sockaddr *)&send_sa,
				    sizeof(send_sa));
		    if (retval < 0) {
			if (errno == EAGAIN) {
			    ewarn("EAGAIN while sending udp");
			}
			else if (errno == EMSGSIZE) {
			    efatal("msg size too big for atomic send");
			}
			else if (retval == EPIPE) {
			    efatal("while writing udp to middleman");
			}
			else {
			    efatal("uncaught error");
			}
		    }
		    else if (retval == udp_msg_size) {
			--remaining_frag_count;
		    }
		    else {
			--remaining_frag_count;
			fprintf(stderr,"WARNING: only sent %d of %d bytes\n",
				retval,udp_msg_size);
		    }
		}
	    }
	    else {
		while (bytesWritten < block_size) {
		    marshall_block_hdr_send(buf,
					    msg_count-remaining_msg_count,
					    block_count-remaining_block_count,
					    frag_count-remaining_frag_count,
					    &tv);
		    retval = write(send_sock,buf,block_size - bytesWritten);
		    
		    if (retval < 0) {
			if (errno == EPIPE) {
			    efatal("while writing to middleman");
			}
			else if (errno == EAGAIN) {
			    ;
			}
			else {
			    ewarn("while writing to middleman");
			}
		    }
		    else {
			bytesWritten += retval;
		    }
		}
	    }
	    gettimeofday(&tv,NULL);

	    fprintf(stdout,"TIME %d %.4f\n",
		    block_count - remaining_block_count,
		    tv.tv_sec + tv.tv_usec / 1000000.0f);

	    --remaining_block_count;

	    if (remaining_block_count % 8 == 0) {
		fprintf(stdout,
			"INFO: %d blocks remaining in msg %d; %d acks\n",
			remaining_block_count,
			msg_count - remaining_msg_count,
			ack_count);
	    }

	    /* interblock sleep time */
	    if (block_pause_us > 0) {
		if (usleep(block_pause_us) < 0) {
		    ewarn("usleep after block failed");
		}
	    }

	    if (!udp) {
		/* check for acks... */
		/* just read one right away to stay ahead of the game... */
		while (read(send_sock,&buf[0],sizeof(char)) == sizeof(char)) {
		    ++ack_count;
		}
	    }

	    if (!udp && unack_threshold > -1) {
		while (((block_count - remaining_block_count) - ack_count) > unack_threshold) {
		    warn("hit the unack threshold!");
		    
		    /* sleep until we get ack'd */
		    if (usleep(100*1000) < 0) {
			ewarn("sleep at ack edge failed");
		    }
		    
		    if (read(send_sock,&buf[0],sizeof(char)) == sizeof(char)) {
			++ack_count;
		    }
		}
	    }
	    
	}

	remaining_block_count = block_size;
	--remaining_msg_count;
	ack_count = 0;

	fprintf(stdout,
		"INFO: only %d msgs remaining\n",
		remaining_msg_count);

	/* intermsg sleep time */
	if (msg_pause_us > 0 && remaining_msg_count > 0) {
	    if (usleep(msg_pause_us) < 0) {
		ewarn("usleep after msg failed");
	    }
	}

    }

    fprintf(stdout,"Done, exiting.\n");

    exit(0);

}
