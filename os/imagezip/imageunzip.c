/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003 University of Utah and the Flux Group.
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
#include <string.h>
#include <zlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/disklabel.h>
#include "imagehdr.h"
#include "queue.h"
#ifndef NOTHREADS
#include <pthread.h>
#endif

/*
 * Define this if you want to test frisbee's random presentation of chunks
 */
#ifndef FRISBEE
#define FAKEFRISBEE
#endif

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

#define sectobytes(s)	((off_t)(s) * SECSIZE)
#define bytestosec(b)	(uint32_t)((b) / SECSIZE)

#define OUTSIZE (256 * 1024)
char		outbuf[OUTSIZE + SECSIZE], zeros[OUTSIZE];

static int	 dostype = -1;
static int	 slice = 0;
static int	 debug = 0;
static int	 outfd;
static int	 dofill = 0;
static int	 nothreads = 0;
static int	 rdycount;
static int	 imageversion = 1;
#ifndef FRISBEE
static int	 infd;
static int	 version= 0;
static unsigned	 fillpat= 0;
static int	 dots   = 0;
static int	 dotcol;
static char	 chunkbuf[SUBBLOCKSIZE];
static struct timeval stamp;
#endif
int		 readmbr(int slice);
int		 fixmbr(int slice, int dtype);
static int	 inflate_subblock(char *);
void		 writezeros(off_t offset, off_t zcount);
void		 writedata(off_t offset, size_t count, void *buf);

static void	getrelocinfo(blockhdr_t *hdr);
static void	applyrelocs(off_t offset, size_t cc, void *buf);

static int	 seekable;
static off_t	 nextwriteoffset;

static int	imagetoobigwarned;

#ifdef FAKEFRISBEE
#include <sys/stat.h>

static int	dofrisbee;
static int	*chunklist, *nextchunk;
#endif

/*
 * Some stats
 */
unsigned long decompidles;
unsigned long writeridles;

#ifdef NOTHREADS
#define		threadinit()
#define		threadwait()
#define		threadquit()
#else
static void	 threadinit(void);
static void	 threadwait(void);
static void	 threadquit(void);
static void	*DiskWriter(void *arg);

static int	writeinprogress; /* XXX */
static pthread_t child_pid;
static pthread_mutex_t	freelist_mutex, readyqueue_mutex;	
static pthread_cond_t	freelist_condvar, readyqueue_condvar;	

/*
 * A queue of ready to write data blocks.
 */
typedef struct {
	queue_chain_t	chain;
	off_t		offset;
	off_t		size;
	int		zero;
} readyhdr_t;

typedef struct {
	readyhdr_t	header;
	unsigned char	buf[OUTSIZE + SECSIZE];
} readyblock_t;
static queue_head_t	readyqueue;
static readyblock_t	*freelist;
#define READYQSIZE	256
#endif

