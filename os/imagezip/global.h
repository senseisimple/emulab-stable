/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Global defns that should go away someday
 */
extern int debug;
extern int secsize;
extern int slicemode;
extern int dorelocs;

extern char *slicename(int slice, u_int32_t offset, u_int32_t size, int type);
extern off_t devlseek(int fd, off_t off, int whence);
extern ssize_t devread(int fd, void *buf, size_t nbytes);
extern void addskip(uint32_t start, uint32_t size);
extern void addfixup(off_t offset, off_t poffset, off_t size, void *data,
		     int reloctype);

extern SLICEMAP_PROCESS_PROTO(read_bsdslice);
extern SLICEMAP_PROCESS_PROTO(read_linuxslice);
extern SLICEMAP_PROCESS_PROTO(read_linuxswap);
extern SLICEMAP_PROCESS_PROTO(read_ntfsslice);
extern SLICEMAP_PROCESS_PROTO(read_fatslice);

#define sectobytes(s)	((off_t)(s) * secsize)
#define bytestosec(b)	(uint32_t)((b) / secsize)
