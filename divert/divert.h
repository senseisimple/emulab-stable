#ifndef _JANOS_DIVERT_H
#define _JANOS_DIVERT_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <netinet/ip_var.h>


#define MAXPACKET 2048

struct portmask {
    int proto;      /* currently IPPROTO_ICMP or IPPROTO_UDP */
    union {
      int port;
      int icmptype;
    } proto_data;
    int ipfw_rule;
    struct portmask *next;
};

struct msockinfo {
    int sock;
    int divert_port;
    struct portmask *ports;
};


/* 
 * Returns the length of snooped packet or -1 if failure.  
 * Failure info is in errno. Fills in srcAddr, srcPort,
 * destAddr, and destPort with header info from snooped packet.
 */
int receive_snoop(struct msockinfo *msock,
		  char *packetdata, int dataSize,
		  struct in_addr *srcAddr,
		  short *srcPort,
		  struct in_addr *destAddr,
		  short *destPort);
/* Ditto */

int send_snoop(struct msockinfo *msock,
	       char *packetdata,
	       int datalen,
	       struct in_addr srcAddr,
	       short srcPort,
	       struct in_addr destAddr,
	       short destPort);

/*
 * Get a new divert socket object.
 * Returns NULL if there was a problem.
 */
struct msockinfo *get_socket();

/*
 * Free a divert socket object created by get_socket()
 */
void free_socket(struct msockinfo *s);

int add_mask_port(struct msockinfo *s, int port);
int rem_mask_port(struct msockinfo *s, int port);

/* We should clean up the interface to merge these..
 * and add a rem_mask_icmp, or equivalent. */
int add_mask_icmp(struct msockinfo *s, int icmptype);

#endif /* _JANOS_DIVERT_H */
