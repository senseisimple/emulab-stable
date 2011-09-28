/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2011 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Usage: imageunzip <input file>
 *
 * Writes the uncompressed data to stdout.
 */

#ifdef WITH_CRYPTO
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <zlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include "imagehdr.h"
#include "queue.h"
#include "checksum.h"
#ifndef NOTHREADS
#include <pthread.h>
#endif
#ifndef WITH_CRYPTO
#include <sha.h>
#endif

/*
 * Define this if you want to test frisbee's random presentation of chunks
 */
#ifndef FRISBEE
#define FAKEFRISBEE
#endif

#define MAXWRITEBUFMEM	0	/* 0 == unlimited */

long long totalddata = 0;	/* total decompressed data */
long long totaledata = 0;	/* total bytes covered by image */
long long totalrdata = 0;	/* total data written to disk (image+zeros) */
long long totalzdata = 0;	/* total zeros written to disk */
long totalwriteops   = 0;	/* total write operations */
long totalzeroops    = 0;	/* total zero write operations */
long totalseekops    = 0;	/* total non-contiguous writes */

int totalchunks, donechunks;

/*
 * In slice mode, we read the DOS MBR to find out where the slice is on
 * the raw disk, and then seek to that spot. This avoids sillyness in
 * the BSD kernel having to do with disklabels.
 *
 * These numbers are in sectors.
 */
static long		outputminsec	= 0;
static long		outputmaxsec	= 0;

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
char		zeros[OUTSIZE];

static int	 dostype = -1;
static int	 slice = 0;
static int	 debug = 0;
static int	 outfd;
static int	 dofill = 0;
static int	 nothreads = 0;
static int	 rdycount;
static int	 imageversion = 1;
static int	 dots   = 0;
static int	 dotcol;
static struct timeval stamp;
#ifndef FRISBEE
static int	 infd;
static int	 version= 0;
static unsigned	 fillpat= 0;
static int	 readretries = 0;
static int	 nodecompress = 0;
static char	 chunkbuf[CHUNKSIZE];
#endif
int		 readmbr(int slice);
int		 fixmbr(int slice, int dtype);
static int	 write_subblock(int, const char *, int);
static int	 inflate_subblock(const char *);
void		 writezeros(off_t offset, off_t zcount);
void		 writedata(off_t offset, size_t count, void *buf);

static void	zero_remainder(void);
static void	getrelocinfo(const blockhdr_t *hdr);
static size_t	applyrelocs(off_t offset, size_t cc, void *buf);

static int	 seekable;
static off_t	 nextwriteoffset;
static off_t	 maxwrittenoffset;

static int	 imagetoobigwarned;

#ifndef FRISBEE
static int	 docrconly = 0;
static u_int32_t crc;
extern void	 compute_crc(u_char *buf, int blen, u_int32_t *crcp);
ssize_t		 read_withretry(int fd, void *buf, size_t nbytes, off_t foff);
#endif

#ifdef FAKEFRISBEE
#include <sys/stat.h>

static int	dofrisbee;
static int	*chunklist, *nextchunk;
#endif

/* UUID for the current image, all chunks must have the same ID */
static unsigned char uuid[UUID_LENGTH];
static int has_id = 0;

/*
 * Some stats
 */
unsigned long decompblocks;
unsigned long writeridles;

#ifdef NOTHREADS
#define		threadinit()
#define		threadwait()
#define		threadquit()

/* XXX keep the code simple */
#define pthread_mutex_lock(l)
#define pthread_mutex_unlock(l)
#define pthread_testcancel()
#undef CONDVARS_WORK
#else
static void	 threadinit(void);
static void	 threadwait(void);
static void	 threadquit(void);
static void	*DiskWriter(void *arg);
#ifdef FRISBEE
extern void (*DiskStatusCallback)(int);
#endif

static int	writeinprogress; /* XXX */
static pthread_t child_pid;
static pthread_mutex_t	writequeue_mutex;
#ifdef CONDVARS_WORK
static pthread_cond_t	writequeue_cond;
#endif
#endif

/*
 * A queue of ready to write data blocks.
 */
typedef struct {
	int		refs;
	size_t		size;
	char		data[0];
} buffer_t;

typedef struct {
	queue_chain_t	chain;
	off_t		offset;
	off_t		size;
	buffer_t	*buf;
	char		*data;
} writebuf_t;

static unsigned long	maxwritebufmem = MAXWRITEBUFMEM;
static volatile unsigned long	curwritebufmem, curwritebufs;
#ifndef NOTHREADS
static queue_head_t	writequeue;
static pthread_mutex_t	writebuf_mutex;
#ifdef CONDVARS_WORK
static pthread_cond_t	writebuf_cond;
#endif
#endif
static volatile int	writebufwanted;

/* stats */
unsigned long		maxbufsalloced, maxmemalloced;
unsigned long		splits;

#ifdef WITH_CRYPTO
/* security */
static int do_checksum = 0;
static int do_decrypt = 0;
static int cipher = ENC_NONE;
static unsigned char encryption_key[ENC_MAX_KEYLEN];
#endif

#ifndef CONDVARS_WORK
int fsleep(unsigned int usecs);
#endif

#ifndef FRISBEE
void
dump_stats(int sig)
{
	struct timeval estamp;

	gettimeofday(&estamp, 0);
	estamp.tv_sec -= stamp.tv_sec;
	if (sig == 0 && debug != 1 && dots) {
		while (dotcol++ <= 66)
			fprintf(stderr, " ");

		fprintf(stderr, "%4ld %6d\n",
			(long)estamp.tv_sec, totalchunks - donechunks);
	}
	else {
		if (sig) {
#ifndef NOTHREADS
			if (pthread_self() == child_pid)
				return;
#endif
			if (dots && dotcol)
				fputc('\n', stderr);
			if (totalchunks)
				fprintf(stderr, "%d of %d chunks decompressed\n",
					donechunks, totalchunks);
			else
				fprintf(stderr, "%d chunks decompressed\n",
					donechunks);
		}
		fprintf(stderr,
			"Decompressed %lld bytes, wrote %lld bytes (%lld actual) in %ld.%03ld seconds\n",
			totalddata, totaledata, totalrdata,
			(long)estamp.tv_sec, (long)estamp.tv_usec/1000);
		fprintf(stderr,
			"%lld bytes of data in %ld ops (%lld bytes/op), %ld seeks\n",
			totalrdata, totalwriteops,
			totalrdata/(totalwriteops?:1), totalseekops);
		if (totalzdata > 0) {
			fprintf(stderr,
				"  %lld bytes of disk data in %ld ops (%lld bytes/op)\n",
				(totalrdata-totalzdata),
				(totalwriteops-totalzeroops),
				(totalrdata-totalzdata)/((totalwriteops-totalzeroops)?:1));
			fprintf(stderr,
				"  %lld bytes of zero data in %ld ops (%lld bytes/op)\n",
				totalzdata, totalzeroops,
				totalzdata/(totalzeroops?:1));
		}
	}
	if (debug)
		fprintf(stderr, "decompressor blocked: %lu, "
			"writer idle: %lu, writes performed: %d\n",
			decompblocks, writeridles, rdycount);
}
#endif

