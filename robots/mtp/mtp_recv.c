
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

int main(int argc, char *argv[])
{
    int fd, port = 0, retval = EXIT_SUCCESS;

    if ((argc == 2) && sscanf(argv[1], "%d", &port) != 1) {
	fprintf(stderr, "error: port argument is not a number: %s\n", argv[1]);
	retval = EXIT_FAILURE;
    }
    else if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	perror("socket");
	retval = EXIT_FAILURE;
    }
    else {
	struct sockaddr_in sin;
	int on_off = 1;
	
	memset(&sin, 0, sizeof(sin));
	sin.sin_len = sizeof(sin);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = INADDR_ANY;

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on_off, sizeof(int));
	
	if (bind(fd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
	    perror("bind");
	    retval = EXIT_FAILURE;
	}
	else if (listen(fd, 5) == -1) {
	    perror("listen");
	    retval = EXIT_FAILURE;
	}
	else {
	    struct sockaddr_in sin_peer;
	    socklen_t slen;
	    int fd_peer;

	    slen = sizeof(sin_peer);
	    
	    getsockname(fd, (struct sockaddr *)&sin, &slen);
	    printf("Listening for mtp packets on port: %d\n",
		   ntohs(sin.sin_port));
	    
	    while ((fd_peer = accept(fd,
				     (struct sockaddr *)&sin_peer,
				     &slen)) != -1) {
		struct mtp_packet *mp;
		
		while (mtp_receive_packet(fd_peer, &mp) == MTP_PP_SUCCESS) {
		    mtp_print_packet(stdout, mp);
		    
		    mtp_free_packet(mp);
		    mp = NULL;
		}

		close(fd_peer);
		fd_peer = -1;
		
		slen = sizeof(sin_peer);
	    }
	    
	    perror("accept");
	}
	
	close(fd);
	fd = -1;
    }
    
    return retval;
}
