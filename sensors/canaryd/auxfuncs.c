#include <sys/param.h>
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

static int getswapouts(void);
static int cpu_state(int which);
static long percentages(int cnt, int *out, register long *new,
                        register long *old, long *diffs);

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

int
getcpubusy(void)
{
	return (1000 - cpu_state(CP_IDLE)) / 10;
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
static
int cpu_state(int which) {

   static long cp_time[CPUSTATES];
   static long cp_old[CPUSTATES];
   static long cp_diff[CPUSTATES];
   static int cpu_states[CPUSTATES];
   static long tot;
   size_t len = sizeof(cp_time);

   /* Copy the last cp_time into cp_old */
   memcpy(&cp_old, &cp_time, CPUSTATES*sizeof(long));
   /* puts kern.cp_time array into cp_time */
   if (sysctlbyname("kern.cp_time", &cp_time, &len, NULL, 0) == -1 || !len)
	return 0;
   /* Use percentages function lifted from top(1) to figure percentages */
   tot = percentages(CPUSTATES, cpu_states, cp_time, cp_old, cp_diff);

   return cpu_states[which];
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
