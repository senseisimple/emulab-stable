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


#define MAX_CLIENTS  1024
#define DEF_SRV_PORT 4545
/* ip addr in str form can't be bigger than this... */
#define MAX_NAME_LEN 15

#define PROTO_TCP    SOCK_STREAM
#define PROTO_UDP    SOCK_DGRAM
#define DEF_PROTO    PROTO_TCP


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

void parse_args() {
    ;
}

/**
 * XXX: no udp support yet, easy hack if anybody wants it, just
 * spin on recvfrom instead of doing a select...
 */

int main(int argc,char **argv) {
    int i,srv_sock,j,retval,ptype;
    struct sockaddr_in srv_sa;
    int clientfds[MAX_CLIENTS];
    char *clientsas[MAX_CLIENTS];
    int client_cnt = 0;
    int block_size = 1024;
    int maxfd = 0;
    int c;
    char *srvhost = NULL;
    short srvport = DEF_SRV_PORT;
    int proto = DEF_PROTO;
    int debug = 0;
    char *buf = NULL;
    fd_set rfds;
    fd_set static_rfds;

    /* grab some quick args, hostname, port, tcp, udp... */
    while ((c = getopt(argc,argv,"h:p:tudb:")) != -1) {
	switch(c) {
	case 'h':
	    srvhost = optarg;
	    break;
	case 'p':
	    srvport = atoi(optarg);
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
	case 'b':
	    block_size = atoi(optarg);
	    break;
	default:
	    break;
	}
    }

    if ((buf = (char *)malloc(sizeof(char)*block_size)) == NULL) {
	efatal("no memory for data buf");
    }
    
    if ((retval = fill_sockaddr(srvhost,srvport,&srv_sa)) != 0) {
	if (retval == -1) {
	    fatal("bad port");
	}
	else {
	    efatal("host lookup failed");
	}
    }

    /* startup server... */
    if ((srv_sock = socket(AF_INET,proto,0)) == -1) {
	efatal("could not get socket");
    }

    if (bind(srv_sock,
	     (struct sockaddr *)&srv_sa,
	     sizeof(struct sockaddr_in)
	     ) < 0) {
	efatal("could not bind");
    }

    if (proto == PROTO_TCP) {
	if (listen(srv_sock,8) < 0) {
	    efatal("could not listen");
	}
    }

    /* daemonize... */
    if (!debug) {
	daemon(0,0);
    }

    for (i = 0; i < MAX_CLIENTS; ++i) {
	clientfds[i] = -1;
	clientsas[i] = (char *)malloc(MAX_NAME_LEN + 1);
    }

    FD_ZERO(&static_rfds);
    FD_SET(srv_sock,&static_rfds);
    maxfd = srv_sock;
    
    /* listen and read forever */
    while (1) {
	/* reset fdsets */
	memcpy(&rfds,&static_rfds,sizeof(static_rfds));

	retval = select(maxfd+1,&rfds,NULL,NULL,NULL);
	
	if (retval > 0) {
	    if (FD_ISSET(srv_sock,&rfds)) {
		struct sockaddr_in client_sin;
		socklen_t slen;
		int client_fd;

		slen = sizeof(client_sin);

		if ((client_fd = accept(srv_sock,
					(struct sockaddr *)&client_sin,
					&slen)) < 0) {
		    warn("accept failed");
		}
		else if (client_cnt >= MAX_CLIENTS) {
		    warn("already at max clients");
		}
		else {
		    /* add new client... */
		    for (i = 0; i < MAX_CLIENTS; ++i) {
			if (clientfds[i] == -1) {
			    break;
			}
		    }
		    
		    clientfds[i] = client_fd;
		    if (client_fd > maxfd) {
			maxfd = client_fd;
		    }
		    
		    FD_SET(client_fd,&static_rfds);
		    
		    if (debug) {
			fprintf(stdout,
				"connect from %s:%d\n",
				inet_ntoa(client_sin.sin_addr),
				ntohs(client_sin.sin_port));
		    }
		    
		    char *addr = inet_ntoa(client_sin.sin_addr);
		    int addrlen = strlen(addr);
		    
		    strncpy(clientsas[i],
			    addr,
			    (addrlen > MAX_NAME_LEN)?MAX_NAME_LEN:addrlen);
		    /* null term if strncpy couldn't */
		    if (addrlen > MAX_NAME_LEN) {
			clientsas[i][MAX_NAME_LEN] = '\0';
		    }
		    
		    ++client_cnt;
		}
	    }
	    else {
		for (i = 0; i < MAX_CLIENTS; ++i) {
		    if (clientfds[i] > -1) {
			if (FD_ISSET(clientfds[i],&rfds)) {
			    /* read a block, or as much as possible */
			    retval = read(clientfds[i],buf,block_size);

			    /* dead client, pretty much */
			    if (retval <= 0) {
				if (debug) {
				    fprintf(stdout,
					    "disconnect from %s\n",
					    clientsas[i]);
				}

				close(clientfds[i]);
				FD_CLR(clientfds[i],&static_rfds);
				clientfds[i] = -1;

				--client_cnt;
			    }
			    else if (debug > 2 ) {
				fprintf(stdout,
					"DEBUG: read %d bytes from %s\n",
					retval,
					clientsas[i]);
			    }
			}
		    }
		}
	    }
	}
	else if (retval < 0) {
	    /* error... */
	    ewarn("error in select");
	}
    }

    return -1;
}