static void
dowrite_request(off_t offset, off_t size, void *buf)
{
#ifndef NOTHREADS
	readyhdr_t *hdr;
#endif
	
	/*
	 * Adjust for partition start and ensure data fits
	 * within partition boundaries.
	 */
	offset += sectobytes(outputminsec);
	assert((offset & (SECSIZE-1)) == 0);
	if (outputmaxsec > 0 && offset + size > sectobytes(outputmaxsec)) {
		if (!imagetoobigwarned) {
			fprintf(stderr, "WARNING: image too large "
				"for target slice, truncating\n");
			imagetoobigwarned = 1;
		}
		if (offset > sectobytes(outputmaxsec))
			return;
		size = sectobytes(outputmaxsec) - offset;
	}

	totaledata += size;

	if (nothreads) {
		/*
		 * Null buf means its a request to zero.
		 * If we are not filling, just return.
		 */
		if (buf == NULL) {
			if (dofill)
				writezeros(offset, size);
		} else {
			assert(size <= OUTSIZE+SECSIZE);

			/*
			 * Handle any relocations
			 */
			applyrelocs(offset, (size_t)size, buf);
			writedata(offset, (size_t)size, buf);
		}
		return;
	}

#ifndef NOTHREADS
	if (buf == NULL) {
		if (!dofill)
			return;

		if ((hdr = (readyhdr_t *)malloc(sizeof(readyhdr_t))) == NULL) {
			fprintf(stderr, "Out of memory\n");
			exit(1);
		}
		hdr->zero   = 1;
		hdr->offset = offset;
		hdr->size   = size;
	}
	else {
		readyblock_t	*rdyblk;
		size_t		cc;

		assert(size <= OUTSIZE+SECSIZE);
		cc = size;

		/*
		 * Handle any relocations
		 */
		applyrelocs(offset, cc, buf);

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
#endif
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
		" -D DOS-ptype    Set the DOS partition type in slice mode.\n"
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
	int		i, ch;
	extern char	build_info[];
	struct timeval  estamp;

#ifdef NOTHREADS
	nothreads = 1;
#endif
	while ((ch = getopt(argc, argv, "vdhs:zp:onFD:")) != -1)
		switch(ch) {
#ifdef FAKEFRISBEE
		case 'F':
			dofrisbee++;
			break;
#endif
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

		case 'D':
			dostype = atoi(optarg);
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

	if (argc == 2 && strcmp(argv[1], "-")) {
		if ((outfd =
		     open(argv[1], O_RDWR|O_CREAT|O_TRUNC, 0666)) < 0) {
			perror("opening output file");
			exit(1);
		}
	}
	else
		outfd = fileno(stdout);

	/*
	 * If the output device isn't seekable we must modify our behavior:
	 * we cannot really handle slice mode, we must always zero fill
	 * (cannot skip free space) and we cannot use pwrite.
	 */
	if (lseek(outfd, (off_t)0, SEEK_SET) < 0) {
		if (slice) {
			fprintf(stderr, "Output file is not seekable, "
				"cannot specify a slice\n");
			exit(1);
		}
		if (!dofill)
			fprintf(stderr,
				"WARNING: output file is not seekable, "
				"must zero-fill free space\n");
		dofill = 1;
		seekable = 0;
	} else
		seekable = 1;

	if (slice) {
		off_t	minseek;
		
		if (readmbr(slice)) {
			fprintf(stderr, "Failed to read MBR\n");
			exit(1);
		}
		minseek = sectobytes(outputminsec);
		
		if (lseek(outfd, minseek, SEEK_SET) < 0) {
			perror("Setting seek pointer to slice");
			exit(1);
		}
	}

	threadinit();
	gettimeofday(&stamp, 0);
	
#ifdef FAKEFRISBEE
	if (dofrisbee) {
		struct stat st;
		int numchunks, i;

		if (fstat(infd, &st) < 0) {
			fprintf(stderr, "Cannot stat input file\n");
			exit(1);
		}
		numchunks = st.st_size / SUBBLOCKSIZE;

		chunklist = (int *) calloc(numchunks+1, sizeof(*chunklist));
		assert(chunklist != NULL);

		for (i = 0; i < numchunks; i++)
			chunklist[i] = i;
		chunklist[i] = -1;

		srandomdev();
		for (i = 0; i < 50 * numchunks; i++) {
			int c1 = random() % numchunks;
			int c2 = random() % numchunks;
			int t1 = chunklist[c1];
			int t2 = chunklist[c2];

			chunklist[c2] = t1;
			chunklist[c1] = t2;
		}
		nextchunk = chunklist;
	}
#endif

	while (1) {
		int	count = sizeof(chunkbuf);
		char	*bp   = chunkbuf;
		
#ifdef FAKEFRISBEE
		if (dofrisbee) {
			if (*nextchunk == -1)
				goto done;
			if (lseek(infd, (off_t)*nextchunk * SUBBLOCKSIZE,
				  SEEK_SET) < 0) {
				perror("seek failed");
				exit(1);
			}
			nextchunk++;
		}
#endif
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
	
	/* Set the MBR type if necesary */
	if (slice && dostype >= 0)
		fixmbr(slice, dostype);

	gettimeofday(&estamp, 0);
	estamp.tv_sec -= stamp.tv_sec;
	if (debug != 1 && dots) {
		while (dotcol++ <= 60)
			fprintf(stderr, " ");
		
		fprintf(stderr, "%4ld %13qd\n", estamp.tv_sec, totaledata);
	}
	else {
		fprintf(stderr, "Done in %ld seconds!\n", estamp.tv_sec);
		fprintf(stderr, "Wrote %qd bytes (%qd actual)\n",
			totaledata, totalrdata);
	}
	if (debug)
		fprintf(stderr, "decompressor blocked: %lu, "
			"writer idle: %lu, writes performed: %d\n",
			decompidles, writeridles, rdycount);
	return 0;
}
#else
/*
 * When compiled for frisbee, act as a library.
 */
int
ImageUnzipInit(char *filename, int _slice, int _debug, int _fill,
	       int _nothreads, int _dostype)
{
	if (outfd >= 0)
		close(outfd);

	if ((outfd = open(filename, O_RDWR|O_CREAT|O_TRUNC, 0666)) < 0) {
		perror("opening output file");
		exit(1);
	}
	slice     = _slice;
	debug     = _debug;
	dofill    = _fill;
	nothreads = _nothreads;
	dostype   = _dostype;

	/*
	 * If the output device isn't seekable we must modify our behavior:
	 * we cannot really handle slice mode, we must always zero fill
	 * (cannot skip free space) and we cannot use pwrite.
	 */
	if (lseek(outfd, (off_t)0, SEEK_SET) < 0) {
		if (slice) {
			fprintf(stderr, "Output file is not seekable, "
				"cannot specify a slice\n");
			exit(1);
		}
		if (!dofill)
			fprintf(stderr,
				"WARNING: output file is not seekable, "
				"must zero-fill free space\n");
		dofill = 1;
		seekable = 0;
	} else
		seekable = 1;

	if (slice) {
		off_t	minseek;
		
		if (readmbr(slice)) {
			fprintf(stderr, "Failed to read MBR\n");
			exit(1);
		}
		minseek = sectobytes(outputminsec);
		
		if (lseek(outfd, minseek, SEEK_SET) < 0) {
			perror("Setting seek pointer to slice");
			exit(1);
		}
	}
	threadinit();
	return 0;
}

int
ImageUnzipChunk(char *chunkdata)
{
	return inflate_subblock(chunkdata);
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

	/* Set the MBR type if necesary */
	if (slice && dostype >= 0)
		fixmbr(slice, dostype);

	fprintf(stderr, "Wrote %qd bytes (%qd actual)\n",
		totaledata, totalrdata);
	fprintf(stderr, "%lu %lu %d\n", decompidles, writeridles, rdycount);
	return 0;
}
#endif

#ifndef NOTHREADS
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
	imagetoobigwarned = 0;

	/*
	 * Allocate blocks for the ready queue.
	 */
	queue_init(&readyqueue);
	if ((ptr = (readyblock_t *) malloc(sizeof(readyblock_t) * READYQSIZE))
	    == NULL) {
		fprintf(stderr, "Not enough memory for readyQ blocks!\n");
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

void *
DiskWriter(void *arg)
{
	readyblock_t	*rdyblk = 0;

	while (1) {
		pthread_testcancel();

		pthread_mutex_lock(&readyqueue_mutex);
		if (queue_empty(&readyqueue)) {
/*			fprintf(stderr, "Writer idle\n"); */
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

		if (rdyblk->header.zero) {
			writezeros(rdyblk->header.offset, rdyblk->header.size);
			free(rdyblk);
			writeinprogress = 0; /* XXX, ok as unlocked access */
			continue;
		}
		rdycount++;

		assert(rdyblk->header.size <= OUTSIZE+SECSIZE);
		writedata(rdyblk->header.offset, (size_t)rdyblk->header.size,
			  rdyblk->buf);
		writeinprogress = 0; /* XXX, ok as unlocked access */

		pthread_mutex_lock(&freelist_mutex);
		rdyblk->header.chain.next = (void *) freelist;
		freelist = rdyblk;
		pthread_mutex_unlock(&freelist_mutex);
		pthread_cond_signal(&freelist_condvar);
	}
}
#endif

static int
inflate_subblock(char *chunkbufp)
{
	int		cc, err, count, ibsize = 0, ibleft = 0;
	z_stream	d_stream; /* inflation stream */
	char		*bp;
	blockhdr_t	*blockhdr;
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
	blockhdr    = (blockhdr_t *) chunkbufp;
	chunkbufp  += DEFAULTREGIONSIZE;
	chunkbytes -= DEFAULTREGIONSIZE;
	
	switch (blockhdr->magic) {
	case COMPRESSED_V1:
	{
		static int didwarn;

		curregion = (struct region *)
			((struct blockhdr_V1 *)blockhdr + 1);
		if (dofill && !didwarn) {
			fprintf(stderr,
				"WARNING: old image file format, "
				"may not zero all unused blocks\n");
			didwarn = 1;
		}
		break;
	}

	case COMPRESSED_V2:
		imageversion = 2;
		curregion = (struct region *)
			((struct blockhdr_V2 *)blockhdr + 1);
		/*
		 * Extract relocation information
		 */
		getrelocinfo(blockhdr);
		break;

	default:
		fprintf(stderr, "Bad Magic Number!\n");
		exit(1);
	}

	/*
	 * Handle any lead-off free space
	 */
	if (imageversion > 1 && curregion->start > blockhdr->firstsect) {
		offset = sectobytes(blockhdr->firstsect);
		size = sectobytes(curregion->start - blockhdr->firstsect);
		dowrite_request(offset, size, NULL);
	}
 
	/*
	 * Start with the first region. 
	 */
	offset = sectobytes(curregion->start);
	size   = sectobytes(curregion->size);
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
			assert(count >= 0);
			assert(size  >= 0);

			/*
			 * Hit the end of the region. Need to figure out
			 * where the next one starts. If desired, we write
			 * a block of zeros in the empty space between this
			 * region and the next.
			 */
			if (! size) {
				off_t	    newoffset;

				/*
				 * No more regions. Must be done.
				 */
				if (!blockhdr->regioncount)
					break;

				newoffset = sectobytes(curregion->start);
				size      = sectobytes(curregion->size);
				assert(size);
				curregion++;
				blockhdr->regioncount--;
				assert((newoffset-offset) > 0);
				dowrite_request(offset, newoffset-offset,
						NULL);
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

	/*
	 * Handle any trailing free space
	 */
	curregion--;
	if (imageversion > 1 &&
	    curregion->start + curregion->size < blockhdr->lastsect) {
		offset = sectobytes(curregion->start + curregion->size);
		size = sectobytes(blockhdr->lastsect -
				  (curregion->start + curregion->size));
		dowrite_request(offset, size, NULL);
		offset += size;
	}
 
#ifndef FRISBEE
	if (debug == 1) {
		fprintf(stderr, "%14qd\n", offset);
	}
	else if (dots) {
		fprintf(stderr, ".");
		if (dotcol++ > 59) {
			struct timeval estamp;

			gettimeofday(&estamp, 0);
			estamp.tv_sec -= stamp.tv_sec;
			fprintf(stderr, "%4ld %13qd\n",
				estamp.tv_sec, totaledata);

			dotcol = 0;
		}
	}
#endif

	return 0;
}

void
writezeros(off_t offset, off_t zcount)
{
	size_t	zcc;

	assert((offset & (SECSIZE-1)) == 0);

	if (seekable) {
		/*
		 * We must always seek, even if offset == nextwriteoffset,
		 * since we are using pwrite.
		 */
		if (lseek(outfd, offset, SEEK_SET) < 0) {
			perror("lseek to write zeros");
			exit(1);
		}
		nextwriteoffset = offset;
	} else if (offset != nextwriteoffset) {
		fprintf(stderr, "Non-contiguous write @ %qu (should be %qu)\n",
			offset, nextwriteoffset);
		exit(1);
	}

	while (zcount) {
		if (zcount <= OUTSIZE)
			zcc = zcount;
		else
			zcc = OUTSIZE;
		
		if ((zcc = write(outfd, zeros, zcc)) != zcc) {
			if (zcc < 0) {
				perror("Writing Zeros");
			}
			exit(1);
		}
		zcount     -= zcc;
		totalrdata += zcc;
		nextwriteoffset += zcc;
	}
}

void
writedata(off_t offset, size_t size, void *buf)
{
	ssize_t	cc;

	/*	fprintf(stderr, "Writing %d bytes at %qd\n", size, offset); */

	if (seekable)
		cc = pwrite(outfd, buf, size, offset);
	else if (offset == nextwriteoffset)
		cc = write(outfd, buf, size);
	else {
		fprintf(stderr, "Non-contiguous write @ %qu (should be %qu)\n",
			offset, nextwriteoffset);
		exit(1);
	}
		
	if (cc != size) {
		if (cc < 0)
			perror("write error");
		else
			fprintf(stderr, "Short write!\n");
		exit(1);
	}
	nextwriteoffset = offset + cc;
	totalrdata += cc;
}

/*
 * DOS partition table handling
 */
struct doslabel {
	char		align[sizeof(short)];	/* Force alignment */
	char		pad2[DOSPARTOFF];
	struct dos_partition parts[NDOSPART];
	unsigned short  magic;
};
#define DOSPARTSIZE \
	(DOSPARTOFF + sizeof(doslabel.parts) + sizeof(doslabel.magic))

/*
 * Parse the DOS partition table to set the bounds of the slice we
 * are writing to. 
 */
int
readmbr(int slice)
{
	struct doslabel doslabel;
	int		cc;

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
	outputmaxsize = (long long)sectobytes(outputmaxsec - outputminsec);

	if (debug) {
		fprintf(stderr, "Slice Mode: S:%d min:%ld max:%ld size:%qd\n",
			slice, outputminsec, outputmaxsec, outputmaxsize);
	}
	return 0;
}

int
fixmbr(int slice, int dtype)
{
	struct doslabel doslabel;
	int		cc;

	if (lseek(outfd, (off_t)0, SEEK_SET) < 0) {
		perror("Could not seek to DOS label");
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

	if (doslabel.parts[slice-1].dp_typ != dostype) {
		doslabel.parts[slice-1].dp_typ = dostype;
		if (lseek(outfd, (off_t)0, SEEK_SET) < 0) {
			perror("Could not seek to DOS label");
			return 1;
		}
		cc = write(outfd, doslabel.pad2, DOSPARTSIZE);
		if (cc != DOSPARTSIZE) {
			perror("Could not write DOS label");
			return 1;
		}
		fprintf(stderr, "Set type of DOS partition %d to %d\n",
			slice, dostype);
	}
	return 0;
}

static struct blockreloc *reloctable;
static int numrelocs;
static void reloc_bsdlabel(struct disklabel *label);

static void
getrelocinfo(blockhdr_t *hdr)
{
	struct blockreloc *relocs;

	if (reloctable) {
		free(reloctable);
		reloctable = NULL;
	}

	if ((numrelocs = hdr->reloccount) == 0)
		return;

	reloctable = malloc(numrelocs * sizeof(struct blockreloc));
	if (reloctable == NULL) {
		fprintf(stderr, "No memory for relocation table\n");
		exit(1);
	}

	relocs = (struct blockreloc *)
		((void *)&hdr[1] + hdr->regioncount * sizeof(struct region));
	memcpy(reloctable, relocs, numrelocs * sizeof(struct blockreloc));
}

static void
applyrelocs(off_t offset, size_t size, void *buf)
{
	struct blockreloc *reloc;
	off_t roffset;
	uint32_t coff;

	if (numrelocs == 0)
		return;

	offset -= sectobytes(outputminsec);

	for (reloc = reloctable; reloc < &reloctable[numrelocs]; reloc++) {
		roffset = sectobytes(reloc->sector) + reloc->sectoff;
		if (offset < roffset+reloc->size && offset+size > roffset) {
			/* XXX lazy: relocation must be totally contained */
			assert(offset <= roffset);
			assert(roffset+reloc->size <= offset+size);

			coff = (u_int32_t)(roffset - offset);
			if (debug > 1)
				fprintf(stderr,
					"Applying reloc type %d [%qu-%qu] "
					"to [%qu-%qu]\n", reloc->type,
					roffset, roffset+reloc->size,
					offset, offset+size);
			switch (reloc->type) {
			case RELOC_NONE:
				break;
			case RELOC_BSDDISKLABEL:
				assert(reloc->size >= sizeof(struct disklabel));
				reloc_bsdlabel((struct disklabel *)(buf+coff));
				break;
			default:
				fprintf(stderr,
					"Ignoring unknown relocation type %d\n",
					reloc->type);
				break;
			}
		}
	}
}

static void
reloc_bsdlabel(struct disklabel *label)
{
	int i;
	uint32_t slicesize;

	/*
	 * This relocation only makes sense in slice mode,
	 * i.e., we are installing a slice image into another slice.
	 */
	if (slice == 0)
		return;

	if (label->d_magic  != DISKMAGIC || label->d_magic2 != DISKMAGIC) {
		fprintf(stderr, "No disklabel at relocation offset\n");
		exit(1);
	}

	assert(outputmaxsize > 0);
	slicesize = bytestosec(outputmaxsize);

	/*
	 * Fixup the partition table.
	 */
	for (i = 0; i < MAXPARTITIONS; i++) {
		uint32_t poffset, psize;

		if (label->d_partitions[i].p_size == 0)
			continue;

		/*
		 * Perform the relocation, making offsets absolute
		 */
		label->d_partitions[i].p_offset += outputminsec;

		poffset = label->d_partitions[i].p_offset;
		psize = label->d_partitions[i].p_size;

		/*
		 * Tweak sizes so BSD doesn't whine:
		 *  - truncate any partitions that exceed the slice size
		 *  - change RAW ('c') partition to match slice size
		 */
		if (poffset + psize > outputmaxsec) {
			fprintf(stderr, "WARNING: partition '%c' "
				"too large for slice, truncating\n", 'a' + i);
			label->d_partitions[i].p_size = outputmaxsec - poffset;
		} else if (i == RAW_PART && psize != slicesize) {
			assert(label->d_partitions[i].p_offset == outputminsec);
			fprintf(stderr, "WARNING: raw partition '%c' "
				"too small for slice, growing\n", 'a' + i);
			label->d_partitions[i].p_size = slicesize;
		}
	}
	label->d_checksum = 0;
	label->d_checksum = dkcksum(label);
}
