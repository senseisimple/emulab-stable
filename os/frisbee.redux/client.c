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

int		debug = 0;
int		portnum;
struct in_addr	mcastaddr;
struct in_addr	mcastif;

/* Forward Decls */
static void	PlayFrisbee(void);
static void	GotBlock(Packet_t *p);
static void	RequestChunk(int timedout);
static int	ImageUnzip(int chunk);
extern int	ImageUnzipInit(char *filename, int slice, int debug);

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
ChunkBuffer_t   *ChunkBuffer;		/* The cache */
char		*ChunkBitmap;		/* Bitmap of which chunks finished */
int		*ChunkRequestList;	/* Randomized chunk request order */
int		LastReceiveChunk;	/* Chunk of last data packet received*/
int		TotalChunkCount;	/* Total number of chunks in file */
int		IdleCounter;		/* Countdown to request more data */

#ifdef STATS
/*
 * Stats gathering.
 */
struct {
	unsigned long	nochunksready;
	unsigned long	nofreechunks;
	unsigned long	dupchunk;
	unsigned long	dupblock;
	unsigned long	lostblocks;
} Stats;
#define DOSTAT(x)	(Stats.x)
#else
#define DOSTAT(x)
#endif

char *usagestr = 
 "usage: frisbee [-d] [-s #] <-p #> <-m mcastaddr> <output filename>\n"
 " -d              Turn on debugging. Multiple -d options increase output.\n"
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
	int	ch, slice = 0;
	char   *filename;

	while ((ch = getopt(argc, argv, "dhp:m:s:i:")) != -1)
		switch(ch) {
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

		case 's':
			slice = atoi(optarg);
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
	ImageUnzipInit(filename, slice, debug);
	ClientNetInit();
	PlayFrisbee();
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
	IdleCounter = CLIENT_IDLETIMER_COUNT;

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
		/*
		 * If we go too long without getting a block, we want
		 * to make another chunk request.
		 */
		 if (PacketReceive(p) < 0) {
			 pthread_testcancel();
			 IdleCounter--;
			 if (! IdleCounter) {
				 RequestChunk(1);
				 IdleCounter = CLIENT_IDLETIMER_COUNT;
				 
				 if (BackOff++) {
					 IdleCounter += BackOff;
					 if (BackOff > TIMEOUT_HZ)
						 BackOff = TIMEOUT_HZ;
				 }
			 }
			 continue;
		 }
		 pthread_testcancel();

		 switch (p->hdr.subtype) {
		 case PKTSUBTYPE_BLOCK:
			 BackOff = 0;
			 GotBlock(p);
			 break;

		 case PKTSUBTYPE_JOIN:
		 case PKTSUBTYPE_LEAVE:
		 case PKTSUBTYPE_REQUEST:
			 /* Ignore these. They are from other clients. */
			 break;

		 default:
			 fatal("ClientRecvThread: Bad packet type!");
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
	 * Allocate the Chunk Buffer and Chunk Bitmap.
	 */
	if ((ChunkBitmap =
	     (char *) calloc(chunkcount, sizeof(*ChunkBitmap))) == NULL)
		fatal("ChunkBitmap: No more memory");

	if ((ChunkRequestList =
	     (int *) calloc(chunkcount, sizeof(*ChunkRequestList))) == NULL)
		fatal("ChunkRequestList: No more memory");

	if ((ChunkBuffer =
	     (ChunkBuffer_t *) malloc(MAXCHUNKBUFS * sizeof(ChunkBuffer_t)))
	    == NULL)
		fatal("ChunkBuffer: No more memory");

	/*
	 * Set all the buffers to "free"
	 */
	for (i = 0; i < MAXCHUNKBUFS; i++) {
		ChunkBuffer[i].inprogress = 0;
		ChunkBuffer[i].ready      = 0;
	}

	/*
	 * We randomize the block selection so that multiple clients
	 * do not end up getting stalled by each other. That is, if
	 * all the clients were requesting blocks in order, then all
	 * the clients would end up waiting until the last client was
	 * done (since the server processes client requests in FIFO
	 * order).
	 */
	for (i = 0; i < TotalChunkCount; i++)
		ChunkRequestList[i] = i;
	
	for (i = 0; i < 50 * TotalChunkCount; i++) {
		int c1 = random() % TotalChunkCount;
		int c2 = random() % TotalChunkCount;
		int t1 = ChunkRequestList[c1];
		int t2 = ChunkRequestList[c2];

		ChunkRequestList[c2] = t1;
		ChunkRequestList[c1] = t2;
	}

	if (pthread_create(&child_pid, NULL,
			   ClientRecvThread, (void *)0)) {
		fatal("Failed to create pthread!");
	}

	/*
	 * Loop until all chunks have been received and written to disk.
	 */
	while (chunkcount) {
		/*
		 * Search the chunk cache for a chunk that is ready to write.
		 */
		for (i = 0; i < MAXCHUNKBUFS; i++) {
			if (ChunkBuffer[i].ready)
				break;
		}

		/*
		 * If nothing to do, then get out of the way for a while.
		 */
		if (i == MAXCHUNKBUFS) {
			log("No chunks ready to write!");
			DOSTAT(nochunksready++);
			fsleep(100000);
			continue;
		}

		/*
		 * We have a completed chunk. Write it to disk.
		 */
		log("Writing chunk %d (buffer %d)",
		    ChunkBuffer[i].thischunk, i);

		ImageUnzip(i);

		/*
		 * Okay, free the slot up for another chunk.
		 */
		ChunkBuffer[i].ready      = 0;
		ChunkBuffer[i].inprogress = 0;
		chunkcount--;
	}
	/*
	 * Kill the child and wait for it before returning. We do not
	 * want the child absorbing any more packets, cause that would
	 * mess up the termination handshake with the server. 
	 */
	pthread_cancel(child_pid);
	pthread_join(child_pid, &ignored);
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
	for (i = 0; i < MAXCHUNKBUFS; i++) {
		if (!ChunkBuffer[i].inprogress) {
			free = i;
			continue;
		}
		
		if (!ChunkBuffer[i].ready &&
		    ChunkBuffer[i].thischunk == chunk)
			break;
	}
	if (i == MAXCHUNKBUFS) {
		/*
		 * Did not find it. Allocate the free one, or drop the
		 * packet if there is no free chunk.
		 */
		if (free == -1) {
			DOSTAT(nofreechunks++);
			if (debug)
				log("No more free buffer slots for chunk %d!",
				    chunk);
			return;
		}

		/*
		 * Was this chunk already processed? 
		 */
		if (ChunkBitmap[chunk]) {
			DOSTAT(dupchunk++);
			if (0)
				log("Duplicate chunk %d ignored!", chunk);
			return;
		}
		ChunkBitmap[chunk] = 1;

		if (debug)
			log("Allocating chunk buffer %d to chunk %d",
			    free, chunk);

		i = free;
		ChunkBuffer[i].ready      = 0;
		ChunkBuffer[i].inprogress = 1;
		ChunkBuffer[i].thischunk  = chunk;
		ChunkBuffer[i].blockcount = CHUNKSIZE;
		bzero(ChunkBuffer[i].bitmap, sizeof(ChunkBuffer[i].bitmap));
	}

	/*
	 * Insert the block and update the metainfo. We have to watch for
	 * duplicate blocks in the same chunk since another client may
	 * issue a request for a lost block, and we will see that even if
	 * we do not need it (cause of broadcast/multicast).
	 */
	if (ChunkBuffer[i].bitmap[block]) {
		DOSTAT(dupblock++);
		if (0)
			log("Duplicate block %d in chunk %d", block, chunk);
		return;
	}
	ChunkBuffer[i].bitmap[block] = 1;
	ChunkBuffer[i].blockcount--;
	memcpy(ChunkBuffer[i].blocks[block].data, p->msg.block.buf, BLOCKSIZE);
	LastReceiveChunk = chunk;

	/*
	 * Anytime we receive a packet thats needed, reset the idle counter.
	 * This will prevent us from sending too many requests.
	 */
	IdleCounter = CLIENT_IDLETIMER_COUNT;

	/*
	 * Is the chunk complete? If so, then release it to the main thread.
	 */
	if (ChunkBuffer[i].blockcount == 0) {
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
	PacketSend(p);
}

static void
RequestChunk(int timedout)
{
	int		i, j, k;

	/*
	 * Look for unfinished chunks.
	 */
	for (i = 0; i < MAXCHUNKBUFS; i++) {
		if (! ChunkBuffer[i].inprogress || ChunkBuffer[i].ready)
			continue;

		/*
		 * If we timed out, then its okay to send in another request
		 * for the last chunk in progress. The point of this test
		 * is to prevent requesting a chunk we are currently getting
		 * packets for. But once we time out, we obviously want to
		 * get more blocks for it.
		 */
		if (ChunkBuffer[i].thischunk == LastReceiveChunk && !timedout)
			continue;

		/*
		 * Scan the bitmap. We do not want to request single blocks,
		 * but rather small groups of them.
		 */
		j = 0;
		while (j < CHUNKSIZE) {
			/*
			 * When we find a missing block, scan forward from
			 * that point till we get to a block that is present.
			 * Send a request for the intervening range.
			 */
			if (! ChunkBuffer[i].bitmap[j]) {
				for (k = j; k < CHUNKSIZE; k++) {
					if (ChunkBuffer[i].bitmap[k])
						break;
				}
				DOSTAT(lostblocks++);
				RequestRange(ChunkBuffer[i].thischunk,
					     j, k - j);
				j = k;
			}
			j++;
		}
	}

	/*
	 * Make sure we have a place to put new data.
	 */
	for (i = 0, k = 0; i < MAXCHUNKBUFS; i++) {
		if (ChunkBuffer[i].inprogress)
			k++;
	}
	if (MAXCHUNKBUFS - k < MAXREADAHEAD || k >= MAXINPROGRESS)
		return;

	/*
	 * Look for a chunk we are not working on yet.
	 */
	for (i = 0, j = 0; i < TotalChunkCount && j < MAXREADAHEAD; i++) {
		int chunk = ChunkRequestList[i];

		if (! ChunkBitmap[chunk]) {
			/*
			 * Not working on this one yet, so send off a
			 * request for it.
			 */
			RequestRange(chunk, 0, CHUNKSIZE);
			j++;
		}
	}
}

/*
 * Join the Frisbee team, and then go into the main loop above.
 */
static void
PlayFrisbee(void)
{
	Packet_t	packet, *p = &packet;
	struct timeval  stamp, estamp;
	unsigned int	myid;

	gettimeofday(&stamp, 0);

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
	 * Send a join the team message. We block waiting for a reply
	 * since we need to know the total block size. We resend the
	 * message (dups are harmless) if we do not get a reply back.
	 */
	while (1) {
		int	countdown = 0;

		if (countdown <= 0) {
			p->hdr.type       = PKTTYPE_REQUEST;
			p->hdr.subtype    = PKTSUBTYPE_JOIN;
			p->hdr.datalen    = sizeof(p->msg.join);
			p->msg.join.clientid = myid;
			PacketSend(p);
			countdown = TIMEOUT_HALFHZ;
		}

		/*
		 * Throw away any data packets. We cannot start until
		 * we get a reply back. Wait several receive delays
		 * before resending the join message.
		 */
		if (PacketReceive(p) < 0) {
			countdown--;
			continue;
		}

		if (p->hdr.subtype == PKTSUBTYPE_JOIN &&
		    p->hdr.type == PKTTYPE_REPLY) {
			break;
		}
	}
	TotalChunkCount = p->msg.join.blockcount / BLOCKSIZE;
	
	log("Joined the team. ID is %u. File is %d chunks (%d blocks)",
	    myid, TotalChunkCount, p->msg.join.blockcount);

	/*
	 * To avoid a blast of messages from a large number of clients,
	 * delay a small random amount before continuing.
	 */
	fsleep(200000 + ((random() % 8) * 100000));

	ChunkerStartup();

	gettimeofday(&estamp, 0);
	estamp.tv_sec -= stamp.tv_sec;
	
	/*
	 * Done! Send off a leave message, but do not worry about whether
	 * the server gets it. All the server does with it is print a
	 * timestamp, and that is not critical to operation.
	 */
	p->hdr.type       = PKTTYPE_REQUEST;
	p->hdr.subtype    = PKTSUBTYPE_LEAVE;
	p->hdr.datalen    = sizeof(p->msg.leave);
	p->msg.leave.clientid = myid;
	p->msg.leave.elapsed  = estamp.tv_sec;
	PacketSend(p);

	log("Left the team after %ld seconds on the field!", estamp.tv_sec);
#ifdef  STATS
	log("Stats:");
	log("  nochunksready:    %d", Stats.nochunksready);
	log("  nofreechunks:     %d", Stats.nofreechunks);
	log("  dupchunk:         %d", Stats.dupchunk);
	log("  dupblock:         %d", Stats.dupblock);
	log("  lostblocks:       %d", Stats.lostblocks);
#endif
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
