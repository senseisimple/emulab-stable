/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2011 University of Utah and the Flux Group.
 * All rights reserved.
 */

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
#include "utils.h"
#ifdef NO_SOCKET_TIMO
#include <sys/select.h>
#endif

#ifdef STATS
unsigned long nonetbufs;
#define DOSTAT(x)	(x)
#else
#define DOSTAT(x)
#endif

/* Max number of times to attempt bind to port before failing. */
#define MAXBINDATTEMPTS		1

/* Max number of hops multicast hops. */
#define MCAST_TTL		5

static int		sock = -1;
struct in_addr		myipaddr;
static int		nobufdelay = -1;
int			broadcast = 0;
static int		isclient = 0;
static int		sndportnum;	/* kept in network order */

/*
 * Convert a string to an IPv4 address.  We first try to interpret it as
 * an IPv4 address.  If that fails, we attempt to resolve it as a host name.
 * Return non-zero on success.
 */
int
GetIP(char *str, struct in_addr *in)
{
	struct hostent *he;

	if (inet_aton(str, in) == 0) {
		if ((he = gethostbyname(str)) == NULL)
			return 0;
		memcpy(in, he->h_addr, sizeof(*in));
	}

	return 1;
}

int
GetSockbufSize(void)
{

	static int sbsize = 0;

	if (sbsize == 0) {
#if DYN_SOCKBUFSIZE > 0
		int sock;

		if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
			pfatal("Could not allocate a socket");

		for (sbsize = SOCKBUFSIZE; sbsize > 0; sbsize -= (16*1024)) {
			int i = sbsize;
			if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF,
				       &i, sizeof(i)) >= 0)
				break;
		}
		if (sbsize < 0) {
			int i = 0;
			unsigned int ilen = sizeof(i);
			if (getsockopt(sock, SOL_SOCKET, SO_SNDBUF,
				       &i, &ilen) < 0)
				i = SOCKBUFSIZE;
			sbsize = i;
		}
		close(sock);
#else
		sbsize = SOCKBUFSIZE;
#endif
		log("Maximum socket buffer size of %d bytes", sbsize);
	}
	return sbsize;
}

static void
CommonInit(int dobind)
{
	struct sockaddr_in	name;
	int			i;
	char			buf[BUFSIZ];
	struct hostent		*he;
	
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		pfatal("Could not allocate a socket");

	i = GetSockbufSize();
	if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &i, sizeof(i)) < 0)
		pwarning("Could not increase send socket buffer size to %d", i);
    
	i = GetSockbufSize();
	if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &i, sizeof(i)) < 0)
		pwarning("Could not increase recv socket buffer size to %d", i);

	if (dobind) {
		name.sin_family      = AF_INET;
		name.sin_port	     = htons(portnum);
		name.sin_addr.s_addr = htonl(INADDR_ANY);

		i = MAXBINDATTEMPTS;
		while (i) {
			if (bind(sock, (struct sockaddr *)&name, sizeof(name)) == 0)
				break;

			/*
			 * Note that we exit with a magic value. 
			 * This is for server wrapper-scripts so that they can
			 * differentiate this case and try again with a
			 * different port.
			 */
			if (--i == 0) {
				error("Could not bind to port %d!\n", portnum);
				exit(EADDRINUSE);
			}

			pwarning("Bind to port %d failed. Will try %d more times!",
				 portnum, i);
			sleep(5);
		}
		log("Bound to port %d", portnum);
	} else {
		log("NOT binding to port %d", portnum);
	}
	sndportnum = htons(portnum);

	/*
	 * At present, we use a multicast address in both directions.
	 */
	if ((ntohl(mcastaddr.s_addr) >> 28) == 14) {
		unsigned int loop = 0, ttl = MCAST_TTL;
		struct ip_mreq mreq;

		log("Using Multicast %s", inet_ntoa(mcastaddr));

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
	else if (broadcast) {
		/*
		 * Otherwise, we use a broadcast addr. 
		 */
		i = 1;

		log("Setting broadcast mode\n");
		
		if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
			       &i, sizeof(i)) < 0)
			pfatal("setsockopt(SOL_SOCKET, SO_BROADCAST)");
	}