#define DODOTS_CHUNKS	1
#define DODOTS_DATA	2
#define DODOTS_ZERO	3
#define DODOTS_SKIP	4

void dodots(int dottype, off_t cc)
{
	static int lastgb = 0;
	int count = 0, newgb;
	char chr = 0;


	switch (dottype) {
	case DODOTS_CHUNKS:
		if ((dots & 1) == 0)
			return;
		count = 1;
		chr = '.';
		break;
	case DODOTS_DATA:
	case DODOTS_ZERO:
	case DODOTS_SKIP:
		if ((dots & 2) == 0)
			return;
		/*
		 * totaledata has not been updated yet, so we have to
		 * figure it out ourselves.
		 */
		newgb = (totaledata + cc) / 1000000000LL;
		if ((count = newgb - lastgb) <= 0)
			return;
		lastgb = newgb;
		chr = (dottype == DODOTS_DATA) ? '.' :
			(dottype == DODOTS_ZERO) ? 'z' : 's';
		break;
	}

	while (count-- > 0) {
		fputc(chr, stderr);
		if (dotcol++ > 65) {
			struct timeval estamp;

			gettimeofday(&estamp, 0);
			estamp.tv_sec -= stamp.tv_sec;
			fprintf(stderr, "%4ld %6d\n",
				(long)estamp.tv_sec, totalchunks - donechunks);
			dotcol = 0;
		}
	}
}

void
dump_writebufs(void)
{
	fprintf(stderr, "%lu max bufs, %lu max memory\n",
		maxbufsalloced, maxmemalloced);
	fprintf(stderr, "%lu buffers split\n",
		splits);
}

static writebuf_t *
alloc_writebuf(off_t offset, off_t size, int allocbuf, int dowait)
{
	writebuf_t *wbuf;
	buffer_t *buf = NULL;
	size_t bufsize;

	pthread_mutex_lock(&writebuf_mutex);
	wbuf = malloc(sizeof(*wbuf));
	if (wbuf == NULL) {
		fprintf(stderr, "could not alloc writebuf header\n");
		exit(1);
	}
	bufsize = allocbuf ? size : 0;
	if (bufsize) {
		do {
			if (maxwritebufmem &&
			    curwritebufmem + bufsize > maxwritebufmem)
				buf = NULL;
			else
				buf = malloc(sizeof(buffer_t) + bufsize);

			if (buf == NULL) {
				if (!dowait) {
					free(wbuf);
					pthread_mutex_unlock(&writebuf_mutex);
					return NULL;
				}

				decompblocks++;
				writebufwanted = 1;
				/*
				 * Once again it appears that linuxthreads
				 * condition variables don't work well.
				 * We seem to sleep longer than necessary.
				 */
				do {
#ifdef CONDVARS_WORK
					pthread_cond_wait(&writebuf_cond,
							  &writebuf_mutex);
#else
					pthread_mutex_unlock(&writebuf_mutex);
					fsleep(1000);
					pthread_mutex_lock(&writebuf_mutex);
#endif
					pthread_testcancel();
				} while (writebufwanted);
			}
		} while (buf == NULL);
		buf->refs = 1;
		buf->size = bufsize;
	}
	curwritebufs++;
	curwritebufmem += bufsize;
	if (curwritebufs > maxbufsalloced)
		maxbufsalloced = curwritebufs;
	if (curwritebufmem > maxmemalloced)
		maxmemalloced = curwritebufmem;
	pthread_mutex_unlock(&writebuf_mutex);

	queue_init(&wbuf->chain);
	wbuf->offset = offset;
	wbuf->size = size;
	wbuf->buf = buf;
	wbuf->data = buf ? buf->data : NULL;

	return wbuf;
}

static writebuf_t *
split_writebuf(writebuf_t *wbuf, off_t doff, int dowait)
{
	writebuf_t *nwbuf;
	off_t size;

	assert(wbuf->buf != NULL);

	splits++;
	assert(doff < wbuf->size);
	size = wbuf->size - doff;
	nwbuf = alloc_writebuf(wbuf->offset+doff, size, 0, dowait);
	if (nwbuf) {
		wbuf->size -= size;
		pthread_mutex_lock(&writebuf_mutex);
		wbuf->buf->refs++;
		pthread_mutex_unlock(&writebuf_mutex);
		nwbuf->buf = wbuf->buf;
		nwbuf->data = wbuf->data + doff;
	}
	return nwbuf;
}

static void
free_writebuf(writebuf_t *wbuf)
{
	assert(wbuf != NULL);

	pthread_mutex_lock(&writebuf_mutex);
	if (wbuf->buf && --wbuf->buf->refs == 0) {
		curwritebufs--;
		curwritebufmem -= wbuf->buf->size;
		assert(curwritebufmem >= 0);
		free(wbuf->buf);
		if (writebufwanted) {
			writebufwanted = 0;
#ifdef CONDVARS_WORK
			pthread_cond_signal(&writebuf_cond);
#endif
		}
	}
	free(wbuf);
	pthread_mutex_unlock(&writebuf_mutex);
}

