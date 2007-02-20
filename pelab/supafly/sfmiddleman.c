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

int udp = 0;

/**
 * defs
 */
void usage(char *bin) {
    fprintf(stdout,
	    "USAGE: %s -scohSRudb  (option defaults in parens)\n"
	    "  -s <block_size>  tx block size (%d)\n"
	    "  -c <block_count> tx block size (%d)\n"
	    "  -m <hostname>    Listen on this host/addr (INADDR_ANY)\n"
	    "  -S <port>        Listen on this port for one sender at a time (%d)\n"
	    "  -R <port>        Listen on this port for receivers (%d)\n"
	    "  -o               Only encrypt (instead of dec/enc)\n"
	    "  -b               Ack all received blocks from sender AFTER crypto\n"
	    "  -U               Use udp instead of tcp\n"
	    "  -d[d..d]         Enable various levels of debug output\n"
	    "  -u               Print this msg\n",
	    bin,block_size,block_count,srv_send_port,srv_recv_port
	    );
}

void parse_args(int argc,char **argv) {
    int c;
    char *ep = NULL;

    while ((c = getopt(argc,argv,"s:c:S:R:m:oudbU")) != -1) {
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
	case 'm':
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
	case 'U':
	    udp = 1;
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
    struct sockaddr_in udp_recv_client_sins[MIDDLEMAN_MAX_CLIENTS];
    int udp_msg_size = 0;
    struct timeval t0;
    int bytesRead = 0;
    struct sockaddr_in udp_send_client_sin;
    struct timeval tvs,tv;

    /* grab some quick args */
    parse_args(argc,argv);

    /* initialize ourself... */
    srand(time(NULL));

    if (udp) {
	memset(udp_recv_client_sins,
	       0,
	       MIDDLEMAN_MAX_CLIENTS * sizeof(struct sockaddr_in));
    }

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
    if ((srv_send_sock = socket(PF_INET,(udp)?SOCK_DGRAM:SOCK_STREAM,0)) 
	== -1) {
	efatal("could not get send listen socket");
    }
    if ((srv_recv_sock = socket(PF_INET,(udp)?SOCK_DGRAM:SOCK_STREAM,0)) 
	== -1) {
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

    if (!udp) {
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
    
    if (debug > 1) {
	fprintf(stdout,"DEBUG: server running...\n");
    }

    /* listen and read forever */
    while (1) {
	/* reset fdsets */
	memcpy(&rfds,&static_rfds,sizeof(static_rfds));

	if (debug > 3) {
	    printf("DEBUG: calling select\n");
	}

	retval = select(max_fd+1,&rfds,NULL,NULL,NULL);
	
	if (retval > 0) {
	    /* 
	     * do we have input on port listening for senders, and not 
	     * currently have a sender? 
	     */
	    if (!udp && FD_ISSET(srv_send_sock,&rfds) 
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
	    /* add a sender udp client */
	    //else if (FD_ISSET(srv_send_sock,&rfds) && udp) {
		
	    //} 
	    /* add a new sfreceiver client */
	    /* OR pass through acks. */
	    else if (FD_ISSET(srv_recv_sock,&rfds)) {
		struct sockaddr_in client_sin;
		socklen_t slen;
		int client_fd;
		char tmpbuf[255];
		block_hdr_t ack;

		slen = sizeof(client_sin);

		if (udp) {
		    if ((bytesRead = recvfrom(srv_recv_sock,
					      tmpbuf,sizeof(tmpbuf),0,
					      (struct sockaddr *)&client_sin,
					      &slen)) < 0) {
			ewarn("in recvfrom while getting a new receiver "
			      "client");
		    }
		    else {
			/*
			 * if this is a client we haven't heard from,
			 * add it; else remove it.  Basic "connection."
			 */
			int first_zero_idx = -1;

			/* record ack time */
			gettimeofday(&tv,NULL);

			for (i = 0; i < MIDDLEMAN_MAX_CLIENTS; ++i) {
			    if (udp_recv_client_sins[i].sin_addr.s_addr == 0 
				&& first_zero_idx < 0) {
				first_zero_idx = i;
			    }
			    else if ((udp_recv_client_sins[i].sin_addr.s_addr 
				      == client_sin.sin_addr.s_addr) 
				     && (udp_recv_client_sins[i].sin_port
					 == client_sin.sin_port)) {
				/* old client, wants to "disconnect" */
				/* OR it's sending an ack... if the msg size
				 * is sizeof(hdr) instead of 1 byte, pass
				 * the ack on to the sender.
				 */

				if (debug > 2) {
				    fprintf(stdout,"read %d-byte msg from "
					    "recv client %s\n",
					    bytesRead,inet_ntoa(client_sin.sin_addr));
				}

				if (bytesRead == sizeof(block_hdr_t)) {
				    /* unmarshall for debug */
				    unmarshall_block_hdr(tmpbuf,&ack);

				    retval = sendto(srv_send_sock,
						    tmpbuf,bytesRead,0,
						    (struct sockaddr *)&udp_send_client_sin,
						    sizeof(struct sockaddr_in));
				    
				    if (retval < 0) {
					ewarn("while sending udp ack to client");
				    }
				    else if (retval != bytesRead) {
					ewarn("only sent part of udp ack msg");
				    }

				    fprintf(stdout,
					    "ACKTIME: recv m%d b%d f%d at %.6f\n",
					    ack.msg_id,
					    ack.block_id,
					    ack.frag_id,
					    tv.tv_sec+tv.tv_usec/1000000.0f);
				    fflush(stdout);
				    
				}
				else if (slen == 1) {
				    
				    if (debug > 1) {
					fprintf(stdout,
						"removed udp client %s\n",
						inet_ntoa(client_sin.sin_addr));
				    }
				    memset(&udp_recv_client_sins[i],
					   0,
					   sizeof(struct sockaddr_in));
				}

				break;
			    }
			}

			if (i == MIDDLEMAN_MAX_CLIENTS) {
			    /* new client */
			    if (first_zero_idx < 0) {
				ewarn("could not add new client, max reached");
			    }
			    else {
				memcpy(&udp_recv_client_sins[first_zero_idx],
				       &client_sin,
				       sizeof(struct sockaddr_in));

				if (debug > 1) {
				    fprintf(stdout,
					    "added new udp receiver client %s\n",
					    inet_ntoa(client_sin.sin_addr));
				}

			    }
			}
		    }
		}
		else {
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
	    }
	    else if ((send_client_fd > 0 
		      && FD_ISSET(send_client_fd,&rfds))
		     || (udp && FD_ISSET(srv_send_sock,&rfds))) {
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


		if (!udp) {
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
		}
		else {
		    /* 
		     * NOTE: we do not establish that there's only one
		     * sender!  We just forward msgs that come to us.
		     */
		    struct sockaddr_in client_sin;
		    socklen_t slen;
		    slen = sizeof(client_sin);

		    if (debug > 3) {
			fprintf(stdout,"DEBUG: calling recvfrom on sender\n");
		    }

		    retval = recvfrom(srv_send_sock,buf,block_size,0,
				      (struct sockaddr *)&client_sin,&slen);

		    /* keep the sender's sin around */
		    udp_send_client_sin = client_sin;

		    if (retval < 0) {
			ewarn("error in recvfrom on sending client");
			udp_msg_size = 0;
		    }
		    else {
			udp_msg_size = retval;

			if (debug > 1) {
			    fprintf(stderr,
				    "DEBUG: udp dgram\n",
				    udp_msg_size);
			}

		    }
		}

		bytesRead = (udp)?udp_msg_size:current_send_rc;
		
		/* grab recv timestamp */
		gettimeofday(&t0,NULL);

		if ((!udp && current_send_rc == block_size) 
		    || (udp && udp_msg_size > 0)) {
		    block_hdr_t hdr;

		    current_send_rc = 0;

		    /* unmarshall header */
		    unmarshall_block_hdr(buf,&hdr);

		    /* wait to send ack until after crypto */

		    /* end of block, do the encryption op and note times... */
		    if (debug > 1) {
			fprintf(stderr,
				"DEBUG: read %d bytes\n",
				bytesRead);
		    }

		    DES_cblock *iv = sgeniv();
		    DES_cblock *k1 = sgenkey();
		    DES_cblock *k2 = sgenkey();
		    struct timeval t1,t2,t3;

		    if (only_encrypt) {
			gettimeofday(&t1,NULL);
			sencrypt(buf,outbuf,bytesRead,
				 k1,k2,iv);
			gettimeofday(&t2,NULL);
		    }
		    else {
			/* do a decrypt, then choose new iv, then encrypt. */
			gettimeofday(&t1,NULL);
			sdecrypt(buf,outbuf,bytesRead,
				 k1,k2,iv);
			iv = sgeniv();
			sencrypt(buf,outbuf,bytesRead,
				 k1,k2,iv);
			gettimeofday(&t2,NULL);
		    }

		    t3.tv_sec = t2.tv_sec - t1.tv_sec;
		    t3.tv_usec = t2.tv_usec - t1.tv_usec;
		    if (t3.tv_usec < 0) {
			t3.tv_sec--;
			t3.tv_usec += 1000000;
		    }

		    if (!udp) {
			/* no app fragmentation with tcp */
			++block_count;
		    }
		    else {
			/* have to actually unmarshall the header to see
			 * which block and frag numbers we have.
			 */

		    }

		    /**
		     * send the ack after crypto ops...
		     */
		    if (!udp && pushback) {
			if ((retval = write(send_client_fd,
					    &buf[0],sizeof(char))) < 0) {
			    ewarn("failed to send ack");
			}
		    }
		    
		    if (debug > 1) {
			fprintf(stderr,
				"DEBUG: sending to clients\n");
		    }

		    /* before sending on, copy the original header and 
		     * note our timestamps... 
		     */
		    memcpy(outbuf,buf,sizeof(block_hdr_t));
		    /* and marshall bits of the hdr that matter to the 
		     * middleman 
		     */
		    marshall_block_hdr_mid(outbuf,&t0,&t2);
		    

		    /* send to clients */
		    if (!udp) {
			for (i = 0; i < MIDDLEMAN_MAX_CLIENTS; ++i) {
			    if (recv_clientfds[i] > -1) {
				retval = write(recv_clientfds[i],
					       outbuf,
					       bytesRead);
				if (retval < 0) {
				    ewarn("client write failed");
				}
				else if (retval != bytesRead) {
				    fprintf(stderr,
					    "ERROR: wrote only %d bytes to "
					    "client %d!\n",
					    retval,i);
				}
				else {
				    if (debug > 1) {
					fprintf(stderr,
						"DEBUG: wrote all "
						"%d bytes to client %d!\n",
						bytesRead,i);
				    }
				}
			    }
			}
		    }
		    else {
			for (i = 0; i < MIDDLEMAN_MAX_CLIENTS; ++i) {
			    if (udp_recv_client_sins[i].sin_addr.s_addr != 0) {
				if (debug > 1) {
				    fprintf(stdout,"sending %d-byte udp msg to"
					    " client %s\n",
					    bytesRead,
					    inet_ntoa(udp_recv_client_sins[i].sin_addr));
				}

				retval = sendto(srv_recv_sock,
						outbuf,bytesRead,0,
						(struct sockaddr *)&udp_recv_client_sins[i],
						sizeof(struct sockaddr_in));

				if (retval < 0) {
				    ewarn("while sending udp to client");
				}
				else if (retval != udp_msg_size) {
				    ewarn("only sent part of udp msg");
				}
			    }
			    else {
				if (debug > 3) {
				    fprintf(stdout,"no client in slot %d\n",i);
				}
			    }
			}
		    }

		    gettimeofday(&tvs,NULL);
		    
		    /* dump times */
		    fprintf(stdout,
			    "BLOCKTIME(%s): m%d b%d f%d %d; read=%.6f; compute=%.6f; send=%.6f\n",
			    (only_encrypt)?"e":"de",
			    hdr.msg_id,hdr.block_id,hdr.frag_id,
			    t3.tv_sec * 1000 + t3.tv_usec,
			    t0.tv_sec + t0.tv_usec / 1000000.0f,
			    t2.tv_sec + t2.tv_usec / 1000000.0f,
			    tvs.tv_sec + tvs.tv_usec / 1000000.0f);
		    fflush(stdout);
		}
				
	    }
	    else {
		if (!udp) {
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
	}
	else if (retval < 0) {
	    /* error... */
	    ewarn("error in select");
	}
    }

    return -1;

}
