/*
 * Shared for defintions for frisbee client/server code.
 */

#include "log.h"

/*
 * Max number of clients we can support at once. Not likely to be an issue
 * since the amount of per client state is very little.
 */
#define MAXCLIENTS	1000

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
#define MAXCHUNKBUFS	16

/*
 * The number of read-ahead chunks that the client will request
 * at a time. No point in requesting to far ahead either, since they
 * are uncompressed/written at a fraction of the network transfer speed.
 * Also, with multiple clients at different stages, each requesting blocks
 * it is likely that there will be plenty more chunks ready or in progress.
 */
#define MAXREADAHEAD	2
#define MAXINPROGRESS	4

/*
 * Timeout (in usecs) for packet receive. The idletimer number is how
 * many PKT timeouts we allow before requesting more data from the server.
 * That is, if we go TIMEOUT usecs without getting a packet, then ask for
 * more (or on the server side, poll the clients). On the server, side
 * use a timeout to check for dead clients. We want that to be longish.
 */
#define PKTRCV_TIMEOUT		30000
#define CLIENT_IDLETIMER_COUNT	1
#define SERVER_IDLETIMER_COUNT	((300 * 1000000) / PKTRCV_TIMEOUT)

/*
 * Timeout (in seconds!) server will hang around with no active clients.
 */
#define SERVER_INACTIVE_SECONDS	30

/*
 * The number of disk read blocks in a single read on the server.
 * Must be an even divisor of CHUNKSIZE.
 */
#define MAXREADBLOCKS	32

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
		 * the number of blocks in the file. 
		 */
		union {
			unsigned int	clientid;
			int		blockcount;
		} join;

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
	} msg;
} Packet_t;
#define PKTTYPE_REQUEST		1
#define PKTTYPE_REPLY		2

#define PKTSUBTYPE_JOIN		1
#define PKTSUBTYPE_LEAVE	2
#define PKTSUBTYPE_BLOCK	3
#define PKTSUBTYPE_REQUEST	4

/*
 * Protos.
 */
int	ClientNetInit(void);
int	ServerNetInit(void);
int	PacketReceive(Packet_t *p);
int	PacketSend(Packet_t *p);
int	PacketReply(Packet_t *p);
char   *CurrentTimeString(void);
int	fsleep(unsigned int usecs);

/*
 * Globals
 */
extern int		debug;
extern int		portnum;
extern struct in_addr	mcastaddr;
extern struct in_addr	mcastif;
extern char	       *filename;