#ifndef NO_SOCKET_TIMO
	/*
	 * We use a socket level timeout instead of polling for data.
	 */
	{
		struct timeval timeout;

		timeout.tv_sec  = 0;
		timeout.tv_usec = PKTRCV_TIMEOUT;
		if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
			       &timeout, sizeof(timeout)) < 0)
			pfatal("setsockopt(SOL_SOCKET, SO_RCVTIMEO)");
	}
#endif

	/*
	 * If a specific interface IP is specified, use that to
	 * tag our outgoing packets.  Otherwise we use the IP address
	 * associated with our hostname.
	 */
	if (mcastif.s_addr)
		myipaddr.s_addr = mcastif.s_addr;
	else {
		if (gethostname(buf, sizeof(buf)) < 0)
			pfatal("gethostname failed");

		if ((he = gethostbyname(buf)) == 0)
			fatal("gethostbyname: %s", hstrerror(h_errno));

		memcpy((char *)&myipaddr, he->h_addr, sizeof(myipaddr));
	}

	/*
	 * Compute the out of buffer space delay.
	 */
	if (nobufdelay < 0)
		nobufdelay = sleeptime(100, NULL, 1);
}

int
ClientNetInit(void)
{
#ifndef SAME_HOST_HACK
	CommonInit(1);
#else
	CommonInit(0);
#endif
	isclient = 1;
	
	return 1;
}

unsigned long
ClientNetID(void)
{
	return ntohl(myipaddr.s_addr);
}

int
ServerNetInit(void)
{
	CommonInit(1);
	isclient = 0;

	return 1;
}

/*
 * XXX hack.
 *
 * Cisco switches without a multicast router defined have an unfortunate
 * habit of losing our IGMP membership.  This function allows us to send
 * a report message to remind the switch we are still around.
 *
 * We need a better way to do this!
 */
int
NetMCKeepAlive(void)
{
	struct ip_mreq mreq;

	if (broadcast || (ntohl(mcastaddr.s_addr) >> 28) != 14)
		return 0;

	if (sock == -1)
		return 1;

	mreq.imr_multiaddr.s_addr = mcastaddr.s_addr;
	if (mcastif.s_addr)
		mreq.imr_interface.s_addr = mcastif.s_addr;
	else
		mreq.imr_interface.s_addr = htonl(INADDR_ANY);

	if (setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP,
		       &mreq, sizeof(mreq)) < 0 ||
	    setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		       &mreq, sizeof(mreq)) < 0)
		return 1;
	return 0;
}

/*
 * Look for a packet on the socket. Propogate the errors back to the caller
 * exactly as the system call does. Remember that we set up a socket timeout
 * above, so we will get EWOULDBLOCK errors when no data is available. 
 *
 * The amount of data received is determined from the datalen of the hdr.
 * All packets are actually the same size/structure. 
 *
 * Returns 0 for a good packet, 1 for a back packet, -1 on timeout.
 */
