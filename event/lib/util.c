/*
 * util.c --
 *
 *      Utility routines for event system.
 *
 * @COPYRIGHT@
 */

#include <stdio.h>
#include <stdlib.h>
#include "event.h"

/* Attempt to allocate SIZE bytes of memory and exit if memory
   allocation fails.  Returns pointer to allocated memory. */
void *
xmalloc(int size)
{
    void *p;
    p = malloc(size);
    if (!p) {
        fprintf(stderr, "virtual memory exhausted\n");
        exit(1);
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
        fprintf(stderr, "virtual memory exhausted\n");
        exit(1);
    }        
    return q;
}
