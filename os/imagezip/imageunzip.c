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

#define CHECK_ERR(err, msg) { \
    if (err != Z_OK) { \
        fprintf(stderr, "%s error: %d\n", msg, err); \
        exit(1); \
    } \
}

#define BSIZE	0x10000
char		inbuf[BSIZE], outbuf[2*BSIZE];

int
main(int argc, char **argv)
{
	int		cc, err, infd;
	z_stream	d_stream; /* inflation stream */
	off_t		offset;
	long long	filesize, magic, total = 0, blockcount = 0;
	char		*prog = argv[0];
	struct timeval  stamp, estamp;

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

	if ((infd = open(argv[1], O_RDONLY, 0666)) < 0) {
		perror("opening input file");
		exit(1);
	}

	/*
	 * Read the 1024 byte header. It is uncompressed, and holds
	 * the real image size and the magic number.
	 */
	if ((cc = read(infd, inbuf, 1024)) < 0) {
		perror("reading zipped image header goo");
		exit(1);
	}
	assert(cc == 1024);
	filesize = *((long long *) inbuf);
	magic    = *((long long *) &inbuf[sizeof(filesize)]);
	fprintf(stderr, "Filesize = %qd, Magic = %qx\n", filesize, magic);

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
		cc = sizeof(outbuf) - d_stream.avail_out;

		if ((cc = write(1, outbuf, cc)) != cc) {
			if (cc < 0) {
				perror("Writing uncompressed data");
			}
			fprintf(stderr, "%s: inflate failed\n", prog);
			exit(1);
		}
		total += cc;
		blockcount += cc;
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
	err = inflateEnd(&d_stream);
	CHECK_ERR(err, "inflateEnd");

	gettimeofday(&estamp, 0);
	estamp.tv_sec -= stamp.tv_sec;
	fprintf(stderr, "Finshed! Wrote %qd bytes ", total);
	fprintf(stderr, "in %ld seconds\n", estamp.tv_sec);

	close(infd);
	return 0;
}
