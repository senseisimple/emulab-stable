/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2002-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

// This file may need to be changed depending on the architecture.
#ifndef __PORT_H
#define __PORT_H
#include <limits.h>

#ifndef WCHAR_MIN
#define WCHAR_MIN INT_MIN
#define WCHAR_MAX INT_MAX
#endif

/*
 * We have to do these includes differently depending on which version of gcc
 * we're compiling with
 */
#if __GNUC__ == 3 && __GNUC_MINOR__ > 0
#include <ext/slist>
using namespace __gnu_cxx;
#else
#include <slist>
#endif

#else
#endif
