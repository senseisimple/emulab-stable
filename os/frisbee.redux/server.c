/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2011 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Frisbee server
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <assert.h>
#include "decls.h"
#include "queue.h"
#include "utils.h"

#include "trace.h"

/* Globals */
int		debug = 0;
int		tracing = 0;
int		dynburst = 0;
int		timeout = SERVER_INACTIVE_SECONDS;
int		keepalive = 0;
int		readsize = SERVER_READ_SIZE;
volatile int	burstsize = SERVER_BURST_SIZE;
int		maxburstsize = SERVER_DYNBURST_SIZE;
int		burstinterval = SERVER_BURST_GAP;
unsigned long	bandwidth;
int		portnum;
int		killme;
int		blockslost;
int		clientretries;
char		*lostmap;
int		sendretries;
struct in_addr	mcastaddr;
struct in_addr	mcastif;
char	       *filename;
struct timeval  IdleTimeStamp, FirstReq, LastReq;
volatile int	activeclients;

/* Forward decls */
void		quit(int);
void		reinit(int);
static ssize_t	mypread(int fd, void *buf, size_t nbytes, off_t offset);
static void	calcburst(void);
static void	compute_sendrate(void);

#ifdef STATS
/*
 * Track duplicate chunks/joins for stats gathering
 */
char		*chunkmap;

#define MAXCLIENTS 256	/* not a realy limit, just for stats */
struct {
	unsigned int id;
	unsigned int ip;
} clients[MAXCLIENTS];

/*
 * Stats gathering.
 */
struct {
	unsigned long	msgin;
	unsigned long	joins;
	unsigned long	leaves;
	unsigned long	requests;
	unsigned long	joinrep;
	unsigned long	blockssent;
	unsigned long   dupsent;
	unsigned long	filereads;
	unsigned long long filebytes;
	unsigned long long fileusecs;
	unsigned long	filemaxusec;
	unsigned long	partialreq;
	unsigned long	qmerges;
	unsigned long	badpackets;
	unsigned long   blockslost;
	unsigned long	clientlost;
	unsigned long	goesidle;
	unsigned long	wakeups;
	unsigned long	intervals;
	unsigned long	missed;
} Stats;
#define DOSTAT(x)	(Stats.x)
#else
#define DOSTAT(x)
#endif

/*
 * This structure defines the file we are spitting back.
 */
struct FileInfo {
	int	fd;		/* Open file descriptor */
	int	blocks;		/* Number of BLOCKSIZE blocks */
	int	chunks;		/* Number of CHUNKSIZE chunks */
	int	isimage;	/* non-zero if this is a 1MB-rounded file */
	off_t	filesize;	/* Real size of file */
};
static struct FileInfo FileInfo;

/*
 * The work queue of regions a client has requested.
 */
typedef struct {
	queue_chain_t	chain;
	int		chunk;		/* Which chunk */
	int		nblocks;	/* Number of blocks in map */
	BlockMap_t	blockmap;	/* Which blocks of the chunk */
} WQelem_t;
static queue_head_t     WorkQ;
static pthread_mutex_t	WorkQLock;
static int		WorkQDelay = -1;
static int		WorkQSize = 0;
static int		WorkChunk, WorkBlock, WorkCount;
#ifdef STATS
static int		WorkQMax = 0;
static unsigned long	WorkQMaxBlocks = 0;
#endif

/*
 * Work queue routines. The work queue is a time ordered list of chunk/blocks
 * pieces that a client is missing. When a request comes in, lock the list
 * and scan it for an existing work item that covers the new request. The new
 * request can be dropped if there already exists a Q item, since the client
 * is going to see that piece eventually.
 *
 * We use a mutex to guard the work queue.
 *
 * XXX - Clients make requests for chunk/block pieces they are
 * missing. For now, map that into an entire chunk and add it to the
 * work queue. This is going to result in a lot more data being sent
 * than is needed by the client, but lets wait and see if that
 * matters.
 */
static void
WorkQueueInit(void)
{
	pthread_mutex_init(&WorkQLock, NULL);
	queue_init(&WorkQ);

	if (WorkQDelay < 0)
		WorkQDelay = sleeptime(1, NULL, 1);

#ifdef STATS
	chunkmap = calloc(FileInfo.chunks, 1);
#endif
}

/*
 * Enqueue a work request.
 * If map==NULL, then we want the entire chunk.
 */
