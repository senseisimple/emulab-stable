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
extern int		broadcast;
extern struct in_addr	mcastaddr;
extern struct in_addr	mcastif;
extern char	       *filename;
