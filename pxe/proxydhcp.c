#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
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
 * Trivial config file format.
 */
struct config {
	struct in_addr	cip;
	struct in_addr	sip;
	char		bootprog[256];
};
#define MAX_CONFIGS	256
struct config		*configurations[MAX_CONFIGS];
struct config		*find_config(struct in_addr *addr);
int			parse_configs(char *filename);

int	binlmode = 0;

int parsehost(char *hostname) {
	struct hostent *he;
	if (!isdigit(hostname[0])) {
		he = gethostbyname(hostname);
		if (he == NULL) {
			fprintf(stderr,"Unknown host %s\n",hostname);
			exit(1);
		}
		return *(int *)he->h_addr;
	} else
		return inet_addr(hostname);
}

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

		printf("IF: %s",ifr->ifr_name);
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

void main(int argc, char *argv[]) 
{
	int servsock;
	struct sockaddr_in server, client;
	int clientlength;
	int i, sip, gip, argn=1;
	char *p, filename[128], ifname[IFNAMSIZ+1];
	char arguments[256];

	if(argc < 2) {
		fprintf(stderr, "usage : "
			"proxydhcp [-ifname] config_filename\n");
		exit(1);
	}
	memset(ifname,0,sizeof(ifname));
	if (argv[1][0]=='-')
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
	
	gip = 0;
	if (parse_configs(argv[argn])) {
		fprintf(stderr, "Error on config file\n");
		exit(1);
	}
	
	printf("Minimal PXE Proxy DHCP daemon, for use with BpBatch\n");
	printf("By David Clerc & Marc Vuilleumier Stuckelberg"
		", CUI, University of Geneva\n");

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
		printf("BOOTP/DHCP port in use, trying BINL mode\n");
		printf("(do not forget to set tag 60 to PXEClient"
			" on your DHCP server)\n");
		binlmode = 1;
		server.sin_port=htons(BINL_PORT);
		if(bind(servsock,(struct sockaddr *)&server,sizeof(server))<0) {
			fprintf(stderr, "Cannot bind to any port !\n");
			perror("bind"); exit(1);
		}
	}
#ifndef SOLARIS
	if ((*ifname)&&(setsockopt(servsock,SOL_SOCKET,SO_BINDTODEVICE,&ifname,
			strlen(ifname))<0))
		perror("setsockopt(SO_BINDTODEVICE)");
#endif
	
	printf("Server started on port %d\n\n", ntohs(server.sin_port));
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
			printf("can't set IP_RECVDSTADDR\n");

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

		printf("Datagram of %d bytes received from %s\n",
			rb,inet_ntoa(client.sin_addr));

		if (dhcppak.op != BOOTP_REQUEST) {
			printf("Not a BOOTP request\n");
			continue;
		}
		if (p > pend) {
			printf("truncated message\n");
			continue;
		}
		if (!gettag(p,53,data,pend)) {
			printf("Not a DHCP packet\n");
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

		if ((configp = find_config(&client.sin_addr)) == NULL) {
			fprintf(stderr, "No matching record!\n");
			continue;
		}
		
		dhcppak.op=BOOTP_REPLY;
		*((int *)dhcppak.sip)=configp->sip.s_addr;
		*((int *)dhcppak.gip)=0;
		strcpy(dhcppak.bootfile,configp->bootprog);

		/* Add DHCP message type optopn (53) */
		data[0]=(binlmode ? DHCPACK : DHCPOFFER);
		p=inserttag(p,53,1,data);

		/* Add server identifer option (54) */
#ifdef USE_RECVMSG
		if (msg.msg_controllen < sizeof(struct cmsghdr) ||
		    (msg.msg_flags & MSG_CTRUNC))
			printf("Aaargh!  No auxiliary information.\n");

		for (cmptr = CMSG_FIRSTHDR(&msg); cmptr != NULL;
		     cmptr = CMSG_NXTHDR(&msg, cmptr)) {

			if (cmptr->cmsg_level == IPPROTO_IP &&
			    cmptr->cmsg_type == IP_RECVDSTADDR) {
				*(int *)data=(*((struct in_addr*)CMSG_DATA(cmptr))).s_addr;
				printf("message directed to %s\n",
				       inet_ntoa((*((struct in_addr*)data))));
					
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
		printf("Sending %d bytes\n",rb);
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

int
parse_configs(char *filename)
{
	FILE	*fp;
	int	i, count = 0;
	char	buf[BUFSIZ], *bp, client[256], server[256], bootprog[256];
	struct  config *configp;

	if ((fp = fopen(filename, "r")) == NULL)
		return 1;

	while (1) {
		if ((bp = fgets(buf, BUFSIZ, fp)) == NULL)
			break;

		if (*bp == '\n' || *bp == '#')
			continue;

		if (count >= MAX_CONFIGS) {
			fclose(fp);
			return 1;
		}

		sscanf(bp, "%s %s %s\n", &client, &server, &bootprog);

		configp = (struct config *) calloc(sizeof(*configp), 1);
		if (!configp) {
			fclose(fp);
			return 1;
		}
		configurations[count++] = configp;
		configp->cip.s_addr = parsehost(client);
		configp->sip.s_addr = parsehost(server);
		strcpy(configp->bootprog, bootprog);
	}

	for (i = 0; i < count; i++) {
		char		c[256], s[256];

		configp = configurations[i];
		strcpy(c, inet_ntoa(configp->cip));
		strcpy(s, inet_ntoa(configp->sip));
		
		printf("%s %s %s\n", c, s, configp->bootprog);
	}

	fclose(fp);
	return 0;
}

struct config *
find_config(struct in_addr *addr)
{
	int		i;
	struct config	*configp;
	
	for (i = 0; i < MAX_CONFIGS; i++) {
		if ((configp = configurations[i]) == NULL)
			return 0;

		if (addr->s_addr == configp->cip.s_addr)
			return configp;
	}
	return 0;
}