static int
WorkQueueEnqueue(int chunk, BlockMap_t *map, int count)
{
	WQelem_t	*wqel;
	int		elt, blocks;
#ifdef STATS
	unsigned long	qblocks = 0;
#endif

	if (count == 0)
		return 0;

	pthread_mutex_lock(&WorkQLock);

	/*
	 * Common case: a full chunk request for the full block we are
	 * currently sending.  Don't queue.
	 */
	if (count == MAXCHUNKSIZE && chunk == WorkChunk && count == WorkCount) {
		EVENT(1, EV_WORKMERGE, mcastaddr, chunk, count, count, ~0);
		pthread_mutex_unlock(&WorkQLock);
		return 0;
	}

	elt = WorkQSize - 1;
	queue_riterate(&WorkQ, wqel, WQelem_t *, chain) {
		if (wqel->chunk == chunk) {
			/*
			 * If this is the head element of the queue
			 * we can only merge if the request is beyond
			 * the range being currently processed.
			 */
			if ((WQelem_t *)queue_first(&WorkQ) == wqel &&
			    chunk == WorkChunk &&
			    BlockMapFirst(map) < WorkBlock + WorkCount) {
				elt--;
				continue;
			}

			/*
			 * We have a queued request for the entire chunk
			 * already, nothing to do.
			 */
			if (wqel->nblocks == MAXCHUNKSIZE)
				blocks = 0;
			/*
			 * Or if incoming request is an entire chunk
			 * just copy that map.
			 */
			else if (count == MAXCHUNKSIZE) {
				wqel->blockmap = *map;
				blocks = MAXCHUNKSIZE - wqel->nblocks;
			}
			/*
			 * Otherwise do the full merge
			 */
			else
				blocks = BlockMapMerge(map, &wqel->blockmap);
			EVENT(1, EV_WORKMERGE, mcastaddr,
			      chunk, wqel->nblocks, blocks, elt);
			wqel->nblocks += blocks;
			assert(wqel->nblocks <= MAXCHUNKSIZE);
			pthread_mutex_unlock(&WorkQLock);
			return 0;
		}
#ifdef STATS
		qblocks += wqel->nblocks;
#endif
		elt--;
	}

	wqel = calloc(1, sizeof(WQelem_t));
	if (wqel == NULL)
		fatal("WorkQueueEnqueue: No more memory");

	wqel->chunk = chunk;
	wqel->nblocks = count;
	wqel->blockmap = *map;
	queue_enter(&WorkQ, wqel, WQelem_t *, chain);
	WorkQSize++;
#ifdef STATS
	if (WorkQSize > WorkQMax)
		WorkQMax = WorkQSize;
	if (qblocks > WorkQMaxBlocks)
		WorkQMaxBlocks = qblocks;
#endif

	pthread_mutex_unlock(&WorkQLock);

	EVENT(1, EV_WORKENQ, mcastaddr, chunk, count, WorkQSize, 0);
	return 1;
}

static int
WorkQueueDequeue(int *chunkp, int *blockp, int *countp)
{
	WQelem_t	*wqel;
	int		chunk, block, count;

	pthread_mutex_lock(&WorkQLock);

	/*
	 * Condvars broken in linux threads impl, so use this rather bogus
	 * sleep to keep from churning cycles. 
	 */
	if (queue_empty(&WorkQ)) {
		WorkChunk = -1;
		pthread_mutex_unlock(&WorkQLock);
		fsleep(WorkQDelay);
		return 0;
	}
	
	wqel = (WQelem_t *) queue_first(&WorkQ);
	chunk = wqel->chunk;
	if (wqel->nblocks == MAXCHUNKSIZE) {
		block = 0;
		count = MAXCHUNKSIZE;
	} else
		count = BlockMapExtract(&wqel->blockmap, &block);
	assert(count <= wqel->nblocks);
	wqel->nblocks -= count;
	if (wqel->nblocks == 0) {
		queue_remove(&WorkQ, wqel, WQelem_t *, chain);
		free(wqel);
		WorkQSize--;
	}
	WorkChunk = chunk;
	WorkBlock = block;
	WorkCount = count;

	pthread_mutex_unlock(&WorkQLock);

	*chunkp = chunk;
	*blockp = block;
	*countp = count;

	EVENT(1, EV_WORKDEQ, mcastaddr, chunk, block, count, WorkQSize);
	return 1;
}

static void
ClientEnqueueMap(int chunk, BlockMap_t *map, int count, int isretry)
{
	int		enqueued;

	if (count != MAXCHUNKSIZE) {
		DOSTAT(blockslost+=count);
		blockslost += count;
		DOSTAT(partialreq++);
	}

	enqueued = WorkQueueEnqueue(chunk, map, count);
	if (!enqueued)
		DOSTAT(qmerges++);
#ifdef STATS
	else if (chunkmap != 0 && count == MAXCHUNKSIZE) {
		if (chunkmap[chunk]) {
			if (debug > 1)
				log("Duplicate chunk request: %d", chunk);
			EVENT(1, EV_DUPCHUNK, mcastaddr, chunk, 0, 0, 0);
			DOSTAT(dupsent++);
		} else
			chunkmap[chunk] = 1;
	}
#endif

	if (isretry) {
		clientretries++;
		/*
		 * We only consider the block lost if we didn't have it
		 * on the server queue.  This is a feeble attempt to
		 * filter out rerequests prompted by a long server queue.
		 * Note we only do it at chunk granularity.
		 */
		if (enqueued) {
			if (lostmap)
				lostmap[chunk]++;
			DOSTAT(clientlost++);
		}
	}
}

/*
 * A client joins. We print out the time at which the client joins, and
 * return a reply packet with the number of chunks in the file so that
 * the client knows how much to ask for. We do not do anything else with
 * this info; clients can crash and go away and it does not matter. If they
 * crash they will start up again later. Inactivity is defined as a period
 * with no data block requests. The client will resend its join message
 * until it gets a reply back; duplicates of either the request or the
 * reply are harmless.
 */
