/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004-2011 University of Utah and the Flux Group.
 * All rights reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "systemf.h"

int vsystemf(const char *fmt, va_list args)
{
	char cmd_buf[1024], *cmd = cmd_buf;
	int rc, retval;

	assert(fmt != NULL);

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
		retval = system(cmd);
		if (cmd != cmd_buf) {
			free(cmd);
			cmd = NULL;
		}
	}
	else {
		retval = -1;
		errno = ENOMEM;
	}
	
	return retval;
}

int systemf(const char *fmt, ...)
{
	int retval;
	va_list args;

	va_start(args, fmt);
	retval = vsystemf(fmt, args);
	va_end(args);

	return retval;
}
