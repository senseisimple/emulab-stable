/*
 * Network routines.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "decls.h"

/* Max number of times to attempt bind to port before failing. */
#define MAXBINDATTEMPTS		10

/* Max number of hops multicast hops. */
#define MCAST_TTL		5

static int		sock;
static struct in_addr	ipaddr;

static void
CommonInit(void)
{
	struct sockaddr_in	name;
	struct timeval		timeout;
	int			i;
	char			buf[BUFSIZ];
	struct hostent		*he;
	
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		pfatal("Could not allocate a socket");

	i = (128 * 1024);
	setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &i, sizeof(i));
    
	i = (128 * 1024);
	setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &i, sizeof(i));

	name.sin_family      = AF_INET;
	name.sin_port	     = htons(portnum);
	name.sin_addr.s_addr = htonl(INADDR_ANY);

	i = MAXBINDATTEMPTS;
	while (i) {
		if (bind(sock, (struct sockaddr *)&name, sizeof(name)) == 0)
			break;

		pwarning("Bind to port %d failed. Will try %d more times!",
			 portnum, i);
		i--;
		sleep(5);
	}
	log("Bound to port %d", portnum);

	/*
	 * At present, we use a multicast address in both directions.
	 */
	if ((ntohl(mcastaddr.s_addr) >> 28) == 14) {
		unsigned int loop = 0, ttl = MCAST_TTL;
		struct ip_mreq mreq;

		log("Using Multicast");

		mreq.imr_multiaddr.s_addr = mcastaddr.s_addr;

		if (mcastif.s_addr)
			mreq.imr_interface.s_addr = mcastif.s_addr;
		else
			mreq.imr_interface.s_addr = htonl(INADDR_ANY);

		if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			       &mreq, sizeof(mreq)) < 0)
			pfatal("setsockopt(IPPROTO_IP, IP_ADD_MEMBERSHIP)");

		if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL,
			       &ttl, sizeof(ttl)) < 0) 
			pfatal("setsockopt(IPPROTO_IP, IP_MULTICAST_TTL)");

		/* Disable local echo */
		if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP,
			       &loop, sizeof(loop)) < 0)
			pfatal("setsockopt(IPPROTO_IP, IP_MULTICAST_LOOP)");

		if (mcastif.s_addr &&
		    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
			       &mcastif, sizeof(mcastif)) < 0) {
			pfatal("setsockopt(IPPROTO_IP, IP_MULTICAST_IF)");
		}
	}
	else {
		/*
		 * Otherwise, we use a broadcast addr. 
		 */
		i = 1;
		
		if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
			       &i, sizeof(i)) < 0)
			pfatal("setsockopt(SOL_SOCKET, SO_BROADCAST)");
	}

	/*
	 * We use a socket level timeout instead of polling for data.
	 */
	timeout.tv_sec  = 0;
	timeout.tv_usec = PKTRCV_TIMEOUT;
	
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
		       &timeout, sizeof(timeout)) < 0)
		pfatal("setsockopt(SOL_SOCKET, SO_RCVTIMEO)");

	/*
	 * We add our (unicast) IP addr to every outgoing message.
	 * This is going to be used to return replies to the sender,
	 * where appropriate.
	 */
	if (gethostname(buf, sizeof(buf)) < 0)
		pfatal("gethostname failed");

	if ((he = gethostbyname(buf)) == 0)
		fatal("gethostbyname: %s", hstrerror(h_errno));

	memcpy((char *)&ipaddr, he->h_addr, sizeof(ipaddr));
}

int
ClientNetInit(void)
{
	CommonInit();
	
	return 1;
}

int
ServerNetInit(void)
{
	CommonInit();

	return 1;
}

/*
 * Look for a packet on the socket. Propogate the errors back to the caller
 * exactly as the system call does. Remember that we set up a socket timeout
 * above, so we will get EWOULDBLOCK errors when no data is available. 
 *
 * The amount of data received is determined from the datalen of the hdr.
 * All packets are actually the same size/structure. 
 */
int
PacketReceive(Packet_t *p)
{
	struct sockaddr_in from;
	int		   mlen, alen = sizeof(from);

	bzero(&from, sizeof(from));

	if ((mlen = recvfrom(sock, p, sizeof(*p), 0,
			     (struct sockaddr *)&from, &alen)) < 0) {
		if (errno == EWOULDBLOCK)
			return -1;
		pfatal("PacketReceive(recvfrom)");
	}
	if (mlen != sizeof(p->hdr) + p->hdr.datalen)
		fatal("PacketReceive: Bad message length %d!=%d",
		      mlen, p->hdr.datalen);

	return p->hdr.datalen;
}

/*
 * We use blocking sends since there is no point in giving up. All packets
 * go to the same place, whether client or server.
 *
 * The amount of data sent is determined from the datalen of the packet hdr.
 * All packets are actually the same size/structure. 
 */
int
PacketSend(Packet_t *p)
{
	struct sockaddr_in to;
	int		   len;

	len = sizeof(p->hdr) + p->hdr.datalen;
	p->hdr.srcip = ipaddr.s_addr;

	to.sin_family      = AF_INET;
	to.sin_port        = htons(portnum);
	to.sin_addr.s_addr = mcastaddr.s_addr;

	while (sendto(sock, (void *)p, len, 0, 
		      (struct sockaddr *)&to, sizeof(to)) < 0) {
		if (errno != ENOBUFS)
			pfatal("PacketSend(sendto)");

		/*
		 * ENOBUFS means we ran out of mbufs. Okay to sleep a bit
		 * to let things drain.
		 */
		fsleep(10000);
	}

	return p->hdr.datalen;
}

/*
 * Basically the same as above, but instead of sending to the multicast
 * group, send to the (unicast) IP in the packet header. This simplifies
 * the logic in a number of places, by avoiding having to deal with
 * multicast packets that are not destined for us, but for someone else.
 */
int
PacketReply(Packet_t *p)
{
	struct sockaddr_in to;
	int		   len;

	len = sizeof(p->hdr) + p->hdr.datalen;

	to.sin_family      = AF_INET;
	to.sin_port        = htons(portnum);
	to.sin_addr.s_addr = p->hdr.srcip;
	p->hdr.srcip       = ipaddr.s_addr;

	while (sendto(sock, (void *)p, len, 0, 
		      (struct sockaddr *)&to, sizeof(to)) < 0) {
		if (errno != ENOBUFS)
			pfatal("PacketSend(sendto)");

		/*
		 * ENOBUFS means we ran out of mbufs. Okay to sleep a bit
		 * to let things drain.
		 */
		fsleep(10000);
	}

	return p->hdr.datalen;
}
