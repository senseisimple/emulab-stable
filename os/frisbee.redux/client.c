/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Frisbee client.
 *
 * TODO: Deal with a dead server. Its possible that too many clients
 * could swamp the boss with unanswerable requests. Might need some 
 * backoff code.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <stdarg.h>
#include <pthread.h>
#include "decls.h"
#include "trace.h"

#ifdef DOEVENTS
#include "event.h"

static char *eventserver;
static Event_t event;
static int exitstatus;
#endif

#ifdef DOLOSSRATE
int		lossrate;
#endif

/* Tunable constants */
int		maxchunkbufs = MAXCHUNKBUFS;
int		pkttimeout = PKTRCV_TIMEOUT;
int		idletimer = CLIENT_IDLETIMER_COUNT;
int		maxreadahead = MAXREADAHEAD;
int		maxinprogress = MAXINPROGRESS;
int		redodelay = CLIENT_REQUEST_REDO_DELAY;
int		idledelay = CLIENT_WRITER_IDLE_DELAY;
int		startdelay = 0;

int		debug = 0;
int		tracing = 0;
char		traceprefix[64];
int		zero = 0;
int		randomize = 1;
int		slice = 0;
int		nothreads = 0;
int		portnum;
struct in_addr	mcastaddr;
struct in_addr	mcastif;
static struct timeval  stamp;
static int	dotcol;

/* Forward Decls */
static void	PlayFrisbee(void);
static void	GotBlock(Packet_t *p);
static void	RequestChunk(int timedout);
static int	ImageUnzip(int chunk);
static void	RequestStamp(int chunk, int block, int count);
static int	RequestRedoTime(int chunk, unsigned long long curtime);
extern int	ImageUnzipInit(char *filename, int slice,
			       int debug, int zero, int nothreads);
extern void	ImageUnzipFlush(void);
extern int	ImageUnzipQuit(void);

/*
 * Chunk descriptor, one for each CHUNKSIZE*BLOCKSIZE bytes of an image file.
 * For each chunk, record its state and the time at which it was last
 * requested by someone.
 */
typedef struct {
	unsigned long long done:1;
	unsigned long long lastreq:63;
} Chunk_t;

/*
 * The chunker data structure. For each chunk in progress, we maintain this
 * array of blocks (plus meta info). This serves as a cache to receive
 * blocks from the server while we write completed chunks to disk. The child
 * thread reads packets and updates this cache, while the parent thread
 * simply looks for completed blocks and writes them. The "inprogress" slot
 * serves a free/allocated flag, while the ready bit indicates that a chunk
 * is complete and ready to write to disk.
 */
typedef struct {
	int	thischunk;		/* Which chunk in progress */
	int	inprogress;		/* Chunk in progress? Free/Allocated */
	int	ready;			/* Ready to write to disk? */
	int	blockcount;		/* Number of blocks not received yet */
	char	bitmap[CHUNKSIZE];	/* Which blocks have been received */
	struct {
		char	data[BLOCKSIZE];
	} blocks[CHUNKSIZE];		/* Actual block data */
} ChunkBuffer_t;

Chunk_t		*Chunks;		/* Chunk descriptors */
ChunkBuffer_t   *ChunkBuffer;		/* The cache */
int		*ChunkRequestList;	/* Randomized chunk request order */
int		TotalChunkCount;	/* Total number of chunks in file */
int		IdleCounter;		/* Countdown to request more data */

#ifdef STATS
ClientStats_t	Stats;
#define DOSTAT(x)	(Stats.u.v1.x)
#else
#define DOSTAT(x)
#endif

char *usagestr = 
 "usage: frisbee [-drzbn] [-s #] <-p #> <-m ipaddr> <output filename>\n"
 " -d              Turn on debugging. Multiple -d options increase output.\n"
 " -r              Randomly delay first request by up to one second.\n"
 " -z              Zero fill unused block ranges (default is to seek past).\n"
 " -b              Use broadcast instead of multicast\n"
 " -n              Do not use extra threads in diskwriter\n"
 " -p portnum      Specify a port number.\n"
 " -m mcastaddr    Specify a multicast address in dotted notation.\n"
 " -i mcastif      Specify a multicast interface in dotted notation.\n"
 " -s slice        Output to DOS slice (DOS numbering 1-4)\n"
 "                 NOTE: Must specify a raw disk device for output filename.\n"
 "\n";

