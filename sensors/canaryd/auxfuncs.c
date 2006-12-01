/*
 * This file has a bit of a checkered past.
 *
 * Parts were inspired by the FreeBSD ganglia module:
 *	ganglia-3.0.1/srclib/libmetrics/freebsd/metrics.c
 * Parts were lifted from FreeBSD vmstat:
 *	src/usr.bin/vmstat/vmstat.c
 * Parts were lifted from FreeBSD top:
 *	src/usr.bin/top/machine.c
 */

/*
 * Ganglia notice:
 *
 *  First stab at support for metrics in FreeBSD
 *  by Preston Smith <psmith@physics.purdue.edu>
 *  Wed Feb 27 14:55:33 EST 2002
 *  Improved by Brooks Davis <brooks@one-eyed-alien.net>,
 *  Fixed libkvm code.
 *  Tue Jul 15 16:42:22 EST 2003
 *
 * $Id: auxfuncs.c,v 1.3 2006-12-01 19:06:04 mike Exp $
 */

/*
 * vmstat notice:
 *
 * Copyright (c) 1980, 1986, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * top notice:
 *
 * top - a top users display for Unix
 *
 * SYNOPSIS:  For FreeBSD-2.x and later
 *
 * DESCRIPTION:
 * Originally written for BSD4.4 system by Christos Zoulas.
 * Ported to FreeBSD 2.x by Steven Wallace && Wolfram Schneider
 * Order support hacked in from top-3.5beta6/machine/m_aix41.c
 *   by Monte Mitzelfelt (for latest top see http://www.groupsys.com/topinfo/)
 *
 * This is the machine-dependent module for FreeBSD 2.2
 * Works for:
 *	FreeBSD 2.2.x, 3.x, 4.x, and probably FreeBSD 2.1.x
 *
 * LIBS: -lkvm
 *
 * AUTHOR:  Christos Zoulas <christos@ee.cornell.edu>
 *          Steven Wallace  <swallace@freebsd.org>
 *          Wolfram Schneider <wosch@FreeBSD.org>
 *
 * $FreeBSD: src/usr.bin/top/machine.c,v 1.29.2.2 2001/07/31 20:27:05 tmm Exp $
 */

#include <sys/param.h>
#include <sys/wait.h>
#include <sys/dkstat.h>
#include <sys/mbuf.h>
#include <sys/sysctl.h>
#include <sys/vmmeter.h>
#include <vm/vm_param.h>

#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <devstat.h>

#include "auxfuncs.h"

int num_cpus;
int num_devices, maxshowdevs;
long generation;
struct device_selection *dev_select;
int num_selected;
struct devstat_match *matches;
int num_matches = 0;
int num_devices_specified, num_selections;
long select_generation;
char **specified_devices;
devstat_select_mode select_mode;
struct statinfo cur, last;

extern void lerror(const char* msgstr);

static int getswapouts(void);
static long percentages(int cnt, int *out, register long *new,
                        register long *old, long *diffs);

#define MAXLINELEN 256

/* XXX change to combine last return value of procfunc with exec'ed process'
   exit status & write macros for access.
*/
int procpipe(char *const prog[], int (procfunc)(char*,void*), void* data) {

  int fdes[2], retcode, cpid, status;
  char buf[MAXLINELEN];
  FILE *in;

  if ((retcode=pipe(fdes)) < 0) {
    lerror("Couldn't alloc pipe");
  }
  
  else {
    switch ((cpid = fork())) {
    case 0:
      close(fdes[0]);
      dup2(fdes[1], STDOUT_FILENO);
      if (execvp(prog[0], prog) < 0) {
        lerror("Couldn't exec program");
        exit(1);
      }
      break;
      
    case -1:
      lerror("Error forking child process");
      close(fdes[0]);
      close(fdes[1]);
      retcode = -1;
      break;
      
    default:
      close(fdes[1]);
      in = fdopen(fdes[0], "r");
      while (!feof(in) && !ferror(in)) {
        if (fgets(buf, sizeof(buf), in)) {
          if ((retcode = procfunc(buf, data)) < 0)  break;
        }
      }
      fclose(in);
      wait(&status);
      if (retcode > -1)  retcode = WEXITSTATUS(status);
      break;
    } /* switch ((cpid = fork())) */
  }
  return retcode;
}

int
getnumcpus(void)
{
	unsigned ncpus;
	size_t len = sizeof(ncpus);

	if (sysctlbyname("hw.ncpu", &ncpus, &len, NULL, 0) == -1)
		return 0;

	return ncpus;
}

int
getcpuspeed(void)
{
	unsigned cpuspeed;
	size_t len = sizeof(cpuspeed);

	if (sysctlbyname("machdep.tsc_freq", &cpuspeed, &len, NULL, 0) == -1)
		return 0;

	return (int)(cpuspeed / 1000000);
}