int
PacketReceive(Packet_t *p)
{
	struct sockaddr_in from;
	int		   mlen;
	unsigned int	   alen;

#ifdef NO_SOCKET_TIMO
	fd_set		ready;
	struct timeval	tv;
	int		rv;

	tv.tv_sec = 0;
	tv.tv_usec = PKTRCV_TIMEOUT;
	FD_ZERO(&ready);
	FD_SET(sock, &ready);
	rv = select(sock+1, &ready, NULL, NULL, &tv);
	if (rv < 0) {
		if (errno == EINTR)
			return -1;
		pfatal("PacketReceive(select)");
	}
	if (rv == 0)
		return -1;
#endif
	alen = sizeof(from);
	bzero(&from, alen);
	if ((mlen = recvfrom(sock, p, sizeof(*p), 0,
			     (struct sockaddr *)&from, &alen)) < 0) {
		if (errno == EWOULDBLOCK)
			return -1;
		pfatal("PacketReceive(recvfrom)");
	}

	/*
	 * Basic integrity checks
	 */
	if (mlen < sizeof(p->hdr) + p->hdr.datalen) {
		log("Bad message length (%d != %d)",
		    mlen, p->hdr.datalen);
		return 1;
	}
#ifdef SAME_HOST_HACK
	/*
	 * If using a host alias for the client, a message may get
	 * the wrong IP, so rig the IP check to make it always work.
	 */
	if (p->hdr.srcip != from.sin_addr.s_addr)
		from.sin_addr.s_addr = p->hdr.srcip;

	/*
	 * Also, we aren't binding to a port on the client side, so the
	 * first message to the server will contain the actual port we
	 * will use from now on.
	 */
	if (!isclient && sndportnum == htons(portnum) &&
	    sndportnum != from.sin_port)
		sndportnum = from.sin_port;
#endif
	if (p->hdr.srcip != from.sin_addr.s_addr) {
		log("Bad message source (%x != %x)",
		    ntohl(from.sin_addr.s_addr), ntohl(p->hdr.srcip));
		return 1;
	}
	if (sndportnum != from.sin_port) {
		log("Bad message port (%d != %d)",
		    ntohs(from.sin_port), ntohs(sndportnum));
		return 1;
	}
	return 0;
}

/*
 * We use blocking sends since there is no point in giving up. All packets
 * go to the same place, whether client or server.
 *
 * The amount of data sent is determined from the datalen of the packet hdr.
 * All packets are actually the same size/structure. 
 */
void
PacketSend(Packet_t *p, int *resends)
{
	struct sockaddr_in to;
	int		   len, delays;

	len = sizeof(p->hdr) + p->hdr.datalen;
	p->hdr.srcip = myipaddr.s_addr;

	to.sin_family      = AF_INET;
	to.sin_port        = sndportnum;
	to.sin_addr.s_addr = mcastaddr.s_addr;

	delays = 0;
	while (sendto(sock, (void *)p, len, 0, 
		      (struct sockaddr *)&to, sizeof(to)) < 0) {
		if (errno != ENOBUFS)
			pfatal("PacketSend(sendto)");

		/*
		 * ENOBUFS means we ran out of mbufs. Okay to sleep a bit
		 * to let things drain.
		 */
		delays++;
		fsleep(nobufdelay);
	}

	DOSTAT(nonetbufs += delays);
	if (resends != 0)
		*resends = delays;
}

/*
 * Basically the same as above, but instead of sending to the multicast
 * group, send to the (unicast) IP in the packet header. This simplifies
 * the logic in a number of places, by avoiding having to deal with
 * multicast packets that are not destined for us, but for someone else.
 */
void
PacketReply(Packet_t *p)
{
	struct sockaddr_in to;
	int		   len;

	len = sizeof(p->hdr) + p->hdr.datalen;

	to.sin_family      = AF_INET;
	to.sin_port        = sndportnum;
	to.sin_addr.s_addr = p->hdr.srcip;
	p->hdr.srcip       = myipaddr.s_addr;

	while (sendto(sock, (void *)p, len, 0, 
		      (struct sockaddr *)&to, sizeof(to)) < 0) {
		if (errno != ENOBUFS)
			pfatal("PacketSend(sendto)");

		/*
		 * ENOBUFS means we ran out of mbufs. Okay to sleep a bit
		 * to let things drain.
		 */
		DOSTAT(nonetbufs++);
		fsleep(nobufdelay);
	}
}

