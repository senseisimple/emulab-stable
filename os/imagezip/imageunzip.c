/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Usage: imageunzip <input file>
 *
 * Writes the uncompressed data to stdout.
 *
 * XXX: Be nice to use pwrite. That would simplify the code a alot, but
 * you cannot pwrite to a device that does not support seek (stdout),
 * and I want to retain that capability.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <zlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/disklabel.h>
#include <pthread.h>
#include "imagehdr.h"
#include "queue.h"

long long totaledata = 0;
long long totalrdata = 0;

/*
 * In slice mode, we read the DOS MBR to find out where the slice is on
 * the raw disk, and then seek to that spot. This avoids sillyness in
 * the BSD kernel having to do with disklabels. 
 *
 * These numbers are in sectors.
 */
static long		outputminsec	= 0;
static long		outputmaxsec	= 0;
static long long	outputmaxsize	= 0;	/* Sanity check */

/* Why is this not defined in a public header file? */
#define BOOT_MAGIC	0xAA55

#define CHECK_ERR(err, msg) { \
    if (err != Z_OK) { \
        fprintf(stderr, "%s error: %d\n", msg, err); \
        exit(1); \
    } \
}

#define SECSIZE 512
#define BSIZE	(32 * 1024)
#define OUTSIZE (8  * BSIZE)
char		outbuf[OUTSIZE + SECSIZE], zeros[BSIZE];

static int	 debug  = 0;
static int	 outfd;
static int	 doseek = 0;
static int	 dofill = 0;
static int	 nothreads = 0;
static pthread_t child_pid;
static int	 rdycount;
#ifndef FRISBEE
static int	 infd;
static int	 version= 0;
static unsigned	 fillpat= 0;
static int	 dots   = 1;
static int	 dotcol;
static char	 chunkbuf[SUBBLOCKSIZE];
static struct timeval stamp;
#endif
static void	 threadinit(void);
static void	 threadwait(void);
static void	 threadquit(void);
int		 readmbr(int slice);
int		 inflate_subblock(char *);
void		 writezeros(off_t offset, size_t zcount);
void	        *DiskWriter(void *arg);

static int	writeinprogress; /* XXX */

/*
 * Some stats
 */
unsigned long decompidles;
unsigned long writeridles;

/*
 * A queue of ready to write data blocks.
 */
typedef struct {
	queue_chain_t	chain;
	off_t		offset;
	size_t		size;
	int		zero;
} readyhdr_t;

typedef struct {
	readyhdr_t	header;
	unsigned char	buf[OUTSIZE + SECSIZE - sizeof(readyhdr_t)];
} readyblock_t;
static queue_head_t	readyqueue;
static readyblock_t	*freelist;
#define READYQSIZE	256

static pthread_mutex_t	freelist_mutex, readyqueue_mutex;	
static pthread_cond_t	freelist_condvar, readyqueue_condvar;	

static void
dowrite_request(off_t offset, int cc, void *buf)
{
	readyhdr_t *hdr;
	
	if (!buf) {
		/*
		 * Null buf means its a request to zero.
		 */
		if (nothreads) {
			writezeros(offset, cc);
			return;
		}

		if ((hdr = (readyhdr_t *) malloc(sizeof(readyhdr_t))) == NULL){
			fprintf(stderr, "Out of memory\n");
			exit(1);
		}
		hdr->zero   = 1;
		hdr->offset = offset;
		hdr->size   = cc;
	}
	else {
		readyblock_t	*rdyblk;

		if (nothreads) {
			int count = pwrite(outfd, buf, cc, offset);
			if (count != cc) {
				printf("Short write!\n");
				exit(1);
			}
			return;
		}

		/*
		 * Try to allocate a block. Wait if none available.
		 */
		pthread_mutex_lock(&freelist_mutex);
		if (! freelist) {
			decompidles++;
			do {
				pthread_cond_wait(&freelist_condvar,
						  &freelist_mutex);
			} while (! freelist);
		}
		rdyblk   = freelist;
		freelist = (void *) rdyblk->header.chain.next;
		pthread_mutex_unlock(&freelist_mutex);

		rdyblk->header.offset = offset;
		rdyblk->header.size   = cc;
		memcpy(rdyblk->buf, buf, cc);
		hdr = (readyhdr_t *) rdyblk;
	}
	pthread_mutex_lock(&readyqueue_mutex);
	queue_enter(&readyqueue, hdr, readyhdr_t *, chain);
	pthread_mutex_unlock(&readyqueue_mutex);
	pthread_cond_signal(&readyqueue_condvar);
}

