/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * This code is derived from the proxydhcp server downloaded from:
 * http://www.bpbatch.org/downloads/proxydhcp.tar.gz
 * That code has no copyright notice, but the README says:
 *
 *    PXE Proxy DHCP Server
 *
 *    Written by David Clerc & Marc Vuilleumier Stuckelberg,
 *    CUI, University of Geneva, June 1998
 *    http://cuiwww.unige.ch/info/pc/remote-boot/
 *
 * Thanks guys!
 */

#undef DEBUG_EC

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#if defined(SOLARIS) || defined(HAVE_SOCKADDR_SA_LEN)
#include <sys/sockio.h>
#else
#ifdef REDHAT5
#include <ioctls.h> 
#endif
#endif
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef USE_RECVMSG
#include <sys/uio.h>
#include <sys/param.h>
#endif
#ifdef EVENTSYS
/*#include "tbdefs.h" Do we need this?*/
#include "event.h"
#endif

#define BOOTPS_PORT 67
#define BOOTPC_PORT 68
#define BINL_PORT 4011
#define RFC1048_MAGIC 0x63538263L
#define BOOTP_REQUEST 1
#define BOOTP_REPLY 2
#define DHCPDISCOVER 1
#define DHCPOFFER 2
#define DHCPREQUEST 3
#define DHCPACK 5

#define uchar unsigned char

#ifdef HAVE_SOCKADDR_SA_LEN
#define max(x, y) ((x) > (y) ? (x) : (y))
#endif

typedef struct {
        uchar   op,                     /* operation code */
                htype,                  /* hardware type */
                hlen,                   /* length of hardware address */
                hops;                   /* gateways hops */
        int     xid;                    /* transaction identification */
        short   secs,                   /* seconds elapsed since boot began */
		flags;			/* flags */
        uchar   cip[4],                 /* client IP address */
                yip[4],                 /* your IP address */
                sip[4],                 /* server IP address */
                gip[4];                 /* gateway IP address */
        uchar   hwaddr[16];             /* client hardware address */
	char	sname[64],		/* server name */
		bootfile[128];		/* bootfile name */
	union {
                uchar   data[1500];     /* vendor-specific stuff */
                int     magic;          /* magic number */
	} vendor;
} BOOTPH;

BOOTPH dhcppak;

/*
 * DB interface
 */
int	open_db(void);
int	query_db(struct in_addr, struct in_addr *, char *, int);
int	close_db(void);

int	binlmode = 0;
int	debug = 0;

/*
 * Event System interface
 */
#ifdef EVENTSYS
int			myevent_send(address_tuple_t address);
static event_handle_t	event_handle = NULL;
address_tuple_t         tuple;
int                     ip2nodeid(struct in_addr, char *, int);
char                    nodeid[16];

/* Cache to avoid duplicate BOOTING events */
#define EVENTCACHESIZE 128
typedef struct {
    int in_addr;
    int  lastmsg;
} ec_elt;

ec_elt evcache[EVENTCACHESIZE];
ec_elt ec_tmp;
ec_elt ec_new;

/* front/back of array - hard bounds */
ec_elt* ec_front;
ec_elt* ec_back;
/* head/tail of list - move around as list changes */
ec_elt* ec_head;
ec_elt* ec_tail;
ec_elt* ec_ptr;

/*
 * Minimum interval between sending events for the same node, in
 * seconds.  Should be less than the shortest possible valid time
 * between BOOTING events. For a pc600, that is just under 60 seconds,
 * and for pc850s, it is more.
 */
int min_interval = 60; 

/* Something to hold the time... */
struct timeval now;
int rv;

/* Some support functions for the event cache */
ec_elt* ec_next(ec_elt* ptr) {
    if (ptr < ec_front || ptr > ec_back) { return 0; }
    ptr += 1;
    if (ptr==ec_back) { ptr = ec_front; }
    return ptr;
}

ec_elt* ec_prev(ec_elt* ptr) {
    if (ptr < ec_front || ptr > ec_back) { return 0; }
    if (ptr==ec_front) { ptr = ec_back; }
    ptr -= 1;
    return ptr;
}

ec_elt* ec_find(uint addr) {
    ec_ptr=ec_head;
    while (ec_ptr != ec_tail) {
	if (ec_ptr->in_addr == addr) {
	    return ec_ptr;
	} else { ec_ptr = ec_next(ec_ptr); }
    }
    return 0;
}