int
PacketValid(Packet_t *p, int nchunks)
{
	switch (p->hdr.type) {
	case PKTTYPE_REQUEST:
	case PKTTYPE_REPLY:
		break;
	default:
		return 0;
	}

	switch (p->hdr.subtype) {
	case PKTSUBTYPE_BLOCK:
		if (p->hdr.datalen < sizeof(p->msg.block))
			return 0;
		if (p->msg.block.chunk < 0 ||
		    p->msg.block.chunk >= nchunks ||
		    p->msg.block.block < 0 ||
		    p->msg.block.block >= MAXCHUNKSIZE)
			return 0;
		break;
	case PKTSUBTYPE_REQUEST:
		if (p->hdr.datalen < sizeof(p->msg.request))
			return 0;
		if (p->msg.request.chunk < 0 ||
		    p->msg.request.chunk >= nchunks ||
		    p->msg.request.block < 0 ||
		    p->msg.request.block >= MAXCHUNKSIZE ||
		    p->msg.request.count < 0 ||
		    p->msg.request.block+p->msg.request.count > MAXCHUNKSIZE)
			return 0;
		break;
	case PKTSUBTYPE_PREQUEST:
		if (p->hdr.datalen < sizeof(p->msg.prequest))
			return 0;
		if (p->msg.prequest.chunk < 0 ||
		    p->msg.prequest.chunk >= nchunks)
			return 0;
		break;
	case PKTSUBTYPE_JOIN:
		if (p->hdr.datalen < sizeof(p->msg.join))
			return 0;
		break;
	case PKTSUBTYPE_JOIN2:
		if (p->hdr.datalen < sizeof(p->msg.join2))
			return 0;
		break;
	case PKTSUBTYPE_LEAVE:
		if (p->hdr.datalen < sizeof(p->msg.leave))
			return 0;
		break;
	case PKTSUBTYPE_LEAVE2:
		if (p->hdr.datalen < sizeof(p->msg.leave2))
			return 0;
		break;
	default:
		return 0;
	}

	return 1;
}

#ifdef MASTER_SERVER
int
MsgSend(int msock, MasterMsg_t *msg, size_t size, int timo)
{
	void *buf = msg;
	int cc;
	struct timeval tv, now, then;
	fd_set wfds;

	if (timo) {
		tv.tv_sec = timo;
		tv.tv_usec = 0;
		gettimeofday(&then, NULL);
		timeradd(&then, &tv, &then);
	}
	while (size > 0) {
		if (timo) {
			gettimeofday(&now, NULL);
			if (timercmp(&now, &then, >=)) {
				cc = 0;
			} else {
				timersub(&then, &now, &tv);
				FD_ZERO(&wfds);
				FD_SET(msock, &wfds);
				cc = select(msock+1, NULL, &wfds, NULL, &tv);
			}
			if (cc <= 0) {
				if (cc == 0) {
					errno = ETIMEDOUT;
					cc = -1;
				}
				break;
			}
		}

		cc = write(msock, buf, size);
		if (cc <= 0)
			break;

		size -= cc;
		buf += cc;
	}

	if (size != 0) {
		char *estr = "master server message send";
		if (cc == 0)
			fprintf(stderr, "%s: Unexpected EOF\n", estr);
		else
			perror(estr);
		return 0;
	}
	return 1;
}

int
MsgReceive(int msock, MasterMsg_t *msg, size_t size, int timo)
{
	void *buf = msg;
	int cc;
	struct timeval tv, now, then;
	fd_set rfds;

	if (timo) {
		tv.tv_sec = timo;
		tv.tv_usec = 0;
		gettimeofday(&then, NULL);
		timeradd(&then, &tv, &then);
	}
	while (size > 0) {
		if (timo) {
			gettimeofday(&now, NULL);
			if (timercmp(&now, &then, >=)) {
				cc = 0;
			} else {
				timersub(&then, &now, &tv);
				FD_ZERO(&rfds);
				FD_SET(msock, &rfds);
				cc = select(msock+1, &rfds, NULL, NULL, &tv);
			}
			if (cc <= 0) {
				if (cc == 0) {
					errno = ETIMEDOUT;
					cc = -1;
				}
				break;
			}
		}

		cc = read(msock, buf, size);
		if (cc <= 0)
			break;

		size -= cc;
		buf += cc;
	}

	if (size != 0) {
		char *estr = "master server message receive";
		if (cc == 0)
			fprintf(stderr, "%s: Unexpected EOF\n", estr);
		else
			perror(estr);
		return 0;
	}
	return 1;
}

