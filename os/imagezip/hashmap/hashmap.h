/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

/* XXX from global.h */
extern int secsize;
#define sectobytes(s)	((off_t)(s) * secsize)
#define bytestosec(b)	(uint32_t)((b) / secsize)

/* XXX from imagezip.c */
struct range {
        uint32_t        start;          /* In sectors */
        uint32_t        size;           /* In sectors */
        void            *data;
        struct range    *next;
};

void free_ranges(struct range **);

#define MAX(x, y)	(((x) > (y)) ? (x) : (y))
#define MIN(x, y)	(((x) > (y)) ? (y) : (x))
