/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2011 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Logging and debug routines.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <assert.h>
#include <errno.h>
#include "decls.h"

#ifndef LOG_TESTBED
#define LOG_TESTBED	LOG_USER
#endif

static int usesyslog = 1;

/*
 * There is really no point in the client using syslog, but its nice
 * to use the same log functions either way.
 */
int
ClientLogInit(void)
{
	usesyslog = 0;
	return 0;
}

int
ServerLogInit(void)
{
	if (debug) {
		usesyslog = 0;
		return 1;
	}

	openlog("frisbeed", LOG_PID, LOG_TESTBED);

	return 0;
}

int
UploadLogInit(void)
{
	if (debug) {
		usesyslog = 0;
		return 1;
	}

	openlog("frisuploadd", LOG_PID, LOG_TESTBED);

	return 0;
}

int
MasterServerLogInit(void)
{
	if (debug) {
		usesyslog = 0;
		return 1;
	}

	openlog("mfrisbeed", LOG_PID, LOG_TESTBED);

	return 0;
}

void
info(const char *fmt, ...)
{
	va_list args;
	char	buf[BUFSIZ];

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	if (!usesyslog) {
		fputs(buf, stderr);
		fputc('\n', stderr);
	}
	else
		syslog(LOG_INFO, "%s", buf);
}

void
log(const char *fmt, ...)
{
	va_list args;
	char	buf[BUFSIZ];

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	if (!usesyslog) {
		fputs(buf, stderr);
		fputc('\n', stderr);
	}
	else
		syslog(LOG_INFO, "%s", buf);
}

void
warning(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	if (!usesyslog) {
		vfprintf(stderr, fmt, args);
		fputc('\n', stderr);
		fflush(stderr);
	}
	else
		vsyslog(LOG_WARNING, fmt, args);
	       
	va_end(args);
}

void
error(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	if (!usesyslog) {
		vfprintf(stderr, fmt, args);
		fputc('\n', stderr);
		fflush(stderr);
	}
	else
		vsyslog(LOG_ERR, fmt, args);
	       
	va_end(args);
}

void
fatal(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	if (!usesyslog) {
		vfprintf(stderr, fmt, args);
		fputc('\n', stderr);
		fflush(stderr);
	}
	else
		vsyslog(LOG_ERR, fmt, args);
	       
	va_end(args);
	exit(-1);
}

void
pwarning(const char *fmt, ...)
{
	va_list args;
	char	buf[BUFSIZ];

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	warning("%s : %s", buf, strerror(errno));
}

void
pfatal(const char *fmt, ...)
{
	va_list args;
	char	buf[BUFSIZ];

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	fatal("%s : %s", buf, strerror(errno));
}
