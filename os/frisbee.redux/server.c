/*
 * Frisbee server
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include "decls.h"
#include "queue.h"

/* Globals */
int		debug = 0;
int		portnum;
struct in_addr	mcastaddr;
struct in_addr	mcastif;
char	       *filename;
struct timeval  InactivityTimeStamp;
long long	TimeCounter;

/*
 * This structure defines the file we are spitting back.
 */
struct FileInfo {
	int	fd;		/* Open file descriptor */
	int	blocks;		/* Number of BLOCKSIZE blocks */
	int	chunks;		/* Number of CHUNKSIZE chunks */
	struct {
		struct in_addr	ipaddr;		/* Client Network Address */
		unsigned int	id;		/* Random ID. */
		long long	lastcontact;	/* Last time heard from */
	} clients[MAXCLIENTS];
	int	numclients;			/* Current count of clients */
};
static struct FileInfo FileInfo;

/*
 * The work queue of regions a client has requested.
 */
typedef struct {
	queue_chain_t	chain;
	int		chunk;		/* Which chunk */
	int		block;		/* Which starting block */
	int		count;		/* How many blocks */
} WQelem_t;
static queue_head_t     WorkQ;
static pthread_mutex_t	WorkQLock;
static pthread_cond_t	WorkQCond;

/*
 * Work queue routines. The work queue is a time ordered list of chunk/blocks
 * pieces that a client is missing. When a request comes in, lock the list
 * and scan it for an existing work item that covers the new request. The new
 * request can be dropped if there already exists a Q item, since the client
 * is going to see that piece eventually.
 *
 * We use a spinlock to guard the work queue, which incidentally will protect
 * malloc/free.
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
	pthread_cond_init(&WorkQCond, NULL);
	queue_init(&WorkQ);
}

static int
WorkQueueEnqueue(int chunk, int block, int blockcount)
{
	WQelem_t	*wqel;
	
	pthread_mutex_lock(&WorkQLock);

	queue_iterate(&WorkQ, wqel, WQelem_t *, chain) {
		if (wqel->chunk == chunk &&
		    wqel->block == block &&
		    wqel->count == blockcount) {
			pthread_mutex_unlock(&WorkQLock);
			return 0;
		}
	}

	if ((wqel = (WQelem_t *) calloc(1, sizeof(WQelem_t))) == NULL)
		fatal("WorkQueueEnqueue: No more memory");

	wqel->chunk = chunk;
	wqel->block = block;
	wqel->count = blockcount;
	queue_enter(&WorkQ, wqel, WQelem_t *, chain);

/*	pthread_cond_signal(&WorkQCond); */
	pthread_mutex_unlock(&WorkQLock);
	return 1;
}

static int
WorkQueueDequeue(int *chunk, int *block, int *blockcount)
{
	WQelem_t	*wqel;

	pthread_mutex_lock(&WorkQLock);
#if 0
	while (queue_empty(&WorkQ)) {
		struct timeval	tval;
		struct timespec tspec;
		int foo;

		if (gettimeofday(&tval, NULL) < 0) {
			pwarning("gettimeofday");
			pthread_mutex_unlock(&WorkQLock);
			return 0;
		}
		TIMEVAL_TO_TIMESPEC(&tval, &tspec);
		tspec.tv_sec += 1;

		foo = pthread_cond_timedwait(&WorkQCond, &WorkQLock, &tspec);

		if (foo == ETIMEDOUT) {
			pthread_mutex_unlock(&WorkQLock);
			return 0;
		}
	}
#else
	if (queue_empty(&WorkQ)) {
		pthread_mutex_unlock(&WorkQLock);
		fsleep(5000);
		return 0;
	}
#endif
	queue_remove_first(&WorkQ, wqel, WQelem_t *, chain);
	*chunk = wqel->chunk;
	*block = wqel->block;
	*blockcount = wqel->count;
	free(wqel);

	pthread_mutex_unlock(&WorkQLock);
	return 1;
}

/*
 * A client joins. This message is information in the sense that we use it
 * to determine if any clients are listening. As long as there are, we
 * continue to throw out chunks. We allow for duplicates from a host, which
 * might occur if the reply was lost or if the host crashed and restarted.
 * If a client crashes and never returns, we will find out about it later,
 * when we poll it to see if its alive. 
 */
