/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Some simple common utility functions.
 */

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include "decls.h"

/*
 * Return current time in a string printable format. Caller must absorb
 * the string. 
 */
char *
CurrentTimeString(void)
{
	static char	buf[32];
	time_t		curtime;
	static struct tm tm;
	
	time(&curtime);
	strftime(buf, sizeof(buf), "%T", localtime_r(&curtime, &tm));

	return buf;
}

/*
 * Sleep. Basically wraps nanosleep, but since the threads package uses
 * signals, it keeps ending early!
 */
int
fsleep(unsigned int usecs)
{
	struct timespec time_to_sleep, time_not_slept;
	int	foo;

	time_to_sleep.tv_nsec  = (usecs % 1000000) * 1000;
	time_to_sleep.tv_sec   = usecs / 1000000;
	time_not_slept.tv_nsec = 0;
	time_not_slept.tv_sec  = 0;

	while ((foo = nanosleep(&time_to_sleep, &time_not_slept)) != 0) {
		if (foo < 0 && errno != EINTR) {
			return -1;
		}
		
		time_to_sleep.tv_nsec  = time_not_slept.tv_nsec;
		time_to_sleep.tv_sec   = time_not_slept.tv_sec;
		time_not_slept.tv_nsec = 0;
		time_not_slept.tv_sec  = 0;
	}
	return 0;
}
