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
#include <sys/sysctl.h>
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

int
sleeptime(unsigned int usecs, char *str)
{
	static unsigned int clockres_us;
	int nusecs;

	if (clockres_us == 0) {
		struct clockinfo ci;
		int cisize = sizeof(ci);

		ci.hz = 0;
		if (sysctlbyname("kern.clockrate", &ci, &cisize, 0, 0) < 0 ||
		    ci.hz == 0) {
			warning("cannot get clock resolution, assuming 100HZ");
			clockres_us = 10000;
		} else
			clockres_us = ci.tick;
		if (debug)
			log("clock resolution: %d us", clockres_us);
	}
	nusecs = ((usecs + clockres_us - 1) / clockres_us) * clockres_us;
/*
 * Rounding is actually a bad idea.  Say the kernel has just gone past
 * system tick N.  If we round a value of 1/2 tick up to 1 tick and then
 * call sleep, the kernel will add our 1 tick to the current value to get
 * a time slightly past tick N+1.  It will then round that up to N+2,
 * so we effectively sleep almost two full ticks.  But if we don't round
 * the tick, then adding that to the current time might yield a value
 * less than N+1, which will round up to N+1 and we will at worst sleep
 * one full tick.
 */
#if 0
	if (nusecs != usecs && str != NULL)
		warning("%s: rounded to %d from %d "
			"due to clock resolution", str, nusecs, usecs);
#else
	if (nusecs != usecs && str != NULL) {
		warning("%s: may be up to %d (instead of %d) "
			"due to clock resolution", str, nusecs, usecs);
	}
	nusecs = usecs;
#endif

	return nusecs;
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

#ifdef STATS
void
ClientStatsDump(unsigned int id, ClientStats_t *stats)
{
	int seconds;

	switch (stats->version) {
	case 1:
		/* compensate for start delay */
		stats->u.v1.runmsec -= stats->u.v1.delayms;
		while (stats->u.v1.runmsec < 0) {
			stats->u.v1.runsec--;
			stats->u.v1.runmsec += 1000;
		}

		log("Client %u Performance:", id);
		log("  runtime:                %d.%03d sec",
		    stats->u.v1.runsec, stats->u.v1.runmsec);
		log("  start delay:            %d.%03d sec",
		    stats->u.v1.delayms/1000, stats->u.v1.delayms%1000);
		seconds = stats->u.v1.runsec;
		if (seconds == 0)
			seconds = 1;
		log("  real data written:      %qu (%ld Bps)",
		    stats->u.v1.rbyteswritten,
		    (long)(stats->u.v1.rbyteswritten/seconds));
		log("  effective data written: %qu (%ld Bps)",
		    stats->u.v1.ebyteswritten,
		    (long)(stats->u.v1.ebyteswritten/seconds));
		log("Client %u Params:", id);
		log("  chunk/block size:     %d/%d",
		    CHUNKSIZE, BLOCKSIZE);
		log("  chunk buffers:        %d",
		    stats->u.v1.chunkbufs);
		log("  readahead/inprogress: %d/%d",
		    stats->u.v1.maxreadahead, stats->u.v1.maxinprogress);
		log("  recv timo/count:      %d/%d",
		    stats->u.v1.pkttimeout, stats->u.v1.idletimer);
		log("  re-request delay:     %d",
		    stats->u.v1.redodelay);
		log("  writer idle delay:    %d",
		    stats->u.v1.idledelay);
		log("  randomize requests:   %d",
		    stats->u.v1.randomize);
		log("Client %u Stats:", id);
		log("  net thread idle/blocked:        %d/%d",
		    stats->u.v1.recvidles, stats->u.v1.nofreechunks);
		log("  decompress thread idle/blocked: %d/%d",
		    stats->u.v1.nochunksready, stats->u.v1.decompidles);
		log("  disk thread idle:               %d",
		    stats->u.v1.writeridles);
		log("  join/request msgs:      %d/%d",
		    stats->u.v1.joinattempts, stats->u.v1.requests);
		log("  dupblocks(chunk done):  %d",
		    stats->u.v1.dupchunk);
		log("  dupblocks(in progress): %d",
		    stats->u.v1.dupblock);
		log("  lost blocks:            %d",
		    stats->u.v1.lostblocks);
		break;

	default:
		log("Unknown stats version %d", stats->version);
		break;
	}
}
#endif
