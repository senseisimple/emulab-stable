/* 
 * Copyright (c) 2000 The University of Utah and the Flux Group.
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation.
 *
 * THE UNIVERSITY OF UTAH ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  THE UNIVERSITY OF UTAH DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * ---------------------------
 *
 * Filename: serv_listen.c
 *   -- Author: Kristin Wright <kwright@cs.utah.edu> 
 *
 * ---------------------------
 *
 * $Id: serv_listen.c,v 1.10 2001-08-02 21:21:05 ikumar Exp $
 */

#include <math.h>
#include "discvr.h"
#include "packet.h"
#include "util.h"

enum {DUP, DUP_NBOR, NEW};

/*
 * Is this used anymore?
 */
extern topd_inqid_t inqid_current;

/*
 * Each td server has a unique nodeID which is
 * simply the lowest MAC address on the machine.
 * The MAC address isn't necessarily *up*.
 * 
 * This global points to the MAC address that serves
 * as the nodeID. 
 */
u_char myNodeID[ETHADDRSIZ];
u_char sendingIF[ETHADDRSIZ];
u_char receivingIF[ETHADDRSIZ];

/*
 * Return 0 if the inquiry is not from us, 1 if it is.
 */
int
from_us(struct in_pktinfo *pktinfo, struct ifi_info *ifihead) 
{
        struct ifi_info *ifi;
        for( ifi = ifihead; ifi != NULL; ifi = ifi->ifi_next) {
	        if ( bcmp(&ifi->ifi_addr->sa_data, &pktinfo->ipi_addr.s_addr,
			  sizeof(pktinfo->ipi_addr.s_addr)) == 0 ) {
	                return 1;
		}
	}
	return 0;
}

int is_my_packet(struct sockaddr *pcliaddr, struct ifi_info *ifihead) 
{
	char *name1,*name2,*name_tmp;
	struct ifi_info *ifi;
        for( ifi = ifihead; ifi != NULL; ifi = ifi->ifi_next) {
		name_tmp=inet_ntoa(((struct sockaddr_in *)ifi->ifi_addr)->sin_addr);
		if((name1 = (char*)malloc(sizeof(char)*strlen(name_tmp)))==NULL)
			perror("is_my_packet: problem in malloc");
		strcpy(name1,name_tmp);
		name_tmp=inet_ntoa(((struct sockaddr_in *)pcliaddr)->sin_addr);
		if((name2 = (char*)malloc(sizeof(char)*strlen(name_tmp)))==NULL)
			perror("is_my_packet: problem in malloc");
		strcpy(name2,name_tmp);
		//printf("name1:\"%s\" name2:\"%s\"\n",name1,name2);
		if(strcmp(name1,name2)==0)
	                return 1;
	}
	return 0;
}

void
get_recvIFADDR(u_char* recvIF,char *name,struct ifi_info * ifihead)
{
	struct ifi_info * ifi=NULL;
	//u_char * ptrRecvIF = (u_char*)malloc(sizeof(u_char)*ETHADDRSIZ);
	for (ifi = ifihead; ifi != NULL; ifi = ifi->ifi_next) 
	{
		if(strcmp(name,ifi->ifi_name)==0) 
		{
			//Will do this copying of actual interface address once I know the conversion from 
			//MAC address to hostname -ik
			//memcpy(ptrRecvIF,&(ifi->ifi_haddr),ETHADDRSIZ);	
			memcpy(recvIF,&(ifi->ifi_haddr),ETHADDRSIZ);	
			
			// Below, I am just copying nodeID just to identify nodes easily.
			//memcpy(&receivingIF,&myNodeID,ETHADDRSIZ);	
			//return ptrRecvIF;
			return ;
		}
	}
	fprintf(stderr,"ERROR: Could not locate the name of receiving interface in i/f list\n");
	//return myNodeID;
	memcpy(recvIF,myNodeID,ETHADDRSIZ);
	return ;
}


/*
 * Return 0 if the two inquiries are identical, non-zero if not.
 */
int 
inqid_cmp(struct topd_inqid *tid1, struct topd_inqid *tid2)
{
	if((tid1->tdi_tv.tv_sec==tid2->tdi_tv.tv_sec) && (tid1->tdi_tv.tv_usec==tid2->tdi_tv.tv_usec)
		 && (strcmp(tid1->tdi_nodeID,tid2->tdi_nodeID)==0))
		return 1;
	return 0;
}