static void
dowrite_request(writebuf_t *wbuf)
{
	off_t offset, size;
	void *buf;

	offset = wbuf->offset;
	size = wbuf->size;
	buf = wbuf->data;
	assert(offset >= 0);
	assert(size > 0);

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
		if (offset >= sectobytes(outputmaxsec)) {
			free_writebuf(wbuf);
			return;
		}
		size = sectobytes(outputmaxsec) - offset;
		wbuf->size = size;
	}
	wbuf->offset = offset;

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
			assert(size <= OUTSIZE);

			/*
			 * Handle any relocations
			 */
			size = (off_t)applyrelocs(offset, (size_t)size, buf);
			writedata(offset, (size_t)size, buf);
		}
		free_writebuf(wbuf);
		return;
	}

#ifndef NOTHREADS
	if (buf == NULL) {
		if (!dofill) {
			free_writebuf(wbuf);
			return;
		}
	} else {
		assert(size <= OUTSIZE);

		/*
		 * Handle any relocations
		 */
		size = (off_t)applyrelocs(offset, (size_t)size, buf);
		wbuf->size = size;
	}

	/*
	 * Queue it up for the writer thread
	 */
	pthread_mutex_lock(&writequeue_mutex);
	queue_enter(&writequeue, wbuf, writebuf_t *, chain);
#ifdef CONDVARS_WORK
	pthread_cond_signal(&writequeue_cond);
#endif
	pthread_mutex_unlock(&writequeue_mutex);
#endif
}

static __inline int devread(int fd, void *buf, size_t size)
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
		" -o              Output progress indicator (compressed chunks processed)\n"
		" -O              Output progress indicator (GBs of uncompressed data written)\n"
		" -n              Single threaded (slow) mode\n"
		" -d              Turn on progressive levels of debugging\n"
		" -r retries      Number of image read retries to attempt\n"
		" -W size         MB of memory to use for write buffering\n"
		" -c              Check per-chunk checksum\n"
		" -a keyfile      File containing pubkey used to sign checksum (implies -c)\n"
		" -e cipher       Decrypt the image with the given cipher\n"
		" -k keyfile      File containing key used for encryption/decryption\n"
		" -u uuid         UUID for image\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	int		i, ch;
	off_t		foff;
	int		chunkno;
	extern char	build_info[];

#ifdef NOTHREADS
	nothreads = 1;
#endif
	while ((ch = getopt(argc, argv, "vdhs:zp:oOnFD:W:Cr:Na:ck:eu:")) != -1)
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
			dots |= 1;
			break;

		case 'O':
			dots |= 2;
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

		case 'C':
			docrconly++;
			dofill++;
			seekable = 0;
			break;

		case 'r':
			readretries = atoi(optarg);
			break;

		case 'N':
			nodecompress++;
			break;

#ifndef NOTHREADS
		case 'W':
			maxwritebufmem = atoi(optarg);
			if (maxwritebufmem >= 4096)
				maxwritebufmem = MAXWRITEBUFMEM;
			maxwritebufmem *= (1024 * 1024);
			break;
#endif
#ifdef WITH_CRYPTO
		case 'c':
#ifndef SIGN_CHECKSUM
			do_checksum = 1;
			break;
#else
			fprintf(stderr, "Unsigned checksums not supported\n");
			exit(1);
#endif

		case 'a':
#ifdef SIGN_CHECKSUM
			if (!init_checksum(optarg))
				exit(1);
			do_checksum = 1;
			break;
#else
			fprintf(stderr, "Signed checksums not supported\n");
			exit(1);
#endif

		case 'e':
			/* Encryption cipher */
			if (strcmp(optarg, "bf_cbc") == 0) {
				cipher = ENC_BLOWFISH_CBC;
			}
			else {
				fprintf(stderr,
					"Only know \"bf_cbc\" (blowfish CBC)\n");
				usage();
			}
			do_decrypt = 1;
			break;

		case 'k':
			if (!encrypt_readkey(optarg, encryption_key,
					     ENC_MAX_KEYLEN))
				exit(1);
			/* XXX can you intuit the cipher from the key? */
			if (cipher == ENC_NONE)
				cipher = ENC_BLOWFISH_CBC;
			do_decrypt = 1;
			break;
#endif
		case 'u':
			/* UUID for image id. */
			if (!hexstr_to_mem(uuid, optarg, UUID_LENGTH))
				usage();
			has_id = 1;
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
		struct stat st;

		if ((infd = open(argv[0], O_RDONLY, 0666)) < 0) {
			perror("opening input file");
			exit(1);
		}
		if (fstat(infd, &st) == 0)
			totalchunks = st.st_size / CHUNKSIZE;
	} else {
		infd = fileno(stdin);
		if (readretries > 0) {
			fprintf(stderr,
				"WARNING: cannot retry read operations\n");
			readretries = 0;
		}
	}

	if (docrconly)
		outfd = -1;
	else if (argc == 2 && strcmp(argv[1], "-")) {
		/*
		 * XXX perform seek and MBR checks before we truncate
		 * the output file.  If they have their input/output
		 * arguments reversed, we don't want to truncate their
		 * image file!  (I speak from bitter experience).
		 */
		if (slice && (outfd = open(argv[1], O_RDONLY, 0666)) >= 0) {
			if (lseek(outfd, (off_t)0, SEEK_SET) < 0) {
				fprintf(stderr, "Output file is not seekable, "
					"cannot specify a slice\n");
				exit(1);
			}
			if (readmbr(slice)) {
				fprintf(stderr, "Failed to read MBR\n");
				exit(1);
			}
			close(outfd);
		}

		/*
		 * Open the output file for writing.
		 */
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
		if (!dofill && !docrconly)
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

		/*
		 * XXX zeroing and slice mode don't mix.
		 *
		 * To do so properly, we would have to zero all space before
		 * and after the slice in question.  But zeroing before would
		 * clobber the MBR and boot info, probably not what was
		 * intended.  So we just don't do it til we figure out what
		 * the "right" behavior is.
		 */
		if (dofill) {
			fprintf(stderr,
				"WARNING: requested zeroing in slice mode, "
				"will NOT zero outside of slice!\n");
		}
	}

	threadinit();
	gettimeofday(&stamp, 0);