static void
ClientJoin(Packet_t *p, int version)
{
	struct in_addr	ipaddr   = { p->hdr.srcip };
	unsigned int    clientid = p->msg.join.clientid;

	/*
	 * Return fileinfo. Duplicates are harmless.
	 */
	EVENT(1, EV_JOINREQ, ipaddr, clientid, version, 0, 0);
	p->hdr.type = PKTTYPE_REPLY;
	if (version == 1) {
		/*
		 * XXX we cannot accept JOINv1 requests if the image we
		 * are serving is not a "traditional" 1MB padded image.
		 * Otherwise, they would ultimately make requests for
		 * parts of the non-rouded file that don't exist and we
		 * would not respond.  We could fake up some data to return
		 * but I'm not sure that is any better.
		 */
		if (!FileInfo.isimage) {
			log("%s requested JOINv1 for non-image file, "
			    "ignoring...", inet_ntoa(ipaddr));
			return;
		}
		p->hdr.datalen = sizeof(p->msg.join);
		p->msg.join.blockcount = FileInfo.blocks;
	} else {
		p->hdr.datalen = sizeof(p->msg.join2);
		p->msg.join2.blockcount = 0;
		p->msg.join2.chunksize = CHUNKSIZE;
		p->msg.join2.blocksize = BLOCKSIZE;
		p->msg.join2.bytecount = FileInfo.filesize;
	}
	PacketReply(p);
#ifdef STATS
	{
		int i, j = -1;

		for (i = 0; i < MAXCLIENTS; i++) {
			if (clients[i].id == clientid) {
				if (clients[i].ip != ipaddr.s_addr) {
					log("%s reuses active client id",
					    inet_ntoa(ipaddr));
					clients[i].ip = ipaddr.s_addr;
				}
				break;
			}
			if (clients[i].ip == ipaddr.s_addr) {
				log("%s rejoins with different cid, ocid=%u",
				    inet_ntoa(ipaddr), clients[i].id);
				clients[i].id = clientid;
				break;
			}
			if (j == -1 && clients[i].id == 0)
				j = i;
		}
		if (i == MAXCLIENTS) {
			activeclients++;
			if (j != -1) {
				clients[j].id = clientid;
				clients[j].ip = ipaddr.s_addr;
			}
		}
	}
	DOSTAT(joinrep++);
#else
	activeclients++;
#endif

	EVENT(1, EV_JOINREP, ipaddr, FileInfo.blocks, 0, 0, 0);

	/*
	 * Log after we send reply so that we get the packet off as
	 * quickly as possible!
	 */
	log("%s (id %u, image %s) joins (v%d) at %s!  %d active clients.",
	    inet_ntoa(ipaddr), clientid, filename, version,
	    CurrentTimeString(), activeclients);
}

/*
 * A client leaves. Not much to it. All we do is print out a log statement
 * about it so that we can see the time. If the packet is lost, no big deal.
 */
static void
ClientLeave(Packet_t *p)
{
	struct in_addr	ipaddr = { p->hdr.srcip };
	unsigned int clientid = p->msg.leave.clientid;

	EVENT(1, EV_LEAVEMSG, ipaddr, clientid, p->msg.leave.elapsed, 0, 0);

#ifdef STATS
	{
		int i;

		for (i = 0; i < MAXCLIENTS; i++)
			if (clients[i].id == clientid) {
				activeclients--;
				clients[i].id = 0;
				clients[i].ip = 0;
				log("%s (id %u, image %s): leaves at %s, "
				    "ran for %d seconds.  %d active clients",
				    inet_ntoa(ipaddr), clientid, filename,
				    CurrentTimeString(),
				    p->msg.leave.elapsed, activeclients);
				break;
			}
		if (i == MAXCLIENTS)
			log("%s (id %u): spurious leave ignored",
			    inet_ntoa(ipaddr), clientid);
	}
#else
	activeclients--;
	log("%s (id %u, image %s): leaves at %s, ran for %d seconds.  "
	    "%d active clients",
	    inet_ntoa(ipaddr), clientid, filename, CurrentTimeString(),
	    p->msg.leave.elapsed, activeclients);
#endif
}

/*
 * A client leaves. Not much to it. All we do is print out a log statement
 * about it so that we can see the time. If the packet is lost, no big deal.
 */
static void
ClientLeave2(Packet_t *p)
{
	struct in_addr	ipaddr = { p->hdr.srcip };
	unsigned int clientid = p->msg.leave2.clientid;

	EVENT(1, EV_LEAVEMSG, ipaddr, clientid, p->msg.leave2.elapsed, 0, 0);

#ifdef STATS
	{
		int i;

		for (i = 0; i < MAXCLIENTS; i++)
			if (clients[i].id == clientid) {
				clients[i].id = 0;
				clients[i].ip = 0;
				activeclients--;
				log("%s (id %u, image %s): leaves at %s, "
				    "ran for %d seconds.  %d active clients",
				    inet_ntoa(ipaddr), clientid, filename,
				    CurrentTimeString(),
				    p->msg.leave2.elapsed, activeclients);
				ClientStatsDump(clientid, &p->msg.leave2.stats);
				break;
			}
		if (i == MAXCLIENTS)
			log("%s (id %u): spurious leave ignored",
			    inet_ntoa(ipaddr), clientid);
	}
#else
	activeclients--;
	log("%s (id %u, image %s): leaves at %s, ran for %d seconds.  "
	    "%d active clients",
	    inet_ntoa(ipaddr), clientid, filename, CurrentTimeString(),
	    p->msg.leave2.elapsed, activeclients);
#endif
}