ec_elt* ec_insert(ec_elt elt) {
#ifdef DEBUG_EC
    syslog(LOG_DEBUG, "head=%d\ttail=%d",ec_head,ec_tail);
#endif
    if (ec_next(ec_tail)==ec_head) {
	/* cache is full, make a victim */
	ec_tail = ec_prev(ec_tail);
	ec_head = ec_prev(ec_head);
    } else {
	/* still have room, don't victimize anything */
	ec_head = ec_prev(ec_head);
    }
#ifdef DEBUG_EC
    syslog(LOG_DEBUG, "head=%d\ttail=%d",ec_head,ec_tail);
#endif
    ec_head->in_addr = elt.in_addr;
    ec_head->lastmsg = elt.lastmsg;
    return ec_head;
}

ec_elt* ec_reinsert(ec_elt* elt) {
    if (elt < ec_front || elt > ec_back) { return 0; }
    ec_tmp.in_addr = elt->in_addr;
    ec_tmp.lastmsg = elt->lastmsg;
    ec_tail = ec_prev(ec_tail);
    while (elt!=ec_tail) {
	ec_ptr = elt;
	elt = ec_next(elt);
	ec_ptr->in_addr = elt->in_addr;
	ec_ptr->lastmsg = elt->lastmsg;
    }
    return ec_insert(ec_tmp);
}

#ifdef DEBUG_EC
void ec_print() {
    int count = 0;
    ec_ptr = ec_head;
    syslog(LOG_DEBUG, "Event cache:");
    while (ec_ptr != ec_tail) {
	syslog(LOG_DEBUG, "%d:\t%u\t%d",count,ec_ptr->in_addr,ec_ptr->lastmsg);
	ec_ptr = ec_next(ec_ptr);
	count++;
    }
    syslog(LOG_DEBUG, "---");
}
#endif

/*
 * The big kahuna. If it's already there, update its time and reinsert
 * it, and return 1 if it is recent enough, or return 0 if it is too
 * old. If it isn't there, insert it and return 0.
 */
int ec_check(uint addr) {
    int newtime,oldtime;
    rv = gettimeofday(&now,0);
    newtime = now.tv_sec;
#ifdef DEBUG_EC
    syslog(LOG_DEBUG, "Checking %u at %d",addr,newtime);
    ec_print();
#endif
    ec_ptr = ec_find(addr);
#ifdef DEBUG_EC
    syslog(LOG_DEBUG, "find returned %d",ec_ptr);
#endif
    if (ec_ptr) {
	/* Found it. Update time, reinsert. */
	oldtime = ec_ptr->lastmsg;
	ec_ptr->lastmsg = newtime;
	ec_reinsert(ec_ptr);
#ifdef DEBUG_EC
	ec_print();
	syslog(LOG_DEBUG, "Time difference: %d (limit=%d)",
	       newtime-oldtime,min_interval);
#endif
	if (oldtime + min_interval < newtime) { return 0; } else { return 1; }
    } else {
	/* Not found. Insert it and return 0. */
	ec_new.in_addr = addr;
	ec_new.lastmsg = newtime;
	ec_insert(ec_new);
#ifdef DEBUG_EC
	syslog(LOG_DEBUG, "Inserted %u,%d",addr,newtime);
	ec_print();
#endif
	return 0;
    }
}
	     
#endif

/*
 * Fall back to default
 */
#ifdef FALLBACK_HOST
#define DEFAULT_HOST	FALLBACK_HOST
#define DEFAULT_PATH	"/tftpboot/pxeboot"
struct in_addr		default_sip;
#endif

int gettag(uchar *p, uchar tag, uchar *data, uchar *pend) {
	uchar t;
	t=*p;
	while (t!=0xFF && t!=tag) {
		if (p > pend) return 0;
		p+=*(p+1)+2;
		t=*p;
	}
	if (t==0xFF) return 0;
	memcpy(data,p+2,*(p+1));
	return 1;
}
			
uchar *inserttag(uchar *p, uchar tag, uchar len, uchar *data) {
	*p++ = tag;
	*p++ = len;
	while (len--) *p++=*data++;
	return p;
}