#ifdef FAKEFRISBEE
	if (dofrisbee) {
		int i;

		if (totalchunks == 0) {
			fprintf(stderr, "Must have statable input file\n");
			exit(1);
		}

		chunklist = (int *) calloc(totalchunks+1, sizeof(*chunklist));
		assert(chunklist != NULL);

		for (i = 0; i < totalchunks; i++)
			chunklist[i] = i;
		chunklist[i] = -1;

		srandom((long)(stamp.tv_usec^stamp.tv_sec));
		for (i = 0; i < 50 * totalchunks; i++) {
			int c1 = random() % totalchunks;
			int c2 = random() % totalchunks;
			int t1 = chunklist[c1];
			int t2 = chunklist[c2];

			chunklist[c2] = t1;
			chunklist[c1] = t2;
		}
		nextchunk = chunklist;
	}
#endif

#ifdef SIGINFO
	signal(SIGINFO, dump_stats);
#endif
	foff = 0;
	chunkno = 0;
	while (1) {
		int	count = sizeof(chunkbuf);
		char	*bp   = chunkbuf;

#ifdef FAKEFRISBEE
		if (dofrisbee) {
			if (*nextchunk == -1)
				goto done;
			foff = (off_t)*nextchunk * CHUNKSIZE;
			if (lseek(infd, foff, SEEK_SET) < 0) {
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

			if ((cc = read_withretry(infd, bp, count, foff)) <= 0) {
				if (cc == 0)
					goto done;
				perror("reading zipped image");
				exit(1);
			}
			count -= cc;
			bp    += cc;
			foff  += cc;
		}
		if (nodecompress) {
			if (write_subblock(chunkno, chunkbuf, CHUNKSIZE))
				break;
		} else {
			if (inflate_subblock(chunkbuf))
				break;
		}
		chunkno++;
	}
 done:
	close(infd);

	/* When zeroing, may need to zero the rest of the disk */
	zero_remainder();

	/* This causes the output queue to drain */
	threadquit();

	/* Set the MBR type if necesary */
	if (slice && dostype >= 0)
		fixmbr(slice, dostype);

	/* Flush any cached data and close the output device */
	if (outfd >= 0) {
		/* can return EINVAL if device doesn't support fsync */
		if (fsync(outfd) < 0 && errno != EINVAL) {
			perror("flushing output data");
			exit(1);
		}
		close(outfd);
	}

	dump_stats(0);
	if (docrconly)
		fprintf(stderr, "%s: CRC=%u\n", argv[0], ~crc);
	dump_writebufs();
#ifdef WITH_CRYPTO
#ifdef SIGN_CHECKSUM
	if (do_checksum)
		cleanup_checksum();
#endif
#endif
	return 0;
}
#else
/*
 * When compiled for frisbee, act as a library.
 */
