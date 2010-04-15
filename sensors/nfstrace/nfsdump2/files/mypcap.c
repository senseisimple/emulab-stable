/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <linuxthreads/pthread.h>
#include <pcap-int.h>

#include "listNode.h"

#define MPD_BUFFER_COUNT 128

static struct {
	pthread_t mpd_reader;
	pthread_mutex_t mpd_mutex;
	pthread_cond_t mpd_cond;
	pcap_handler mpd_callback;
	struct lnMinList mpd_buflist;
	struct lnMinList mpd_freelist;
	char *mpd_buffer[MPD_BUFFER_COUNT];
	int mpd_buflen[MPD_BUFFER_COUNT];
	struct lnMinNode mpd_nodes[MPD_BUFFER_COUNT];
} mypcap_data;

static void *mypcap_reader(void *pcap)
{
	struct lnMinNode *next_node = NULL;
	pcap_t *p = pcap;
	
	while (1) {
		int cc, idx;

		pthread_mutex_lock(&mypcap_data.mpd_mutex);
		if (next_node != NULL) {
			lnAddTail(&mypcap_data.mpd_buflist, next_node);
			pthread_cond_signal(&mypcap_data.mpd_cond);
		}
		if ((next_node = lnRemHead(&mypcap_data.mpd_freelist))
		    == NULL) {
			next_node = lnRemHead(&mypcap_data.mpd_buflist);
		}
		pthread_mutex_unlock(&mypcap_data.mpd_mutex);

		idx = next_node - mypcap_data.mpd_nodes;
	again:
		cc = read(p->fd,
			  (char *)mypcap_data.mpd_buffer[idx],
			  p->bufsize);
		if (cc < 0) {
			/* Don't choke when we get ptraced */
			switch (errno) {
			case EINTR:
				goto again;
				
			case EWOULDBLOCK:
				return (0);
			}
			snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "read: %s",
				 pcap_strerror(errno));
			return NULL;
		}
		mypcap_data.mpd_buflen[idx] = cc;
	}
}

int mypcap_init(pcap_t *pd, pcap_handler callback)
{
	int retval = 0;
	
	mypcap_data.mpd_callback = callback;

	if (pthread_mutex_init(&mypcap_data.mpd_mutex, NULL) != 0) {
		fprintf(stderr, "error: pthread_mutex_init\n");
	}
	else if (pthread_cond_init(&mypcap_data.mpd_cond, NULL) != 0) {
		fprintf(stderr, "error: pthread_cond_init\n");
	}
	else if (pthread_create(&mypcap_data.mpd_reader,
				NULL,
				mypcap_reader,
				pd) != 0) {
		fprintf(stderr, "error: pthread_cond_init\n");
	}
	else {
		int lpc;

		lnNewList(&mypcap_data.mpd_buflist);
		lnNewList(&mypcap_data.mpd_freelist);
		retval = 1;
		for (lpc = 0; lpc < MPD_BUFFER_COUNT && retval; lpc++) {
			if ((mypcap_data.mpd_buffer[lpc] =
			     malloc(pd->bufsize)) == NULL) {
				retval = 0;
			}
			lnAddTail(&mypcap_data.mpd_freelist,
				  &mypcap_data.mpd_nodes[lpc]);
		}
	}
	
	return retval;
}

int mypcap_read(pcap_t *pd, void *user)
{
	static struct lnMinNode *node = NULL;
	int idx, cc, retval = 0;
	char *bp, *ep;

	pthread_mutex_lock(&mypcap_data.mpd_mutex);
	if (node != NULL) {
		lnAddHead(&mypcap_data.mpd_freelist, node);
	}
	while ((node = lnRemHead(&mypcap_data.mpd_buflist)) == NULL) {
		pthread_cond_wait(&mypcap_data.mpd_cond,
				  &mypcap_data.mpd_mutex);
	}
	pthread_mutex_unlock(&mypcap_data.mpd_mutex);

	idx = node - mypcap_data.mpd_nodes;
	bp = mypcap_data.mpd_buffer[idx];
	cc = mypcap_data.mpd_buflen[idx];
	/*
	 * Loop through each packet.
	 */
#define bhp ((struct bpf_hdr *)bp)
	ep = bp + cc;
	while (bp < ep) {
		register int caplen, hdrlen;
		caplen = bhp->bh_caplen;
		hdrlen = bhp->bh_hdrlen;
		/*
		 * XXX A bpf_hdr matches a pcap_pkthdr.
		 */
		(*mypcap_data.mpd_callback)(user,
					    (struct pcap_pkthdr*)bp,
					    bp + hdrlen);
		bp += BPF_WORDALIGN(caplen + hdrlen);
	}
#undef bhp

	return retval;
}