int findiface(struct sockaddr_in *client) {
	int i,sock;
	struct ifconf ic;
	char buf[8192];

	sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	ic.ifc_len= sizeof(buf);
	ic.ifc_ifcu.ifcu_buf = (caddr_t)buf;
	if (ioctl(sock,SIOCGIFCONF, &ic)<0) {
		perror("ioctl()");
		exit(1);
	}
	for (i=0;i<ic.ifc_len;) {
		struct in_addr ifaddr;
		struct sockaddr_in *iftemp;
		struct in_addr t;
		struct ifreq *ifr = (struct ifreq *)((caddr_t)ic.ifc_req+i);
#ifdef HAVE_SOCKADDR_SA_LEN
		i += max(sizeof(struct sockaddr), ifr->ifr_addr.sa_len) +
			sizeof(ifr->ifr_name);
#else		
		i += sizeof(*ifr);
		ifr++;
#endif

#if 0
		syslog(LOG_DEBUG, "IF: %s",ifr->ifr_name);
#endif
		if (ifr->ifr_addr.sa_family==AF_INET) {
			u_long network, clientnet, netmask;
			iftemp = (struct sockaddr_in *)&ifr->ifr_addr;
			memcpy(&ifaddr,&iftemp->sin_addr,
				 sizeof(struct in_addr));
			ioctl(sock,SIOCGIFNETMASK,ifr);
			netmask = iftemp->sin_addr.s_addr;
			network = ifaddr.s_addr&netmask;
			clientnet = client->sin_addr.s_addr&netmask; 
			return ifaddr.s_addr;
		}
	}
	close(sock);
	return 0;
}

