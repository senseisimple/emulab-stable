/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

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

/**
 * XXX: no udp support yet, not hard to do though.
 */

/**
 * this app has two modes:
 * first, simply accepts strings of the form on stdin
 *   INTERVAL=%d BLOCKSIZE=%d DURATION=%d
 * where INTERVAL's value is in milliseconds, and BLOCKSIZE's value
 * is in bytes, and DURATION's value is in seconds (if DURATION is 0, keep
 * going until we see the next event on stdin); and adjusts its packet send
 * rates accordingly as soon as it receives the strings.
 * 
 * second, if you specify a file, each line should contain repeated 
 * strings like
 *   INTERVAL=%d BLOCKSIZE=%d STARTTIME=%d DURATION=%d
 * where STARTTIME is a time offset from program start.  When each line's
 * STARTTIME is reached, the previous event (if any) is terminated, and
 * we start sending packets at the new rate specified by INT and BS.
 * 
 * in this second mode, you can also specify DURATION instead of STARTTIME;
 * DURATION's value is also in seconds; as soon as that DURATION has elapsed,
 * we start sending packets at the next INTERVAL and BLOCKSIZE specified in
 * the file, if any.  If DURATION's value is 
 *
 * also: if run in first modes, does NOT DO ANYTHING until it sees something
 * on std input... only connects to server.
 *
 */

/* basic args: -h srvhost -p srvport -f <scriptfile> */


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

#define MODE_STDIN    1
#define MODE_SCRIPT   2

#define MAX_LINE_LEN  128

int main(int argc, char **argv) {
    int mode = MODE_STDIN;
    char *srvhost;
    short srvport;
    int proto;
    char linebuf[MAX_LINE_LEN];
    char *databuf = NULL;

    long duration;
    long interval;
    long 
    
    
    /* grab some quick args, hostname, port, tcp, udp... */
    while ((c = getopt(argc,argv,"h:p:tud")) != -1) {
	switch(c) {
	case 'h':
	    srvhost = optarg;
	    break;
	case 'p':
	    srvport = (short)atoi(optarg);
	    break;
	case 't':
	    proto = SOCK_STREAM;
	    break;
	case 'u':
	    proto = SOCK_DGRAM;
	    fatal("no udp support yet!");
	    break;
	case 'd':
	    ++debug;
	    break;
	default:
	    break;
	}
    }

    while(1) {
	/* read stdin:
	 * if read duration = 0, check for new cmd
	 * each time we are about to write a packet.
	 * if duration > 0, go for that long, then read next cmd.
	 */

	

    return 0;
}