int
ImageUnzipInitKeys(char *uuidstr, char *sig_keyfile, char *enc_keyfile)
{
	if (uuidstr) {
		if (!hexstr_to_mem(uuid, uuidstr, UUID_LENGTH)) {
			fprintf(stderr, "Bogus UUID\n");
			exit(1);
		}
		has_id = 1;
	}

#ifdef WITH_CRYPTO
	if (sig_keyfile) {
#ifdef SIGN_CHECKSUM
		if (!init_checksum(sig_keyfile))
			exit(1);
		do_checksum = 1;
#else
		fprintf(stderr, "Signed checksums not supported\n");
		exit(1);
#endif
	}
	if (enc_keyfile) {
		if (!encrypt_readkey(enc_keyfile,
				     encryption_key, sizeof(encryption_key)))
			exit(1);
		/* XXX can you intuit the cipher from the key? */
		if (cipher == ENC_NONE)
			cipher = ENC_BLOWFISH_CBC;
		do_decrypt = 1;
	}
#else
	if (sig_keyfile != NULL || enc_keyfile != NULL) {
		fprintf(stderr, "Authentication/encryption not supported\n");
		exit(1);
	}
#endif

	return 0;
}
int
ImageUnzipInit(char *filename, int _slice, int _debug, int _fill,
	       int _nothreads, int _dostype, int _dodots,
	       unsigned long _writebufmem)
{
	gettimeofday(&stamp, 0);

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
	dots      = _dodots;
#ifndef NOTHREADS
	maxwritebufmem = _writebufmem;
#endif

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

void
ImageUnzipSetChunkCount(unsigned long _chunkcount)
{
	totalchunks = _chunkcount;
}

void
ImageUnzipSetMemory(unsigned long _writebufmem)
{
#ifndef NOTHREADS
	maxwritebufmem = _writebufmem;
#endif
}

int
ImageWriteChunk(int chunkno, char *chunkdata, int chunksize)
{
	return write_subblock(chunkno, chunkdata, chunksize);
}

int
ImageUnzipChunk(char *chunkdata, int chunksize)
{
	/* XXX cannot handle partial chunks yet */
	assert(chunksize == CHUNKSIZE);

	return inflate_subblock(chunkdata);
}

int
ImageUnzipFlush(void)
{
	int rv = 0;

	/* When zeroing, may need to zero the rest of the disk */
	zero_remainder();

	threadwait();

	if (outfd >= 0) {
		if (DiskStatusCallback)
			(*DiskStatusCallback)(3);

		/* can return EINVAL if device doesn't support fsync */
		if (fsync(outfd) < 0 && errno != EINVAL) {
			perror("fsync");
			rv = -1;
		}

		if (DiskStatusCallback)
			(*DiskStatusCallback)(4);

		/* XXX don't close it here, wait til UnzipQuit */
	}

	return(rv);
}

int
ImageUnzipQuit(void)
{
	threadquit();

	/* Set the MBR type if necesary */
	if (slice && dostype >= 0)
		fixmbr(slice, dostype);

	/* Now we can close it */
	if (outfd >= 0)
		close(outfd);

#ifdef WITH_CRYPTO
#ifdef SIGN_CHECKSUM
	if (do_checksum)
		cleanup_checksum();
#endif
#endif

	fprintf(stderr, "Wrote %qd bytes (%qd actual)\n",
		totaledata, totalrdata);
	fprintf(stderr, "%lu %lu %d\n", decompblocks, writeridles, rdycount);
	return 0;
}
#endif

#ifndef NOTHREADS
static void
threadinit(void)
{
	static int	called;

	if (nothreads)
		return;

	decompblocks = writeridles = 0;
	imagetoobigwarned = 0;

	/*
	 * Allocate blocks for the ready queue.
	 */
	queue_init(&writequeue);

	if (!called) {
		called = 1;
		pthread_mutex_init(&writebuf_mutex, 0);
		pthread_mutex_init(&writequeue_mutex, 0);
#ifdef CONDVARS_WORK
		pthread_cond_init(&writebuf_cond, 0);
		pthread_cond_init(&writequeue_cond, 0);
#endif
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
		pthread_mutex_lock(&writequeue_mutex);
		done = (queue_empty(&writequeue) && !writeinprogress);
		pthread_mutex_unlock(&writequeue_mutex);
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
}

void *
DiskWriter(void *arg)
{
	writebuf_t	*wbuf = 0;
	static int	gotone = 0;

	while (1) {
		pthread_testcancel();

		pthread_mutex_lock(&writequeue_mutex);
		if (queue_empty(&writequeue)) {
			if (gotone) {
				writeridles++;
#ifdef FRISBEE
				if (DiskStatusCallback)
					(*DiskStatusCallback)(1);
#endif
			}
			do {
#ifdef CONDVARS_WORK
				pthread_cond_wait(&writequeue_cond,
						  &writequeue_mutex);
#else
				pthread_mutex_unlock(&writequeue_mutex);
				fsleep(1000);
				pthread_mutex_lock(&writequeue_mutex);
#endif
				pthread_testcancel();
			} while (queue_empty(&writequeue));
#ifdef FRISBEE
			if (DiskStatusCallback)
				(*DiskStatusCallback)(0);
#endif
		} else {
#ifdef FRISBEE
			if (DiskStatusCallback)
				(*DiskStatusCallback)(2);
#endif
			;
		}
		queue_remove_first(&writequeue, wbuf, writebuf_t *, chain);
		writeinprogress = 1; /* XXX */
		gotone = 1;
		pthread_mutex_unlock(&writequeue_mutex);

		if (wbuf->data == NULL) {
			writezeros(wbuf->offset, wbuf->size);
		} else {
			rdycount++;
			assert(wbuf->size <= OUTSIZE);
			writedata(wbuf->offset, (size_t)wbuf->size, wbuf->data);
		}
		free_writebuf(wbuf);
		writeinprogress = 0; /* XXX, ok as unlocked access */
	}
}
#endif

/*
 * Just write the raw, compressed chunk data to disk
 */
static int
write_subblock(int chunkno, const char *chunkbufp, int chunksize)
{
	writebuf_t	*wbuf;
	off_t		offset, size, bytesleft;
	
	offset = (off_t)chunkno * CHUNKSIZE;
	bytesleft = chunksize;
	while (bytesleft > 0) {
		size = (bytesleft >= OUTSIZE) ? OUTSIZE : bytesleft;
		wbuf = alloc_writebuf(offset, size, 1, 1);
		memcpy(wbuf->data, chunkbufp, size);
		dowrite_request(wbuf);
		chunkbufp += size;
		offset += size;
		bytesleft -= size;
	}

	donechunks++;
	dodots(DODOTS_CHUNKS, 0);

	return 0;
}

#ifdef WITH_CRYPTO
/* returns the number of characters decrypted */
static int
decrypt_buffer(unsigned char *dest, const unsigned char *source,
	       const blockhdr_t *header)
{
	/* init */
	int update_count = 0;
	int final_count = 0;
	int error = 0;
	EVP_CIPHER_CTX context;
	EVP_CIPHER const *ecipher;

	EVP_CIPHER_CTX_init(&context);
	ecipher = EVP_bf_cbc();

	EVP_DecryptInit(&context, ecipher, NULL, header->enc_iv);
	EVP_CIPHER_CTX_set_key_length(&context, ENC_MAX_KEYLEN);
	EVP_DecryptInit(&context, NULL, encryption_key, NULL);

	/* decrypt */
	EVP_DecryptUpdate(&context, dest, &update_count, source, header->size);

	/* cleanup */
	error = EVP_DecryptFinal(&context, dest + update_count, &final_count);
	if (!error) {
		char keystr[ENC_MAX_KEYLEN*2 + 1];
		fprintf(stderr, "Padding was incorrect.\n");
		mem_to_hexstr(keystr, encryption_key, ENC_MAX_KEYLEN);
		fprintf(stderr, "  Key: %s\n", keystr);
		mem_to_hexstr(keystr, header->enc_iv, ENC_MAX_KEYLEN);
		fprintf(stderr, "  IV:  %s\n", keystr);
		exit(1);
	}
	return update_count + final_count;
}
#endif

static int
inflate_subblock(const char *chunkbufp)
{
	int		cc, err, count, ibsize = 0, ibleft = 0;
	z_stream	d_stream; /* inflation stream */
	const blockhdr_t *blockhdr;
	int		regioncount;
	struct region	*curregion;
	off_t		offset, size;
	char		resid[SECSIZE];
	writebuf_t	*wbuf;
#ifdef WITH_CRYPTO
	char		plaintext[CHUNKMAX];
#endif

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
	blockhdr    = (const blockhdr_t *) chunkbufp;
	chunkbufp  += DEFAULTREGIONSIZE;

#ifdef WITH_CRYPTO
	/*
	 * If decryption and/or signing is required and the image is too
	 * old, bail.
	 *
	 * XXX this assumes that version nums are monotonically increasing,
	 * I cannot imagine why they would not be in the future!
	 */
	if (blockhdr->magic < COMPRESSED_V4 &&
	    (do_checksum || do_decrypt)) {
		fprintf(stderr,
			"%s requested but image is old format (V%d)\n",
			do_checksum ? "checksum" : "decryption",
			blockhdr->magic - COMPRESSED_MAGIC_BASE);
		exit(1);
	}
#endif
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
	case COMPRESSED_V3:
		imageversion = 2;
		curregion = (struct region *)
			((struct blockhdr_V2 *)blockhdr + 1);
		/*
		 * Extract relocation information
		 */
		getrelocinfo(blockhdr);
		break;

	case COMPRESSED_V4:
#ifdef WITH_CRYPTO
		/*
		 * Verify the checksum before looking at anything else.
		 */
		if (do_checksum &&
		    !verify_checksum((blockhdr_t *)blockhdr,
				     (const unsigned char *)blockhdr,
				     blockhdr->csum_type))
			exit(1);
#endif

		/*
		 * Track the current image UUID.
		 *
		 * If a UUID was specified on the command line, use that as
		 * the expected UUID. All chunks much match this UUID.
		 *
		 * XXX if no UUID was specified, we use the one from the
		 * first chunk received and match all others against that.
		 * This is not ideal, since it is vulnerable to spoofing if
		 * the client is not using a signature and/or encryption.
		 */
		if (has_id == 0) {
			has_id = 1;
			memcpy(uuid, blockhdr->imageid, UUID_LENGTH);
		} else if (memcmp(uuid, blockhdr->imageid, UUID_LENGTH)) {
			char uuidstr[UUID_LENGTH*2 + 1];

			fprintf(stderr, "Incorrect Image ID in chunk %d:\n",
				blockhdr->blockindex);
			mem_to_hexstr(uuidstr, blockhdr->imageid, UUID_LENGTH);
			fprintf(stderr, "  In chunk:  0x%s\n", uuidstr);
			mem_to_hexstr(uuidstr, uuid, UUID_LENGTH);
			fprintf(stderr, "  Should be: 0x%s\n", uuidstr);
			exit(1);
		}

#ifdef WITH_CRYPTO
		/*
		 * Decrypt the rest of the chunk if encrypted.
		 */
		if (do_decrypt) {
			if (blockhdr->enc_cipher == ENC_NONE) {
				fprintf(stderr, "Chunk has no cipher\n");
				exit(1);
			}
			if (blockhdr->enc_cipher != cipher) {
				fprintf(stderr,
					"Wrong cipher type %d in chunk\n",
					blockhdr->enc_cipher);
				exit(1);
			}

			((blockhdr_t *)blockhdr)->size
				= decrypt_buffer((unsigned char *)plaintext,
						 (unsigned char *)chunkbufp,
						 blockhdr);
			chunkbufp = plaintext;
		}
#endif
		imageversion = 4;
		curregion = (struct region *) (blockhdr + 1);
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
		if (dofill) {
			wbuf = alloc_writebuf(offset, size, 0, 1);
			dowrite_request(wbuf);
		} else {
			dodots(DODOTS_SKIP, size);
			totaledata += size;
		}
	}

	/*
	 * Start with the first region.
	 */
	offset = sectobytes(curregion->start);
	size   = sectobytes(curregion->size);
	assert(size > 0);

	regioncount = blockhdr->regioncount;

	curregion++;
	regioncount--;

	if (debug == 1)
		fprintf(stderr, "Decompressing chunk %04d: %14lld --> ",
			blockhdr->blockindex, (long long)offset);

	wbuf = NULL;

	/*
	 * Read just up to the end of compressed data.
	 */
	d_stream.next_in   = (Bytef *)chunkbufp;
	d_stream.avail_in  = blockhdr->size;
	assert(blockhdr->size > 0);

	while (d_stream.avail_in) {
		assert(wbuf == NULL);
		wbuf = alloc_writebuf(offset, OUTSIZE, 1, 1);

		/*
		 * Must operate on multiples of the sector size so first we
		 * restore any residual from the last decompression.
		 */
		if (ibleft)
			memcpy(wbuf->data, resid, ibleft);

		/*
		 * Adjust the decompression params to account for the resid
		 */
		d_stream.next_out  = (Bytef *)&wbuf->data[ibleft];
		d_stream.avail_out = OUTSIZE - ibleft;

#if !defined(NOTHREADS) && defined(FRISBEE)
		/*
		 * XXX this may be a FreeBSD/linuxthreads specific thing
		 * but it seems that the decompression thread can run way
		 * longer than a timeslice causing incoming packets to
		 * get dropped before the network thread can be rescheduled.
		 * So force the decompressor to take a break once in awhile.
		 */
		if (!nothreads)
			sched_yield();
#endif
		/*
		 * Inflate a chunk
		 */
		err = inflate(&d_stream, Z_SYNC_FLUSH);
		if (err != Z_OK && err != Z_STREAM_END) {
			fprintf(stderr, "inflate failed, err=%d\n", err);
			exit(1);
		}

		/*
		 * Figure out how much valid data is in the buffer and
		 * save off any SECSIZE residual for the next round.
		 *
		 * Yes the ibsize computation is correct, just not obvious.
		 * The obvious expression is:
		 *	ibsize = (OUTSIZE - ibleft) - avail_out + ibleft;
		 * so ibleft cancels out.
		 */
		ibsize = OUTSIZE - d_stream.avail_out;
		count  = ibsize & ~(SECSIZE - 1);
		ibleft = ibsize - count;
		if (ibleft)
			memcpy(resid, &wbuf->data[count], ibleft);
		wbuf->size = count;

		while (count) {
			/*
			 * Move data into the output block only as far as
			 * the end of the current region. Since outbuf is
			 * same size as rdyblk->buf, its guaranteed to fit.
			 */
			if (count <= size) {
				dowrite_request(wbuf);
				wbuf = NULL;
				cc = count;
			} else {
				writebuf_t *wbtail;

				/*
				 * Data we decompressed belongs to physically
				 * distinct areas, we have to split the
				 * write up, meaning we have to allocate a
				 * new writebuf and copy the remaining data
				 * into it.
				 */
				wbtail = split_writebuf(wbuf, size, 1);
				dowrite_request(wbuf);
				wbuf = wbtail;
				cc = size;
			}

			if (debug == 2) {
				fprintf(stderr,
					"%12lld %8d %8d %12lld %10lld %8d %5d %8d"
					"\n",
					(long long)offset, cc, count,
					totaledata, (long long)size,
					ibsize, ibleft, d_stream.avail_in);
			}

			count  -= cc;
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
			if (size == 0) {
				off_t	    newoffset;
				writebuf_t *wbzero;

				/*
				 * No more regions. Must be done.
				 */
				if (!regioncount)
					break;

				newoffset = sectobytes(curregion->start);
				size      = sectobytes(curregion->size);
				assert(size);
				curregion++;
				regioncount--;
				assert((newoffset-offset) > 0);
				if (dofill) {
					wbzero = alloc_writebuf(offset,
							newoffset-offset,
							0, 1);
					dowrite_request(wbzero);
				} else {
					dodots(DODOTS_SKIP, newoffset-offset);
					totaledata += newoffset-offset;
				}
				offset = newoffset;
				if (wbuf)
					wbuf->offset = newoffset;
			}
		}
		assert(wbuf == NULL);

		totalddata += ibsize;

		/*
		 * Exhausted our output buffer but may still have more input in
		 * the current chunk.
		 */
	}

	/*
	 * All input inflated and all output written, done.
	 */
	assert(err == Z_STREAM_END);

	err = inflateEnd(&d_stream);
	CHECK_ERR(err, "inflateEnd");

	assert(wbuf == NULL);
	assert(regioncount == 0);
	assert(size == 0);

	/*
	 * Handle any trailing free space
	 */
	curregion--;
	if (imageversion > 1 &&
	    curregion->start + curregion->size < blockhdr->lastsect) {
		offset = sectobytes(curregion->start + curregion->size);
		size = sectobytes(blockhdr->lastsect -
				  (curregion->start + curregion->size));
		if (dofill) {
			wbuf = alloc_writebuf(offset, size, 0, 1);
			dowrite_request(wbuf);
		} else {
			dodots(DODOTS_SKIP, size);
			totaledata += size;
		}
		offset += size;
	}

	donechunks++;
	if (debug == 1) {
		fprintf(stderr, "%14lld\n", (long long)offset);
	}
	dodots(DODOTS_CHUNKS, 0);

	return 0;
}

void
writezeros(off_t offset, off_t zcount)
{
	size_t	zcc, wcc;
	off_t ozcount;

	assert((offset & (SECSIZE-1)) == 0);

	if (offset != nextwriteoffset)
		totalseekops++;

#ifndef FRISBEE
	if (docrconly)
		nextwriteoffset = offset;
	else
#endif
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
		fprintf(stderr, "Non-contiguous write @ %lld (should be %lld)\n",
			(long long)offset, (long long)nextwriteoffset);
		exit(1);
	}

	ozcount = zcount;
	while (zcount) {
		if (zcount <= OUTSIZE)
			zcc = zcount;
		else
			zcc = OUTSIZE;

#ifndef FRISBEE
		if (docrconly)
			compute_crc((u_char *)zeros, zcc, &crc);
		else
#endif
		if ((wcc = write(outfd, zeros, zcc)) != zcc) {
			if (wcc < 0) {
				perror("Writing Zeros");
			} else if ((wcc & (SECSIZE-1)) != 0) {
				fprintf(stderr, "Writing Zeros: "
					"partial sector write (%ld bytes)\n",
					(long)wcc);
				wcc = -1;
			} else if (wcc == 0) {
				fprintf(stderr, "Writing Zeros: "
					"unexpected EOF\n");
				wcc = -1;
			}
			if (wcc < 0)
				exit(1);
			zcc = wcc;
		}
		zcount     -= zcc;
		totalrdata += zcc;
		totalzdata += zcc;
		totalwriteops++;
		totalzeroops++;
		nextwriteoffset += zcc;
		dodots(DODOTS_ZERO, ozcount-zcount);
	}
	if (nextwriteoffset > maxwrittenoffset)
		maxwrittenoffset = nextwriteoffset;
}