/*
 * A client requests a chunk/block. Add to the workqueue, but do not
 * send a reply. The client will make a new request later if the packet
 * got lost.
 */
static void
ClientRequest(Packet_t *p)
{
	struct in_addr	ipaddr = { p->hdr.srcip };
	int		chunk = p->msg.request.chunk;
	int		block = p->msg.request.block;
	int		count = p->msg.request.count;
	BlockMap_t	tmp;

	if (count == 0)
		log("WARNING: ClientRequest with zero count");

	EVENT(1, EV_REQMSG, ipaddr, chunk, block, count, 0);
	if (block + count > MAXCHUNKSIZE)
		fatal("Bad request from %s - chunk:%d block:%d size:%d", 
		      inet_ntoa(ipaddr), chunk, block, count);

	BlockMapInit(&tmp, block, count);
	ClientEnqueueMap(chunk, &tmp, count, 0);

	if (debug > 1) {
		log("Client %s requests chunk:%d block:%d size:%d",
		    inet_ntoa(ipaddr), chunk, block, count);
	}
}

/*
 * A client requests a chunk/block. Add to the workqueue, but do not
 * send a reply. The client will make a new request later if the packet
 * got lost.
 */
static void
ClientPartialRequest(Packet_t *p)
{
	struct in_addr	ipaddr = { p->hdr.srcip };
	int		chunk = p->msg.prequest.chunk;
	int		count;

	count = BlockMapIsAlloc(&p->msg.prequest.blockmap, 0, MAXCHUNKSIZE);

	if (count == 0)
		log("WARNING: ClientPartialRequest with zero count");

	EVENT(1, EV_PREQMSG, ipaddr, chunk, count, p->msg.prequest.retries, 0);
	ClientEnqueueMap(chunk, &p->msg.prequest.blockmap, count,
			 p->msg.prequest.retries);

	if (debug > 1) {
		log("Client %s requests %d blocks of chunk:%d",
		    inet_ntoa(ipaddr), count, chunk);
	}
}

/*
 * The server receive thread. This thread does nothing more than receive
 * request packets from the clients, and add to the work queue.
 */
void *
ServerRecvThread(void *arg)
{
	Packet_t	packet, *p = &packet;
	int		idles = 0, kafails = 0;
	static int	gotone;

	if (debug > 1)
		log("Server pthread starting up ...");
	
	/*
	 * Recalculate keepalive interval in terms of packet receive
	 * timeouts for simplicity.
	 */
	if (keepalive)
		keepalive = (int)(((unsigned long long)keepalive * 1000000) /
				  PKTRCV_TIMEOUT);
	while (1) {
		pthread_testcancel();
		if (PacketReceive(p) != 0) {
			if (keepalive && ++idles > keepalive) {
				if (NetMCKeepAlive()) {
					warning("Multicast keepalive failed");
					if (++kafails > 5) {
						warning("too many failures, disabled");
						keepalive = 0;
					}
				} else {
					kafails = 0;
					idles = 0;
					if (debug > 1)
						log("Ping...");
				}
			}
			continue;
		}
		idles = 0;
		DOSTAT(msgin++);

		if (! PacketValid(p, FileInfo.chunks)) {
			struct in_addr ipaddr = { p->hdr.srcip };
			DOSTAT(badpackets++);
			log("bad packet %d/%d from %s, ignored",
			    p->hdr.type, p->hdr.subtype, inet_ntoa(ipaddr));
			if (p->hdr.type == PKTTYPE_REQUEST &&
			    (p->hdr.subtype == PKTSUBTYPE_REQUEST ||
			     p->hdr.subtype == PKTSUBTYPE_PREQUEST))
				log("  len=%d, chunk=%d(%d), word2=%d",
				    p->hdr.datalen, p->msg.request.chunk,
				    FileInfo.chunks, p->msg.request.block);
			continue;
		}
		gettimeofday(&LastReq, 0);
		if (!gotone) {
			FirstReq = LastReq;
			gotone = 1;
		}

		switch (p->hdr.subtype) {
		case PKTSUBTYPE_JOIN:
			DOSTAT(joins++);
			ClientJoin(p, 1);
			break;
		case PKTSUBTYPE_JOIN2:
			DOSTAT(joins++);
			ClientJoin(p, 2);
			break;
		case PKTSUBTYPE_LEAVE:
			DOSTAT(leaves++);
			ClientLeave(p);
			break;
		case PKTSUBTYPE_LEAVE2:
			DOSTAT(leaves++);
			ClientLeave2(p);
			break;
		case PKTSUBTYPE_REQUEST:
			DOSTAT(requests++);
			ClientRequest(p);
			break;
		case PKTSUBTYPE_PREQUEST:
			DOSTAT(requests++);
			ClientPartialRequest(p);
			break;

		}
	}
}