static void
ClientJoin(Packet_t *p)
{
	struct in_addr	ipaddr = { p->hdr.srcip };
	int		i;

	/*
	 * Check to see if this is a duplicate. I do not think this matters
	 * but lets record the info anyway.
	 */
	for (i = 0; i < MAXCLIENTS; i++) {
		if (FileInfo.clients[i].ipaddr.s_addr == ipaddr.s_addr) {
			log("Duplicate join request from %s at %s!",
			    inet_ntoa(ipaddr), CurrentTimeString());
			break;
		}
	}
	
	if (i == MAXCLIENTS) {
		/*
		 * Okay, not a current subscriber. Record the info in a free
		 * client slot. 
		 */
		for (i = 0; i < FileInfo.numclients; i++) {
			if (! FileInfo.clients[i].ipaddr.s_addr)
				break;
		}
		if (i == MAXCLIENTS)
			fatal("Too many subscribers!");

		FileInfo.clients[i].lastcontact   = TimeCounter;
		FileInfo.clients[i].id            = p->msg.join.clientid;
		FileInfo.clients[i].ipaddr.s_addr = ipaddr.s_addr;
		FileInfo.numclients++;
	}
	log("Client %s (id %u) joins at %s!",
	    inet_ntoa(ipaddr), p->msg.join.clientid, CurrentTimeString());

	/*
	 * Return fileinfo even for duplicates since that could mean that
	 * the last reply got lost. Duplicate replies are harmless.
	 */
	p->hdr.type       = PKTTYPE_REPLY;
	p->hdr.datalen    = sizeof(p->msg.join);
	p->msg.join.blockcount = FileInfo.blocks;
	PacketReply(p);

	/*
	 * Whenever a client joins, add the first chunk to the work queue.
	 */
	WorkQueueEnqueue(0, 0, CHUNKSIZE);
}

/*
 * A client leaves. Not much to it. We return a reply.
 */
static void
ClientLeave(Packet_t *p)
{
	struct in_addr	ipaddr = { p->hdr.srcip };
	int		i;

	/*
	 * Check to see if this is a valid leave message.
	 */
	for (i = 0; i < MAXCLIENTS; i++) {
		if (FileInfo.clients[i].ipaddr.s_addr == ipaddr.s_addr) {
			if (FileInfo.clients[i].id != p->msg.join.clientid) {
				log("ClientLeave: Bad ID for %s. ID %u!=%u!",
				    inet_ntoa(ipaddr), p->msg.join.clientid,
				    FileInfo.clients[i].id);    
			}
			break;
		}
	}
	if (i == MAXCLIENTS) {
		log("ClientLeave: No such client %s (id %u) connected!",
		    inet_ntoa(ipaddr), p->msg.join.clientid);
		return;
	}

	FileInfo.clients[i].ipaddr.s_addr = 0;
	FileInfo.clients[i].id            = 0;
	FileInfo.numclients--;

	p->hdr.type       = PKTTYPE_REPLY;
	p->hdr.datalen    = sizeof(p->msg.join);
	PacketReply(p);

	/*
	 * Whenever the count goes to zero, set the idletimer. We use this
	 * to dump out of the program. 
	 */
	if (! FileInfo.numclients)
		gettimeofday(&InactivityTimeStamp, 0);
}

/*
 * Check for dead clients.
 */