int
main(int argc, char *argv[]) 
{
	int servsock;
	struct sockaddr_in server, client;
	struct in_addr sip;
	int clientlength;
	int i, argn=1;
	char *p, bootprog[256], ifname[IFNAMSIZ+1];
	char arguments[256];

	memset(ifname,0,sizeof(ifname));
	
	if (argc == 2 && argv[1][0]=='-')
	{
		/* Bind to a specific interface */
#ifdef SOLARIS
		printf("The -ifname option is not supported on Solaris\n");
		exit(1);
#endif
		p=argv[1]+1;
		strncpy(ifname,p,IFNAMSIZ);
		argn=2;
	}
	
	openlog("proxydhcp", LOG_PID, LOG_USER);
	syslog(LOG_NOTICE,
	       "Minimal PXE Proxy DHCP daemon, for use with BpBatch");
	syslog(LOG_NOTICE, "By David Clerc & Marc Vuilleumier Stuckelberg"
	       ", CUI, University of Geneva");
	syslog(LOG_NOTICE, "with mods for Emulab.net");

	if (open_db()) {
		syslog(LOG_ERR, "Error opening database");
		exit(1);
	}
	
	if (!debug)
		(void)daemon(0, 0);

	servsock=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if (servsock<=0) {
		perror("socket"); exit(1);
	}
	{
	  int optval = 1;
	  if (setsockopt(servsock,SOL_SOCKET,SO_REUSEADDR,(void *)&optval,
		sizeof(optval))<0)
		perror("setsockopt(SO_REUSEADDR)");
	}
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=htonl(INADDR_ANY);
	server.sin_port=htons(BOOTPS_PORT);
	if (bind(servsock,(struct sockaddr *)&server,sizeof(server))<0) {
		syslog(LOG_WARNING, "BOOTP/DHCP port in use, trying BINL mode");
		syslog(LOG_WARNING, "(do not forget to set tag 60 to PXEClient"
			" on your DHCP server)");
		binlmode = 1;
		server.sin_port=htons(BINL_PORT);
		if(bind(servsock,(struct sockaddr *)&server,sizeof(server))<0) {
			syslog(LOG_ERR, "Cannot bind to any port !");
			perror("bind"); exit(1);
		}
	}
#ifndef SOLARIS
	if ((*ifname)&&(setsockopt(servsock,SOL_SOCKET,SO_BINDTODEVICE,&ifname,
			strlen(ifname))<0))
		perror("setsockopt(SO_BINDTODEVICE)");
#endif
#ifdef FALLBACK_HOST
	{
		struct hostent	*he;

		if ((he = gethostbyname(DEFAULT_HOST)) == 0) {
			syslog(LOG_ERR, "Cannot map %s!", DEFAULT_HOST);
			exit(1);
		}
		memcpy((char *)&default_sip, he->h_addr, sizeof(default_sip));
	}
#endif
#ifdef EVENTSYS
	ec_front=&(evcache[0]);
	ec_back=&(evcache[EVENTCACHESIZE]);
	ec_head=ec_front;
	ec_tail=ec_front;
#ifdef DEBUG_EC
	syslog(LOG_DEBUG, "event_cache_front: %u\nevent_cache_back: %u\nhead:      %u\nhead-next: %u\nhead-prev: %u",
	       ec_front, ec_back, ec_head, ec_next(ec_head),
	       ec_prev(ec_head));
#endif
#endif
		
	syslog(LOG_NOTICE, "Server started on port %d", ntohs(server.sin_port));
	while (1) {
		int rb;
		uchar data[128];
		uchar *pend = NULL;
		uchar *p = dhcppak.vendor.data+4;
#ifdef USE_RECVMSG
		int on = 1;
		struct msghdr msg;
		struct iovec iov[1];

		struct cmsghdr *cmptr;
		struct cmessage {
			struct cmsghdr cmsg;
			struct in_addr ina;
		} cm;
#endif		
		struct config	*configp;
		
		clientlength = sizeof(client);
#ifdef USE_RECVMSG
#ifdef IP_RECVDSTADDR
		if (setsockopt(servsock, IPPROTO_IP, IP_RECVDSTADDR, &on,
			       sizeof(on)) < 0)
#endif			
			syslog(LOG_WARNING, "can't set IP_RECVDSTADDR");

		/* Set up msg struct. */
		msg.msg_control = (caddr_t)&cm;
		msg.msg_controllen = sizeof(struct cmessage);
		msg.msg_flags = 0;
		
		msg.msg_name = (caddr_t)&client;
		msg.msg_namelen = clientlength;
		iov[0].iov_base = (char *) &dhcppak;
		iov[0].iov_len = sizeof(dhcppak);
		msg.msg_iov = iov;
		msg.msg_iovlen = 1;

		rb = recvmsg(servsock, &msg, 0);
#else	
		rb = recvfrom(servsock,(char *)&dhcppak,sizeof(dhcppak),0,
			(struct sockaddr *)&client,&clientlength);
#endif		

		pend = (uchar *)&dhcppak+rb-1; /*pointer to last valid byte*/

		syslog(LOG_INFO, "Request from %s", inet_ntoa(client.sin_addr));

		if (dhcppak.op != BOOTP_REQUEST) {
			syslog(LOG_INFO, "  not a BOOTP request");
			continue;
		}
		if (p > pend) {
			syslog(LOG_INFO, "  truncated message");
			continue;
		}
		if (!gettag(p,53,data,pend)) {
			syslog(LOG_INFO, "  not a DHCP packet");
			continue;
		}
		if (!binlmode && data[0] != DHCPDISCOVER) {
			continue;
		}
		if (binlmode && data[0] != DHCPREQUEST) {
			continue;
		}
		if (!gettag(p,60,data,pend)) {
			continue;
		}
		if (strncmp((char *)data,"PXEClient",9)) {
			continue;
		}

#ifdef EVENTSYS

		/*
		 * By this point, we know it is a DHCP packet that we
		 * need to respond to, so the node must be booting.
		 * So we send out a change to state BOOTING if we
		 * haven't sent one recently.
		 */
#ifdef DEBUG_EC
		syslog(LOG_DEBUG, "Searching for %s in cache",
		       inet_ntoa(*((struct in_addr*)&client.sin_addr)));
#endif
		if (!ec_check((int)(client.sin_addr.s_addr))) {
		    tuple = address_tuple_alloc();
		    if (tuple != NULL &&
			!ip2nodeid(client.sin_addr, nodeid, sizeof(nodeid))) {
			
		        tuple->host      = BOSSNODE;
			tuple->objtype   = "TBNODESTATE";
			tuple->objname   = nodeid;
			tuple->eventtype = "BOOTING";
			
			if (myevent_send(tuple)) {
			    syslog(LOG_WARNING,
				   "Couldn't send state event for %s",nodeid);
			}
#ifdef DEBUG_EC
			else {
			    syslog(LOG_DEBUG,
				   "Sent sent state event for %s",nodeid);
			}
#endif
			address_tuple_free(tuple);
		    } else {
		        syslog(LOG_WARNING, "Couldn't lookup nodeid for %s",
			       inet_ntoa(*((struct in_addr*)&client.sin_addr)));
		    }
		} else {
		    syslog(LOG_INFO, "Quenching duplicate event for %s",
			   inet_ntoa(*((struct in_addr*)&client.sin_addr)));
		}
#endif /* EVENTSYS */
		
		if (query_db(client.sin_addr,
			     &sip, bootprog, sizeof(bootprog))) {
#ifdef FALLBACK_HOST
			syslog(LOG_WARNING, "No record for %s in DB!",
			       "  Using default.",
			       inet_ntoa(*((struct in_addr*)&client.sin_addr)));
			sip = default_sip;
			strcpy(bootprog, DEFAULT_PATH);
#else
			syslog(LOG_WARNING, "No record for %s in DB!",
			       inet_ntoa(*((struct in_addr*)&client.sin_addr)));
			continue;
#endif
		}
		
		dhcppak.op=BOOTP_REPLY;
		*((int *)dhcppak.sip)=sip.s_addr;
		*((int *)dhcppak.gip)=0;
		strcpy(dhcppak.bootfile, bootprog);

		/* Add DHCP message type option (53) */
		data[0]=(binlmode ? DHCPACK : DHCPOFFER);
		p=inserttag(p,53,1,data);

		/* Add server identifer option (54) */
#ifdef USE_RECVMSG
		if (msg.msg_controllen < sizeof(struct cmsghdr) ||
		    (msg.msg_flags & MSG_CTRUNC))
			syslog(LOG_WARNING,
			       "Aaargh!  No auxiliary information.");

		for (cmptr = CMSG_FIRSTHDR(&msg); cmptr != NULL;
		     cmptr = CMSG_NXTHDR(&msg, cmptr)) {

			if (cmptr->cmsg_level == IPPROTO_IP &&
			    cmptr->cmsg_type == IP_RECVDSTADDR) {
				*(int *)data=(*((struct in_addr*)CMSG_DATA(cmptr))).s_addr;
				if (debug)
					syslog(LOG_INFO,
					       "message directed to %s",
					       inet_ntoa(*((struct in_addr*)data)));
			}
		}
#else
		*(int *)data=findiface(&client);
#endif
		p=inserttag(p,54,sizeof(int),data);

		strcpy((char *)data,"PXEClient");
		p=inserttag(p,60,strlen((char *)data),data);

		/* Add vendor specific information option (43) */
		data[0] = 0xFF;
		p=inserttag(p,43,1,data);

		data[0] = 0;
		memcpy(data+1, dhcppak.hwaddr, 16);
		p=inserttag(p,97,17,data);
		if(*arguments)
			p=inserttag(p,155,strlen(arguments),(uchar *)arguments);
		*p=0xFF;
		{ uchar *q = (uchar *)&dhcppak;
		  rb = p-q+1;
		}
		client.sin_family = AF_INET;
		client.sin_port = htons(BOOTPC_PORT);
		if(!binlmode) {
		  int optval = 1;  
		  client.sin_addr.s_addr=htonl(INADDR_BROADCAST);
	  	  if (setsockopt(servsock,SOL_SOCKET,SO_BROADCAST,
			(void *)&optval,sizeof(optval))<0) perror("setsockopt");
		}

		syslog(LOG_INFO, "Reply to %s, bootfile=%s",
		       inet_ntoa(client.sin_addr), bootprog);

		if (sendto(servsock,(char *)&dhcppak,rb,0,
			(struct sockaddr *)&client,sizeof(client))<0)
			perror("sendto");
		if(!binlmode) {
		  int optval = 0;
	  	  setsockopt(servsock,SOL_SOCKET,SO_BROADCAST,(void *)&optval,
			sizeof(optval));
		}
	}
}