void
print_ifi_info(struct ifi_info *ifihead)
{
        struct ifi_info *ifi;
	struct sockaddr *sa;

	for (ifihead = ifi = get_ifi_info(AF_INET, 0); 
		 ifi != NULL; ifi = ifi->ifi_next) {
		printf("%s: <", ifi->ifi_name);
		if (ifi->ifi_flags & IFF_UP)			printf("UP ");
		if (ifi->ifi_flags & IFF_BROADCAST)		printf("BCAST ");
		if (ifi->ifi_flags & IFF_MULTICAST)		printf("MCAST ");
		if (ifi->ifi_flags & IFF_LOOPBACK)		printf("LOOP ");
		if (ifi->ifi_flags & IFF_POINTOPOINT)   printf("P2P ");
		printf(">\n");
		print_nodeID(ifi->ifi_haddr);

		if ( (sa = ifi->ifi_addr) != NULL)
			printf("  IP addr: %s\n", sock_ntop(sa, sa->sa_len));
		if ( (sa = ifi->ifi_brdaddr) != NULL)
			printf("  broadcast addr: %s\n", sock_ntop(sa, sa->sa_len));
		if ( (sa = ifi->ifi_dstaddr) != NULL)
			printf("  destination addr: %s\n", sock_ntop(sa, sa->sa_len));
	}
}	


