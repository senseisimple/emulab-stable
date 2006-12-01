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

int block_size = 4096;
int block_count = 1024;
int only_encrypt = 0;
int debug = 0;
char *srv_host = INADDR_ANY;
short srv_send_port = MIDDLEMAN_SEND_CLIENT_PORT;
short srv_recv_port = MIDDLEMAN_RECV_CLIENT_PORT;

char *deadbeef = "deadbeef";

int pushback = 0;

/**
 * defs
 */
void usage(char *bin) {
    fprintf(stdout,
	    "USAGE: %s -scohSRudb  (option defaults in parens)\n"
	    "  -s <block_size>  tx block size (%d)\n"
	    "  -c <block_count> tx block size (%d)\n"
	    "  -h <hostname>    Listen on this host/addr (INADDR_ANY)\n"
	    "  -S <port>        Listen on this port for one sender at a time (%d)\n"
	    "  -R <port>        Listen on this port for receivers (%d)\n"
	    "  -o               Only encrypt (instead of dec/enc)\n"
	    "  -b               Ack all received blocks from sender AFTER crypto\n"
	    "  -d[d..d]         Enable various levels of debug output\n"
	    "  -u               Print this msg\n",
	    bin,block_size,block_count,srv_send_port,srv_recv_port
	    );
}