/*
 * The main thread spits out blocks. 
 *
 * NOTES: Perhaps use readv into a vector of packet buffers?
 */
static void
PlayFrisbee(void)
{
	int		chunk, block, blockcount, cc, j, idlelastloop = 0;
	int		startblock, lastblock, throttle = 0;
	Packet_t	packet, *p = &packet;
	char		*databuf;
	off_t		offset;
	struct timeval	startnext;

	if ((databuf = malloc(readsize * MAXBLOCKSIZE)) == NULL)
		fatal("could not allocate read buffer");

	while (1) {
		if (killme) {
			log("Interrupted!");
			break;
		}
		
		/*
		 * Look for a WorkQ item to process. When there is nothing
		 * to process, check for being idle too long, and exit if
		 * no one asks for anything for a long time. Note that
		 * WorkQueueDequeue will delay for a while, so this will not
		 * spin.
		 */
		if (! WorkQueueDequeue(&chunk, &startblock, &blockcount)) {
			struct timeval stamp;

			gettimeofday(&stamp, 0);

			/*
			 * Restart an interval on every idle
			 */
			if (burstinterval > 0) {
				addusec(&startnext, &stamp, burstinterval);
				throttle = 0;
			}

			/* If zero, never exit */
			if (timeout == 0)
				continue;
			
#ifdef STATS
			/* If less than zero, exit when last cilent leaves */
			if (timeout < 0 &&
			    Stats.joins > 0 && activeclients == 0) {
				fsleep(2000000);
				log("Last client left!");
				break;
			}
#endif

			if (idlelastloop) {
				if (timeout > 0 &&
				    stamp.tv_sec - IdleTimeStamp.tv_sec >
				    timeout) {
					log("No requests for %d seconds!",
					    timeout);
					break;
				}
			} else {
				DOSTAT(goesidle++);
				IdleTimeStamp = stamp;
				idlelastloop = 1;
			}
			continue;
		}
		idlelastloop = 0;
		
		lastblock = startblock + blockcount;

		/* Offset within the file */
		offset = (((off_t) MAXBLOCKSIZE * chunk * MAXCHUNKSIZE) +
			  ((off_t) MAXBLOCKSIZE * startblock));

		for (block = startblock; block < lastblock; ) {
			int	readcount;
			int	readbytes;
			int	resends;
			int	thisburst = 0;
			int	resid = 0;
#if defined(NEVENTS) || defined(STATS)
			struct timeval rstamp;
			gettimeofday(&rstamp, 0);
#endif

			/*
			 * Read blocks of data from disk.
			 */
			if (lastblock - block > readsize)
				readcount = readsize;
			else
				readcount = lastblock - block;
			readbytes = readcount * MAXBLOCKSIZE;

			/*
			 * Check for final partial block and truncate
			 * read if necessary.  This should only happen
			 * for the last block of a file, a request beyond
			 * that is an error (and do what?)
			 */
			if (offset+readbytes > FileInfo.filesize) {
				off_t diff =
					(offset+readbytes) - FileInfo.filesize;
				if (!FileInfo.isimage &&
				    diff < (off_t)MAXBLOCKSIZE) {
					readbytes -= diff;
					resid = (int)diff;
				} else {
					warning("Attempt to read beyond EOF "
						"(offset %llu > %llu)\n",
						offset+readbytes,
						FileInfo.filesize);
					/* XXX just do not respond */
					break;
				}
			}

			if ((cc = mypread(FileInfo.fd, databuf,
					  readbytes, offset)) <= 0) {
				if (cc < 0)
					pfatal("Reading File");
				fatal("EOF on file");
			}
#ifdef STATS
			{
				struct timeval now;
				int us;
				gettimeofday(&now, 0);
				timersub(&now, &rstamp, &now);
				us = now.tv_sec * 1000000 + now.tv_usec;
				assert(us >= 0);
				Stats.fileusecs += us;
				if (us > Stats.filemaxusec)
					Stats.filemaxusec = us;
			}
#endif
			DOSTAT(filereads++);
			DOSTAT(filebytes += cc);
			EVENT(2, EV_READFILE, mcastaddr,
			      offset, readbytes, rstamp.tv_sec, rstamp.tv_usec);
			if (cc != readbytes)
				fatal("Short read: %d!=%d", cc, readbytes);

			for (j = 0; j < readcount; j++) {
				p->hdr.type    = PKTTYPE_REQUEST;
				p->hdr.subtype = PKTSUBTYPE_BLOCK;
				p->hdr.datalen = sizeof(p->msg.block);
				p->msg.block.chunk = chunk;
				p->msg.block.block = block + j;

				/*
				 * If final block is short, pad it out.
				 * The receiver knows the exact size and
				 * can compensate.
				 */
				if (resid && j == readcount-1) {
					int count = MAXBLOCKSIZE - resid;
					memcpy(p->msg.block.buf,
					       &databuf[j * MAXBLOCKSIZE],
					       count);
					memset(&p->msg.block.buf[count],
					       0, resid);
					if (debug)
						log("Handle partial final block, padded with %d bytes", resid);
				} else {
					memcpy(p->msg.block.buf,
					       &databuf[j * MAXBLOCKSIZE],
					       MAXBLOCKSIZE);
				}
				PacketSend(p, &resends);
				sendretries += resends;
				DOSTAT(blockssent++);
				EVENT(resends ? 1 : 3, EV_BLOCKMSG, mcastaddr,
				      chunk, block+j, resends, 0);

				/*
				 * Completed a burst.  Adjust the busrtsize
				 * if necessary and delay as required.
				 */
				if (burstinterval > 0 &&
				    ++throttle >= burstsize) {
					thisburst += throttle;

					/*
					 * XXX if we overran our interval, we
					 * reset the base time so we don't
					 * accumulate error.
					 */
					if (!sleeptil(&startnext)) {
						EVENT(1, EV_OVERRUN, mcastaddr,
						      startnext.tv_sec,
						      startnext.tv_usec,
						      chunk, block+j);
						gettimeofday(&startnext, 0);
						DOSTAT(missed++);
					} else {
						if (thisburst > burstsize)
							EVENT(1, EV_LONGBURST,
							      mcastaddr,
							      thisburst,
							      burstsize,
							      chunk, block+j);
						thisburst = 0;
					}
					if (dynburst)
						calcburst();
					addusec(&startnext, &startnext,
						burstinterval);
					throttle = 0;
					DOSTAT(intervals++);
				}
			}
			offset   += readbytes;
			block    += readcount;
		}
	}
	free(databuf);
}

