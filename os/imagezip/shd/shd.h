/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * XXX all this should come out of a standard include file, this is just
 * here to get everything to compile.
 */

#include <sys/ioctl.h>

struct shd_range {
	u_int32_t start;
	u_int32_t end;
};

struct shd_modinfo {
	int command;		/* init=1, data=2, deinit=3 */
	struct shd_range *buf;	/* range buffer */
	long bufsiz;		/* buffer size (in entries) */
	long retsiz;		/* size of returned data (in entries) */
};

struct shd_allocinfo {
	struct shd_range *buf;	/* range buffer */
	long bufsiz;		/* buffer size (in entries) */
};

#define SHDGETMODIFIEDRANGES  _IOWR('S', 29, struct shd_modinfo)
#define SHDSETALLOCATEDRANGES _IOWR('S', 30, struct shd_allocinfo)
