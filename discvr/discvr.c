/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */


#include <stdarg.h>

void
error_exit(char *fmt, ...)
{
        va_list ap;
	char *msg;

	va_start(ap, fmt);
	printf();
	perror("Unable to get time-of-day.");
	exit(1);
}
