/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Glue to code from fsck_msdosfs
 */

#include <err.h>
#include <sys/types.h>
#include "dosfs.h"

#define	FSOK		0		/* Check was OK */
#define	FSBOOTMOD	1		/* Boot block was modified */
#define	FSDIRMOD	2		/* Some directory was modified */
#define	FSFATMOD	4		/* The FAT was modified */
#define	FSERROR		8		/* Some unrecovered error remains */
#define	FSFATAL		16		/* Some unrecoverable error occured */
#define FSDIRTY		32		/* File system is dirty */
#define FSFIXFAT	64		/* Fix file system FAT */

int readboot(int fd, struct bootblock *boot);
int readfat(int fd, struct bootblock *boot, int no, struct fatEntry **fp);
void fat_addskip(struct bootblock *boot, int startcl, int ncl);
off_t fat_lseek(int fd, off_t off, int whence);
ssize_t devread(int fd, void *buf, size_t nbytes);

#define lseek(f,o,w)	fat_lseek((f), (o), (w))
#define read(f,b,s)	devread((f), (b), (s))
#define pfatal		warnx
#define pwarn		warnx

#ifndef __RCSID
#define __RCSID(s)
#endif