int
getmempages(void)
{
	size_t len;
	unsigned total;
	int mib[2];
	static int pagesize;

	if (pagesize == 0)
		pagesize = getpagesize();

	mib[0] = CTL_HW;
	mib[1] = HW_PHYSMEM;
	len = sizeof (total);
	sysctl(mib, 2, &total, &len, NULL, 0);
	return (int)(total / pagesize);
}

int *
getmembusy(unsigned totalpages)
{
	size_t len;
	struct vmtotal data;
	int mib[2], sos, busy;
        static int res[2];


	mib[0] = CTL_VM;
	mib[1] = VM_METER;
	len = sizeof (data);
	if (sysctl(mib, 2, &data, &len, NULL, 0) < 0)
		err(1, "sysctl(CTL_VM, VM_METER)");
	sos = getswapouts();
	if (totalpages == 0) totalpages = 1;
	busy = 100 - (data.t_free*100 + totalpages/2) / totalpages;

        res[0] = sos;
        res[1] = busy;
	return res;
}

int *
getnetmembusy(void)
{
	size_t len;
	struct mbstat mbstat;
	unsigned pct;
	int mib[3], waits, drops;
	static int nmbclusters, lastwait, lastdrops;
        static unsigned int res[3];

	mib[0] = CTL_KERN;
	mib[1] = KERN_IPC;

	if (nmbclusters == 0) {
		mib[2] = KIPC_NMBCLUSTERS;
		len = sizeof(nmbclusters);
		if (sysctl(mib, 3, &nmbclusters, &len, 0, 0) < 0)
			err(1, "sysctl(CTL_KERN, KERN_IPC, KIPC_NMBCLUSTERS)");
		if (nmbclusters == 0) nmbclusters = 1;
	}

	mib[2] = KIPC_MBSTAT;
	len = sizeof(mbstat);
	if (sysctl(mib, 3, &mbstat, &len, 0, 0) < 0)
		err(1, "sysctl(CTL_KERN, KERN_IPC, KIPC_MBSTAT)");

	/*
	 * What do we really want here?
	 * (m_clusters - m_clfree) is what is in use
	 * m_clusters is what is allocated
	 *	(clusters are never returned to the system)
	 * nmbclusters is the max possible memory that could be used
	 * m_wait is # of calls blocked waiting for memory
	 * m_drops is # of calls that failed
	 *	(map full on non-blocking call failed)
	 */
	pct = ((mbstat.m_clusters-mbstat.m_clfree)*100 + nmbclusters/2) /
		nmbclusters;
	waits = mbstat.m_wait - lastwait;
	lastwait = mbstat.m_wait;
	drops = mbstat.m_drops - lastdrops;
	lastdrops = mbstat.m_drops;

        res[0] = pct;
        res[1] = waits;
        res[2] = drops;
        return res;
}

int
getswapouts(void)
{
 	int count, tmp;
	size_t len = sizeof(count);
	static int last;

	if (sysctlbyname("vm.stats.vm.v_swapout", &count, &len, NULL, 0) < 0 ||
	    len == 0)
		return -1;
	tmp = count - last;
	last = count;
	return tmp;
}

/*
 * Slurped from vmstat:
 * $FreeBSD: src/usr.bin/vmstat/vmstat.c,v 1.38.2.5 2003/09/20 19:10:01 bms Exp $
 */
char **
getdrivedata(argv)
	char **argv;
{
	if ((num_devices = getnumdevs()) < 0)
		errx(1, "%s", devstat_errbuf);

	cur.dinfo = (struct devinfo *)malloc(sizeof(struct devinfo));
	last.dinfo = (struct devinfo *)malloc(sizeof(struct devinfo));
	bzero(cur.dinfo, sizeof(struct devinfo));
	bzero(last.dinfo, sizeof(struct devinfo));

	if (getdevs(&cur) == -1)
		errx(1, "%s", devstat_errbuf);

	num_devices = cur.dinfo->numdevs;
	generation = cur.dinfo->generation;

	specified_devices = (char **)malloc(sizeof(char *));
	for (num_devices_specified = 0; *argv; ++argv) {
		if (isdigit(**argv))
			break;
		num_devices_specified++;
		specified_devices = (char **)realloc(specified_devices,
						     sizeof(char *) *
						     num_devices_specified);
		specified_devices[num_devices_specified - 1] = *argv;
	}
	dev_select = NULL;

	if (maxshowdevs < num_devices_specified)
		maxshowdevs = num_devices_specified;

	/*
	 * People are generally only interested in disk statistics when
	 * they're running vmstat.  So, that's what we're going to give
	 * them if they don't specify anything by default.  We'll also give
	 * them any other random devices in the system so that we get to
	 * maxshowdevs devices, if that many devices exist.  If the user
	 * specifies devices on the command line, either through a pattern
	 * match or by naming them explicitly, we will give the user only
	 * those devices.
	 */
	if ((num_devices_specified == 0) && (num_matches == 0)) {
		if (buildmatch("da", &matches, &num_matches) != 0)
			errx(1, "%s", devstat_errbuf);

		select_mode = DS_SELECT_ADD;
	} else
		select_mode = DS_SELECT_ONLY;

	/*
	 * At this point, selectdevs will almost surely indicate that the
	 * device list has changed, so we don't look for return values of 0
	 * or 1.  If we get back -1, though, there is an error.
	 */
	if (selectdevs(&dev_select, &num_selected, &num_selections,
		       &select_generation, generation, cur.dinfo->devices,
		       num_devices, matches, num_matches, specified_devices,
		       num_devices_specified, select_mode,
		       maxshowdevs, 0) == -1)
		errx(1, "%s", devstat_errbuf);

	return(argv);
}

