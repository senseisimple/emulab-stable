/*
 * Requires divert sockets (options IPDIVERT) and firewalling support
 * (options IPFIREWALL).
 *
 * To run, first configure the divert stuff:
 * 1 - make sure you have an entry in /etc/services for port 9696:
 *     davedivert    9696/divert # Dave's Janos Testing
 * 2 - Run you some ipfw to set up the divert:
 *     /sbin/ipfw add 100 divert 9696 udp from any to any PORT
 *
 *       (where PORT is the udp port you want to grab).
 * 3 - Compile and run this program.  If you want to monitor
 * the results, or send data to it, I strongly suggest the
 * use of something like netcat (~danderse/bin/....).
 * Very handy program.
 */

#include <sys/types.h>

#include <bitstring.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <sys/queue.h>

#include "divert.h"
#include "divert_hash.h"

#include <net/if.h>
#include <netinet/ip_fw.h>

#ifdef DEBUG
#define DPRINTF(fmt, args...) printf(fmt , ##args )
#else
#define DPRINTF(args...)
#endif

struct in_addr lastdst;
int controlsock = -1;

void IPDUMP(struct ip *ip) {
	DPRINTF("IP dump:\n"
		"  version:  %d\n"
		"  HL:       %d\n"
		"  TOS:      %d\n"
		"  LEN:      %d\n"
		"  ID:       %d\n"
		"  Offset:   %d\n"
		"  Protocol: %d\n"
		"  Checksum: %d\n"
		"  TTL:      %d\n"
		"  Src:      %s\n",
		ip->ip_v,
		ip->ip_hl,
		ip->ip_tos,
		ntohs(ip->ip_len),
		ntohs(ip->ip_id),
		ntohs(ip->ip_off),
		ip->ip_p,
		ntohs(ip->ip_sum),
		ip->ip_ttl,
		inet_ntoa(ip->ip_src));
	DPRINTF("  Dst:      %s\n",
		inet_ntoa(ip->ip_dst));
}

void UDPDUMP(struct udphdr *udp) {
	DPRINTF("UDP DUMP\n"
	       "   Source port: %d\n"
	       "   Dest port:   %d\n"
	       "   Length:      %d\n"
	       "   Checksum:    %d\n",
	       ntohs(udp->uh_sport),
	       ntohs(udp->uh_dport),
	       ntohs(udp->uh_ulen),
	       ntohs(udp->uh_sum));
}


/*
 * Needed for recomputing the udp checksum of a packet if we modify
 * its contents.
 *
 * Stolen from FreeBSD's networking stack (3.1).
 */

#define MAX_SOCKETS 128
#define MAX_RULES 128

#define RULE_BASE 666 /* XXX - make the range configurable */
#define DIVERT_PORT_BASE 9696 /* XXX - ditto */

bitstr_t *rulesused = NULL;
int numbits = 0;
bitstr_t *divert_ports_used = NULL;
int numports = 0;

int getrule() {
    int rule;
    if (rulesused == NULL) {
	rulesused = bit_alloc(MAX_RULES);
	numbits = MAX_RULES;
	bit_nclear(rulesused, 0, MAX_RULES-1);
    }
    if (!numbits) return -1;
    bit_ffc(rulesused, numbits, &rule);
    bit_set(rulesused, rule);
    return RULE_BASE - rule;
}

void freerule(int rule) {
    bit_clear(rulesused, rule - RULE_BASE);
}

struct portmask *find_mask_port(struct msockinfo *s, int proto, int port) {
    struct portmask *next;
    
    if (s == 0) return NULL;
    
    next = s->ports;;
    while (next != NULL) {
	if (next->proto == proto &&
	    next->proto_data.port == port) return next;
	next = next->next;
    }
    return NULL;
}

/* Add a port to the list */


