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

#define BSIZE	0x4000
char		inbuf[BSIZE], outbuf[BSIZE];

int
main(int argc, char **argv)
{
	int		cc, err, infd, outfd;
	z_stream	d_stream; /* decompression stream */
	off_t		offset;
	long long	filesize = 0;

	d_stream.zalloc = (alloc_func)0;
	d_stream.zfree = (free_func)0;
	d_stream.opaque = (voidpf)0;

	err = deflateInit(&d_stream, Z_DEFAULT_COMPRESSION);
	CHECK_ERR(err, "inflateInit");

	if (argc != 3) {
		fprintf(stderr, "usage: "
		       "%s <input filename> <output filename>\n", argv[0]);
		exit(1);
	}

	if ((infd = open(argv[1], O_RDONLY, 0666)) < 0) {
		perror("opening input file");
		exit(1);
	}
	if ((outfd = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC, 0666)) < 0) {
		perror("opening output file");
		exit(1);
	}

	/*
	 * Prepend room for the filesize, written later
	 */
	if ((cc = write(outfd, outbuf, 1024)) < 0) {
		perror("writing initial output file");
		exit(1);
	}
	assert(cc == 1024);

	while (1) {
		cc = read(infd, inbuf, BSIZE);
		if (cc < 0) {
			perror("reading input file");
			exit(1);
		}
		if (cc == 0)
			break;

		d_stream.next_in   = inbuf;
		d_stream.avail_in  = cc;
		d_stream.next_out  = outbuf;
		d_stream.avail_out = BSIZE;

		err = deflate(&d_stream, Z_SYNC_FLUSH);
		CHECK_ERR(err, "deflate");

		if (d_stream.avail_in) {
			fprintf(stderr, "Something went wrong!\n");
			exit(1);
		}

		if ((cc = write(outfd, outbuf, BSIZE - d_stream.avail_out))
		    < 0) {
			perror("writing output file");
			exit(1);
		}
		assert(cc == (BSIZE - d_stream.avail_out));
	}
	
	/* Finish it off */
	d_stream.next_in   = 0;
	d_stream.avail_in  = 0;
	d_stream.next_out  = outbuf;
	d_stream.avail_out = BSIZE;
	err = deflate(&d_stream, Z_FINISH);
	if (err != Z_STREAM_END)
		CHECK_ERR(err, "deflate");
	
	err = deflateEnd(&d_stream);
	CHECK_ERR(err, "deflateEnd");

	fprintf(stderr, "Final %d\n", d_stream.total_out);

	/*
	 * Seek to the end to get the file size
	 */
	if ((offset = lseek(outfd, 0L, SEEK_END)) < 0) {
		perror("seeking to end");
		exit(1);
	}
	filesize = offset - 1024;

	/*
	 * And back to the beggining to write the size.
	 */
	if ((err = lseek(outfd, 0L, SEEK_SET)) < 0) {
		perror("seeking to end");
		exit(1);
	}
	if ((cc = write(outfd, &filesize, sizeof(filesize))) < 0) {
		perror("writing output file size");
		exit(1);
	}
	assert(cc == sizeof(filesize));

	/*
	 * And write a magic number too
	 */
	filesize = 0x69696969;
	if ((cc = write(outfd, &filesize, sizeof(filesize))) < 0) {
		perror("writing magic number");
		exit(1);
	}
	assert(cc == sizeof(filesize));

	close(infd);
	close(outfd);
	return 0;
}
