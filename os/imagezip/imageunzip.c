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
#define BSIZE	0x10000
char		inbuf[BSIZE], outbuf[2*BSIZE], zeros[BSIZE];

int
main(int argc, char **argv)
{
	int		cc, err, count, infd;
	z_stream	d_stream; /* inflation stream */
	long long	total = 0, blockcount = 0;
	char		*bp, *prog = argv[0];
	struct imagehdr hdr;
	struct region	*regions, *curregion;
	struct timeval  stamp, estamp;
	off_t		offset, size;

	d_stream.zalloc   = (alloc_func)0;
	d_stream.zfree    = (free_func)0;
	d_stream.opaque   = (voidpf)0;
	d_stream.next_in  = 0;
	d_stream.avail_in = 0;
	d_stream.next_out = 0;

	err = inflateInit(&d_stream);
	CHECK_ERR(err, "inflateInit");

	if (argc != 2) {
		fprintf(stderr, "usage: "
		       "%s <input filename>\n", argv[0]);
		exit(1);
	}

	if (strcmp(argv[1], "-")) {
		if ((infd = open(argv[1], O_RDONLY, 0666)) < 0) {
			perror("opening input file");
			exit(1);
		}
	}
	else
		infd = fileno(stdin);

	/*
	 * Read the header. It is uncompressed, and holds the real
	 * image size and the magic number.
	 */
	if ((cc = read(infd, inbuf, IMAGEHDRMINSIZE)) < 0) {
		perror("reading zipped image header goo");
		exit(1);
	}
	assert(cc == IMAGEHDRMINSIZE);
	memcpy(&hdr, inbuf, sizeof(hdr));
	fprintf(stderr, "Filesize: %qd, Magic: %x, "
		        "Regionsize %d, RegionCount %d\n",
		hdr.filesize, hdr.magic, hdr.regionsize, hdr.regioncount);

	/*
	 * Read in the valid data region information.
	 */
	regions   = (struct region *) calloc(hdr.regionsize, 1);
	curregion = regions;
	if ((cc = read(infd, regions, hdr.regionsize)) < 0) {
		perror("reading region info");
		exit(1);
	}
	assert(cc == hdr.regionsize);

	/*
	 * Start with the first region. 
	 */
	offset = curregion->start * (off_t) SECSIZE;
	size   = curregion->size  * (off_t) SECSIZE;
	curregion++;
	hdr.regioncount--;

	gettimeofday(&stamp, 0);
	while (1) {
		if ((cc = read(infd, inbuf, sizeof(inbuf))) <= 0) {
			if (cc == 0)
				break;
			perror("reading zipped image");
			exit(1);
		}

		d_stream.next_in   = inbuf;
		d_stream.avail_in  = cc;
	inflate_again:
		d_stream.next_out  = outbuf;
		d_stream.avail_out = sizeof(outbuf);

		err = inflate(&d_stream, Z_SYNC_FLUSH);
		if (err != Z_OK && err != Z_STREAM_END) {
			fprintf(stderr,
				"%s: inflate failed, err=%ld\n", prog, err);
			exit(1);
		}
		count = sizeof(outbuf) - d_stream.avail_out;
		bp = outbuf;

		while (count) {
			/*
			 * Write data only as far as the end of the current
			 * region.
			 */
			if (count < size)
				cc = count;
			else
				cc = size;

			if ((cc = write(1, bp, cc)) != cc) {
				if (cc < 0) {
					perror("Writing uncompressed data");
				}
				fprintf(stderr, "%s: inflate failed\n", prog);
				exit(1);
			}

			count  -= cc;
			bp     += cc;
			size   -= cc;
			offset += cc;
			total  += cc;
			blockcount += cc;

			/*
			 * Hit the end of the region. Need to figure out
			 * where the next one starts. We write a block of
			 * zeros in the empty space between this region
			 * and the next. We could lseek too, but only if
			 * not writing to stdout. 
			 */
			if (! size) {
				off_t	newoffset, zcount;
				int	zcc;

				/*
				 * No more regions. Must be done.
				 */
				if (!hdr.regioncount)
					break;
				
				newoffset = curregion->start * (off_t) SECSIZE;
				zcount    = newoffset - offset;

				while (zcount) {
					if (zcount <= BSIZE)
						zcc = (int) zcount;
					else
						zcc = BSIZE;

					if ((zcc =
					     write(1, zeros, zcc)) != zcc) {
						if (zcc < 0) {
							perror("Writing "
							       "Zeros");
						}
						exit(1);
					}
					zcount -= zcc;
					total  += zcc;
					blockcount += zcc;
				}

				offset = newoffset;
				size   = curregion->size * (off_t) SECSIZE;
				curregion++;
				hdr.regioncount--;
			}
		}
		if (d_stream.avail_in)
			goto inflate_again;

		if (blockcount > (1024 * 16 * 8192)) {
			gettimeofday(&estamp, 0);
			estamp.tv_sec -= stamp.tv_sec;
			fprintf(stderr, "Wrote %qd bytes ", total);
			fprintf(stderr, "in %ld seconds\n", estamp.tv_sec);
			blockcount = 0;
		}
	}
	assert(hdr.regioncount == 0);
	
	err = inflateEnd(&d_stream);
	CHECK_ERR(err, "inflateEnd");

	gettimeofday(&estamp, 0);
	estamp.tv_sec -= stamp.tv_sec;
	fprintf(stderr, "Finshed! Wrote %qd bytes ", total);
	fprintf(stderr, "in %ld seconds\n", estamp.tv_sec);

	close(infd);
	return 0;
}
