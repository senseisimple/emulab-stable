/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2011 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Shared for defintions for frisbee client/server code.
 */

#include <inttypes.h>
#include <limits.h>	/* CHAR_BIT */
#include "log.h"

/*
 * Ethernet MTU (1514 or 9000) - eth header (14) - min UDP/IP (28) - BLOCK msg
 * header (24).
 */
#ifdef JUMBO
#define MAXPACKETDATA	8934
#else
#define MAXPACKETDATA	1448
#endif

/*
 * Images are broken into chunks which are the standalone unit of decompression
 * Chunks are broken into blocks which are the unit of transmission
 */
#ifdef JUMBO
#define MAXCHUNKSIZE	128
#define MAXBLOCKSIZE	8192
#else
#define MAXCHUNKSIZE	1024
#define MAXBLOCKSIZE	1024
#endif

/*
 * Make sure we can fit a block in a single ethernet MTU.
 */
#if MAXBLOCKSIZE > MAXPACKETDATA
#error "Invalid block size"
#endif

/*
 * Make sure we can represent a bitmap of blocks in a single packet.
 * This limits the maximum number of blocks in a chunk to 1448*8 == 11584.
 * With the maximum block size of 1448, this limits a chunk to no more
 * than 16,773,632 bytes (just under 16MB).
 */
#if (MAXCHUNKSIZE%CHAR_BIT) != 0 || (MAXCHUNKSIZE/CHAR_BIT) > MAXPACKETDATA
#error "Invalid chunk size"
#endif

/*
 * Chunk buffers and output write buffers constitute most of the memory
 * used in the system.  These should be sized to fit in the physical memory
 * of the client (forcing pieces of frisbee to be paged out to disk, even
 * if there is a swap disk to use, is not a very efficient way to load disks!)
 *
 * MAXCHUNKBUFS is the number of MAXBLOCKSIZE*MAXCHUNKSIZE chunk buffers used
 * to receive data from the network.  With the default values, these are 1MB
 * each.
 *
 * MAXWRITEBUFMEM is the amount, in MB, of write buffer memory in the client.
 * This is the amount of queued write data that can be pending.  A value of
 * zero means unlimited.
 *
 * The ratio of the number of these two buffer types depends on the ratio
 * of network to disk speed and the degree of compression in the image.
 */
#define MAXCHUNKBUFS	64	/* 64MB with default chunk size */
#define MAXWRITEBUFMEM	64	/* in MB */

/*
 * Socket buffer size, used for both send and receive in client and
 * server right now.  If DYN_SOCKBUFSIZE is set, we find the larger
 * socketbuffer size possible that is less than or equal to SOCKBUFSIZE.
 */
#define SOCKBUFSIZE	(512 * 1024)
#define DYN_SOCKBUFSIZE	1

/*
 * The number of read-ahead chunks that the client will request
 * at a time. No point in requesting too far ahead either, since they
 * are uncompressed/written at a fraction of the network transfer speed.
 * Also, with multiple clients at different stages, each requesting blocks,
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
 * Must be an integer divisor of MAXCHUNKSIZE.
 */
#define SERVER_READ_SIZE	32

/*
 * Parameters for server network usage:
 *
 *	SERVER_BURST_SIZE	Max MAXBLOCKSIZE packets sent in a burst.
 *				Should be a multiple of SERVER_READ_SIZE
 *				Should be less than SOCKBUFSIZE/MAXBLOCKSIZE,
 *				bursts of greater than the send socket
 *				buffer size are almost certain to cause
 *				lost packets.
 *	SERVER_BURST_GAP	Delay in usec between output bursts.
 *				Given the typical scheduling granularity
 *				of 10ms for most unix systems, this
 *				will likely be set to either 0 or 10000.
 *				On FreeBSD we set the clock to 1ms
 *				granularity.
 *
 * Together with the MAXBLOCKSIZE, these two params form a theoretical upper
 * bound on bandwidth consumption for the server.  That upper bound (for
 * ethernet) is:
 *
 *	(1000000 / SERVER_BURST_GAP)		   # bursts per second
 *	* (MAXBLOCKSIZE+24+42) * SERVER_BURST_SIZE # * wire size of a burst
 *
 * which for the default 1k packets, gap of 1ms and burst of 16 packets
 * is about 17.4MB/sec.  That is beyond the capacity of a 100Mb ethernet
 * but with a 1ms granularity clock, the average gap size is going to be
 * 1.5ms yielding 11.6MB/sec.  In practice, the server is ultimately
 * throttled by clients' ability to generate requests which is limited by
 * their ability to decompress and write to disk.
 */
