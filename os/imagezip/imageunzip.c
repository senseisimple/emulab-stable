/*
 * Usage: imageunzip <input file>
 *
 * Writes the uncompressed data to stdout.
 */

#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <zlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include "imagehdr.h"

#define CHECK_ERR(err, msg) { \
    if (err != Z_OK) { \
        fprintf(stderr, "%s error: %d\n", msg, err); \
        exit(1); \
    } \
}

#define SECSIZE 512
#define BSIZE	(32 * 1024)
#define OUTSIZE (2 * BSIZE)
char		inbuf[BSIZE], outbuf[OUTSIZE + SECSIZE], zeros[BSIZE];

int		infd, outfd;
int		doseek = 0;
int		debug = 1;
long long	total = 0;
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
	return write(fd, buf, size);
}
#endif


int
main(int argc, char **argv)
{
	struct timeval  stamp, estamp;

	if (argc < 2 || argc > 3) {
		fprintf(stderr, "usage: "
		       "%s <input filename> [output filename]\n", argv[0]);
		exit(1);
	}

	if ((infd = open(argv[1], O_RDONLY, 0666)) < 0) {
		perror("opening input file");
		exit(1);
	}
	if (argc == 3) {
		if ((outfd =
		     open(argv[2], O_WRONLY|O_CREAT|O_TRUNC, 0666)) < 0) {
			perror("opening output file");
			exit(1);
		}
		doseek = 1;
	}
	else
		outfd = fileno(stdout);
	
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

int
inflate_subblock(void)
{
	int		cc, err, count, ibsize = 0, ibleft = 0;
	z_stream	d_stream; /* inflation stream */
	char		*bp;
	struct blockhdr *blockhdr;
	struct region	*curregion;
	off_t		offset, size;
	char		buf[DEFAULTREGIONSIZE];

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
	if ((cc = read(infd, buf, DEFAULTREGIONSIZE)) <= 0) {
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

	if (debug)
		fprintf(stderr, "Decompressing: %14qd --> ", offset);

	while (1) {
		/*
		 * Read just up to the end of compressed data.
		 */
		if (blockhdr->size >= sizeof(inbuf))
			count = sizeof(inbuf);
		else
			count = blockhdr->size;
			
		if ((cc = read(infd, inbuf, count)) <= 0) {
			if (cc == 0) {
				return 1;
			}
			perror("reading zipped image");
			exit(1);
		}
		assert(cc == count);
		blockhdr->size -= cc;

		d_stream.next_in   = inbuf;
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

			if ((cc = devwrite(outfd, bp, cc)) != cc) {
				if (cc < 0) {
					perror("Writing uncompressed data");
				}
				fprintf(stderr, "inflate failed\n");
				exit(1);
			}

			count  -= cc;
			bp     += cc;
			size   -= cc;
			offset += cc;
			total  += cc;

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

				writezeros(newoffset - offset);

				offset = newoffset;
				size   = curregion->size * (off_t) SECSIZE;
				assert(size);
				curregion++;
				blockhdr->regioncount--;
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

	if (debug)
		fprintf(stderr, "%14qd\n", total);

	return 0;
}

void
writezeros(off_t zcount)
{
	int	zcc;

	if (doseek) {
		if (devlseek(outfd, zcount, SEEK_CUR) < 0) {
			perror("Skipping ahead");
			exit(1);
		}
		total  += zcount;
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