#ifdef EVENTSYS
/*
 * Connect to the event system. It's not an error to call this function if
 * already connected. Returns 1 on failure, 0 on sucess.
 */
int
event_connect()
{
	if (!event_handle) {
		event_handle = event_register("elvin://" BOSSNODE,0);
	}

	if (event_handle) {
		return 0;
	} else {
		syslog(LOG_WARNING,
		       "event_connect: Unable to register with event system!");
		return 1;
	}
}

/*
 * Send an event to the event system. Automatically connects (registers)
 * if not already done. Returns 0 on sucess, 1 on failure.
 */
int myevent_send(address_tuple_t tuple) {
	event_notification_t notification;

	if (event_connect()) {
		return 1;
	}

	notification = event_notification_alloc(event_handle,tuple);
	if (notification == NULL) {
		syslog(LOG_WARNING,
		       "myevent_send: Unable to allocate notification!");
		return 1;
	}

	if (event_notify(event_handle, notification) == NULL) {
		event_notification_free(event_handle, notification);

		syslog(LOG_WARNING,
		       "myevent_send: Unable to send notification!");
		/*
		 * Let's try to disconnect from the event system, so that
		 * we'll reconnect next time around.
		 */
		if (!event_unregister(event_handle)) {
			syslog(LOG_WARNING,
			       "myevent_send: Unable to unregister from event system!");
		}
		event_handle = NULL;
		return 1;
	} else {
		event_notification_free(event_handle,notification);
		return 0;
	}
}
#endif /* EVENTSYS */

/* syslog hacks follow */
void
perror(const char *str)
{
	char errbuf[128];

	snprintf(errbuf, sizeof errbuf, "%s: %s", str, strerror(errno));
	syslog(LOG_ERR, "%s", errbuf);

	if (debug)
		fprintf(stderr, "%s\n", errbuf);
}