int add_mask_port(struct msockinfo *s, int port)
{
    struct portmask *addem;
    int rc;
    struct ip_fw myipfw;

    if (s == 0) return -1;

    DPRINTF("Adding sniffer for socket %d on port %d\n",
	    s->sock, port);
    if (find_mask_port(s, IPPROTO_UDP, port)) return 0;
    addem = malloc(sizeof(*addem));
    if (!addem) return ENOMEM; /* Sigh */
    addem->proto = IPPROTO_UDP;
    addem->proto_data.port = port;
    addem->next = s->ports;
    s->ports = addem;
    addem->ipfw_rule = getrule();

    DPRINTF("Grabbing port %d with ipfw rule %d\n",
	    addem->proto_data.port,
	    addem->ipfw_rule);

    if (controlsock == -1) {
	    DPRINTF("Grabbing new control socket\n");
	    /* Grab it */
	    controlsock = socket( AF_INET, SOCK_RAW, IPPROTO_RAW );
	    if (controlsock < 0) {
		    perror("Could not allocate control socket!\n");
	    }
    }
    bzero(&myipfw, sizeof(myipfw));
    myipfw.fw_number = addem->ipfw_rule;
    myipfw.fw_divert_port = s->divert_port;
    myipfw.fw_flg = IP_FW_F_DIVERT | IP_FW_F_IN | IP_FW_F_OUT;
    myipfw.fw_prot = IPPROTO_UDP;
    IP_FW_SETNSRCP(&myipfw, 0);
    IP_FW_SETNDSTP(&myipfw, 1);
    myipfw.fw_uar.fw_pts[0] = addem->proto_data.port;

    rc = setsockopt(controlsock, IPPROTO_IP, IP_FW_ADD,
		    &myipfw, sizeof(myipfw));

    DPRINTF("Result of setsockopt: %d\n", rc);
    if (rc) {
	    perror("setsock for fw addition failed");
	    return errno;
    }
    
    return 0;
}

int add_mask_icmp(struct msockinfo *s, int icmptype)
{
    struct portmask *addem;
    int rc;
    struct ip_fw myipfw;

    if (s == 0) return -1;

    DPRINTF("Adding sniffer for ICMP socket %d type %d\n",
	    s->sock, icmptype);
    if (find_mask_port(s, IPPROTO_ICMP, icmptype)) return 0;
    addem = malloc(sizeof(*addem));
    if (!addem) return ENOMEM; /* Sigh */
    addem->proto = IPPROTO_ICMP;
    addem->proto_data.icmptype = icmptype;
    addem->next = s->ports;
    s->ports = addem;
    addem->ipfw_rule = getrule();

    DPRINTF("Grabbing ICMP type %d with ipfw rule %d\n",
	    addem->proto_data.icmptype,
	    addem->ipfw_rule);

    if (controlsock == -1) {
	    DPRINTF("Grabbing new control socket\n");
	    /* Grab it */
	    controlsock = socket( AF_INET, SOCK_RAW, IPPROTO_RAW );
	    if (controlsock < 0) {
		    perror("Could not allocate control socket!\n");
	    }
    }
    bzero(&myipfw, sizeof(myipfw));
    myipfw.fw_number = addem->ipfw_rule;
    myipfw.fw_divert_port = s->divert_port;
    myipfw.fw_flg = IP_FW_F_DIVERT | IP_FW_F_IN | IP_FW_F_OUT | IP_FW_F_ICMPBIT;
    myipfw.fw_prot = IPPROTO_ICMP;
    myipfw.fw_uar.fw_icmptypes[icmptype / (sizeof(unsigned)*8)] |=
                               1 << (icmptype % (sizeof(unsigned) * 8));

    DPRINTF("Setting int %d to 0x%x\n",
	    icmptype / (sizeof(unsigned)*8),
	    icmptype % (sizeof(unsigned) *8));

    rc = setsockopt(controlsock, IPPROTO_IP, IP_FW_ADD,
		    &myipfw, sizeof(myipfw));

    DPRINTF("Result of setsockopt: %d\n", rc);
    if (rc) {
	    perror("setsock for fw addition failed");
	    return errno;
    }
    
    return 0;
}