int
getdiskbusy(void)
{
	int dn;
        int retval = 0;
	long double busy_seconds, device_busy = 0;
	struct devinfo *tmp_dinfo;
	
	tmp_dinfo = last.dinfo;
	last.dinfo = cur.dinfo;
	cur.dinfo = tmp_dinfo;
	last.busy_time = cur.busy_time;
	getdevs(&cur);

	busy_seconds = compute_etime(cur.busy_time, last.busy_time);

        /* XXX: just grabbing the first one for now */
	for (dn = 0; dn < num_devices; dn++) {
		int di;

		if ((dev_select[dn].selected == 0)
		 || (dev_select[dn].selected > maxshowdevs))
			continue;

		di = dev_select[dn].position;

		device_busy = compute_etime(cur.dinfo->devices[di].busy_time,
					    last.dinfo->devices[di].busy_time);
		if (device_busy > busy_seconds)
			device_busy = busy_seconds;
		device_busy = (device_busy * 100) / busy_seconds;
		retval = (int) device_busy;
        }

        return retval;
}

/*
 * Yoinked from ganglia gmond probe for freebsd
 */

/* Get the CPU state given by index, from kern.cp_time
 * Use the constants in <sys/dkstat.h>
 * CP_USER=0, CP_NICE=1, CP_SYS=2, CP_INTR=3, CP_IDLE=4
 */
int *getcpustates() {

   static long cp_time[CPUSTATES];
   static long cp_old[CPUSTATES];
   static long cp_diff[CPUSTATES];
   static int cpu_states[CPUSTATES];
   size_t len = sizeof(cp_time);
   
   /* Copy the last cp_time into cp_old */
   memcpy(&cp_old, &cp_time, CPUSTATES*sizeof(long));
   /* puts kern.cp_time array into cp_time */
   if (sysctlbyname("kern.cp_time", &cp_time, &len, NULL, 0) == -1 || !len)
	return 0;
   /* Use percentages function lifted from top(1) to figure percentages */
   percentages(CPUSTATES, cpu_states, cp_time, cp_old, cp_diff);

   return cpu_states;
}

/*
 * Function to get cpu percentages.
 * Might be changed ever so slightly, but is still mostly:
 * AUTHOR:  Christos Zoulas <christos@ee.cornell.edu>
 *          Steven Wallace  <swallace@freebsd.org>
 *          Wolfram Schneider <wosch@FreeBSD.org>
 *
 * $FreeBSD: src/usr.bin/top/machine.c,v 1.29.2.2 2001/07/31 20:27:05 tmm Exp $
 */

static long percentages(int cnt, int *out, register long *new, 
                        register long *old, long *diffs)  {

    register int i;
    register long change;
    register long total_change;
    register long *dp;
    long half_total;

    /* initialization */
    total_change = 0;
    dp = diffs;

    /* calculate changes for each state and the overall change */
    for (i = 0; i < cnt; i++) {
        if ((change = *new - *old) < 0) {
            /* this only happens when the counter wraps */
            change = (int)
                ((unsigned long)*new-(unsigned long)*old);
        }
        total_change += (*dp++ = change);
        *old++ = *new++;
    }
    /* avoid divide by zero potential */
    if (total_change == 0) { total_change = 1; }

    /* calculate percentages based on overall change, rounding up */
    half_total = total_change / 2l;

    /* Do not divide by 0. Causes Floating point exception */
    if(total_change) {
        for (i = 0; i < cnt; i++) {
          *out++ = (int)((*diffs++ * 1000 + half_total) / total_change);
        }
    }

    /* return the total in case the caller wants to use it */
    return(total_change);
}
