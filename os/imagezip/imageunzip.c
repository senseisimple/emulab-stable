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
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <zlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/disklabel.h>
#include "imagehdr.h"

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
#define OUTSIZE (3 * BSIZE)
char		inbuf[BSIZE], outbuf[OUTSIZE + SECSIZE], zeros[BSIZE];

static int	 infd, outfd;
static int	 doseek = 0;
static int	 debug  = 0;
static long long total  = 0;

int		inflate_subblock(void);
void		writezeros(off_t zcount);

#ifdef linux
#define devlseek	lseek
#define devwrite	write
#else
static inline off_t devlseek(int fd, off_t off, int whence)
{
	off_t noff;
	assert((off & (SECSIZE-1)) == 0);
	noff = lseek(fd, off, whence);
	assert(noff == (off_t)-1 || (noff & (SECSIZE-1)) == 0);
	return noff;
}

static inline int devwrite(int fd, const void *buf, size_t size)
{
	assert((size & (SECSIZE-1)) == 0);
	assert(outputmaxsize == 0 || (total + size) <= outputmaxsize);
	return write(fd, buf, size);
}

static inline int devread(int fd, void *buf, size_t size)
{
	assert((size & (SECSIZE-1)) == 0);
	return read(fd, buf, size);
}
#endif

static void
usage(void)
{
	fprintf(stderr, "usage: "
		"imageunzip [-d] [-s #] <input filename> [output filename]\n"
		" -s slice        Output to DOS slice (DOS numbering 1-4)\n"
		"                 NOTE: Must specify a raw disk device.\n"
		" -d              Turn on progressive levels of debugging\n");
	exit(1);
}	

