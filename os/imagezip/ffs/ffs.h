/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003, 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Glue for FreeBSD 5.x <ufs/ffs/fs.h>.  This version supports UFS2.
 */

#include "dinode.h"
union dinode {
	struct ufs1_dinode dp1;
	struct ufs2_dinode dp2;
};
#define	DIP(magic, dp, field) \
	(((magic) == FS_UFS1_MAGIC) ? (dp)->dp1.field : (dp)->dp2.field)

#ifndef BBSIZE
#define	BBSIZE		8192
#endif
#ifndef MAXBSIZE
#define MAXBSIZE	65536
#endif

#include "fs.h"
#define FSTYPENAMES
#include "disklabel.h"