void
writedata(off_t offset, size_t size, void *buf)
{
	ssize_t	cc;

	/*	fprintf(stderr, "Writing %d bytes at %qd\n", size, offset); */

	if (offset != nextwriteoffset)
		totalseekops++;

#ifndef FRISBEE
	if (docrconly) {
		compute_crc((u_char *)buf, size, &crc);
		cc = size;
	} else
#endif
	if (seekable) {
		cc = pwrite(outfd, buf, size, offset);
	} else if (offset == nextwriteoffset) {
		cc = write(outfd, buf, size);
	} else {
		fprintf(stderr, "Non-contiguous write @ %lld (should be %lld)\n",
			(long long)offset, (long long)nextwriteoffset);
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
	if (nextwriteoffset > maxwrittenoffset)
		maxwrittenoffset = nextwriteoffset;
	totalrdata += cc;
	totalwriteops++;
	dodots(DODOTS_DATA, cc);
}

/*
 * If the target disk is larger than the disk on which the image was made,
 * there will be some remaining space on the disk that needs to be zeroed.
 */
static void
zero_remainder()
{
	extern unsigned long getdisksize(int fd);
	off_t disksize;

	if (!dofill)
		return;

	/* XXX zeroing and slice mode don't mix; see earlier comment. */
	if (slice)
		return;

	if (outputmaxsec == 0)
		outputmaxsec = getdisksize(outfd);
	disksize = sectobytes(outputmaxsec);
	if (debug)
		fprintf(stderr, "\ndisksize = %lld\n", (long long)disksize);

	/* XXX must wait for writer thread to finish to get maxwrittenoffset value */
	threadwait();

	if (disksize > maxwrittenoffset) {
		off_t remaining = disksize - maxwrittenoffset;
		writebuf_t *wbuf;

		if (debug)
			fprintf(stderr, "zeroing %lld bytes at offset %lld "
				"(%u sectors at %u)\n",
				(long long)remaining,
				(long long)maxwrittenoffset,
				bytestosec(remaining),
				bytestosec(maxwrittenoffset));
		wbuf = alloc_writebuf(maxwrittenoffset, remaining, 0, 1);
		dowrite_request(wbuf);
	} else {
		if (debug)
			fprintf(stderr, "not zeroing: disksize = %lld, "
				"maxwritten =  %lld\n",
				(long long)disksize,
				(long long)maxwrittenoffset);
	}
}

#include "sliceinfo.h"

static long long outputmaxsize = 0;

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
		fprintf(stderr, "Slice Mode: S:%d min:%ld max:%ld size:%lld\n",
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
static void reloc_lilo(void *addr, int reloctype, uint32_t size);
static void reloc_lilocksum(void *addr, uint32_t off, uint32_t size);

#include "ffs/disklabel.h"	/* XXX doesn't belong here */
static void reloc_bsdlabel(struct disklabel *label, int reloctype);

static void
getrelocinfo(const blockhdr_t *hdr)
{
	const struct blockreloc *relocs;

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

	relocs = (const struct blockreloc *)
		((const char *)&hdr[1] +
		 hdr->regioncount * sizeof(struct region));
	memcpy(reloctable, relocs, numrelocs * sizeof(struct blockreloc));
}

/*
 * Perform relocations that apply to this chunk.
 * Return value is the new size of the valid data in buf.  This value
 * only changes for the special SHORTSECTOR reloc that indicates a chunk
 * that is not a multiple of the sector size.
 */
static size_t
applyrelocs(off_t offset, size_t size, void *buf)
{
	struct blockreloc *reloc;
	off_t roffset;
	uint32_t coff;
	size_t nsize = size;

	if (numrelocs == 0)
		return nsize;

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
					"Applying reloc type %d [%lld-%lld] "
					"to [%lld-%lld]\n", reloc->type,
					(long long)roffset,
					(long long)roffset+reloc->size,
					(long long)offset,
					(long long)offset+size);
			switch (reloc->type) {
			case RELOC_NONE:
				break;
			case RELOC_FBSDDISKLABEL:
			case RELOC_OBSDDISKLABEL:
				assert(reloc->size >= sizeof(struct disklabel));
				reloc_bsdlabel((struct disklabel *)((char *)buf+coff),
					       reloc->type);
				break;
			case RELOC_LILOSADDR:
			case RELOC_LILOMAPSECT:
				reloc_lilo((char *)buf+coff, reloc->type,
					   reloc->size);
				break;
			case RELOC_LILOCKSUM:
				reloc_lilocksum(buf, coff, reloc->size);
				break;
			case RELOC_SHORTSECTOR:
				assert(reloc->sectoff == 0);
				assert(reloc->size < SECSIZE);
				assert(roffset+SECSIZE == offset+size);
				nsize -= (SECSIZE - reloc->size);
				break;
			default:
				fprintf(stderr,
					"Ignoring unknown relocation type %d\n",
					reloc->type);
				break;
			}
		}
	}
	return nsize;
}