#ifndef FRISBEE
int
main(int argc, char **argv)
{
	int		ch, slice = 0;
	struct timeval  stamp, estamp;

	while ((ch = getopt(argc, argv, "dhs:")) != -1)
		switch(ch) {
		case 'd':
			debug++;
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

	if (argc < 1 || argc > 2)
		usage();
	if (argc == 1 && slice) {
		fprintf(stderr, "Cannot specify a slice when using stdout!\n");
		usage();
	}

	if ((infd = open(argv[0], O_RDONLY, 0666)) < 0) {
		perror("opening input file");
		exit(1);
	}
	if (argc == 2) {
		if ((outfd =
		     open(argv[1], O_RDWR|O_CREAT|O_TRUNC, 0666)) < 0) {
			perror("opening output file");
			exit(1);
		}
		doseek = 1;
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
	
	gettimeofday(&stamp, 0);
	
	while (1) {
		off_t		offset, correction;
		
		/*
		 * Decompress one subblock at a time. After its done
		 * make sure the input file pointer is at a block
		 * boundry. Move it up if not. 
		 */
		if (inflate_subblock())
			break;

		if ((offset = lseek(infd, (off_t) 0, SEEK_CUR)) < 0) {
			perror("Getting current seek pointer");
			exit(1);
		}
		if (offset & (SUBBLOCKSIZE - 1)) {
			correction = SUBBLOCKSIZE -
				(offset & (SUBBLOCKSIZE - 1));

			if ((offset = lseek(infd, correction, SEEK_CUR)) < 0) {
				perror("correcting seek pointer");
				exit(1);
			}
		}
	}
	close(infd);

	gettimeofday(&estamp, 0);
	estamp.tv_sec -= stamp.tv_sec;
	printf("Done in %ld seconds!\n", estamp.tv_sec);
	
	return 0;
}
#else
/*
 * When compiled for frisbee, act as a library.
 */
ImageUnzipInit(char *filename, int slice, int dbg)
{
	if ((outfd = open(filename, O_RDWR|O_CREAT|O_TRUNC, 0666)) < 0) {
		perror("opening output file");
		exit(1);
	}
	doseek = 1;
	debug  = dbg;

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
}
#endif

int
inflate_subblock(void)
{
	int		cc, ccres, err, count, ibsize = 0, ibleft = 0;
	z_stream	d_stream; /* inflation stream */
	char		*bp;
	struct blockhdr *blockhdr;
	struct region	*curregion;
	off_t		offset, size;
	char		*buf = inbuf;
	
	d_stream.zalloc   = (alloc_func)0;
	d_stream.zfree    = (free_func)0;
	d_stream.opaque   = (voidpf)0;
	d_stream.next_in  = 0;
	d_stream.avail_in = 0;
	d_stream.next_out = 0;

	err = inflateInit(&d_stream);
	CHECK_ERR(err, "inflateInit");

	/*
	 * Read the header. It is uncompressed, and holds the real
	 * image size and the magic number.
	 */
#ifdef  FRISBEE
	if ((cc = FrisbeeRead(&buf, DEFAULTREGIONSIZE)) <= 0) {
#else
	if ((cc = read(infd, buf, DEFAULTREGIONSIZE)) <= 0) {
#endif
		if (cc == 0)
			return 1;
		perror("reading zipped image header goo");
		exit(1);
	}
	assert(cc == DEFAULTREGIONSIZE);
	blockhdr = (struct blockhdr *) buf;

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

	/*
	 * Set the output pointer to the beginning of the region.
	 */
	if (doseek) {
		if (devlseek(outfd, offset, SEEK_SET) < 0) {
			perror("Skipping to start of output region");
			exit(1);
		}
		total += offset - total;
	}
	else {
		assert(offset > total);
		writezeros(offset - total);
	}

	if (debug == 1)
		printf("Decompressing: %14qd --> ", offset);

	while (1) {
		/*
		 * Read just up to the end of compressed data.
		 */
		if (blockhdr->size >= sizeof(inbuf))
			count = sizeof(inbuf);
		else
			count = blockhdr->size;
			
#ifdef  FRISBEE
		if ((cc = FrisbeeRead(&buf, count)) <= 0) {
#else
		if ((cc = read(infd, buf, count)) <= 0) {
#endif
			if (cc == 0) {
				return 1;
			}
			perror("reading zipped image");
			exit(1);
		}
		assert(cc == count);
		blockhdr->size -= cc;

		d_stream.next_in   = buf;
		d_stream.avail_in  = cc;
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
			fprintf(stderr, "inflate failed, err=%ld\n", err);
			exit(1);
		}
		ibsize = (OUTSIZE - d_stream.avail_out) + ibleft;
		count  = ibsize & ~(SECSIZE - 1);
		ibleft = ibsize - count;
		bp     = outbuf;

		while (count) {
			/*
			 * Write data only as far as the end of the current
			 * region.
			 */
			if (count < size)
				cc = count;
			else
				cc = size;

			if (debug == 2) {
				printf("%12qd %8d %8d %12qd %10qd %8d %5d %8d"
				       "\n",
				       offset, cc, count, total, size, ibsize,
				       ibleft, d_stream.avail_in);
			}

			if ((ccres = devwrite(outfd, bp, cc)) != cc) {
				if (ccres < 0) {
					perror("Writing uncompressed data");
				}
				fprintf(stderr, "inflate failed\n");
				exit(1);
			}
			cc = ccres;

			count  -= cc;
			bp     += cc;
			size   -= cc;
			offset += cc;
			total  += cc;
			assert(count >= 0);
			assert(size  >= 0);
#ifndef	FRISBEE
			assert(total == offset);
#endif
			/*
			 * Hit the end of the region. Need to figure out
			 * where the next one starts. We write a block of
			 * zeros in the empty space between this region
			 * and the next. We can lseek, but only if
			 * not writing to stdout. 
			 */
			if (! size) {
				off_t	newoffset;

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
#ifdef FRISBEE
				writezeros(newoffset - offset);
				offset = newoffset;
#else
				offset = newoffset;
				assert(offset >= total);
				if (offset > total)
					writezeros(offset - total);
#endif
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

	if (debug == 1)
		printf("%14qd\n", total);

	return 0;
}

void
writezeros(off_t zcount)
{
	int	zcc;
	off_t	offset;

	if (doseek) {
		if ((offset = devlseek(outfd, zcount, SEEK_CUR)) < 0) {
			perror("Skipping ahead");
			exit(1);
		}
		total  += zcount;
#ifndef FRISBEE
		assert(offset == total + (((long long)outputminsec)*SECSIZE));
#endif
		return;
	}
	
	while (zcount) {
		if (zcount <= BSIZE)
			zcc = (int) zcount;
		else
			zcc = BSIZE;
		
		if ((zcc = devwrite(outfd, zeros, zcc)) != zcc) {
			if (zcc < 0) {
				perror("Writing Zeros");
			}
			exit(1);
		}
		zcount -= zcc;
		total  += zcc;
	}
}

/*
 * Parse the DOS partition table to set the bounds of the slice we
 * are writing to. 
 */
int
readmbr(int slice)
{
	int		i, cc, rval = 0;
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
		fprintf(stderr, "Slice Mode: S:%d min:%d max:%d size:%qd\n",
			slice, outputminsec, outputmaxsec, outputmaxsize);
	}
	return 0;
}