static inline int devread(int fd, void *buf, size_t size)
{
	assert((size & (SECSIZE-1)) == 0);
	return read(fd, buf, size);
}

#ifndef FRISBEE
static void
usage(void)
{
	fprintf(stderr, "usage: "
		"imageunzip options <input filename> [output filename]\n"
		" -v              Print version info and exit\n"
		" -s slice        Output to DOS slice (DOS numbering 1-4)\n"
		"                 NOTE: Must specify a raw disk device.\n"
		" -z              Write zeros to free blocks.\n"
		" -p pattern      Write 32 bit pattern to free blocks.\n"
		"                 NOTE: Use -z/-p to avoid seeking.\n"
		" -o              Output 'dots' indicating progress\n"
		" -n              Single threaded (slow) mode\n"
		" -d              Turn on progressive levels of debugging\n");
	exit(1);
}	

int
main(int argc, char **argv)
{
	int		i, ch, slice = 0;
	extern char	build_info[];
	struct timeval  estamp;

	while ((ch = getopt(argc, argv, "vdhs:zp:on")) != -1)
		switch(ch) {
		case 'd':
			debug++;
			break;

		case 'n':
			nothreads++;
			break;

		case 'v':
			version++;
			break;

		case 'o':
			dots++;
			break;

		case 's':
			slice = atoi(optarg);
			break;

		case 'p':
			fillpat = strtoul(optarg, NULL, 0);
		case 'z':
			dofill++;
			break;

		case 'h':
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (version || debug) {
		fprintf(stderr, "%s\n", build_info);
		if (version)
			exit(0);
	}

	if (argc < 1 || argc > 2)
		usage();
	if (argc == 1 && slice) {
		fprintf(stderr, "Cannot specify a slice when using stdout!\n");
		usage();
	}

	if (fillpat) {
		unsigned	*bp = (unsigned *) &zeros;

		for (i = 0; i < sizeof(zeros)/sizeof(unsigned); i++)
			*bp++ = fillpat;
	}

	if (strcmp(argv[0], "-")) {
		if ((infd = open(argv[0], O_RDONLY, 0666)) < 0) {
			perror("opening input file");
			exit(1);
		}
	}
	else
		infd = fileno(stdin);

	if (argc == 2) {
		if ((outfd =
		     open(argv[1], O_RDWR|O_CREAT|O_TRUNC, 0666)) < 0) {
			perror("opening output file");
			exit(1);
		}
		doseek = !dofill;
	}
	else
		outfd = fileno(stdout);

	if (slice) {
		off_t	minseek;
		
		if (readmbr(slice)) {
			fprintf(stderr, "Failed to read MBR\n");
			exit(1);
		}
		minseek = ((off_t) outputminsec) * SECSIZE;
		
		if (lseek(outfd, minseek, SEEK_SET) < 0) {
			perror("Setting seek pointer to slice");
			exit(1);
		}
	}

	threadinit();
	gettimeofday(&stamp, 0);
	
	while (1) {
		int	count = sizeof(chunkbuf);
		char	*bp   = chunkbuf;
		
		/*
		 * Decompress one subblock at a time. We read the entire
		 * chunk and hand it off. Since we might be reading from
		 * stdin, we have to make sure we get the entire amount.
		 */
		while (count) {
			int	cc;
			
			if ((cc = read(infd, bp, count)) <= 0) {
				if (cc == 0)
					goto done;
				perror("reading zipped image");
				exit(1);
			}
			count -= cc;
			bp    += cc;
		}
		if (inflate_subblock(chunkbuf))
			break;
	}
 done:
	close(infd);

	/* This causes the output queue to drain */
	threadquit();
	
	gettimeofday(&estamp, 0);
	estamp.tv_sec -= stamp.tv_sec;
	if (dots) {
		while (dotcol++ <= 60)
			printf(" ");
		
		printf("%4ld %13qd\n", estamp.tv_sec, totaledata);
	}
	else {
		printf("Done in %ld seconds!\n", estamp.tv_sec);
	}
	fprintf(stderr, "%lu %lu %d\n", decompidles, writeridles, rdycount);
	return 0;
}
#else
/*
 * When compiled for frisbee, act as a library.
 */
int
ImageUnzipInit(char *filename, int slice, int dbg, int zero, int goslow)
{
	if (outfd >= 0)
		close(outfd);

	if ((outfd = open(filename, O_RDWR|O_CREAT|O_TRUNC, 0666)) < 0) {
		perror("opening output file");
		exit(1);
	}
	if (zero)
		dofill = doseek = 1;
	else
		doseek = 1;
	debug     = dbg;
	nothreads = goslow;

	if (slice) {
		off_t	minseek;
		
		if (readmbr(slice)) {
			fprintf(stderr, "Failed to read MBR\n");
			exit(1);
		}
		minseek = ((off_t) outputminsec) * SECSIZE;
		
		if (lseek(outfd, minseek, SEEK_SET) < 0) {
			perror("Setting seek pointer to slice");
			exit(1);
		}
	}
	threadinit();
	return 0;
}

void
ImageUnzipFlush(void)
{
	threadwait();
}

int
ImageUnzipQuit(void)
{
	threadquit();
	fprintf(stderr, "Wrote %qd bytes (%qd actual)\n",
		totaledata, totalrdata);
	fprintf(stderr, "%lu %lu %d\n", decompidles, writeridles, rdycount);
	return 0;
}
#endif

static void *readyqueuemem;

static void
threadinit(void)
{
	int		i;
	readyblock_t   *ptr;
	static int	called;

	if (nothreads)
		return;

	decompidles = writeridles = 0;

	/*
	 * Allocate blocks for the ready queue.
	 */
	queue_init(&readyqueue);
	if ((ptr = (readyblock_t *) malloc(sizeof(readyblock_t) * READYQSIZE))
	    == NULL) {
		fprintf(stderr, "Out of memory!\n");
		exit(1);
	}
	readyqueuemem = ptr;
	for (i = 0; i < READYQSIZE; i++, ptr++) {
		ptr->header.zero       = 0;
		ptr->header.chain.next = (void *) freelist;
		freelist = ptr;
	}
	if (!called) {
		called = 1;
		pthread_mutex_init(&freelist_mutex, 0);
		pthread_cond_init(&freelist_condvar, 0);
		pthread_mutex_init(&readyqueue_mutex, 0);
		pthread_cond_init(&readyqueue_condvar, 0);
	}

	if (pthread_create(&child_pid, NULL, DiskWriter, (void *)0)) {
		fprintf(stderr, "Failed to create pthread!\n");
		exit(1);
	}
}

static void
threadwait(void)
{
	int		done;

	if (nothreads)
		return;

	while (1) {
		pthread_mutex_lock(&readyqueue_mutex);
		done = (queue_empty(&readyqueue) && !writeinprogress);
		pthread_mutex_unlock(&readyqueue_mutex);
		if (done)
			return;
		usleep(300000);
	}
}

static void
threadquit(void)
{
	void	       *ignored;

	if (nothreads)
		return;

	threadwait();
	pthread_cancel(child_pid);
	pthread_join(child_pid, &ignored);
	free(readyqueuemem);
	freelist = 0;
}

int
inflate_subblock(char *chunkbufp)
{
	int		cc, err, count, ibsize = 0, ibleft = 0;
	z_stream	d_stream; /* inflation stream */
	char		*bp;
	struct blockhdr *blockhdr;
	struct region	*curregion;
	off_t		offset, size;
	int		chunkbytes = SUBBLOCKSIZE;
	
	d_stream.zalloc   = (alloc_func)0;
	d_stream.zfree    = (free_func)0;
	d_stream.opaque   = (voidpf)0;
	d_stream.next_in  = 0;
	d_stream.avail_in = 0;
	d_stream.next_out = 0;

	err = inflateInit(&d_stream);
	CHECK_ERR(err, "inflateInit");

	/*
	 * Grab the header. It is uncompressed, and holds the real
	 * image size and the magic number. Advance the pointer too.
	 */
	blockhdr    = (struct blockhdr *) chunkbufp;
	chunkbufp  += DEFAULTREGIONSIZE;
	chunkbytes -= DEFAULTREGIONSIZE;
	
	if (blockhdr->magic != COMPRESSED_MAGIC) {
		fprintf(stderr, "Bad Magic Number!\n");
		exit(1);
	}
	curregion = (struct region *) (blockhdr + 1);

	/*
	 * Start with the first region. 
	 */
	offset = curregion->start * (off_t) SECSIZE;
	size   = curregion->size  * (off_t) SECSIZE;
	assert(size);
	curregion++;
	blockhdr->regioncount--;

	if (debug == 1)
		fprintf(stderr, "Decompressing: %14qd --> ", offset);

	while (1) {
		/*
		 * Read just up to the end of compressed data.
		 */
		count              = blockhdr->size;
		blockhdr->size    -= count;
		d_stream.next_in   = chunkbufp;
		d_stream.avail_in  = count;
		chunkbufp	  += count;
		chunkbytes	  -= count;
		assert(chunkbytes >= 0);
	inflate_again:
		/*
		 * Must operate on multiples of the sector size!
		 */
		if (ibleft) {
			memcpy(outbuf, &outbuf[ibsize - ibleft], ibleft);
		}
		d_stream.next_out  = &outbuf[ibleft];
		d_stream.avail_out = OUTSIZE;

		err = inflate(&d_stream, Z_SYNC_FLUSH);
		if (err != Z_OK && err != Z_STREAM_END) {
			fprintf(stderr, "inflate failed, err=%d\n", err);
			exit(1);
		}
		ibsize = (OUTSIZE - d_stream.avail_out) + ibleft;
		count  = ibsize & ~(SECSIZE - 1);
		ibleft = ibsize - count;
		bp     = outbuf;

		while (count) {
			/*
			 * Move data into the output block only as far as
			 * the end of the current region. Since outbuf is
			 * same size as rdyblk->buf, its guaranteed to fit.
			 */
			if (count < size)
				cc = count;
			else
				cc = size;

			/*
			 * Put it on the output queue.
			 */
			dowrite_request(offset, cc, bp);

			if (debug == 2) {
				fprintf(stderr,
					"%12qd %8d %8d %12qd %10qd %8d %5d %8d"
					"\n",
					offset, cc, count, totaledata, size,
					ibsize, ibleft, d_stream.avail_in);
			}

			count  -= cc;
			bp     += cc;
			size   -= cc;
			offset += cc;
			totaledata  += cc;
			assert(count >= 0);
			assert(size  >= 0);

			/*
			 * Hit the end of the region. Need to figure out
			 * where the next one starts. We write a block of
			 * zeros in the empty space between this region
			 * and the next. We can lseek, but only if
			 * not writing to stdout. 
			 */
			if (! size) {
				off_t	    newoffset;

				/*
				 * No more regions. Must be done.
				 */
				if (!blockhdr->regioncount)
					break;

				newoffset = curregion->start * (off_t) SECSIZE;
				size      = curregion->size  * (off_t) SECSIZE;
				assert(size);
				curregion++;
				blockhdr->regioncount--;

				if (dofill) {
					assert((newoffset-offset) > 0);
					dowrite_request(offset,
							newoffset-offset,
							NULL);
				} else
					totaledata += (newoffset - offset);
				offset = newoffset;
			}
		}
		if (d_stream.avail_in)
			goto inflate_again;

		if (err == Z_STREAM_END)
			break;
	}
	err = inflateEnd(&d_stream);
	CHECK_ERR(err, "inflateEnd");

	assert(blockhdr->regioncount == 0);
	assert(size == 0);
	assert(blockhdr->size == 0);

#ifndef FRISBEE
	if (debug == 1) {
		fprintf(stderr, "%14qd\n", offset);
	}
	else if (dots) {
		struct timeval  estamp;

		gettimeofday(&estamp, 0);
		estamp.tv_sec -= stamp.tv_sec;
		
		printf(".");
		fflush(stdout);
		if (dotcol++ > 59) {
			dotcol = 0;
			printf("%4ld %13qd\n", estamp.tv_sec, totaledata);
		}
	}
#endif

	return 0;
}

void
writezeros(off_t offset, size_t zcount)
{
	int	zcc;

	if (doseek)
		return;

	assert((offset & (SECSIZE-1)) == 0);

	if (lseek(outfd, offset, SEEK_SET) < 0) {
		perror("lseek to write zeros");
		exit(1);
	}

	while (zcount) {
		if (zcount <= BSIZE)
			zcc = (int) zcount;
		else
			zcc = BSIZE;
		
		if ((zcc = write(outfd, zeros, zcc)) != zcc) {
			if (zcc < 0) {
				perror("Writing Zeros");
			}
			exit(1);
		}
		zcount     -= zcc;
		totalrdata += zcc;
	}
}

/*
 * Parse the DOS partition table to set the bounds of the slice we
 * are writing to. 
 */
int
readmbr(int slice)
{
	int		cc;
	struct doslabel {
		char		align[sizeof(short)];	/* Force alignment */
		char		pad2[DOSPARTOFF];
		struct dos_partition parts[NDOSPART];
		unsigned short  magic;
	} doslabel;
#define DOSPARTSIZE \
	(DOSPARTOFF + sizeof(doslabel.parts) + sizeof(doslabel.magic))

	if (slice < 1 || slice > 4) {
		fprintf(stderr, "Slice must be 1, 2, 3, or 4\n");
 		return 1;
	}

	if ((cc = devread(outfd, doslabel.pad2, DOSPARTSIZE)) < 0) {
		perror("Could not read DOS label");
		return 1;
	}
	if (cc != DOSPARTSIZE) {
		fprintf(stderr, "Could not get the entire DOS label\n");
 		return 1;
	}
	if (doslabel.magic != BOOT_MAGIC) {
		fprintf(stderr, "Wrong magic number in DOS partition table\n");
 		return 1;
	}

	outputminsec  = doslabel.parts[slice-1].dp_start;
	outputmaxsec  = doslabel.parts[slice-1].dp_start +
		        doslabel.parts[slice-1].dp_size;
	outputmaxsize = ((long long) (outputmaxsec - outputminsec)) * SECSIZE;

	if (debug) {
		fprintf(stderr, "Slice Mode: S:%d min:%ld max:%ld size:%qd\n",
			slice, outputminsec, outputmaxsec, outputmaxsize);
	}
	return 0;
}

void *
DiskWriter(void *arg)
{
	readyblock_t	*rdyblk = 0;
	off_t		offset;
	size_t		size;
	ssize_t		cc;

	while (1) {
		pthread_testcancel();

		pthread_mutex_lock(&readyqueue_mutex);
		if (queue_empty(&readyqueue)) {
/*			printf("Writer idle\n"); */
			writeridles++;
			do {
				pthread_cond_wait(&readyqueue_condvar,
						  &readyqueue_mutex);
				pthread_testcancel();
			} while (queue_empty(&readyqueue));
		}
		queue_remove_first(&readyqueue, rdyblk,
				   readyblock_t *, header.chain);
		writeinprogress = 1; /* XXX */
		pthread_mutex_unlock(&readyqueue_mutex);

		offset = rdyblk->header.offset +
			(((off_t) outputminsec) * SECSIZE);
		assert((offset & (SECSIZE-1)) == 0);
		size   = rdyblk->header.size;

		if (rdyblk->header.zero) {
			writezeros(offset, size);
			free(rdyblk);
			continue;
		}
		rdycount++;

/*		fprintf(stderr, "Writing %d bytes at %qd\n", size, offset); */

		cc = pwrite(outfd, rdyblk->buf, size, offset);
		if (cc != size) {
			printf("Short write!\n");
			exit(1);
		}
		totalrdata += cc;
		writeinprogress = 0; /* XXX, ok as unlocked access */

		pthread_mutex_lock(&freelist_mutex);
		rdyblk->header.chain.next = (void *) freelist;
		freelist = rdyblk;
		pthread_mutex_unlock(&freelist_mutex);
		pthread_cond_signal(&freelist_condvar);
	}
}
