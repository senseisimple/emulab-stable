/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Shared for defintions for frisbee client/server code.
 */

#include "log.h"

/*
 * We operate in terms of this blocksize (in bytes). 
 */
#define BLOCKSIZE	1024

/*
 * Each chunk is this many blocks.
 */
#define CHUNKSIZE	1024

/*
 * The number of chunk buffers in the client.
 */
#define MAXCHUNKBUFS	64

/*
 * Socket buffer size, used for both send and receive in client and
 * server right now.
 */
#define SOCKBUFSIZE	(128 * 1024)

/*
 * The number of read-ahead chunks that the client will request
 * at a time. No point in requesting too far ahead either, since they
 * are uncompressed/written at a fraction of the network transfer speed.
 * Also, with multiple clients at different stages, each requesting blocks
 * it is likely that there will be plenty more chunks ready or in progress.
 */
#define MAXREADAHEAD	2
#define MAXINPROGRESS	8

/*
 * Timeout (in usecs) for packet receive. The idletimer number is how
 * many PKT timeouts we allow before requesting more data from the server.
 * That is, if we go TIMEOUT usecs without getting a packet, then ask for
 * more.
 */
#define PKTRCV_TIMEOUT		30000
#define CLIENT_IDLETIMER_COUNT	3
#define TIMEOUT_HZ		(1000000 / PKTRCV_TIMEOUT)
#define TIMEOUT_HALFHZ		(TIMEOUT_HZ / 2)

/*
 * Timeout (in seconds!) server will hang around with no active clients.
 * Make it zero to never exit. 
 */
#define SERVER_INACTIVE_SECONDS	(60 * 30)

/*
 * The number of disk read blocks in a single read on the server.
 * Must be an even divisor of CHUNKSIZE.  Should also be an even
 * divisor of SERVER_BURST_SIZE below.
 */
#define SERVER_READ_SIZE	32

/*
 * Parameters for server network usage:
 *
 *	SERVER_BURST_SIZE	Max BLOCKSIZE packets sent in a burst.
 *				Should be a multiple of SERVER_READ_SIZE
 *				Should be less than SOCKBUFSIZE/BLOCKSIZE,
 *				bursts of greater than the send socket
 *				buffer size are almost certain to cause
 *				lost packets.
 *	SERVER_BURST_GAP	Delay in usec between output bursts.
 *				Given the typical scheduling granularity
 *				of 10ms for most unix systems, this
 *				will likely be set to either 0 or 10000.
 *
 * Together with the BLOCKSIZE, these two params form a theoretical upper
 * bound on bandwidth consumption for the server.  That upper bound (for
 * ethernet) is:
 *
 *	(1000000 / SERVER_BURST_GAP)		# bursts per second
 *	* (BLOCKSIZE + 42) * SERVER_BURST_SIZE	# * wire size of a burst
 *
 * which for the default 1k packets, gap of 10ms and burst of 64 packets
 * is about 6.8MB/sec.  In practice, the server is ultimately throttled by
 * clients' ability to generate requests which is limited by their ability
 * to decompress and write to disk.
 */
#define SERVER_BURST_SIZE	96
#define SERVER_BURST_GAP	10000

/*
 * Max burst size when doing dynamic bandwidth adjustment.
 * Needs to be large enough to induce loss.
 */ 
#define SERVER_DYNBURST_SIZE	(SOCKBUFSIZE/BLOCKSIZE)

/*
 * How long (in usecs) to wait before re-reqesting a chunk.
 * It will take the server more than:
 *
 *	(CHUNKSIZE/SERVER_BURST_SIZE) * SERVER_BURST_GAP
 *
 * usec (0.16 sec with defaults) for each each chunk it pumps out,
 * and we conservatively assume that there are a fair number of other
 * chunks that must be processed before it gets to our chunk.
 */
#define CLIENT_REQUEST_REDO_DELAY	\
	(10 * ((CHUNKSIZE/SERVER_BURST_SIZE)*SERVER_BURST_GAP))

/*
 * How long for the writer to sleep if there are no blocks currently
 * ready to write.  Allow a full server burst period, assuming that
 * something in the next burst will complete a block.
 */
#define CLIENT_WRITER_IDLE_DELAY	SERVER_BURST_GAP

/*
 * Client parameters and statistics.
 */
#define CLIENT_STATS_VERSION	1
typedef struct {
	int	version;
	union {
		struct {
			int	runsec;
			int	runmsec;
			int	delayms;
			unsigned long long rbyteswritten;
			unsigned long long ebyteswritten;
			int	chunkbufs;
			int	maxreadahead;
			int	maxinprogress;
			int	pkttimeout;
			int	startdelay;
			int	idletimer;
			int	idledelay;
			int	redodelay;
			int	randomize;
			unsigned long	nochunksready;
			unsigned long	nofreechunks;
			unsigned long	dupchunk;
			unsigned long	dupblock;
			unsigned long	lostblocks;
			unsigned long	recvidles;
			unsigned long	joinattempts;
			unsigned long	requests;
			unsigned long	decompidles;
			unsigned long	writeridles;
		} v1;
		unsigned long limit[256];
	} u;
} ClientStats_t;

/*
 * Packet defs.
 */
typedef struct {
	struct {
		int		type;
		int		subtype;
		int		datalen; /* Useful amount of data in packet */
		unsigned int	srcip;   /* Filled in by network level. */
	} hdr;
	union {
		/*
		 * Join/leave the Team. Send a randomized ID, and receive
		 * the number of blocks in the file. This is strictly
		 * informational; the info is reported in the log file.
		 * We must return the number of chunks in the file though.
		 */
		union {
			unsigned int	clientid;
			int		blockcount;
		} join;
		
		struct {
			unsigned int	clientid;
			int		elapsed;	/* Stats only */
		} leave;

		/*
		 * A data block, indexed by chunk,block.
		 */
		struct {
			int		chunk;
			int		block;
			char		buf[BLOCKSIZE];
		} block;

		/*
		 * A request for a data block, indexed by chunk,block.
		 */
		struct {
			int		chunk;
			int		block;
			int		count;	/* Number of blocks */
		} request;

		/*
		 * Leave reporting client params/stats
		 */
		struct {
			unsigned int	clientid;
			int		elapsed;
			ClientStats_t	stats;
		} leave2;
	} msg;
} Packet_t;
#define PKTTYPE_REQUEST		1
#define PKTTYPE_REPLY		2

#define PKTSUBTYPE_JOIN		1
#define PKTSUBTYPE_LEAVE	2
#define PKTSUBTYPE_BLOCK	3
#define PKTSUBTYPE_REQUEST	4
#define PKTSUBTYPE_LEAVE2	5

/*
 * Protos.
 */
int	ClientNetInit(void);
int	ServerNetInit(void);
int	PacketReceive(Packet_t *p);
void	PacketSend(Packet_t *p, int *resends);
void	PacketReply(Packet_t *p);
int	PacketValid(Packet_t *p, int nchunks);
char   *CurrentTimeString(void);
int	sleeptime(unsigned int usecs, char *str);
int	fsleep(unsigned int usecs);
void	ClientStatsDump(unsigned int id, ClientStats_t *stats);

/*
 * Globals
 */
extern int		debug;
extern int		portnum;
extern int		broadcast;
extern struct in_addr	mcastaddr;
extern struct in_addr	mcastif;
extern char	       *filename;
extern int		clockres;
