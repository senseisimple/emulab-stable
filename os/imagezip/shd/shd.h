/*
 * XXX all this should come out of a standard include file, this is just
 * here to get everything to compile.
 */

#define SHDGETMODIFIEDRANGES	1	/* the ioctl */

struct shd_range {
	u_int32_t start;
	u_int32_t size;
};

struct shd_modinfo {
	int command;		/* init=1, data=2, deinit=3 */
	struct shd_range *buf;	/* range buffer */
	long bufsiz;		/* buffer size (in entries) */
	long retsiz;		/* size of returned data (in entries) */
};