char *usagestr = 
 "usage: frisbeed [-d] <-p #> <-m mcastaddr> <filename>\n"
 " -d              Turn on debugging. Multiple -d options increase output.\n"
 " -p portnum      Specify a port number to listen on.\n"
 " -m mcastaddr    Specify a multicast address in dotted notation.\n"
 " -i mcastif      Specify a multicast interface in dotted notation.\n"
 " -b              Use broadcast instead of multicast\n"
 "\n";

void
usage()
{
	fprintf(stderr, "%s", usagestr);
	exit(1);
}

int
main(int argc, char **argv)
{
	int		ch, fd;
	pthread_t	child_pid;
	off_t		fsize;
	void		*ignored;

	while ((ch = getopt(argc, argv, "dhp:m:i:tbDT:R:B:G:L:W:K:")) != -1)
		switch(ch) {
		case 'b':
			broadcast++;
			break;
			
		case 'd':
			debug++;
			break;
			
		case 'p':
			portnum = atoi(optarg);
			break;
			
		case 'm':
			inet_aton(optarg, &mcastaddr);
			break;

		case 'i':
			inet_aton(optarg, &mcastif);
			break;
		case 't':
			tracing++;
			break;
		case 'D':
			dynburst = 1;
			break;
		case 'T':
			timeout = atoi(optarg);
			break;
		case 'R':
			readsize = atoi(optarg);
			if (readsize == 0 || readsize > MAXCHUNKSIZE) {
				warning("readsize set to %d", MAXCHUNKSIZE);
				readsize = MAXCHUNKSIZE;
			}
			break;
		case 'B':
			burstsize = atoi(optarg);
			break;
		case 'G':
			burstinterval = atoi(optarg);
			break;
		case 'W':
			bandwidth = atol(optarg);
			break;
		case 'K':
			keepalive = atoi(optarg);
			if (keepalive < 0)
				keepalive = 0;
			break;
		case 'h':
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;
	if (argc != 1)
		usage();

	if (!portnum || !mcastaddr.s_addr)
		usage();

	if (timeout > 0 && keepalive > timeout) {
		warning("keepalive > timeout, disabling keepalive");
		keepalive = 0;
	}

	signal(SIGINT, quit);
	signal(SIGTERM, quit);
	signal(SIGHUP, reinit);

	ServerLogInit();
	
	filename = argv[0];
	if (access(filename, R_OK) < 0)
		pfatal("Cannot read %s", filename);

	/*
	 * Open the file and get its size so that we can tell clients how
	 * much to expect/require.
	 */
	if ((fd = open(filename, O_RDONLY)) < 0)
		pfatal("Cannot open %s", filename);

	if ((fsize = lseek(fd, (off_t)0, SEEK_END)) < 0)
		pfatal("Cannot lseek to end of file");

	InitSizes(MAXCHUNKSIZE, MAXBLOCKSIZE, fsize);
	FileInfo.fd = fd;
	FileInfo.filesize = fsize;
	FileInfo.blocks = TotalBlocks();
	FileInfo.chunks = TotalChunks();
	if ((FileInfo.filesize % (BLOCKSIZE * CHUNKSIZE)) == 0)
		FileInfo.isimage = 1;
	else
		warning("NOTE: serving non-image file, will ignore V1 JOINs");

	log("Opened %s: %d blocks (%lld bytes)",
	    filename, FileInfo.blocks, FileInfo.filesize);

	compute_sendrate();

	WorkQueueInit();
	lostmap = calloc(FileInfo.chunks, 1);

	/*
	 * Everything else done, now init the network.
	 */
	ServerNetInit();

	if (tracing) {
		ServerTraceInit("frisbeed");
		TraceStart(tracing);
	}

	/*
	 * Create the subthread to listen for packets.
	 */
	if (pthread_create(&child_pid, NULL, ServerRecvThread, (void *)0)) {
		fatal("Failed to create pthread!");
	}
	gettimeofday(&IdleTimeStamp, 0);
	
	PlayFrisbee();
	pthread_cancel(child_pid);
	pthread_join(child_pid, &ignored);

	if (tracing) {
		TraceStop();
		TraceDump(1, tracing);
	}
	subtime(&LastReq, &LastReq, &FirstReq);

#ifdef  STATS
	{
		struct rusage ru;
		extern unsigned long nonetbufs;

		getrusage(RUSAGE_SELF, &ru);
		log("Params:");
		log("  chunk/block size    %d/%d", MAXCHUNKSIZE, MAXBLOCKSIZE);
		log("  burst size/interval %d/%d", burstsize, burstinterval);
		log("  file read size      %d", readsize);
		log("  file:size           %s:%qd",
		    filename, (long long)fsize);
		log("Stats:");
		log("  service time:      %d.%03d sec",
		    LastReq.tv_sec, LastReq.tv_usec/1000);
		log("  user/sys CPU time: %d.%03d/%d.%03d",
		    ru.ru_utime.tv_sec, ru.ru_utime.tv_usec/1000,
		    ru.ru_stime.tv_sec, ru.ru_stime.tv_usec/1000);
		log("  msgs in/out:       %d/%d",
		    Stats.msgin, Stats.joinrep + Stats.blockssent);
		log("  joins/leaves:      %d/%d", Stats.joins, Stats.leaves);
		log("  requests:          %d (%d merged in queue)",
		    Stats.requests, Stats.qmerges);
		log("  partial req/blks:  %d/%d",
		    Stats.partialreq, Stats.blockslost);
		log("  duplicate req:     %d",
		    Stats.dupsent);
		log("  client re-req:     %d",
		    Stats.clientlost);
		log("  %dk blocks sent:    %d (%d repeated)",
		    (MAXBLOCKSIZE/1024),
		    Stats.blockssent, Stats.blockssent ?
		    (Stats.blockssent-FileInfo.blocks) : 0);
		log("  file reads:        %d (%qu bytes, %qu repeated)",
		    Stats.filereads, Stats.filebytes, Stats.filebytes ?
		    (Stats.filebytes - FileInfo.filesize) : 0);
		log("  file read time:    %d.%03d sec (%llu us/op, %d us max)",
		    (int)(Stats.fileusecs / 1000000),
		    (int)((Stats.fileusecs % 1000000) / 1000),
		    Stats.fileusecs / (Stats.filereads ? Stats.filereads : 1),
		    Stats.filemaxusec);
		log("  net idle/blocked:  %d/%d", Stats.goesidle, nonetbufs);
		log("  send intvl/missed: %d/%d",
		    Stats.intervals, Stats.missed);
		log("  spurious wakeups:  %d", Stats.wakeups);
		log("  max workq size:    %d elts, %lu blocks",
		    WorkQMax, WorkQMaxBlocks);
	}
#endif

	/*
	 * Exit from main thread will kill all the children.
	 */
	log("Exiting!");
	exit(0);
}

/*
 * We catch the signals, but do not do anything. We exit with 0 status
 * for these, since it indicates a desired shutdown.
 */
void
quit(int sig)
{
	killme = 1;
}

/*
 * We cannot reinit, so exit with non-zero to indicate it was unexpected.
 */
void
reinit(int sig)
{
	log("Caught signal %d. Exiting ...", sig);
	exit(1);
}

#define NFS_READ_DELAY	100000

/*
 * Wrap up pread with a retry mechanism to help protect against
 * transient NFS errors.
 */
static ssize_t
mypread(int fd, void *buf, size_t nbytes, off_t offset)
{
	int		cc, i, count = 0;

	while (nbytes) {
		int	maxretries = 100;

		for (i = 0; i < maxretries; i++) {
			cc = pread(fd, buf, nbytes, offset);
			if (cc == 0)
				fatal("EOF on file");

			if (cc > 0) {
				nbytes -= cc;
				buf    += cc;
				offset += cc;
				count  += cc;
				goto again;
			}

			if (i == 0)
				pwarning("read error: will retry");

			fsleep(NFS_READ_DELAY);
		}
		pfatal("read error: busted for too long");
		return -1;
	again:
		;
	}
	return count;
}

#define LOSS_INTERVAL	250	/* interval in which we collect data (ms) */
#define MULT_DECREASE	0.95	/* mult factor to decrease burst rate */
#define ADD_INCREASE	1	/* add factore to increase burst rate */

#define CHUNK_LIMIT	0

/*
 * Should we consider PacketSend retries?   They indicated that we are over
 * driving the socket?  Even though they have a builtin delay between retries,
 * we might be better off detecting the case and avoiding the delays.
 *
 * From Dave:
 *
 * A smoother one that is still fair with TCP is:
 *    W_{next} = W_{cur} - sqrt( W_{cur} ) if loss
 *    W_{next} = W_{cur} + 1 / sqrt( W_{cur} )  if no loss
 */
static void
calcburst(void)
{
	static int		lastsendretries, bursts, lastclientretries;
	static struct timeval	nextstamp;
	struct timeval		stamp;
	int			clientlost, lostchunks, hadloss = 0;

	gettimeofday(&stamp, 0);
	if (nextstamp.tv_sec == 0) {
		addusec(&nextstamp, &stamp, LOSS_INTERVAL * 1000);
		return;
	}

	bursts++;

	/*
	 * Has a full interval passed?
	 */
	if (!pasttime(&stamp, &nextstamp))
		return;

	/*
	 * An interval has past, now what constitiues a significant loss?
	 * The number of explicit client retry requests during the interval
	 * is the basis right now.
	 */
	clientlost = clientretries - lastclientretries;

	/*
	 * If we are overrunning our UDP socket then we are certainly
	 * transmitting too fast.  Allow one overrun per burst.
	 */
	if (sendretries - lastsendretries > bursts)
		hadloss = 1;

	lostchunks = 0;
	if (lostmap) {
		int i;
		for (i = 0; i < FileInfo.chunks; i++)
			if (lostmap[i]) {
				lostchunks++;
				lostmap[i] = 0;
			}
	}

	if (lostchunks > CHUNK_LIMIT)
		hadloss = 1;

	if (debug && hadloss)
		log("%d client retries for %d chunks from %d clients, "
		    "%d overruns in %d bursts",
		    clientlost, lostchunks, activeclients,
		    sendretries-lastsendretries, bursts);

	if (hadloss) {
		/*
		 * Decrement the burstsize slowly.
		 */
		if (burstsize > 1) {
			burstsize = (int)(burstsize * MULT_DECREASE);
			if (burstsize < 1)
				burstsize = 1;
			if (debug)
				log("Decrement burstsize to %d", burstsize);
		}
	} else {
		/*
		 * Increment the burstsize even more slowly.
		 */
		if (burstsize < maxburstsize) {
			burstsize += ADD_INCREASE;
			if (burstsize > maxburstsize)
				burstsize = maxburstsize;
			if (debug)
				log("Increment burstsize to %d", burstsize);
		}
	}

	/*
	 * Update for next time
	 */
	addusec(&nextstamp, &nextstamp, LOSS_INTERVAL * 1000);
	lastclientretries = clientretries;
	lastsendretries = sendretries;
	bursts = 0;
}

#define LINK_OVERHEAD	(14+4+8+12)	/* ethernet (hdr+CRC+preamble+gap) */
#define IP_OVERHEAD	(20+8)		/* IP + UDP hdrs */

/*
 * Compute the approximate send rate.  Due to typically coarse grained
 * timers, send rate is implemented as a burst rate and a burst interval;
 * i.e. we put out "burst size" blocks every "burst interval" microseconds.
 * The user can specify either an aggregate bandwidth (bandwidth) or the
 * individual components (burstsize, burstinterval).
 */
static void
compute_sendrate(void)
{
	double blockspersec, burstspersec;
	int clockres, wireblocksize, minburst;

	if (burstinterval == 0) {
		burstsize = 1;
		log("Maximum send bandwidth unlimited");
		return;
	}

	/* clock resolution in usec */
	clockres = sleeptime(1, 0, 1);

	burstspersec = 1000000.0 / clockres;
	wireblocksize = (sizeof(Packet_t) + IP_OVERHEAD + LINK_OVERHEAD) * 8;

	if (bandwidth != 0) {
		/*
		 * Convert Mbits/sec to blocks/sec
		 */
		blockspersec = bandwidth / wireblocksize;

		/*
		 * If blocks/sec less than maximum bursts/sec,
		 * crank down the clock.
		 */
		if (blockspersec < burstspersec)
			burstspersec = blockspersec;

		burstsize = blockspersec / burstspersec;
		burstinterval = (int)(1000000 / burstspersec);
	}

	burstinterval = sleeptime(burstinterval, 0, 1);
	burstspersec = 1000000.0 / burstinterval;
	bandwidth = (unsigned long)(burstspersec*burstsize*wireblocksize);

	/*
	 * For the dynamic rate throttle, we use the standard parameters
	 * as a cap.  We adjust the burstsize to ensure it is large
	 * enough to ensure a reasonable starting multiplicitive decrement.
	 * If we cannot do that while still maintaining a reasonable
	 * burstinterval (< 0.5 seconds), just cancel the dynamic behavior.
	 */
	minburst = (int)(4.0 / (1.0 - MULT_DECREASE));
	if (dynburst && burstsize < minburst) {
		double burstfactor = (double)minburst / burstsize;

		if (burstinterval * burstfactor < 500000) {
			burstsize = minburst;
			burstinterval =
				sleeptime((int)burstinterval*burstfactor,
					  0, 1);
			burstspersec = (double)1000000.0 / burstinterval;
			bandwidth = (unsigned long)
				(burstspersec*burstsize*wireblocksize);
		} else
			dynburst = 0;
	}
	if (dynburst)
		maxburstsize = burstsize;

	if (burstsize * sizeof(Packet_t) > GetSockbufSize()) {
		warning("NOTE: burst size exceeds socket buffer size, "
			"may drop packets");
	}

	log("Maximum send bandwidth %.3f Mbits/sec (%d blocks/sec)",
	    bandwidth / 1000000.0, bandwidth / wireblocksize);
	if (debug)
		log("  burstsize=%d, burstinterval=%dus",
		    burstsize, burstinterval);
}