void
usage()
{
	fprintf(stderr, usagestr);
	exit(1);
}

int
main(int argc, char **argv)
{
	int	ch;
	char   *filename;

	while ((ch = getopt(argc, argv, "dhp:m:s:i:tbznT:r:E:")) != -1)
		switch(ch) {
		case 'd':
			debug++;
			break;
			
		case 'b':
			broadcast++;
			break;
			
#ifdef DOEVENTS
		case 'E':
			eventserver = optarg;
			break;
#endif

		case 'p':
			portnum = atoi(optarg);
			break;
			
		case 'm':
			inet_aton(optarg, &mcastaddr);
			break;

		case 'n':
			nothreads++;
			break;

		case 'i':
			inet_aton(optarg, &mcastif);
			break;

		case 'r':
			startdelay = atoi(optarg);
			break;

		case 's':
			slice = atoi(optarg);
			break;

		case 't':
			tracing++;
			break;

		case 'T':
			strncpy(traceprefix, optarg, sizeof(traceprefix));
			break;

		case 'z':
			zero++;
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
	filename = argv[0];

	if (!portnum || ! mcastaddr.s_addr)
		usage();

	ClientLogInit();
	ClientNetInit();

#ifdef DOEVENTS
	if (eventserver != NULL && EventInit(eventserver) != 0) {
		log("Failed to initialize event system, events ignored");
		eventserver = NULL;
	}
	if (eventserver != NULL) {
		log("Waiting for START event...");
		EventWait(EV_ANY, &event);
		if (event.type != EV_START)
			goto done;

	again:
		if (event.data.start.startdelay > 0)
			startdelay = event.data.start.startdelay;
		else
			startdelay = 0;
		if (event.data.start.pkttimeout >= 0)
			pkttimeout = event.data.start.pkttimeout;
		else
			pkttimeout = PKTRCV_TIMEOUT;
		if (event.data.start.idletimer >= 0)
			idletimer = event.data.start.idletimer;
		else
			idletimer = CLIENT_IDLETIMER_COUNT;
		if (event.data.start.chunkbufs >= 0 &&
		    event.data.start.chunkbufs <= 1024)
			maxchunkbufs = event.data.start.chunkbufs;
		else
			maxchunkbufs = MAXCHUNKBUFS;
		if (event.data.start.readahead >= 0 &&
		    event.data.start.readahead <= maxchunkbufs)
			maxreadahead = event.data.start.readahead;
		else
			maxreadahead = MAXREADAHEAD;
		if (event.data.start.inprogress >= 0 &&
		    event.data.start.inprogress <= maxchunkbufs)
			maxinprogress = event.data.start.inprogress;
		else
			maxinprogress = MAXINPROGRESS;
		if (event.data.start.redodelay >= 0)
			redodelay = event.data.start.redodelay;
		else
			redodelay = CLIENT_REQUEST_REDO_DELAY;
		if (event.data.start.idledelay >= 0)
			idledelay = event.data.start.idledelay;
		else
			idledelay = CLIENT_WRITER_IDLE_DELAY;

		if (event.data.start.slice >= 0)
			slice = event.data.start.slice;
		else
			slice = 0;
		if (event.data.start.zerofill >= 0)
			zero = event.data.start.zerofill;
		else
			zero = 0;
		if (event.data.start.randomize >= 0)
			randomize = event.data.start.randomize;
		else
			randomize = 1;
		if (event.data.start.nothreads >= 0)
			nothreads = event.data.start.nothreads;
		else
			nothreads = 0;
		if (event.data.start.debug >= 0)
			debug = event.data.start.debug;
		else
			debug = 0;
		if (event.data.start.trace >= 0) {
			tracing = event.data.start.trace;
			traceprefix[0] = 0;
		} else
			tracing = 0;

		log("Starting: slice=%d, startdelay=%d, zero=%d, "
		    "randomize=%d, nothreads=%d, debug=%d, tracing=%d, "
		    "pkttimeout=%d, idletimer=%d, redodelay=%d, "
		    "chunkbufs=%d maxreadahead=%d, maxinprogress=%d",
		    slice, startdelay, zero, randomize, nothreads,
		    debug, tracing, pkttimeout, idletimer, redodelay,
		    maxchunkbufs, maxreadahead, maxinprogress);
	}
#endif

	redodelay = sleeptime(redodelay, "request retry delay");
	idledelay = sleeptime(idledelay, "writer idle delay");

	ImageUnzipInit(filename, slice, debug, zero, nothreads);

	if (tracing) {
		ClientTraceInit(traceprefix);
		TraceStart(tracing);
	}

	PlayFrisbee();

	if (tracing) {
		TraceStop();
		TraceDump();
	}

	ImageUnzipQuit();

#ifdef DOEVENTS
	if (eventserver != NULL) {
		log("Waiting for START/STOP event...");
		EventWait(EV_ANY, &event);
		if (event.type == EV_START) {
#ifdef STATS
			memset(&Stats, 0, sizeof(Stats));
#endif
			goto again;
		}
	done:
		if (event.type == EV_STOP && event.data.stop.exitstatus >= 0)
			exitstatus = event.data.stop.exitstatus;
		exit(exitstatus);
	}
#endif

	exit(0);
}

/*
 * The client receive thread. This thread takes in packets from the server.
 */
void *
ClientRecvThread(void *arg)
{
	Packet_t	packet, *p = &packet;
	int		BackOff;

	if (debug)
		log("Receive pthread starting up ...");

	/*
	 * Use this to control the rate at which we request blocks.
	 * The IdleCounter is how many ticks we let pass without a
	 * useful block, before we make another request. We want that to
	 * be short, but not too short; we do not want to pummel the
	 * server. 
	 */
	IdleCounter = idletimer;

	/*
	 * This is another throttling mechanism; avoid making repeated
	 * requests to a server that is not running. That is, if the server
	 * is not responding, slowly back off our request rate (to about
	 * one a second) until the server starts responding.  This will
	 * prevent a large group of clients from pummeling the server
	 * machine, when there is no server running to respond (say, if the
	 * server process died).
	 */
	BackOff = 0;

	while (1) {
#ifdef NEVENTS
		static int needstamp = 1;
		struct timeval pstamp;
		if (needstamp) {
			gettimeofday(&pstamp, 0);
			needstamp = 0;
		}
#endif

		/*
		 * If we go too long without getting a block, we want
		 * to make another chunk request.
		 *
		 * XXX fixme: should probably be if it hasn't received
		 * a block that it is able to make use of.  But that has
		 * problems in that any new request we make will wind up
		 * at the end of the server work list, and we might not
		 * see that block for longer than our timeout period,
		 * leading us to issue another request, etc.
		 */
		if (PacketReceive(p) < 0) {
			pthread_testcancel();
			if (--IdleCounter <= 0) {
				DOSTAT(recvidles++);
				CLEVENT(2, EV_CLIRTIMO,
					pstamp.tv_sec, pstamp.tv_usec, 0, 0);
#ifdef NEVENTS
				needstamp = 1;
#endif
				RequestChunk(1);
				IdleCounter = idletimer;

				if (BackOff++) {
					IdleCounter += BackOff;
					if (BackOff > TIMEOUT_HZ)
						BackOff = TIMEOUT_HZ;
				}
			}
			continue;
		}
		pthread_testcancel();

		if (! PacketValid(p, TotalChunkCount)) {
			log("received bad packet %d/%d, ignored",
			    p->hdr.type, p->hdr.subtype);
			continue;
		}

		switch (p->hdr.subtype) {
		case PKTSUBTYPE_BLOCK:
			CLEVENT(2, EV_CLIGOTPKT,
				pstamp.tv_sec, pstamp.tv_usec, 0, 0);
#ifdef NEVENTS
			needstamp = 1;
#endif
			BackOff = 0;
			GotBlock(p);
			break;

		case PKTSUBTYPE_REQUEST:
			CLEVENT(4, EV_CLIREQMSG,
				p->hdr.srcip, p->msg.request.chunk,
				p->msg.request.block, p->msg.request.count);
			RequestStamp(p->msg.request.chunk,
				     p->msg.request.block,
				     p->msg.request.count);
			break;

		case PKTSUBTYPE_JOIN:
		case PKTSUBTYPE_LEAVE:
			/* Ignore these. They are from other clients. */
			CLEVENT(4, EV_OCLIMSG,
				p->hdr.srcip, p->hdr.subtype, 0, 0);
			break;
		}
	}
}

/*
 * The heart of the game.
 */
static void
ChunkerStartup(void)
{
	pthread_t		child_pid;
	void		       *ignored;
	int			chunkcount = TotalChunkCount;
	int			i;

	/*
	 * Allocate the chunk descriptors, request list and cache buffers.
	 */
	if ((Chunks =
	     (Chunk_t *) calloc(chunkcount, sizeof(*Chunks))) == NULL)
		fatal("Chunks: No more memory");

	if ((ChunkRequestList =
	     (int *) calloc(chunkcount, sizeof(*ChunkRequestList))) == NULL)
		fatal("ChunkRequestList: No more memory");

	if ((ChunkBuffer =
	     (ChunkBuffer_t *) malloc(maxchunkbufs * sizeof(ChunkBuffer_t)))
	    == NULL)
		fatal("ChunkBuffer: No more memory");

	/*
	 * Set all the buffers to "free"
	 */
	for (i = 0; i < maxchunkbufs; i++) {
		ChunkBuffer[i].inprogress = 0;
		ChunkBuffer[i].ready      = 0;
	}

	for (i = 0; i < TotalChunkCount; i++)
		ChunkRequestList[i] = i;
	
	/*
	 * We randomize the block selection so that multiple clients
	 * do not end up getting stalled by each other. That is, if
	 * all the clients were requesting blocks in order, then all
	 * the clients would end up waiting until the last client was
	 * done (since the server processes client requests in FIFO
	 * order).
	 */
	if (randomize) {
		for (i = 0; i < 50 * TotalChunkCount; i++) {
			int c1 = random() % TotalChunkCount;
			int c2 = random() % TotalChunkCount;
			int t1 = ChunkRequestList[c1];
			int t2 = ChunkRequestList[c2];

			ChunkRequestList[c2] = t1;
			ChunkRequestList[c1] = t2;
		}
	}

	if (pthread_create(&child_pid, NULL,
			   ClientRecvThread, (void *)0)) {
		fatal("Failed to create pthread!");
	}

	/*
	 * Loop until all chunks have been received and written to disk.
	 */
	while (chunkcount) {
#ifdef DOEVENTS
		Event_t event;
		if (eventserver != NULL &&
		    EventCheck(&event) && event.type == EV_STOP) {
			log("Aborted after %d chunks",
			    TotalChunkCount-chunkcount);
			break;
		}
#endif
		/*
		 * Search the chunk cache for a chunk that is ready to write.
		 */
		for (i = 0; i < maxchunkbufs; i++)
			if (ChunkBuffer[i].ready)
				break;

		/*
		 * If nothing to do, then get out of the way for a while.
		 * XXX should be a condition variable.
		 */
		if (i == maxchunkbufs) {
			CLEVENT(1, EV_CLIWRIDLE, 0, 0, 0, 0);
			if (debug)
				log("No chunks ready to write!");
			DOSTAT(nochunksready++);
			fsleep(idledelay);
			continue;
		}

		/*
		 * We have a completed chunk. Write it to disk.
		 */
		if (debug)
			log("Writing chunk %d (buffer %d)",
			    ChunkBuffer[i].thischunk, i);
		else {
			struct timeval estamp;

			gettimeofday(&estamp, 0);
			estamp.tv_sec -= stamp.tv_sec;
		
			printf(".");
			fflush(stdout);
			if (dotcol++ > 65) {
				dotcol = 0;
				printf("%4ld %6d\n",
				       estamp.tv_sec, chunkcount);
			}
		}

		ImageUnzip(i);

		/*
		 * Okay, free the slot up for another chunk.
		 */
		ChunkBuffer[i].ready      = 0;
		ChunkBuffer[i].inprogress = 0;
		chunkcount--;
		CLEVENT(1, EV_CLIWRDONE,
			ChunkBuffer[i].thischunk, chunkcount, 0, 0);
	}
	/*
	 * Kill the child and wait for it before returning. We do not
	 * want the child absorbing any more packets, cause that would
	 * mess up the termination handshake with the server. 
	 */
	pthread_cancel(child_pid);
	pthread_join(child_pid, &ignored);

	/*
	 * Make sure any asynchronous writes are done
	 * and collect stats from the unzipper.
	 */
	ImageUnzipFlush();
#ifdef STATS
	{
		extern unsigned long decompidles, writeridles;
		extern long long totaledata, totalrdata;
		
		Stats.u.v1.decompidles = decompidles;
		Stats.u.v1.writeridles = writeridles;
		Stats.u.v1.ebyteswritten = totaledata;
		Stats.u.v1.rbyteswritten = totalrdata;
	}
#endif

	free(ChunkBuffer);
	free(ChunkRequestList);
	free(Chunks);
}

/*
 * Note that someone has made a request from the server right now.
 * This is either a request by us or one we snooped.
 *
 * We use the time stamp to determine when we should repeat a request to
 * the server.  If we update the stamp here, we are further delaying
 * a re-request.  The general strategy is: if a chunk request contains
 * any blocks that we will be able to use, we update the stamp to delay
 * what would otherwise be a redundant request.
 */
static void
RequestStamp(int chunk, int block, int count)
{
	int stampme = 0;

	/*
	 * If not doing delays, don't bother with the stamp
	 */
	if (redodelay == 0)
		return;

	/*
	 * Common case of a complete chunk request, always stamp.
	 * This will include chunks we have already written and wouldn't
	 * be re-requesting, but updating the stamp doesn't hurt anything.
	 */
	if (block == 0 && count == CHUNKSIZE)
		stampme = 1;
	/*
	 * Else, request is for a partial chunk. If we are not currently
	 * processing this chunk, then the chunk data will be of use to
	 * us so we update the stamp.  Again, this includes chunks we
	 * are already finished with, but no harm.
	 */
	else if (! Chunks[chunk].done)
		stampme = 1;
	/*
	 * Otherwise, this is a partial chunk request for which we have
	 * already received some blocks.  We need to determine if the
	 * request contains any blocks that we need to complete our copy
	 * of the chunk.  If so, we conservatively update the stamp as it
	 * implies there is at least some chunk data coming that we will
	 * be able to use.  If the request contains only blocks that we
	 * already have, then the returned data will be of no use to us
	 * for completing our copy and we will still have to make a
	 * further request (i.e., we don't stamp).
	 */
	else {
		int i, j;

		for (i = 0; i < maxchunkbufs; i++)
			if (ChunkBuffer[i].thischunk == chunk &&
			    ChunkBuffer[i].inprogress &&
			    !ChunkBuffer[i].ready)
				break;
		if (i < maxchunkbufs) {
			for (j = 0; j < count; j++)
				if (! ChunkBuffer[i].bitmap[block+j]) {
					stampme = 1;
					break;
				}
		}

	}

	if (stampme) {
		struct timeval tv;

		gettimeofday(&tv, 0);
		Chunks[chunk].lastreq =
			(unsigned long long)tv.tv_sec * 1000000 + tv.tv_usec;
		CLEVENT(5, EV_CLISTAMP, chunk, tv.tv_sec, tv.tv_usec, 0);
	}
}

/*
 * Returns 1 if we have not made (or seen) a request for the given chunk
 * "for awhile", 0 otherwise.
 */
static int
RequestRedoTime(int chunk, unsigned long long curtime)
{
	if (Chunks[chunk].lastreq == 0 || redodelay == 0 ||
	    (int)(curtime - Chunks[chunk].lastreq) >= redodelay)
		return 1;
	return 0;
}

/*
 * Receive a single data block. If the block is for a chunk in progress, then
 * insert the data and check for a completed chunk. It will be up to the main
 * thread to process that chunk.
 *
 * If the block is the first of some chunk, then try to allocate a new chunk.
 * If the chunk buffer is full, then drop the block. If this happens, it
 * indicates the chunk buffer is not big enough, and should be increased.
 */
static void
GotBlock(Packet_t *p)
{
	int	chunk = p->msg.block.chunk;
	int	block = p->msg.block.block;
	int	i, free = -1;

	/*
	 * Search the chunk buffer for a match (or a free one).
	 */
	for (i = 0; i < maxchunkbufs; i++) {
		if (!ChunkBuffer[i].inprogress) {
			if (free == -1)
				free = i;
			continue;
		}
		
		if (!ChunkBuffer[i].ready &&
		    ChunkBuffer[i].thischunk == chunk)
			break;
	}
	if (i == maxchunkbufs) {
		/*
		 * Did not find it. Allocate the free one, or drop the
		 * packet if there is no free chunk.
		 */
		if (free == -1) {
			CLEVENT(1, EV_CLINOROOM, chunk, block, 0, 0);
			DOSTAT(nofreechunks++);
			if (debug)
				log("No more free buffer slots for chunk %d!",
				    chunk);
			return;
		}

		/*
		 * Was this chunk already processed? 
		 */
		if (Chunks[chunk].done) {
			CLEVENT(3, EV_CLIDUPCHUNK, chunk, block, 0, 0);
			DOSTAT(dupchunk++);
			if (0)
				log("Duplicate chunk %d ignored!", chunk);
			return;
		}
		Chunks[chunk].done = 1;

		if (debug)
			log("Starting chunk %d (buffer %d)", chunk, free);

		i = free;
		ChunkBuffer[i].ready      = 0;
		ChunkBuffer[i].inprogress = 1;
		ChunkBuffer[i].thischunk  = chunk;
		ChunkBuffer[i].blockcount = CHUNKSIZE;
		bzero(ChunkBuffer[i].bitmap, sizeof(ChunkBuffer[i].bitmap));
		CLEVENT(1, EV_CLISCHUNK, chunk, block, 0, 0);
	}

	/*
	 * Insert the block and update the metainfo. We have to watch for
	 * duplicate blocks in the same chunk since another client may
	 * issue a request for a lost block, and we will see that even if
	 * we do not need it (cause of broadcast/multicast).
	 */
	if (ChunkBuffer[i].bitmap[block]) {
		CLEVENT(3, EV_CLIDUPBLOCK, chunk, block, 0, 0);
		DOSTAT(dupblock++);
		if (0)
			log("Duplicate block %d in chunk %d", block, chunk);
		return;
	}
	ChunkBuffer[i].bitmap[block] = 1;
	ChunkBuffer[i].blockcount--;
	memcpy(ChunkBuffer[i].blocks[block].data, p->msg.block.buf, BLOCKSIZE);
	CLEVENT(3, EV_CLIBLOCK, chunk, block, ChunkBuffer[i].blockcount, 0);

	/*
	 * Anytime we receive a packet thats needed, reset the idle counter.
	 * This will prevent us from sending too many requests.
	 */
	IdleCounter = idletimer;

	/*
	 * Is the chunk complete? If so, then release it to the main thread.
	 */
	if (ChunkBuffer[i].blockcount == 0) {
		CLEVENT(1, EV_CLIECHUNK, chunk, block, 0, 0);
		if (debug)
			log("Releasing chunk %d to main thread", chunk);
		ChunkBuffer[i].ready = 1;

		/*
		 * Send off a request for a chunk we do not have yet. This
		 * should be enough to ensure that there is more work to do
		 * by the time the main thread finishes the chunk we just
		 * released.
		 */
		RequestChunk(0);
	}
}

/*
 * Request a chunk/block/range we do not have.
 */
static void
RequestRange(int chunk, int block, int count)
{
	Packet_t	packet, *p = &packet;

	if (debug)
		log("Requesting chunk:%d block:%d count:%d",
		    chunk, block, count);
	
	p->hdr.type       = PKTTYPE_REQUEST;
	p->hdr.subtype    = PKTSUBTYPE_REQUEST;
	p->hdr.datalen    = sizeof(p->msg.request);
	p->msg.request.chunk = chunk;
	p->msg.request.block = block;
	p->msg.request.count = count;
	PacketSend(p, 0);
	CLEVENT(1, EV_CLIREQ, chunk, block, count, 0);
	DOSTAT(requests++);

	RequestStamp(chunk, block, count);
}

static void
RequestChunk(int timedout)
{
	int		   i, j, k;
	int		   emptybufs, fillingbufs;
	unsigned long long stamp = 0;

	CLEVENT(1, EV_CLIREQCHUNK, timedout, 0, 0, 0);

	if (! timedout) {
		struct timeval tv;

		gettimeofday(&tv, 0);
		stamp = (unsigned long long)tv.tv_sec * 1000000 + tv.tv_usec;
	}

	/*
	 * Look for unfinished chunks.
	 */
	emptybufs = fillingbufs = 0;
	for (i = 0; i < maxchunkbufs; i++) {
		/*
		 * Skip empty and full buffers
		 */
		if (! ChunkBuffer[i].inprogress) {
			/*
			 * Keep track of empty chunk buffers while we are here
			 */
			emptybufs++;
			continue;
		}
		if (ChunkBuffer[i].ready)
			continue;

		fillingbufs++;

		/*
		 * Make sure this chunk is eligible for re-request.
		 */
		if (! timedout &&
		    ! RequestRedoTime(ChunkBuffer[i].thischunk, stamp))
			continue;

		/*
		 * Scan the bitmap. We do not want to request single blocks,
		 * but rather small groups of them.
		 */
		for (j = 0; j < CHUNKSIZE; j++) {
			/*
			 * When we find a missing block, scan forward from
			 * that point till we get to a block that is present.
			 * Send a request for the intervening range.
			 */
			if (! ChunkBuffer[i].bitmap[j]) {
				for (k = j + 1; k < CHUNKSIZE; k++) {
					if (ChunkBuffer[i].bitmap[k])
						break;
				}
				DOSTAT(lostblocks++);
				RequestRange(ChunkBuffer[i].thischunk,
					     j, k - j);
				j = k;
			}
		}
	}

	/*
	 * Issue read-ahead requests.
	 *
	 * If we already have enough unfinished chunks on our plate
	 * or we have no room for read-ahead, don't do it.
	 */
	if (emptybufs < maxreadahead || fillingbufs >= maxinprogress)
		return;

	/*
	 * Scan our request list looking for candidates.
	 */
	for (i = 0, j = 0; i < TotalChunkCount && j < maxreadahead; i++) {
		int chunk = ChunkRequestList[i];
		
		/*
		 * If already working on this chunk, skip it.
		 */
		if (Chunks[chunk].done)
			continue;

		/*
		 * Issue a request for the chunk if it isn't already
		 * on the way.  This chunk, whether requested or not
		 * is considered a read-ahead to us.
		 */
		if (timedout || RequestRedoTime(chunk, stamp))
			RequestRange(chunk, 0, CHUNKSIZE);

		j++;
	}
}

/*
 * Join the Frisbee team, and then go into the main loop above.
 */
static void
PlayFrisbee(void)
{
	Packet_t	packet, *p = &packet;
	struct timeval  estamp, timeo;
	unsigned int	myid;

	gettimeofday(&stamp, 0);
	CLEVENT(1, EV_CLISTART, 0, 0, 0, 0);

	/*
	 * Init the random number generator. We randomize the block request
	 * sequence above, and its important that each client have a different
	 * sequence!
	 */
	srandomdev();

	/*
	 * A random number ID. I do not think this is really necessary,
	 * but perhaps might be useful for determining when a client has
	 * crashed and returned.
	 */
	myid = random();
	
	/*
	 * To avoid a blast of messages from a large number of clients,
	 * we delay a small random amount before startup.  The delay
	 * value is uniformly distributed between 0 and startdelay seconds,
	 * with ms granularity.
	 */
	if (startdelay > 0) {
		int delay = random() % (startdelay * 1000);

		if (debug)
			log("Starup delay: %d.%03d seconds",
			    delay/1000, delay%1000);
		DOSTAT(delayms = delay);
		fsleep(delay * 1000);
	}

	/*
	 * Send a join the team message. We block waiting for a reply
	 * since we need to know the total block size. We resend the
	 * message (dups are harmless) if we do not get a reply back.
	 */
	gettimeofday(&timeo, 0);
	while (1) {
		struct timeval now;

		gettimeofday(&now, 0);
		if (timercmp(&timeo, &now, <=)) {
#ifdef DOEVENTS
			Event_t event;
			if (eventserver != NULL &&
			    EventCheck(&event) && event.type == EV_STOP) {
				log("Aborted during JOIN");
				return;
			}
#endif
			CLEVENT(1, EV_CLIJOINREQ, myid, 0, 0, 0);
			DOSTAT(joinattempts++);
			p->hdr.type       = PKTTYPE_REQUEST;
			p->hdr.subtype    = PKTSUBTYPE_JOIN;
			p->hdr.datalen    = sizeof(p->msg.join);
			p->msg.join.clientid = myid;
			PacketSend(p, 0);
			timeo.tv_sec = 0;
			timeo.tv_usec = 500000;
			timeradd(&timeo, &now, &timeo);
		}

		/*
		 * Throw away any data packets. We cannot start until
		 * we get a reply back.
		 */
		if (PacketReceive(p) >= 0 && 
		    p->hdr.subtype == PKTSUBTYPE_JOIN &&
		    p->hdr.type == PKTTYPE_REPLY) {
			CLEVENT(1, EV_CLIJOINREP,
				p->msg.join.blockcount, 0, 0, 0);
			break;
		}
	}
	gettimeofday(&timeo, 0);
	TotalChunkCount = p->msg.join.blockcount / CHUNKSIZE;
	
	log("Joined the team after %d sec. ID is %u. "
	    "File is %d chunks (%d blocks)",
	    timeo.tv_sec - stamp.tv_sec,
	    myid, TotalChunkCount, p->msg.join.blockcount);

	ChunkerStartup();

	gettimeofday(&estamp, 0);
	timersub(&estamp, &stamp, &estamp);
	
	/*
	 * Done! Send off a leave message, but do not worry about whether
	 * the server gets it. All the server does with it is print a
	 * timestamp, and that is not critical to operation.
	 */
	CLEVENT(1, EV_CLILEAVE, myid, estamp.tv_sec, 0, 0);
#ifdef STATS
	p->hdr.type       = PKTTYPE_REQUEST;
	p->hdr.subtype    = PKTSUBTYPE_LEAVE2;
	p->hdr.datalen    = sizeof(p->msg.leave2);
	p->msg.leave2.clientid = myid;
	p->msg.leave2.elapsed  = estamp.tv_sec;
	Stats.version            = CLIENT_STATS_VERSION;
	Stats.u.v1.runsec        = estamp.tv_sec;
	Stats.u.v1.runmsec       = estamp.tv_usec / 1000;
	Stats.u.v1.chunkbufs     = maxchunkbufs;
	Stats.u.v1.maxreadahead  = maxreadahead;
	Stats.u.v1.maxinprogress = maxinprogress;
	Stats.u.v1.pkttimeout    = pkttimeout;
	Stats.u.v1.startdelay    = startdelay;
	Stats.u.v1.idletimer     = idletimer;
	Stats.u.v1.idledelay     = idledelay;
	Stats.u.v1.redodelay     = redodelay;
	Stats.u.v1.randomize     = randomize;
	p->msg.leave2.stats      = Stats;
	PacketSend(p, 0);

	log("");
	ClientStatsDump(myid, &Stats);
#else
	p->hdr.type       = PKTTYPE_REQUEST;
	p->hdr.subtype    = PKTSUBTYPE_LEAVE;
	p->hdr.datalen    = sizeof(p->msg.leave);
	p->msg.leave.clientid = myid;
	p->msg.leave.elapsed  = estamp.tv_sec;
	PacketSend(p, 0);
#endif
	log("\nLeft the team after %ld seconds on the field!", estamp.tv_sec);
}

/*
 * Supply a read function to the imageunzip library. Kinda hokey right
 * now. The original imageunzip code depends on read to keep track of
 * the seek offset within the input file. Well, in our case we know what
 * chunk the inflator is working on, and we keep track of the block offset
 * as well.
 *
 * XXX We copy out the data. Need to change this interface to avoid that.
 */
extern int	inflate_subblock(char *);

static int
ImageUnzip(int slot)
{
	char	*data = (char *) &ChunkBuffer[slot].blocks;

	if (inflate_subblock(data))
		pfatal("ImageUnzip: inflate_subblock failed");

	return 0;
}
