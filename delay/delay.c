/*
 * Delay packets using divert sockets in FreeBSD or the OSKit with
 * the FreeBSD 3.0 libs.  Uses some functions from my divert sockets
 * library to set up the ipfw rules and perform socket accounting.
 *
 * Inefficient, and the timers have potentially poor granularity.
 * Nevertheless, it should be sufficient for large-scale delays
 * (on the order of 10s of miliseconds), but will *not* work for
 * delays < 1ms, for sure.
 *
 * Author:  Dave Andersen <danderse@cs.utah.edu> <angio@pobox.com>
 * 1999-08-27
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>
#include <assert.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include "divert.h"

struct msockinfo *msock, *outsock;

#define DPRINTF(fmt, args...) /* Shut up */

/*
 * I'm lazy, but want to see if an allocation fails...
 */

inline void *C_MALLOC(size_t size) {
	void *tmp;
	tmp = malloc(size);
	assert(tmp != NULL);
	return tmp;
}

struct packet *phead = NULL;
struct packet *ptail = NULL;

int sniffsock;

struct packet {
	char *data;
	int datalen;
	long usecs;
	struct sockaddr_in from;
	int fromlen;
	struct packet *next;
};

/*
 * Put a packet at the end of the waiting queue.
 *
 * Right now, I simply run the full list at each clock tick, which is pretty
 * inefficient, but this is a prototype. :-)  It would be better to be more
 * intelligent about this - keep a counter of the times elapsed, process
 * packets until we hit one that isn't ready to go out, and then just
 * increment that counter.  Then start running the list with the counter.
 * Would take more accounting, of course.
 *
 * Or just use the TCP timeout counters from the kernel. :)
 */

int packet_enq(char *data, int datalen, int usecs, struct sockaddr_in from,
	       int fromlen)
{
	struct packet *tmp;
	
	tmp = C_MALLOC(sizeof(*tmp));
	if (ptail) {
		ptail->next = tmp;
		ptail = tmp;
	} else {
		phead = ptail = tmp;
	}
	ptail->data = data;
        ptail->datalen = datalen;
        ptail->usecs = usecs;
	ptail->from = from;
	ptail->fromlen = fromlen;
	ptail->next = NULL;
	return 0;
}

/*
 * Put the packet back on the wire, unmodified.
 */

void packet_forw(struct packet *packet)
{
	int wlen;
#if 0
	printf("Sock: %d, len: %d, fromlen: %d\n",
	       msock->sock, packet->datalen, packet->fromlen);
	IPDUMP((struct ip *)(packet->data));
#endif
	wlen = sendto(msock->sock,
		      packet->data,
		      packet->datalen,
		      0,
		      (struct sockaddr *)&(packet->from),
		      packet->fromlen);
	if (wlen <= 0) {
		perror("sendto");
	}
#if 0
	printf("Forwarded, got %d\n", wlen);
#endif
}

/*
 * Process the packet queue, forwarding any packets which are ready for
 * forwarding, and removing them from the queue.
 */

int pq_run(int usecs) {
	struct packet *tmp;
	struct packet *prev = NULL;
	struct packet *next = NULL;
	
	tmp = phead;

	while (tmp != NULL) {
		next = tmp->next;
		tmp->usecs -= usecs;
		if (tmp->usecs <= 0) {
			packet_forw(tmp);
			if (prev) {
				prev->next = next;
			} else {
				if (tmp == phead) {
					phead = next;
				}
			}
			if (tmp == ptail) {
				ptail = NULL;
			}
			free(tmp->data);
			free(tmp);
			tmp = NULL;
		} else {
			prev = tmp;
		}
		if (!prev && !next) {
			phead = ptail = NULL;
			return 0;
		}
		tmp = next;
	}
	return 0;
}

/*
 * Close things down, delete the ipfw mappings, and exit
 */

void cleanup(int sig) {
	DPRINTF("Sig %d caught\n", sig);
	free_socket(msock);
	exit(0);
}

/*
 * Return the # of microseconds difference between two
 * timevals.  Note that this could overflow if you start
 * dealing with large delays, but 35 minutes is an awfully
 * long time to delay a packet. :-)
 */

long timediff(struct timeval *time_now, struct timeval *time_prev)
{
	long usecs_now;
	
	if (time_prev->tv_sec == 0 &&
	    time_prev->tv_usec == 0) {
		return 0;
	}
	
	usecs_now = (time_now->tv_sec - time_prev->tv_sec) * 1000000;
	usecs_now += time_now->tv_usec;

	return usecs_now - time_prev->tv_usec;
}

/*
 * Set up our sockets, and start the receive / enqueue / run queue
 * loop
 */

int main(int argc, char **argv) {
	int port = 69;
	char *packetbuf = NULL;
	int granularity; /* Kind of */
	int delay = 1000000;
	struct timeval mytv, time_prev, time_now;
	int rc;

	time_prev.tv_sec = time_prev.tv_usec = 0;

	if (argc > 1) {
		port = atoi(argv[1]);
	}

	if (argc > 2) {
		delay = atoi(argv[2]);
	}

	signal(SIGTERM, cleanup);
	signal(SIGQUIT, cleanup);
	signal(SIGINT, cleanup);

	msock = get_socket();
	
	add_mask_port(msock, port);

	granularity = delay / 100;
	mytv.tv_sec = granularity / 1000000;
	mytv.tv_usec = granularity % 1000000;

	/* Do our delays by simply breaking out of the recvfrom every
	 * now and then.  Note that this is potentially INACCURATE if
	 * we're getting a trickle of data - the timeout is actually
	 * an inactivity timer, so the "right pace" of data arriving could
	 * throw things off.  Not to mention, we're only accurate to about
	 * 2% of the timeout anyway.  We should be able to squeeze more
	 * accuracy out of this by decreasing the granularity, at the
	 * expense of effectively spinning waiting for data...
	 *
	 * I've tested it with a 1ms delay (and 10us granularity) and
	 * it doesn't seem to kill things.
	 *
	 * Yes, better ways exist and should be used.
	 */
	rc = setsockopt(msock->sock, SOL_SOCKET, SO_RCVTIMEO,
			&mytv, sizeof(mytv));

	if (rc) {
		perror("Setsockopt for timeout failed");
		exit(errno);
	}

	/*
	 * The meat:
	 *    1 - Receive any packets.  Enqueue them if necessary.
	 *    2 - Run the queue.
	 *    3 - Loop dat puppy.
	 *
	 * Note that order matters here - you have to enqueue the packets
	 * after running the queue, or they don't rest long enough.
	 */

	while (1) {
		struct sockaddr_in from;
		int fromlen;
		int len;

		if (!packetbuf) {
			packetbuf = (char *)malloc(MAXPACKET);
		}

		fromlen = sizeof(struct sockaddr_in);
		len = recvfrom(msock->sock, packetbuf, MAXPACKET, 0,
			       (struct sockaddr *)&from, &fromlen);

		gettimeofday(&time_now, NULL);
		
		pq_run(timediff(&time_now, &time_prev));

		time_prev = time_now;
		if (len <= 0) {
			continue;
		}

		packet_enq(packetbuf, len, delay, from, fromlen);
		packetbuf = NULL;

	}
	free_socket(msock);
	DPRINTF("Exiting\n");
	exit(0);
}

