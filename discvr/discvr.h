/*$Id: discvr.h,v 1.1 2000-07-06 17:42:35 kwright Exp $*/

#ifndef _TOPD_DISCVR_H_
#define _TOPD_DISCVR_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>		
#include <arpa/inet.h>

#define MAX_HOSTNAME       256
#define	INET6_ADDRSTRLEN    46	/* Max size of IPv6 address string: */
#define	SERV_PORT         9788	/* Server port */
#define	IFHADDRSIZ           8	/* UNP recommends allowing for 64-bit EUI-64 in future */
#define ETHADDRSIZ           6  /* Cheat and use for for many MAC address sizes */
#define	MAXLINE	          4096	/* Max text line length; only 
				 * needed until true packets are generated.  -lkw
				 */

#define print_nodeID(x) println_haddr((x), ETHADDRSIZ)

#define INCR_ALIGN(x,y)   ((x) += (y); ALIGN(x);)
                            
#define	socklen_t unsigned int	/* Length of sockaddr address */

/* The structure returned by recvfrom_flags() */
struct in_pktinfo {
  int     ipi_ifindex;          /* received interface index */
  struct in_addr ipi_addr;	/* dst IPv4 address */
};

#define MY_IFF_RECVIF   0x1     /* receiving interface */

struct ifi_info {
  char    ifi_name[IFNAMSIZ];	/* interface name, null terminated */
  u_char  ifi_haddr[IFHADDRSIZ];/* hardware address */
  u_short ifi_hlen;		/* #bytes in hardware address: 0, 6, 8 */
  short   ifi_flags;		/* IFF_xxx constants from <net/if.h> */
  short   ifi_myflags;		/* our own IFI_xxx flags */
  struct sockaddr  *ifi_addr;	/* primary address */
  struct sockaddr  *ifi_brdaddr;/* broadcast address */
  struct sockaddr  *ifi_dstaddr;/* destination address */
  struct ifi_info  *ifi_next;	/* next of these structures */
  struct topd_nborlist *ifi_nbors;/* neighbors */

};

/* Prototypes */

void 
serv_listen(int sockfd, struct sockaddr *pcliaddr, socklen_t clilen);

char *
forward_request(struct ifi_info *ifi, const struct in_pktinfo *pktinfo,
		     const char *mesg, const int mesglen);
struct ifi_info	*
get_ifi_info(int, int);

void			 
free_ifi_info(struct ifi_info *);

ssize_t	 
recvfrom_flags(int, void *, size_t, int *, struct sockaddr *, socklen_t *,
	       struct in_pktinfo *);

char*
if_indextoname(unsigned int, char *);

char*
sock_ntop(const struct sockaddr *, socklen_t);

char*
net_rt_iflist(int, int, size_t *);

void	 
get_rtaddrs(int, struct sockaddr *, struct sockaddr **);

u_int32_t
compose_reply(struct ifi_info *ifi, char *mesg, const int mesglen);


#endif /* _TOPD_DISCVR_H_ */