#define SERVER_BURST_SIZE	16
#define SERVER_BURST_GAP	2000

/*
 * Max burst size when doing dynamic bandwidth adjustment.
 * Needs to be large enough to induce loss.
 */ 
#define SERVER_DYNBURST_SIZE	128

/*
 * How long (in usecs) to wait before re-reqesting a chunk.
 * It will take the server more than:
 *
 *	(MAXCHUNKSIZE/SERVER_BURST_SIZE) * SERVER_BURST_GAP
 *
 * usec (0.13 sec with defaults) for each each chunk it pumps out,
 * and we conservatively assume that there are a fair number of other
 * chunks that must be processed before it gets to our chunk.
 *
 * XXX don't like making the client rely on compiled in server constants,
 * lets just set it to 1 second right now.
 */
#define CLIENT_REQUEST_REDO_DELAY	1000000

/*
 * How long for the writer to sleep if there are no blocks currently
 * ready to write.  Allow a full server burst period, assuming that
 * something in the next burst will complete a block.
 */
#define CLIENT_WRITER_IDLE_DELAY	1000

/*
 * Client parameters and statistics.
 */
#define CLIENT_STATS_VERSION	1
typedef struct {
	int32_t	version;
	union {
		struct {
			int32_t	 runsec;
			int32_t	 runmsec;
			int32_t	 delayms;
			uint64_t rbyteswritten;
			uint64_t ebyteswritten;
			int32_t	 chunkbufs;
			int32_t	 maxreadahead;
			int32_t	 maxinprogress;
			int32_t	 pkttimeout;
			int32_t	 startdelay;
			int32_t	 idletimer;
			int32_t	 idledelay;
			int32_t	 redodelay;
			int32_t	 randomize;
			uint32_t nochunksready;
			uint32_t nofreechunks;
			uint32_t dupchunk;
			uint32_t dupblock;
			uint32_t prequests;
			uint32_t recvidles;
			uint32_t joinattempts;
			uint32_t requests;
			uint32_t decompblocks;
			uint32_t writeridles;
			int32_t	 writebufmem;
			uint32_t lostblocks;
			uint32_t rerequests;
		} __attribute__((__packed__)) v1;
		uint32_t limit[256];
	} u;
} __attribute__((__packed__)) ClientStats_t;

typedef struct {
	char	map[MAXCHUNKSIZE/CHAR_BIT];
} BlockMap_t;

/*
 * Packet defs.
 */
