/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file systemf.h
 */

#ifndef _systemf_h
#define _systemf_h

#include <stdlib.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

int vsystemf(const char *fmt, va_list args);
int systemf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