void
serv_listen(int servSockFd, struct sockaddr *pcliaddr, socklen_t clilen)
{
	int				flags=0, class=NEW, i=0, ttl=0, factor=0, selectReturn=0 ;
	int 			sock_num=0, *sock_list = NULL, state=Q_LISTEN;
	const int		on = 1;
   	socklen_t		len;
	ssize_t			n;
	char			mesg[MAXLINE], str[INET6_ADDRSTRLEN], ifname[IFNAMSIZ];
	char            *reply;
	u_char          *myNodeIDtmp = 0;
	struct in_addr			in_zero;
	struct ifi_info         *ifi, *ifihead;
	struct in_pktinfo		pktinfo;
	//struct topd_inqnode     *inqn;
	struct topd_inqid 		*temp_inqid, *inqhead=NULL;

	fd_set readFd,tempReadFd;
	struct timeval timeLimit;
	struct timeval *ptrTimeLimit;
	struct topd_nborlist    *saveNbr;
	char	recvbuf[BUFSIZ];
	double elapsed=0.0,begin=0.0,end=0.0,deadline=0.0,start=0.0;
	struct sockaddr sender_addr;
	u_char senderIF[ETHADDRSIZ];
	u_char recverIF[ETHADDRSIZ];
	u_char *ptrTempRecvIF;
	/*
	struct sockaddr     name;
    int             namelen=sizeof(name);
	*/

	if (setsockopt(servSockFd, IPPROTO_IP, IP_RECVIF, &on, sizeof(on)) < 0) 
	{
		fprintf(stderr,"setsockopt of IP_RECVIF");
		return;
	}

	bzero(&in_zero, sizeof(struct in_addr)); // All 0 IPv4 address

	// Print the interfaces and my Node ID:
	
	//Get the list of interfaces using the function "get_ifi_info" into the
	//link list "ifi" and the max hardware address of all the i/fs is the Node ID
	for (ifihead = ifi = get_ifi_info(AF_INET, 0); ifi != NULL; ifi = ifi->ifi_next) 
	{
		if ( ifi->ifi_hlen > 0) 
		{
		        myNodeIDtmp = max_haddr(myNodeIDtmp, ifi->ifi_haddr);
		}
	}
	memcpy(&myNodeID, myNodeIDtmp, ETHADDRSIZ);
	//printf("My node id:");
	//print_nodeID(myNodeID);

	print_ifi_info(ifihead);
	free_ifi_info(ifihead);

	// Reset the set of Fds to be used in select
	FD_ZERO(&readFd);
	FD_ZERO(&tempReadFd);
	
	//timeout := 0
	timeLimit.tv_sec  = 0;
	timeLimit.tv_usec = 0;
	ptrTimeLimit = NULL;
	
	ifihead = get_ifi_info(AF_INET,0);

	FD_SET(servSockFd,&readFd);
	
	start = tod();
	state = Q_LISTEN;	/* Intially, just wait for query */			
	while(1)
	{
		tempReadFd = readFd;
		/*
		 * Not necessary to set begin here. I am setting it after
		 * calculating elapsed time.
		   if(state == QR_LISTEN)
			begin = tod();
		*/
    	//Do select untill timeout. When ptrTimeLimit is NULL then it blocks
		if((selectReturn = select(FD_SETSIZE, &tempReadFd, NULL, NULL,
								  ptrTimeLimit)) < 0)
		{
			perror("Select returned less than 0");
		}
		else if(selectReturn == 0)
		{
			printf("Select timed out\n");
			// Send Reply
			if( (reply=(char *)malloc(BUFSIZ)) == NULL) 
			{
		        fprintf(stderr, "Ran out of memory for reply mesg.\n");
				exit(1);
			}
			n = compose_reply(ifihead, reply, BUFSIZ, 1,sendingIF,receivingIF);
			printf("replying: ");
			print_tdreply(reply, n);
			printf("done!\n");
		
			printf("Sending response to: %s\n",sock_ntop(&sender_addr,sizeof(struct sockaddr)));
			sendto(servSockFd, reply, n, 0, &sender_addr, sizeof(struct sockaddr));

			free_ifi_info(ifihead);
			ifihead = get_ifi_info(AF_INET,0);
			timeLimit.tv_sec  = 0;
		    timeLimit.tv_usec = 0;
			ptrTimeLimit = NULL;
			printf("Listen for any query\n");

			// Clear the temp sockets returned by the "forward_request" function
			// from the readFd set
			for(i=0;i<sock_num;i++)
			{
				FD_CLR(sock_list[i], &readFd);
				close(sock_list[i]);
			}
			sock_num=0;
			free(sock_list);

			state = Q_LISTEN;
			free(inqhead);
			inqhead=NULL;
			
			continue;
		}

    	//If(Query)
		if(FD_ISSET(servSockFd,&tempReadFd))
		{
			len = clilen;
			flags = 0;
			n = recvfrom_flags(servSockFd, mesg, MAXLINE, &flags,
						   pcliaddr, &len, &pktinfo);
			printf("%d-byte datagram from %s", n, sock_ntop(pcliaddr, len));
			if (memcmp(&pktinfo.ipi_addr, &in_zero, sizeof(in_zero)) != 0)
				printf(", to %s", inet_ntop(AF_INET, &pktinfo.ipi_addr,str, sizeof(str)));
			if (pktinfo.ipi_ifindex > 0)
				printf(", recv i/f = %s",if_indextoname(pktinfo.ipi_ifindex, ifname));
			if (flags & MSG_TRUNC)	printf(" (datagram truncated)");
			if (flags & MSG_CTRUNC)	printf(" (control info truncated)");
			#ifdef	MSG_BCAST
			if (flags & MSG_BCAST)	printf(" (broadcast)");
			#endif
			printf("\n");

			//Print the received inquiry
			print_tdinq(mesg);
			if(inqhead==NULL) class = NEW;
			else if (inqid_cmp(inqhead, (struct topd_inqid *)mesg))
			{
				if(is_my_packet(pcliaddr, ifihead)) 
				{
					printf("Duplicate inquiry from me\n");
					class = DUP;
				} 
				else 
				{
					printf("Duplicate inquiry from others\n");
					class = DUP_NBOR;
				}
			} 
			else
			{
				printf("Error: Stray packet... earlier enquiry may be.\n");
			}
			if(class == NEW)
				printf("Received a new query packet!\n");

			//ptrTempRecvIF = 
			get_recvIFADDR(recverIF,if_indextoname(pktinfo.ipi_ifindex, ifname),ifihead);
			printf("The query receiving interface is:==>\n");
			print_haddr(recverIF,ETHADDRSIZ);
			/*
			print_haddr(ptrTempRecvIF,ETHADDRSIZ);
			memcpy(&recverIF,ptrTempRecvIF,ETHADDRSIZ);
			free(ptrTempRecvIF);
			*/
			memcpy(&inqid_current, mesg, sizeof(topd_inqid_t));
			temp_inqid = (struct topd_inqid *)mesg;
			memcpy(&senderIF, &(temp_inqid->tdi_p_nodeIF), ETHADDRSIZ);
			ttl = ntohs(temp_inqid->tdi_ttl);
			factor = ntohs(temp_inqid->tdi_factor);
			
			if(ttl==0)
                printf("The TTL is zero so i will not forward!!\n");
			
			switch(class)
			{
				case DUP: break;
				case DUP_NBOR: 
					if((reply=(char *)malloc(BUFSIZ)) == NULL) 
					{
						fprintf(stderr, "Ran out of memory for reply mesg.\n");
						exit(1);
					}
					n = compose_reply(ifihead, reply, BUFSIZ, 0,senderIF,recverIF);
					sendto(servSockFd, reply, n, 0, pcliaddr, sizeof(struct sockaddr));
					printf("replying: ");
					print_tdreply(reply, n);
					printf("done!\n");
					break;
				case NEW:
					if(state == QR_LISTEN)
					{
						printf("Received another new query, cannt handle it\n");
						break;
					}
					if(ttl!=0)
					{
						/*
						struct topd_inqnode *save = inqhead;
						inqhead = (struct topd_inqnode *)malloc(sizeof(struct topd_inqnode));
                		inqhead->inqn_inq = (struct topd_inqid *)malloc(sizeof(struct topd_inqid));
						if (inqhead == NULL || inqhead->inqn_inq == NULL ) 
						{
							fprintf(stderr, "Ran out of room while expanding inquiry list.\n");
							exit(1);
						}
						memcpy(inqhead->inqn_inq, mesg, sizeof(struct topd_inqid));
						inqhead->inqn_next = save;
						*/
						inqhead = (struct topd_inqid *)malloc(sizeof(struct topd_inqid));
						if(inqhead == NULL)
						{
							fprintf(stderr, "Ran out of room while copying
									inquiry.\n");
							exit(1);
						}
						memcpy(inqhead, mesg, sizeof(struct topd_inqid));
						sock_list=forward_request(ifihead, &pktinfo, mesg, n, &sock_num );
						// Do FD_SET on all these opened sockets
						for(i=0;i<sock_num;i++)
						{
							FD_SET(sock_list[i],&readFd);
						}
						timeLimit.tv_sec = ttl * factor;
						timeLimit.tv_usec = 0;
						ptrTimeLimit = &timeLimit;
						printf("Setting select timer to: %d\n",ttl * factor);
						deadline = timeLimit.tv_sec + (timeLimit.tv_usec * 1e-6);
						state = QR_LISTEN;
						sender_addr = *pcliaddr;
						memcpy(&sendingIF,&senderIF,ETHADDRSIZ);
						memcpy(&receivingIF,&recverIF,ETHADDRSIZ);
						begin = tod();
					}
					break;
				default:
					break;
			}
			
		}

		for(i=0;i<sock_num;i++)
		{
			/*
			if(getsockname(sock_list[i], (struct sockaddr *)&name, &namelen)<0)
    		{
        		perror("getsockname\n");
    		}
    		printf("I am listening on: \"%s\" num:%d\n",sock_ntop(&name,name.sa_len),sock_num);
			*/
			if (FD_ISSET(sock_list[i], &tempReadFd)) 
			{
				n = recvfrom(sock_list[i], recvbuf, BUFSIZ, 0, NULL, NULL);
				printf ("received:==>\n");
				print_tdreply(recvbuf,n);

				if((ifi=get_ifi_struct(sock_list[i],ifihead)) == NULL)
				{
					fprintf(stderr,"Socket \"%d\" is not in any ifi struct\n",sock_list[i]);
				}

				saveNbr = ifi->ifi_nbors;
				ifi->ifi_nbors = (struct topd_nborlist *)malloc(sizeof(struct topd_nborlist)); 
				if ( ifi->ifi_nbors == NULL ) 
				{
					perror("Not enough memory for neighbor list.");
					exit(1);
				}
				// skip inquiry ID
				ifi->ifi_nbors->tdnbl_nbors = (u_char *)malloc(n-sizeof(topd_inqid_t));
				if ( ifi->ifi_nbors->tdnbl_nbors == NULL ) 
				{
					perror("Not enough memory for neighbor list.");
					exit(1);
				}
				memcpy((void *)ifi->ifi_nbors->tdnbl_nbors, recvbuf +
				   sizeof(topd_inqid_t), (n-sizeof(topd_inqid_t)));

				// The "tdnbl_n" contains the number of neighbors that the node, along this
				// interface, has
				ifi->ifi_nbors->tdnbl_n = (n-sizeof(topd_inqid_t)) / sizeof(struct topd_nbor);
				ifi->ifi_nbors->tdnbl_next = saveNbr;
			}
		}
				
		if(state == QR_LISTEN)	/* Waiting for Querry as well as Replies */
		{
			end=tod();
			/* Since I have come out of the select so lets calculate how much time
			 * I spent listening for query replies. If its greater than the
			 * deadline I have then i will have to send reply otherwise keep
			 * waiting for the reply by setting new deadline
			 */

			elapsed = end - begin;
			if(elapsed >= deadline)
			{
				/* This i am doing so that select times out and and i send a
				 * reply from there
				 */
				printf("Select crossed deadline. Set timer to 0 sec\n");
				timeLimit.tv_sec = 0;
				timeLimit.tv_usec = 0;
				ptrTimeLimit = &timeLimit;
			}
			else if(elapsed < deadline)
			{
				deadline -=elapsed;
				begin=end;
				timeLimit.tv_sec = floor(deadline);
				timeLimit.tv_usec = (deadline - timeLimit.tv_sec) * 1e+6;
				ptrTimeLimit = &timeLimit;
				printf("Deadline still not met. Set timer to %ld\n", timeLimit.tv_sec);
			}
		}
			
	}
}
