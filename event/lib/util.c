/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * util.c --
 *
 *      Utility routines for event system.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "event.h"
#include "log.h"

/* Attempt to allocate SIZE bytes of memory and exit if memory
   allocation fails.  Returns pointer to allocated memory. */
void *
xmalloc(int size)
{
    void *p;
    p = malloc(size);
    if (!p) {
	fatal("virtual memory exhausted!");
    }        
    return p;
}

/* Attempt to change the size of allocated memory block P to SIZE
   bytes and exit if memory allocation fails.  Returns pointer to
   (re)allocated memory. */
void *
xrealloc(void *p, int size)
{
    void *q;
    q = realloc(p, size);
    if (!q) {
	fatal("virtual memory exhausted!");
    }        
    return q;
}