void parse_args(int argc,char **argv) {
    int c;
    char *ep = NULL;

    while ((c = getopt(argc,argv,"s:c:S:R:h:oudb")) != -1) {
	switch(c) {
	case 's':
	    block_size = (int)strtol(optarg,&ep,10);
	    if (ep == optarg) {
		usage(argv[0]);
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
	case 'h':
	    srv_host = optarg;
	    break;
	case 'S':
	    srv_send_port = (short)strtol(optarg,&ep,10);
	    if (ep == optarg) {
		usage(argv[0]);
		exit(-1);
	    }
	    break;
	case 'R':
	    srv_recv_port = (short)strtol(optarg,&ep,10);
	    if (ep == optarg) {
		usage(argv[0]);
		exit(-1);
	    }
	    break;
	case 'o':
	    only_encrypt = 1;
	    break;
	case 'u':
	    usage(argv[0]);
	    exit(0);
	case 'd':
	    ++debug;
	    break;
	case 'b':
	    pushback = 1;
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
    int srv_send_sock,srv_recv_sock;
    int retval,ptype;
    int i,j;
    struct sockaddr_in srv_send_sa,srv_recv_sa;
    int recv_clientfds[MIDDLEMAN_MAX_CLIENTS];
    char *recv_clientsas[MIDDLEMAN_MAX_CLIENTS];
    int recv_client_cnt = 0;
    int max_fd = 0;
    int send_client_fd = -1;
    int c;
    char *buf = NULL;
    char *outbuf = NULL;
    fd_set rfds;
    fd_set static_rfds;
    int current_send_rc;
    int block_count = 0;

    /* grab some quick args */
    parse_args(argc,argv);

    /* initialize ourself... */
    srand(time(NULL));

    /* grab the bufs... */
    if ((buf = (char *)malloc(sizeof(char)*block_size)) == NULL) {
	efatal("no memory for data buf");
    }
    if ((outbuf = (char *)malloc(sizeof(char)*block_size)) == NULL) {
	efatal("no memory for output data buf");
    }

    /* fill with deadbeef */
    for (i = 0; i < block_size; i += 8) {
	memcpy(&buf[i],deadbeef,8);
    }
    
    /* setup the sockaddrs */
    if ((retval = fill_sockaddr(srv_host,srv_send_port,&srv_send_sa)) != 0) {
	if (retval == -1) {
	    fatal("bad port");
	}
	else {
	    efatal("host lookup failed");
	}
    }
    if ((retval = fill_sockaddr(srv_host,srv_recv_port,&srv_recv_sa)) != 0) {
	if (retval == -1) {
	    fatal("bad port");
	}
	else {
	    efatal("host lookup failed");
	}
    }

    /* startup server... */
    if ((srv_send_sock = socket(PF_INET,SOCK_STREAM,0)) == -1) {
	efatal("could not get send listen socket");
    }
    if ((srv_recv_sock = socket(PF_INET,SOCK_STREAM,0)) == -1) {
	efatal("could not get send listen socket");
    }

    if (bind(srv_send_sock,
	     (struct sockaddr *)&srv_send_sa,
	     sizeof(struct sockaddr_in)
	     ) < 0) {
	efatal("could not bind");
    }
    if (bind(srv_recv_sock,
	     (struct sockaddr *)&srv_recv_sa,
	     sizeof(struct sockaddr_in)
	     ) < 0) {
	efatal("could not bind");
    }

    /* only listen for len 1 cause we won't accept on this guy if we 
     * already have a sender
     */
    if (listen(srv_send_sock,1) < 0) {
	efatal("could not listen");
    }
    /* more sane listen queue for this one */
    if (listen(srv_recv_sock,8) < 0) {
	efatal("could not listen");
    }

    /* daemonize... */
    if (!debug) {
	daemon(0,0);
    }

    for (i = 0; i < MIDDLEMAN_MAX_CLIENTS; ++i) {
	recv_clientfds[i] = -1;
	recv_clientsas[i] = (char *)malloc(MAX_NAME_LEN + 1);
    }

    FD_ZERO(&static_rfds);
    FD_SET(srv_send_sock,&static_rfds);
    FD_SET(srv_recv_sock,&static_rfds);
    max_fd = (srv_send_sock > srv_recv_sock) ? srv_send_sock : srv_recv_sock;
    
    /* listen and read forever */
    while (1) {
	/* reset fdsets */
	memcpy(&rfds,&static_rfds,sizeof(static_rfds));

	retval = select(max_fd+1,&rfds,NULL,NULL,NULL);
	
	if (retval > 0) {
	    if (FD_ISSET(srv_send_sock,&rfds) 
		&& send_client_fd < 0) {
		struct sockaddr_in client_sin;
		socklen_t slen;
		
		slen = sizeof(client_sin);
		
		if ((send_client_fd = accept(srv_send_sock,
					     (struct sockaddr *)&client_sin,
					     &slen)) < 0) {
		    warn("accept failed");
		}
		
		if (send_client_fd > max_fd) {
		    max_fd = send_client_fd;
		}
		
		char *addr = inet_ntoa(client_sin.sin_addr);
		int addrlen = strlen(addr);
		
		if (debug > 1) {
		    fprintf(stdout,
			    "DEBUG: (send client!) connect from %s:%d\n",
			    addr,
			    ntohs(client_sin.sin_port));
		}

		/* add the new send client for consideration */
		FD_SET(send_client_fd,&static_rfds);

		/* zero the counter */
		current_send_rc = 0;
		block_count = 0;
	    }
	    else if (FD_ISSET(srv_recv_sock,&rfds)) {
		struct sockaddr_in client_sin;
		socklen_t slen;
		int client_fd;

		slen = sizeof(client_sin);

		if ((client_fd = accept(srv_recv_sock,
					(struct sockaddr *)&client_sin,
					&slen)) < 0) {
		    warn("accept failed");
		}
		else if (recv_client_cnt >= MIDDLEMAN_MAX_CLIENTS) {
		    warn("already at max clients");
		    close(client_fd);
		}
		else {
		    /* add new recv client... */
		    for (i = 0; i < MIDDLEMAN_MAX_CLIENTS; ++i) {
			if (recv_clientfds[i] == -1) {
			    break;
			}
		    }
		    
		    recv_clientfds[i] = client_fd;
		    if (client_fd > max_fd) {
			max_fd = client_fd;
		    }
		    
		    FD_SET(client_fd,&static_rfds);
		    
		    char *addr = inet_ntoa(client_sin.sin_addr);
		    int addrlen = strlen(addr);
		    
		    if (debug > 1) {
			fprintf(stdout,
				"DEBUG: connect from %s:%d\n",
				addr,
				ntohs(client_sin.sin_port));
		    }
		    
		    strncpy(recv_clientsas[i],
			    addr,
			    (addrlen > MAX_NAME_LEN)?MAX_NAME_LEN:addrlen);
		    /* null term if strncpy couldn't */
		    if (addrlen > MAX_NAME_LEN) {
			recv_clientsas[i][MAX_NAME_LEN] = '\0';
		    }
		    
		    ++recv_client_cnt;
		}
	    }
	    else if (send_client_fd > 0 
		     && FD_ISSET(send_client_fd,&rfds)) {
		/**
		 * ok, here we need to read a block from this guy for awhile,
		 * then [decrypt/munge]/encrypt (according to command line
		 * arg), then forward the block to all the recv clients.
		 */
		
		/**
		 * we don't fork or spin a thread for this, since none of 
		 * the clients will be writing and I don't have time to
		 * write any app-level buffering to process the sender's
		 * packets.  The sender should just have a fixed rate.  :-)
		 * We'll see how that works out.
		 */

		/**
		 * Of course, we put send_client_fd in nonblocking mode, so
		 * we read what we can when we can...
		 */

		//printf("going for another read\n");

		retval = read(send_client_fd,
			      &buf[current_send_rc],
			      block_size - current_send_rc);

		if (retval < 0) {
		    if (errno == ECONNRESET) {
			/* dead sender */
			close(send_client_fd);
			FD_CLR(send_client_fd,&static_rfds);
			send_client_fd = -1;

			current_send_rc = 0;
			block_count = 0;

			if (debug > 1) {
			    fprintf(stderr,
				    "DEBUG: (send client!) disconnect from %s\n",
				    recv_clientsas[i]);
			}
		    }
		    else if (errno == EAGAIN) {
			// ignore, doh
			//ewarn("while reading");
		    }
		    else {
			//ewarn("unexpected while reading");
		    }
		}
		else if (retval == 0) {
		    /* dead sender */
		    close(send_client_fd);
		    FD_CLR(send_client_fd,&static_rfds);
		    send_client_fd = -1;
		    
		    current_send_rc = 0;
		    block_count = 0;

		    if (debug > 1) {
			fprintf(stderr,
				"DEBUG: (send client!) disconnect!\n");
		    }
		}
		else {
		    current_send_rc += retval;
		}


		if (current_send_rc == block_size) {
		    current_send_rc = 0;

		    ++block_count;

		    /* wait to send ack until after crypto */

		    /* end of block, do the encryption op and note times... */
		    if (debug > 1) {
			fprintf(stderr,
				"DEBUG: read %d bytes (a block)\n",
				block_size);
		    }

		    DES_cblock *iv = sgeniv();
		    DES_cblock *k1 = sgenkey();
		    DES_cblock *k2 = sgenkey();
		    struct timeval t1,t2,t3;

		    if (only_encrypt) {
			gettimeofday(&t1,NULL);
			sencrypt(buf,outbuf,block_size,
				 k1,k2,iv);
			gettimeofday(&t2,NULL);
		    }
		    else {
			/* do a decrypt, then choose new iv, then encrypt. */
			gettimeofday(&t1,NULL);
			sdecrypt(buf,outbuf,block_size,
				 k1,k2,iv);
			iv = sgeniv();
			sencrypt(buf,outbuf,block_size,
				 k1,k2,iv);
			gettimeofday(&t2,NULL);
		    }

		    t3.tv_sec = t2.tv_sec - t1.tv_sec;
		    t3.tv_usec = t2.tv_usec - t1.tv_usec;
		    if (t3.tv_usec < 0) {
			t3.tv_sec--;
			t3.tv_usec += 1000000;
		    }
		    fprintf(stdout,
			    "BLOCKTIME(%s): %d %d at %.6f\n",
			    (only_encrypt)?"e":"de",
			    block_count,
			    t3.tv_sec * 1000 + t3.tv_usec,
			    t2.tv_sec + t2.tv_usec / 1000000.0f);
		    fflush(stdout);

		    /**
		     * send the ack after crypto ops...
		     */
		    if (pushback) {
			if ((retval = write(send_client_fd,&buf[0],sizeof(char))) < 0) {
			    ewarn("failed to send ack");
			}
		    }
		    
		    if (debug > 1) {
			fprintf(stderr,
				"DEBUG: sending to clients\n");
		    }

		    /* send to clients */
		    for (i = 0; i < MIDDLEMAN_MAX_CLIENTS; ++i) {
			if (recv_clientfds[i] > -1) {
			    retval = write(recv_clientfds[i],
					   outbuf,
					   block_size);
			    if (retval < 0) {
				ewarn("client write failed");
			    }
			    else if (retval != block_size) {
				fprintf(stderr,
					"ERROR: wrote only %d bytes to client %d!\n",
					retval,i);
			    }
			    else {
				if (debug > 1) {
				    fprintf(stderr,
					    "DEBUG: wrote all block_size %d bytes to client %d!\n",
					    block_size,i);
				}
			    }
			}
		    }

		}
				
	    }
	    else {
		/**
		 * If we get here, assume that one of the recv clients
		 * has died (cause they are not supposed to write anything
		 * to us)... so nuke the doofus!
		 */
		for (i = 0; i < MIDDLEMAN_MAX_CLIENTS; ++i) {
		    if (recv_clientfds[i] > -1) {
			if (FD_ISSET(recv_clientfds[i],&rfds)) {
			    /* read a block, or as much as possible */
			    //retval = read(recv_clientfds[i],buf,block_size);

			    /* dead client, pretty much */
			    //if (retval <= 0) {
				if (debug > 1) {
				    fprintf(stderr,
					    "DEBUG: disconnect from %s\n",
					    recv_clientsas[i]);
				}

				close(recv_clientfds[i]);
				FD_CLR(recv_clientfds[i],&static_rfds);
				recv_clientfds[i] = -1;

				--recv_client_cnt;
			    //}
			    //else if (debug > 2 ) {
				//fprintf(stdout,
				//"DEBUG: read %d bytes from %s\n",
				//retval,
				//recv_clientsas[i]);
			    //}
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