void freeport(struct portmask *pm)
{
    struct ip_fw myipfw;
    int rc;
    /* XXX */
    DPRINTF("Removing port snork for port %d\n", pm->proto_data.port);

    myipfw.fw_number = pm->ipfw_rule;
    rc = setsockopt(controlsock, IPPROTO_IP, IP_FW_DEL,
		    &myipfw, sizeof(myipfw));
    if (rc) {
	    perror("setsockopt failed on delete");
    }

    freerule(pm->ipfw_rule);
    free(pm);
}

int rem_mask_port(struct msockinfo *s, int port)
{
    struct portmask *prev;
    struct portmask *cur;

    if (s == 0) return -1;
    if (s->ports == 0) return -1;
    if (s->ports->proto == IPPROTO_UDP && s->ports->proto_data.port == port) {
	cur = s->ports;
	s->ports = s->ports->next;
	freeport(cur);
	return 0;
    }

    prev = s->ports;
    cur = prev->next;
    while (cur != NULL) {
	if (cur->proto_data.port == port) {
	    prev->next = cur->next;
	    freeport(cur);
	    return 0;
	}
	prev = cur;
	cur = cur->next;
    }
    return -1;
}

/*
 * Allocate a divert port, etc.  Fill in the msockinfo struct
 */

struct msockinfo *get_socket() {
	struct msockinfo *tmp;
	int divert_port;
	struct sockaddr_in divertsock;
	int rc;
	
	addrtable_init();

	tmp = malloc(sizeof(*tmp));
	if (!tmp) return NULL;
	bzero(tmp, sizeof(*tmp));
	
	
	if (divert_ports_used == NULL) {
		divert_ports_used = bit_alloc(MAX_RULES);
		bit_nclear(divert_ports_used, 0, MAX_RULES-1);
		numports = MAX_RULES;
	}
	if (!numports) {
		DPRINTF("Numports was zero.  sigh.\n");
		return NULL;
	}
	bit_ffc(divert_ports_used, numports, &divert_port);
	if (divert_port > 0) {
		bit_set(divert_ports_used, divert_port);
	}
	tmp->divert_port = divert_port + DIVERT_PORT_BASE;
	bit_set(divert_ports_used, divert_port);
	DPRINTF("Created socket using divert port %d\n",
		tmp->divert_port);
	
	DPRINTF("Opening raw socket\n");
	rc = 0;
	tmp->sock = socket(PF_INET, SOCK_RAW, IPPROTO_DIVERT);
	if ((tmp->sock < 0) || rc) {
		perror("could not create raw socket");
		exit(errno); /* XXX */
	}
	
	
	bzero(&divertsock, sizeof(divertsock));
	divertsock.sin_addr.s_addr = INADDR_ANY;
	divertsock.sin_port = htons(tmp->divert_port);
	divertsock.sin_len = sizeof(divertsock);
	divertsock.sin_family = AF_INET;

	DPRINTF("Binding raw socket (fd %d)to divert port %d\n",
		tmp->sock,
		tmp->divert_port);
	rc = bind(tmp->sock, (struct sockaddr *) &divertsock, sizeof(divertsock));
	if (rc) {
		perror("Could not bind divert socket");
		exit(errno);
	}

	return tmp;
}

void free_socket(struct msockinfo *s) {
    struct portmask *p, *n;
    bit_clear(divert_ports_used, s->divert_port - RULE_BASE);
    p = s->ports;
    while (p != NULL) {
	    n = p->next;
	    freeport(p);
	    p = n;
    }
    DPRINTF("Destroying socket using divert port %d\n",
	    s->divert_port);
    close(s->sock);
}


u_short
in_cksum(addr, len)
	u_short *addr;
