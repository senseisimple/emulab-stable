
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "mtp.h"

static int mygethostbyname(struct sockaddr_in *host_addr, char *host)
{
    struct hostent *host_ent;
    int retval = 0;

    assert(host_addr != NULL);
    assert(host != NULL);
    assert(strlen(host) > 0);

    if( (host_ent = gethostbyname(host)) != NULL ) {
	memcpy((char *)&host_addr->sin_addr.s_addr,
	       host_ent->h_addr,
	       host_ent->h_length);
	retval = 1;
    }
    else {
	retval = inet_aton(host, &host_addr->sin_addr);
    }
    return( retval );
}

static void usage(void)
{
    fprintf(stderr, "Usage: mtp_dump <host> <port>\n");
}

int main(int argc, char *argv[])
{
    int port = 0, max_packets = 0, retval = EXIT_FAILURE;
    struct sockaddr_in sin;
    
    memset(&sin, 0, sizeof(sin));
    
    if (argc < 3) {
	fprintf(stderr, "error: not enough arguments\n");
	usage();
    }
    else if (sscanf(argv[2], "%d", &port) != 1) {
	fprintf(stderr, "error: port is not a number\n");
    }
    else if ((argc > 3) && sscanf(argv[3], "%d", &max_packets) != 1) {
	fprintf(stderr, "error: max packets argument is not a number\n");
    }
    else if (mygethostbyname(&sin, argv[1]) == 0) {
	fprintf(stderr, "error: unknown host %s\n", argv[1]);
    }
    else {
	int fd;
	
	sin.sin_len = sizeof(sin);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	    perror("socket");
	    retval = EXIT_FAILURE;
	}
	else if (connect(fd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
	    perror("connect");
	}
	else {
	    struct mtp_packet *mp;
	    int lpc = 0;
	    
	    while (((max_packets == 0) || (lpc < max_packets)) &&
		   (mtp_receive_packet(fd, &mp) == MTP_PP_SUCCESS)) {
		mtp_print_packet(stdout, mp);
		
		mtp_free_packet(mp);
		mp = NULL;

		lpc += 1;
	    }
	    
	    close(fd);
	    fd = -1;
	}

	retval = EXIT_SUCCESS;
    }
    
    return retval;
}