typedef struct {
	struct {
		int32_t		type;
		int32_t		subtype;
		int32_t 	datalen; /* Useful amount of data in packet */
		uint32_t	srcip;   /* Filled in by network level. */
	} hdr;
	union {
		/*
		 * Join/leave the Team. Send a randomized ID, and receive
		 * the number of blocks in the file. This is strictly
		 * informational; the info is reported in the log file.
		 * We must return the number of chunks in the file though.
		 */
		union {
			uint32_t	clientid;
			int32_t		blockcount;
		} join;
		
		struct {
			uint32_t	clientid;
			int32_t		elapsed;	/* Stats only */
		} leave;

		/*
		 * A data block, indexed by chunk,block.
		 */
		struct {
			int32_t		chunk;
			int32_t		block;
			int8_t		buf[MAXBLOCKSIZE];
		} block;

		/*
		 * A request for a data block, indexed by chunk,block.
		 */
		struct {
			int32_t		chunk;
			int32_t		block;
			int32_t		count;	/* Number of blocks */
		} request;

		/*
		 * Partial chunk request, a bit map of the desired blocks
		 * for a chunk.  An alternative to issuing multiple standard
		 * requests.  Retries is a hint to the server for congestion
		 * control, non-zero if this is a retry of an earlier request
		 * we made.
		 */
		struct {
			int32_t		chunk;
			int32_t		retries;
			BlockMap_t	blockmap;
		} prequest;

		/*
		 * Join V2 allows:
		 * - client to request a specific chunk/block size
		 *   server will return what it will provide
		 * - server to return the size in bytes
		 *   so that we can transfer files that are not a
		 *   multiple of the block/chunk size
		 * Note the blockcount field remains for vague
		 * compatibility-ish reasons.
		 */
		struct {
			uint32_t	clientid;
			int32_t		blockcount;
			int32_t		chunksize;
			int32_t		blocksize;
			uint64_t		bytecount;
		} join2;

		/*
		 * Leave reporting client params/stats
		 */
		struct {
			uint32_t	clientid;
			int32_t		elapsed;
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
#define PKTSUBTYPE_PREQUEST	6
#define PKTSUBTYPE_JOIN2	7

#ifdef MASTER_SERVER
#include <netinet/in.h>

/* default port number: 0xfbee */
#define MS_PORTNUM	64494

/* imageid length: large enough to hold an ascii encoded SHA 1024 hash */
#define MS_MAXIDLEN	256
/* ditto for signature */
#define MS_MAXSIGLEN	256

/*
 * Master server messages.
 * These are sent via unicast TCP.
 */
typedef struct {
	uint32_t	hostip;
	uint8_t		methods;
	uint8_t		status;
	uint16_t	idlen;
	uint8_t		imageid[MS_MAXIDLEN];
} __attribute__((__packed__)) GetRequest;

typedef struct {
	uint8_t		method;
	uint8_t		isrunning;
	uint16_t	error;	
	uint32_t	servaddr;
	uint32_t	addr;
	uint16_t	port;
	uint16_t	sigtype;
	uint8_t		signature[MS_MAXSIGLEN];
	uint32_t	hisize;
	uint32_t	losize;
} __attribute__((__packed__)) GetReply;

typedef struct {
	uint32_t	hostip;
	uint8_t		status;
	uint16_t	idlen;
	uint8_t		imageid[MS_MAXIDLEN];
	uint32_t	hisize;
	uint32_t	losize;
	uint32_t	mtime;
	uint32_t	timeout;
} __attribute__((__packed__)) PutRequest;

typedef struct {
	uint16_t	error;	
	uint32_t	addr;
	uint16_t	port;
	uint8_t		exists;
	uint16_t	sigtype;
	uint8_t		signature[MS_MAXSIGLEN];
	uint32_t	hisize;
	uint32_t	losize;
	uint32_t	himaxsize;
	uint32_t	lomaxsize;
} __attribute__((__packed__)) PutReply;

typedef struct {
	struct {
		int8_t		version[4];
		int32_t		type;
	} hdr;
	union {
		GetRequest	getrequest;
		GetReply	getreply;
		PutRequest	putrequest;
		PutReply	putreply;
	} body;
} MasterMsg_t;

#define MS_MSGVERS_1		"V01"

#define MS_MSGTYPE_GETREQUEST	1
#define MS_MSGTYPE_GETREPLY	2
#define MS_MSGTYPE_PUTREQUEST	3
#define MS_MSGTYPE_PUTREPLY	4

#define MS_METHOD_UNKNOWN	0
#define MS_METHOD_UNICAST	1
#define MS_METHOD_MULTICAST	2
#define MS_METHOD_BROADCAST	4
#define MS_METHOD_ANY		7

#define MS_SIGTYPE_NONE		0
#define MS_SIGTYPE_MTIME	1
#define MS_SIGTYPE_MD5		2
#define MS_SIGTYPE_SHA1		3

#define MS_ERROR_FAILED		1	/* internal host auth error */
#define MS_ERROR_NOHOST		2	/* no such host */
#define MS_ERROR_NOIMAGE	3	/* no such image */
#define MS_ERROR_NOACCESS	4	/* access not allowed for host */
#define MS_ERROR_NOMETHOD	5	/* not avail to host via method */
#define MS_ERROR_INVALID	6	/* invalid argument */
#define MS_ERROR_TRYAGAIN	7	/* try again later */
#define MS_ERROR_TOOBIG		8	/* attempted PUT is too large */
#define MS_ERROR_BADMTIME	9	/* attempt to set bad mtime */
#define MS_ERROR_NOTIMPL	10	/* operation not implemented */
#endif

/*
 * Protos.
 */
int	GetIP(char *str, struct in_addr *in);
int	GetSockbufSize(void);
int	ClientNetInit(void);
int	ServerNetInit(void);
int	NetMCKeepAlive(void);
unsigned long ClientNetID(void);
int	PacketReceive(Packet_t *p);
void	PacketSend(Packet_t *p, int *resends);
void	PacketReply(Packet_t *p);
int	PacketValid(Packet_t *p, int nchunks);
void	dump_network(void);
#ifdef MASTER_SERVER
int	ClientNetFindServer(in_addr_t, in_port_t, in_addr_t, char *,
			    int, int, int, GetReply *, struct in_addr *);
int	MsgSend(int, MasterMsg_t *, size_t, int);
int	MsgReceive(int, MasterMsg_t *, size_t, int);
#endif

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
