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
/** -M <middleman hostport> */
short middleman_port = MIDDLEMAN_RECV_CLIENT_PORT;
int debug = 0;

char *deadbeef = "deadbeef";

/** 
 * functions
 */
void usage(char *bin) {
    fprintf(stdout,
	    "USAGE: %s -scCmMud  (option defaults in parens)\n",
	    bin
	    );
}

void parse_args(int argc,char **argv) {
    int c;
    char *ep = NULL;

    while ((c = getopt(argc,argv,"s:m:M:ud")) != -1) {
	switch(c) {
	case 's':
	    block_size = (int)strtol(optarg,&ep,10);
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
    if ((recv_sock = socket(PF_INET,SOCK_STREAM,0)) == -1) {
	efatal("could not get recv socket");
    }

    /* connect to the middleman if we can... */
    if (connect(recv_sock,
		(struct sockaddr *)&recv_sa,
		sizeof(struct sockaddr_in)) < 0) {
	efatal("could not connect to middleman");
    }

    /**
     * read blocks forever, noting times at which a block was completely read.
     */

    while (1) {
	bytesRead = 0;
	while (bytesRead < block_size) {
	    retval = read(recv_sock,buf,block_size);

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

	gettimeofday(&t1,NULL);

	fprintf(stdout,
		"INFO: read %d bytes (a block) at %.6f\n",
		block_size,
		t1.tv_sec + t1.tv_usec / 1000000.0f);
    }

    /* never get here... */
    exit(0);

}
