/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file garciaUtil.hh
 *
 * Header file for various utility functions related to the garcia.
 */

#ifndef _garcia_util_hh
#define _garcia_util_hh

#include "acpGarcia.h"

/**
 * The current debugging level, where zero means generate no debugging and
 * higher values increase the level.
 */
extern int debug;

/**
 * Find the number range where a given value lies.  For example, searching for
 * 250 in the following range set:
 *
 * @code
 *   static unsigned long myrange[] = { 0, 100, 200, 300, 0 };
 * @endcode
 *
 * Will return two because index two in "myrange" is the start of the subrange
 * where 250 lies.
 *
 * @param ranges The ranges to search for the given value.  The array should be
 * a series of ever-increasing numbers that represent the number ranges and
 * terminated with a zero.
 * @param index The index to search for in the given ranges.
 * @return The array index in ranges that indicates the start of the range or
 * -1 if no match was found.
 */
int find_range_start(unsigned long *range, unsigned long index);

/**
 * Wait for the garcia class to connect to the brainstem.
 *
 * @param ioRef IO-library reference.
 * @param garcia The garcia object to check for an active link.
 * @return True if the link is now active, false otherwise.
 */
bool wait_for_brainstem_link(aIOLib ioRef, acpGarcia &garcia);

/**
 * Callback protocol for classes that wish to perform some task during a
 * regular polling interval.
 */
class pollCallback
{
    
public:

    /**
     * Method called during the polling interval, which is about every 50 ms.
     *
     * @param now The current time in milliseconds.
     */
    virtual bool update(unsigned long now) = 0;
    
};

#endif