static void
reloc_bsdlabel(struct disklabel *label, int reloctype)
{
	int i, npart;
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
	npart = label->d_npartitions;
	for (i = 0; i < npart; i++) {
		uint32_t poffset, psize;

		if (label->d_partitions[i].p_size == 0)
			continue;

		/*
		 * Don't mess with OpenBSD partitions 8-15 which map
		 * extended DOS partitions.  Also leave raw partition
		 * alone as it maps the entire disk (not just slice)
		 * and we don't know how big that is.
		 */
		if (reloctype == RELOC_OBSDDISKLABEL &&
		    (i == 2 || (i >= 8 && i < 16)))
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

#include "extfs/lilo.h"

/*
 * Relocate a LILO sector address.  The address has been made relative
 * to the partition start so we need to add back the current partition
 * offset.
 *
 * XXX we still need to update the device portion of the sector to plug in
 * the appropriate BIOS device number for the drive we are being loaded on.
 * Either the user will have to specify that explicitly, or imageunzip will
 * have to get it from the OS or figure it out itself.
 */
static void
reloc_lilo(void *addr, int reloctype, uint32_t size)
{
	sectaddr_t *sect = addr;
	int i, count = 0;
	u_int32_t sector;

	switch (reloctype) {
	case RELOC_LILOSADDR:
		assert(size == 5);
		count = 1;
		break;
	case RELOC_LILOMAPSECT:
		assert(size == 512);
		count = MAX_MAP_SECT + 1;
		break;
	}

	for (i = 0; i < count; i++) {
		sector = getsector(sect);
		if (sector == 0)
			break;
		sector += outputminsec;
		putsector(sect, sector, sect->device, sect->nsect);
		sect++;
	}
}

void
reloc_lilocksum(void *addr, uint32_t off, uint32_t size)
{
	struct idtab *id;

	assert(size == 2);
	assert(off >= sizeof(struct idtab));
	addr = (void *)((char *)addr + off);

	/*
	 * XXX total hack: reloc entry points to the end of the
	 * descriptor table.  We back up sizeof(struct idtab)
	 * and checksum that many bytes.
	 */
	id = (struct idtab *)addr - 1;
	id->sum = 0;
	id->sum = lilocksum((union idescriptors *)id, LILO_CKSUM);
}


#if !defined(CONDVARS_WORK) && !defined(FRISBEE)
/*
 * Sleep. Basically wraps nanosleep, but since the threads package uses
 * signals, it keeps ending early!
 */
int
fsleep(unsigned int usecs)
{
	struct timespec time_to_sleep, time_not_slept;
	int	foo;

	time_to_sleep.tv_nsec  = (usecs % 1000000) * 1000;
	time_to_sleep.tv_sec   = usecs / 1000000;
	time_not_slept.tv_nsec = 0;
	time_not_slept.tv_sec  = 0;

	while ((foo = nanosleep(&time_to_sleep, &time_not_slept)) != 0) {
		if (foo < 0 && errno != EINTR) {
			return -1;
		}
		time_to_sleep.tv_nsec  = time_not_slept.tv_nsec;
		time_to_sleep.tv_sec   = time_not_slept.tv_sec;
		time_not_slept.tv_nsec = 0;
		time_not_slept.tv_sec  = 0;
	}
	return 0;
}
#endif

#ifndef FRISBEE
/*
 * Wrap up read in a retry mechanism to persist in the face of IO errors,
 * primarily for transient NFS errors.
 */
ssize_t
read_withretry(int fd, void *buf, size_t nbytes, off_t foff)
{
	ssize_t cc = 0;
	int i;

	if (readretries == 0)
		return read(fd, buf, nbytes);

	for (i = 0; i < readretries; i++) {
		if (i > 0)
			fprintf(stderr, "retrying...");
		cc = read(fd, buf, nbytes);
		if (cc >= 0) {
			if (i > 0)
				fprintf(stderr, "OK\n");
			break;
		}
		perror("image read");
		sleep(1);
		if (lseek(fd, foff, SEEK_SET) < 0) {
			perror("image seek");
			break;
		}
	}
	return cc;
}
#endif
