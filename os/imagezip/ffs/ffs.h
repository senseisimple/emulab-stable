/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Glue for FreeBSD 5.x <ufs/ffs/fs.h>.  This version supports UFS2.
 */
typedef int64_t ufs_time_t;
typedef	int32_t	ufs1_daddr_t;
typedef	int64_t	ufs2_daddr_t;

#ifndef BBSIZE
#define	BBSIZE		8192
#endif
#ifndef MAXBSIZE
#define MAXBSIZE	65536
#endif

#include "fs.h"
#define FSTYPENAMES
#include "disklabel.h"
