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
	long long	filesize, magic;
	char		*prog = argv[0];

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
		if (d_stream.avail_in)
			goto inflate_again;
		
	}
	err = inflateEnd(&d_stream);
	CHECK_ERR(err, "inflateEnd");

	close(infd);
	return 0;
}