/*
 * Contact the master server to discover download information for imageid.
 * 'sip' and 'sport' are the addr/port of the master server, 'method'
 * specifies the desired download method, 'askonly' is set to just ask
 * for information about the image (without starting a server), 'timeout'
 * is how long to wait for a response.
 *
 * If 'hostip' is not zero, then we are requesting information on behalf of
 * that node.  The calling node (us) must have "proxy" permission on the
 * server for this to work.
 *
 * On success, return non-zero with 'reply' filled in with the server's
 * response IN HOST ORDER.  On failure returns zero.
 */
int
ClientNetFindServer(in_addr_t sip, in_port_t sport,
		    in_addr_t hostip, char *imageid,
		    int method, int askonly, int timeout,
		    GetReply *reply, struct in_addr *myip)
{
	struct sockaddr_in name;
	MasterMsg_t msg;
	int msock, len;
	
	if ((msock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("Could not allocate socket for master server");
		return 0;
	}
	if (sport == 0)
		sport = MS_PORTNUM;

	name.sin_family = AF_INET;
	name.sin_addr.s_addr = htonl(sip);
	name.sin_port = htons(sport);
	if (connect(msock, (struct sockaddr *)&name, sizeof(name)) < 0) {
		perror("Connecting to master server");
		close(msock);
		return 0;
	}

	/*
	 * XXX recover the IP address of the interface used to talk to
	 * the server.
	 */
	if (myip) {
		struct sockaddr_in me;
		socklen_t len = sizeof me;

		if (getsockname(msock, (struct sockaddr *)&me, &len) < 0) {
			perror("getsockname");
			close(msock);
			return 0;
		}
		*myip = me.sin_addr;
	}

	memset(&msg, 0, sizeof msg);
	strncpy((char *)msg.hdr.version, MS_MSGVERS_1,
		sizeof(msg.hdr.version));
	msg.hdr.type = htonl(MS_MSGTYPE_GETREQUEST);
	msg.body.getrequest.hostip = htonl(hostip);
	if (askonly) {
		msg.body.getrequest.status = 1;
		msg.body.getrequest.methods = MS_METHOD_ANY;
	} else {
		msg.body.getrequest.methods = method;
	}
	len = strlen(imageid);
	if (len > MS_MAXIDLEN)
		len = MS_MAXIDLEN;
	msg.body.getrequest.idlen = htons(len);
	strncpy((char *)msg.body.getrequest.imageid, imageid, MS_MAXIDLEN);

	len = sizeof msg.hdr + sizeof msg.body.getrequest;
	if (!MsgSend(msock, &msg, len, timeout)) {
		close(msock);
		return 0;
	}

	memset(&msg, 0, sizeof msg);
	len = sizeof msg.hdr + sizeof msg.body.getreply;
	if (!MsgReceive(msock, &msg, len, timeout)) {
		close(msock);
		return 0;
	}
	close(msock);

	if (strncmp((char *)msg.hdr.version, MS_MSGVERS_1,
		    sizeof(msg.hdr.version))) {
		fprintf(stderr, "Got incorrect version from master server\n");
		return 0;
	}
	if (ntohl(msg.hdr.type) != MS_MSGTYPE_GETREPLY) {
		fprintf(stderr, "Got incorrect reply from master server\n");
		return 0;
	}

	/*
	 * Convert the reply info to host order
	 */
	*reply = msg.body.getreply;
	reply->error = ntohs(reply->error);
	reply->servaddr = ntohl(reply->servaddr);
	reply->addr = ntohl(reply->addr);
	reply->port = ntohs(reply->port);
	reply->sigtype = ntohs(reply->sigtype);
	if (reply->sigtype == MS_SIGTYPE_MTIME)
		*(uint32_t *)reply->signature =
			ntohl(*(uint32_t *)reply->signature);
	reply->hisize = ntohl(reply->hisize);
	reply->losize = ntohl(reply->losize);
	return 1;
}
#endif