int len;
{
	register int nleft = len;
	register u_short *w = addr;
	register int sum = 0;
	u_short answer = 0;
	
	/*
	 * Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}
	
	/* mop up an odd byte, if necessary */
	if (nleft == 1) {
		*(u_char *)(&answer) = *(u_char *)w ;
		sum += answer;
	}
	
	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
	sum += (sum >> 16);                     /* add carry */
	answer = ~sum;                          /* truncate to 16 bits */
	return(answer);
}


int receive_snoop(struct msockinfo *msock,
		  char *packetdata,
		  int dataSize,
		  struct in_addr *srcAddr,
		  short *srcPort,
		  struct in_addr *destAddr,
		  short *destPort)
{
	char packetbuf[MAXPACKET]; /* for receiving the entire raw packet */
	struct sockaddr_in from;
	int fromlen, len;
	struct ip *ip;
	struct udphdr *udp;
	char *payload;
	int payloadlen;
	int rc;

	DPRINTF(__FUNCTION__ ": about to call krecvfrom\n");
	
	
	fromlen = sizeof(from);
	bzero(&from, fromlen);
	
	bzero(packetbuf, MAXPACKET);

	DPRINTF(__FUNCTION__ ":  krecv: sock %d, fromlen %d\n", msock->sock, fromlen);
	
	len = recvfrom(msock->sock, packetbuf, MAXPACKET, 0,
			(struct sockaddr *)&from, &fromlen);
	rc = 0;
	DPRINTF("KRECVFROM returned %d, len %d\n",
		rc, len);
	
	if ((len < 0) || rc) {
		DPRINTF(__FUNCTION__ ": error on recvfrom.  errno %d\n", errno);
		return len;
	}

	ip = (struct ip *)packetbuf;
	udp = (struct udphdr *)(packetbuf + (ip->ip_hl * 4));

	*srcAddr = ip->ip_src;
	*destAddr = ip->ip_dst;
	*srcPort = udp->uh_sport;
	*destPort = udp->uh_dport;
	payloadlen = len - ((ip->ip_hl * 4) + sizeof(*udp));
	payload = (packetbuf + ((ip->ip_hl * 4) + sizeof(*udp)));

	if (payloadlen > dataSize)
		payloadlen = dataSize;
	memcpy(packetdata, payload, payloadlen);

	DPRINTF("Received packet, dst 0x%lx port %d (%d)!\n",
		ntohl(from.sin_addr.s_addr),
		ntohs(from.sin_port),
		from.sin_port);
	DPRINTF("Recieved len: %d family %d\n",
		from.sin_len,
		from.sin_family);
	IPDUMP(ip);
	UDPDUMP(udp);

	if (from.sin_addr.s_addr != 0) { /* Local! */
		addrtable_insert(ip->ip_dst.s_addr, 1);
	} else {
		addrtable_insert(ip->ip_dst.s_addr, 0);
	}
	
	return (len - ((ip->ip_hl * 4) + sizeof(*udp)));
}