static void
CheckClients(void)
{
	int	i, oldcount = FileInfo.numclients;

	for (i = 0; i < MAXCLIENTS; i++) {
		if (FileInfo.clients[i].ipaddr.s_addr &&
		    (TimeCounter - FileInfo.clients[i].lastcontact) >
		    SERVER_IDLETIMER_COUNT) {
			log("Client %s (id %u) idle for too long.",
			    inet_ntoa(FileInfo.clients[i].ipaddr),
			    FileInfo.clients[i].id);

			FileInfo.clients[i].ipaddr.s_addr = 0;
			FileInfo.clients[i].id            = 0;
			FileInfo.numclients--;
		}
	}
	
	/*
	 * Whenever the count goes to zero, set the idletimer. We use this
	 * to dump out of the program. 
	 */
	if (oldcount && ! FileInfo.numclients)
		gettimeofday(&InactivityTimeStamp, 0);
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
	int		enqueued, i;

	if (block + count > CHUNKSIZE)
		fatal("Bad request from %s - chunk:%d block:%d size:%d", 
		      inet_ntoa(ipaddr), chunk, block, count);
	
	enqueued = WorkQueueEnqueue(chunk, block, count);

	if (debug) {
		log("Client %s requests chunk:%d block:%d size:%d new:%d",
		    inet_ntoa(ipaddr), chunk, block, count, enqueued);
	}

	/*
	 * Set the lastcontact for this client. This serves as a crude
	 * mechansim for determining when a client crashes. If an existing
	 * client goes for too long without being heard from, we drop it
	 * from the client list so that we can idle (and then exit if
	 * we go inactive for too long).
	 */
	for (i = 0; i < MAXCLIENTS; i++) {
		if (FileInfo.clients[i].ipaddr.s_addr == ipaddr.s_addr) {
		    FileInfo.clients[i].lastcontact = TimeCounter;
		}
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

	if (debug)
		log("Server pthread starting up ...");
	
	while (1) {
		if (PacketReceive(p) < 0) {
			TimeCounter++;
			continue;
		}

		switch (p->hdr.subtype) {
		case PKTSUBTYPE_JOIN:
			ClientJoin(p);
			break;
		case PKTSUBTYPE_LEAVE:
			ClientLeave(p);
			break;
		case PKTSUBTYPE_REQUEST:
			ClientRequest(p);
			break;
		default:
			fatal("ServerRecvThread: Bad packet type!");
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
	int		chunk, block, blockcount, cc, j;
	int		startblock, lastblock, throttle;
	Packet_t	packet, *p = &packet;
	static char	databuf[MAXREADBLOCKS * BLOCKSIZE];
	off_t		offset;

	while (1) {
		/*
		 * We want to exit the program if no clients connect for a
		 * long time.
		 */
		if (! FileInfo.numclients) {
			struct timeval  estamp;

			gettimeofday(&estamp, 0);

			if ((estamp.tv_sec - InactivityTimeStamp.tv_sec) >
			    SERVER_INACTIVE_SECONDS) {
				log("No clients for too long!");
				return;
			}
			fsleep(10000);
		}
			
		/*
		 * Look for a WorkQ item to process. When there is nothing
		 * to process, check to see if any of the clients went away
		 * mid stream. We want to dump those clients so that we can
		 * inactive after a while (see above test).
		 */
		if (! WorkQueueDequeue(&chunk, &startblock, &blockcount)) {
			CheckClients();
			continue;
		}
		lastblock = startblock + blockcount;
		throttle  = 0;

		/* Offset within the file */
		offset = (((off_t) BLOCKSIZE * chunk * CHUNKSIZE) +
			  ((off_t) BLOCKSIZE * startblock));

		for (block = startblock; block < lastblock; ) {
			int	readcount;
			int	readsize;
			
			/*
			 * Read blocks of data from disk.
			 */
			if (lastblock - block > MAXREADBLOCKS)
				readcount = MAXREADBLOCKS;
			else
				readcount = lastblock - block;
			readsize = readcount * BLOCKSIZE;

			if ((cc = pread(FileInfo.fd, databuf,
					readsize, offset)) <= 0) {
				if (cc < 0)
					pfatal("Reading File");
				fatal("EOF on file");
			}
			if (cc != readsize)
				fatal("Short read: %d!=%d", cc, readsize);

			for (j = 0; j < readcount; j++) {
				p->hdr.type    = PKTTYPE_REQUEST;
				p->hdr.subtype = PKTSUBTYPE_BLOCK;
				p->hdr.datalen = sizeof(p->msg.block);
				p->msg.block.chunk = chunk;
				p->msg.block.block = block + j;
				memcpy(p->msg.block.buf,
				       &databuf[j * BLOCKSIZE],
				       BLOCKSIZE);

				PacketSend(p);
			}
			offset   += readsize;
			block    += readcount;
			throttle += readcount;

			if (throttle > 64) {
				fsleep(10000);
				throttle = 0;
			}
		}

		/*
		 * Delay a bit to keep from overloading the network.
		 *
		 * XXX - I think this timeout value needs to be tuned on
		 * the fly. Not sure yet how that would be done. 
		 if (blockcount > (2 * MAXREADBLOCKS))
			 fsleep(50000);
		 */
	}
}

char *usagestr = 
 "usage: frisbeed [-d] <-p #> <-m mcastaddr> <filename>\n"
 " -d              Turn on debugging. Multiple -d options increase output.\n"
 " -p portnum	   Specify a port number to listen on.\n"
 " -m mcastaddr	   Specify a multicast address in dotted notation.\n"
 " -i mcastif	   Specify a multicast interface in dotted notation.\n"
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
	int		ch, fd;
	pthread_t	child_pid;
	off_t		fsize;

	while ((ch = getopt(argc, argv, "dhp:m:i:")) != -1)
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
		case 'h':
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	if (!portnum || ! mcastaddr.s_addr)
		usage();

	ServerLogInit();
	WorkQueueInit();
	ServerNetInit();

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

	FileInfo.fd     = fd;
	FileInfo.blocks = (int) roundup(fsize, BLOCKSIZE) / BLOCKSIZE;
	FileInfo.chunks = FileInfo.blocks / CHUNKSIZE;
	log("Opened %s: %d blocks", filename, FileInfo.blocks);

	/*
	 * Create the subthread to listen for packets.
	 */
	if (pthread_create(&child_pid, NULL, ServerRecvThread, (void *)0)) {
		fatal("Failed to create pthread!");
	}
	gettimeofday(&InactivityTimeStamp, 0);
	
	PlayFrisbee();

	/*
	 * Exit from main thread will kill all the children.
	 */
	log("Exiting!");
	exit(0);
}

