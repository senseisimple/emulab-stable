/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004, 2006, 2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "config.h"

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "popenf.h"

/*
 * Define vfork to fork. Why? Well starting at 6.X FreeBSD switched
 * its underlying pthread impl, and popen is broken in threaded apps,
 * since it uses vfork. I have no understanding of any of this, only
 * that avoiding vfork solves the problem.  We can back this change
 * out once we figure out a real solution.
 */
int vfork()
{
  return fork();
}

FILE *vpopenf(const char *fmt, const char *type, va_list args)
{
	char cmd_buf[1024], *cmd = cmd_buf;
	FILE *retval;
	int rc;

	assert(fmt != NULL);
	assert(type != NULL);

	if ((rc = vsnprintf(cmd,
			    sizeof(cmd_buf),
			    fmt,
			    args)) >= sizeof(cmd_buf)) {
		if ((cmd = malloc(rc + 1)) != NULL) {
			vsnprintf(cmd, rc + 1, fmt, args);
			cmd[rc] = '\0';
		}
	}

	if (cmd != NULL) {
		retval = popen(cmd, type);
		if (cmd != cmd_buf) {
			free(cmd);
			cmd = NULL;
		}
	}
	else {
		retval = NULL;
		errno = ENOMEM;
	}
	
	return retval;
}

FILE *popenf(const char *fmt, const char *type, ...)
{
	FILE *retval;
	va_list args;

	va_start(args, type);
	retval = vpopenf(fmt, type, args);
	va_end(args);

	return retval;
}
