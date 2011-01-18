/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2002-2010 University of Utah and the Flux Group.
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
 *
 * In G++ 4.3, hash_set and hash_map were formally deprecated and
 * moved from ext/ to backward/.  Well, that's what the release notes
 * claim.  In fact, on my system, hash_set and hash_map appear in both
 * ext/ and backward/.  But, hash_fun.h is only in backward/, necessi-
 * tating the NEWER_GCC macro.
 *
 * The real fix is to replace
 *   hash_set with tr1::unordered_set in <tr1/unordered_set>
 *   hash_map with tr1::unordered_map in <tr1/unordered_map>
 */
#if (__GNUC__ == 3 && __GNUC_MINOR__ > 0) || (__GNUC__ > 3)
#define NEW_GCC
#endif

#if (__GNUC__ == 4 && __GNUC_MINOR__ >= 3) || (__GNUC__ > 4)
#define NEWER_GCC
#endif

#ifdef NEW_GCC
#include <ext/slist>
using namespace __gnu_cxx;
#else
#include <slist>
#endif

#else
#endif
