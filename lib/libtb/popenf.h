/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file popenf.h
 */

#ifndef _popenf_h
#define _popenf_h

#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The varargs version of popenf.
 *
 * @param fmt The printf(3)-like format for the command to execute.
 * @param type The direction of data flow for the pipe: "r" for reading and "w"
 * for writing.
 * @param args The arguments for the format.
 * @return The initialized FILE object that is connected to the other process
 * or NULL if there was an error.
 */
FILE *vpopenf(const char *fmt, const char *type, va_list args);

/**
 * A version of popen(3) that executes a command produced from a format.
 *
 * @code
 * @endcode
 *
 * @param fmt The printf(3)-like format for the command to execute.
 * @param type The direction of data flow for the pipe: "r" for reading and "w"
 * for writing.
 * @param ... The arguments for the format.
 * @return The initialized FILE object that is connected to the other process
 * or NULL if there was an error.
 */
FILE *popenf(const char *fmt, const char *type, ...);

#ifdef __cplusplus
}
#endif

#endif