int send_snoop(struct msockinfo *msock,
	       char *packetdata,
	       int datalen,
	       struct in_addr srcAddr,
	       short srcPort,
	       struct in_addr destAddr,
	       short destPort)
{
	char packetbuf[MAXPACKET];
	int wlen;
	struct sockaddr_in dst;
	struct ip *ip;
	struct udphdr *udp;
	char *payload;
	int packetlen, csumlen;
	struct ipovly *ovl; /* Overlay for UDP computation */
	int islocal;
	int rc;

	/* Cons up the packet */
	bzero(packetbuf, MAXPACKET);
	
	ip = (struct ip *)packetbuf;
	/* Ignore IP options */
	udp = (struct udphdr *)((char *)packetbuf + sizeof(*ip));
	payload = (char *)(packetbuf + sizeof(*ip) + sizeof(*udp));
	packetlen = datalen + sizeof(*udp) + sizeof(*ip);

	csumlen = packetlen;
	if (csumlen % 2) csumlen++; /* Round to nearest two-byte boundary */

	/* Set up the UDP packet */
	
	udp->uh_sport = srcPort;
	udp->uh_dport = destPort;
	udp->uh_ulen = htons(sizeof(*udp) + datalen);
	memcpy(payload, packetdata, datalen);


#define DO_UDP_CHECKSUM 1
#ifdef DO_UDP_CHECKSUM
	ovl = (struct ipovly *)ip;
	ovl->ih_pr = IPPROTO_UDP;
	ovl->ih_len = udp->uh_ulen;
	ovl->ih_src = srcAddr;
	ovl->ih_dst = destAddr;
	udp->uh_sum = in_cksum(packetbuf, csumlen);
	bzero(ip, sizeof(*ip));
#else
	udp->uh_sum = 0;
#endif

	ip->ip_id = 3; /* XXX */
	ip->ip_hl = 5; /* Yeah, yeah */
	ip->ip_v = 4;
	ip->ip_src = srcAddr;
	ip->ip_dst = destAddr;
	ip->ip_ttl = 64; /* XXX */
	ip->ip_p = IPPROTO_UDP;
	ip->ip_len = udp->uh_ulen;
	ip->ip_len = htons(packetlen);
	/* The system should calculate the IP checksum for us */
	/* But it doesn't appear to be doing so, NOW DOES IT. :-) */
	ip->ip_sum = in_cksum(packetbuf, sizeof *ip);

	/* Set up the sockaddr. */
	
	bzero(&dst, sizeof(dst));

	islocal = addrtable_find(ip->ip_dst.s_addr);
	if (islocal == 0) {
		DPRINTF("%x is not local\n", ip->ip_dst.s_addr);
		dst.sin_addr.s_addr = 0;
	} else {
		DPRINTF("is local!!\n");
		dst.sin_addr.s_addr = destAddr.s_addr;
		strcpy(dst.sin_zero, "fxp0");
	}

	dst.sin_len = sizeof(dst);
	dst.sin_family = AF_INET;
	dst.sin_port = 800; /* XXX  - must find highest # used by filter */

	/* This is so bogus.  We have to subtract 8,
	   because we're patched into an internal function
	   which compares against the IP address *and*
	   possibly the cookiefied name of the interface.
	   GARRRRRRR.
	*/

	DPRINTF("Sending on FD %d to   addr 0x%lx port %d\n", msock->sock,
		ntohl(ip->ip_dst.s_addr),
		ntohs(udp->uh_dport));
	DPRINTF("Sending from addr 0x%lx port %d\n",
	       ntohl(ip->ip_src.s_addr),
	       ntohs(udp->uh_sport));
	DPRINTF("sendtoargs:  To:  0x%lx  port: %d  Len: %d\n",
		ntohl(dst.sin_addr.s_addr),
		dst.sin_port,
		dst.sin_len);
	
	IPDUMP(ip);
	UDPDUMP(udp);

	rc = 0;
	wlen = sendto(msock->sock, packetbuf, packetlen, 0,
		     (struct sockaddr *)&dst, dst.sin_len);
	
	if ((wlen != packetlen) || rc) {
		perror("sendto failed");
	}
	DPRINTF("Sent %d bytes\n", wlen);
	return datalen;
}


/*
 * Differences - he allows a bound socket to proxy for a particular
 * port, where I require an unbound socket.
 *
 * 2 - It does some nice dest addr munging.. I'm not sure I see the
 * value of this, when I can simply write it back out.
 *
 * 3 - Allows a connecting socket to masquerade as a different source
 *     (does it do it for TCP, or just UDP, and do we care about anything
 *      but UDP?  The TCP case would be harder, because I'd need
 *      to fake more of the TCP protocol).
 *
 * 5 - No can do that part, I'm afraid.  That's TCP semantics.
 */
