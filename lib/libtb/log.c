/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Logging and debug routines.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <assert.h>
#include <errno.h>
#include "config.h"
#include "log.h"

static int	usesyslog = 0;
static char    *filename;
#define LOG_IDENT   "Testbed"

/*
 * Initialize.  If slog is non-zero use syslog with name (if provided)
 * as the identifier.  If slog is zero, then name (if set) is the name of
 * a logfile to which stdout and stderr are redirected.
 */
int
loginit(int slog, char *name)
{
	if (slog) {
		usesyslog = 1;
		if (! name)
			name = LOG_IDENT;
		openlog(name, LOG_PID, LOG_TESTBED);
		return 0;
	}

	usesyslog = 0;

	if (name) {
		int	fd;

		if ((fd = open(name, O_RDWR|O_CREAT|O_APPEND, 0644)) != -1) {
			(void)dup2(fd, STDOUT_FILENO);
			(void)dup2(fd, STDERR_FILENO);
			if (fd > 2)
				(void)close(fd);
		}
		filename = name;
	}
	return 0;
}

/*
 * Switch to syslog; caller has already opened syslog connection.
 */
void
logsyslog(void)
{
	usesyslog = 1;
}

/*
 * Flush any buffered log output
 */
void
logflush(void)
{
	if (!usesyslog)
		fflush(stderr);
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
		fflush(stderr);
	}
	else
		vsyslog(LOG_ERR, fmt, args);
	       
	va_end(args);
}

void
errorc(const char *fmt, ...)
{
	va_list args;
	char	buf[BUFSIZ];

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	error("%s : %s\n", buf, strerror(errno));
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

	warning("%s : %s\n", buf, strerror(errno));
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
