/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file garciaUtil.cc
 *
 * Implementation file for various utility functions related to the garcia.
 */

#include "config.h"

#include <stdio.h>
#include <assert.h>

#include "garciaUtil.hh"

int debug = 0;

int find_range_start(unsigned long *ranges, unsigned long index)
{
    int lpc, retval = -1;

    assert(ranges != NULL);

    for (lpc = 0; (retval == -1) && (ranges[lpc + 1] != 0); lpc++) {
	if ((ranges[lpc] <= index) && (index < ranges[lpc + 1])) {
	    retval = lpc;
	}
    }

    return retval;
}

bool wait_for_brainstem_link(aIOLib ioRef, acpGarcia &garcia)
{
    int lpc;

    if (debug) {
	fprintf(stderr, "debug: establishing connection to brainstem\n");
    }
    
    aIO_MSSleep(ioRef, 500, NULL);
    for (lpc = 0;
	 (lpc < 30) && !garcia.getNamedValue("active")->getBoolVal();
	 lpc++) {
	printf(".");
	fflush(stdout);
	aIO_MSSleep(ioRef, 100, NULL);
    }
    if (lpc > 0)
	printf("\n");

    return garcia.getNamedValue("active")->getBoolVal();
}
