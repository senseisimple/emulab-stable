/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file canaryd.c
 *
 * Implementation file for the main bits of canaryd.
 */

#include <config.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/dkstat.h>
#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/stat.h>
#include <sys/rtprio.h>
#include <sys/socket.h>

#include <syslog.h>
#include <fcntl.h>
#include <devstat.h>
#include <dirent.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <net/route.h> /* def. of struct route */
#include <netinet/in.h>
#include <netinet/ip_fw.h>
#include <netinet/ip_dummynet.h>

#include <unistd.h>

#include "auxfuncs.h"
#include "canarydEvents.h"
#include "childProcess.h"
#include "networkInterface.h"

/**
 * The PATH to use when canaryd executes another program.
 */
#define CANARYD_PATH_ENV "/bin:/usr/bin:/sbin:/usr/sbin:" CLIENT_BINDIR

/**
 * The path to this program's PID file.
 */
#define PIDFILE "/var/run/canaryd.pid"

/**
 * The canaryd log file path.
 */
#define CANARYD_LOG "/var/emulab/logs/canaryd.log"

/**
 * Version information.
 *
 * @see version.c
 */
extern char build_info[];

enum {
    CDB_LOOPING,
    CDB_TRACE_IMMEDIATELY,
    CDB_TRACING,
    CDB_DEBUG,
};

/**
 * Flags for the canaryd_data.cd_Flags variable.
 */
enum {
    CDF_LOOPING = (1L << CDB_LOOPING),	/**< Continue Looping waiting for
					   events. */
     /**
      * Start tracing immediately, do not wait for START event.
      */
    CDF_TRACE_IMMEDIATELY = (1L << CDB_TRACE_IMMEDIATELY),
    CDF_TRACING = (1L << CDB_TRACING),	/**< Generate trace data. */
    CDF_DEBUG = (1L << CDB_DEBUG),	/**< Debugging mode. */
};

/*
 * Global data for canaryd.
 *
 * cd_Flags - Holds the CDF_ flags.
 * cd_Socket - Raw socket used for dummynet.
 * cd_IntervalTime - The time to wait between samples.
 * cd_CurrentTime - The current time of day.
 * cd_OutputFile - The output file for the trace.
 */
struct {
    int cd_Flags;
    int cd_Socket;
    struct itimerval cd_IntervalTime;
    struct timeval cd_CurrentTime;
    struct timeval cd_StopTime;
    FILE *cd_OutputFile;
} canaryd_data;

/**
 * Log an error.
 *
 * @param msgstr The message to log.
 */
void lerror(const char* msgstr)
{
    if( msgstr != NULL )
    {
	syslog(LOG_ERR, "%s: %m", msgstr);
	fprintf(stderr, "canaryd: %s: %s\n", msgstr, strerror(errno));
    }
}

/**
 * Log a warning.
 *
 * @param msgstr The message to log.
 */
void lwarn(const char* msgstr)
{
    if( msgstr != NULL )
    {
	syslog(LOG_WARNING, "%s", msgstr);
	fprintf(stderr, "canaryd: %s\n", msgstr);
    }
}

/**
 * Log a notice.
 *
 * @param msgstr The message to log.
 */
void lnotice(const char *msgstr)
{
    if( msgstr != NULL )
    {
	syslog(LOG_NOTICE, "%s", msgstr);
	printf("canaryd: %s\n", msgstr);
    }
}

/**
 * Empty signal handler for SIGALRM.  The mechanics are done in the main loop,
 * we just use SIGALRM to break the sigsuspend.
 */
void sigalrm(int signum)
{
}

/**
 * Signal handler that will cause the program to exit from the main loop.
 */
void sigexit(int signum)
{
    canaryd_data.cd_Flags &= ~CDF_LOOPING;
}

/**
 * Print the tool's usage message.
 *
 * @param file The file to send the usage to.
 * @param prog_name The program's real name, as given on the command line.
 */
static void cdUsage(FILE *file, char *prog_name)
{
    fprintf(file,
	    "Usage: %s [-hVdt]\n"
	    "\n"
	    "Physical node monitoring daemon.\n"
	    "\n"
	    "Options:\n"
	    "  -h\t\tThis help message.\n"
	    "  -V\t\tShow the version number.\n"
	    "  -d\t\tDebug mode; do not fork into the background.\n"
	    "  -t\t\tStart tracing immediately.\n",
	    prog_name);
}

/**
 * Process the command line options.
 *
 * @param argc The argc passed to main.
 * @param argv The argv passed to main.
 * @return Zero on success, one if there was an error and the usage message
 * should be printed, or -1 if there was no error but the tool should exit
 * immediately.
 */
static int cdProcessOptions(int argc, char *argv[])
{
    int ch, retval = 0;
    char *prog_name;

    prog_name = argv[0];
    while( ((ch = getopt(argc, argv, "hVdt")) != -1) && (retval == 0) )
    {
	switch( ch )
	{
	case 'd':
	    canaryd_data.cd_Flags |= CDF_DEBUG;
	    break;
	case 't':
	    canaryd_data.cd_Flags |= CDF_TRACE_IMMEDIATELY;
	    break;
	case 'V':
	    fprintf(stderr, "%s\n", build_info);
	    retval = -1;
	    break;
	case 'h':
	case '?':
	default:
	    retval = 1;
	    break;
	}
    }
    return( retval );
}

/**
 * Write the PID file for this program.
 *
 * @param path The path to the file.
 * @param pid The PID to store in the file.
 */
static int cdWritePIDFile(char *path, pid_t pid)
{
    int fd, retval = 1;

    if( (fd = open(path, O_EXCL | O_CREAT | O_WRONLY)) < 0 )
    {
	retval = 0;
    }
    else
    {
	char scratch[32];
	int rc;
	
	rc = snprintf(scratch, sizeof(scratch), "%d", pid);
	write(fd, scratch, rc);
	fchmod(fd, S_IRUSR | S_IRGRP | S_IROTH);
	close(fd);
	fd = -1;
    }
    return( retval );
}

/**
 * Start tracing the system's performance.
 *
 * @param path The trace file path.
 * @return True on success, false otherwise.
 */
static int cdStartTracing(char *path)
{
    int retval = 1;
    
    if( canaryd_data.cd_Flags & CDF_TRACING )
    {
	/* We're already tracing, nothing to do. */
    }
    else if( canaryd_data.cd_Flags & CDF_DEBUG )
    {
	/* Debug sends to stdout... */
	canaryd_data.cd_OutputFile = stdout;
	canaryd_data.cd_Flags |= CDF_TRACING;
    }
    else if( (canaryd_data.cd_OutputFile = fopen(path, "w")) == NULL )
    {
	lerror("Could not create trace file.");
	retval = 0;
    }
    else
    {
	/* Good to go. */
	canaryd_data.cd_Flags |= CDF_TRACING;
    }
    return( retval );
}

/**
 * Stop tracing the system's performance, if we haven't already.
 */
static void cdStopTracing(void)
{
    if( !(canaryd_data.cd_Flags & CDF_TRACING) )
    {
	/* We're already stopped, nothing to do. */
    }
    else
    {
	if( !(canaryd_data.cd_Flags & CDF_DEBUG) )
	{
	    fclose(canaryd_data.cd_OutputFile);
	}
	canaryd_data.cd_OutputFile = NULL;
	canaryd_data.cd_Flags &= ~CDF_TRACING;
    }
}

/**
 * Scan the "/proc" file system and gather statistics for all of the processes.
 *
 * @return True on success, false otherwise.
 */
static int cdScanProcFS(void)
{
    int retval = 1;
    DIR *pfs;

    if( (pfs = opendir("/proc")) == NULL )
    {
	lerror("Could not open /proc");
	retval = 0;
    }
    else
    {
	struct dirent *de;

	/* Walk the directories. */
	while( retval && ((de = readdir(pfs)) != NULL) )
	{
	    pid_t pid;

	    /*
	     * If the directory name is a number, assume its for a real
	     * process.
	     */
	    if( sscanf(de->d_name, "%d", &pid) == 1 )
	    {
		struct cpChildProcess *cp;

		/* Its a valid process, try to find/create it. */
		if( (cp = cpFindChildProcess(pid)) == NULL )
		{
		    cp = cpCreateChildProcess(pid);
		}
		if( cp == NULL )
		{
		    lerror("out of memory");
		    retval = 0;
		}
		else
		{
		    /*
		     * Increment the generation so the cpChildProcess object
		     * won't be garbage collected.
		     */
		    cp->cp_Generation += 1;
		    if( cp->cp_VNode != NULL )
		    {
			/* Its a vnode, sample its usage. */
			cpSampleUsage(cp);
		    }
		}
	    }
	}
	closedir(pfs);

	/* Do the garbage collection. */
	cpCollectChildProcesses();
    }
    return( retval );
}

/**
 * Print the statistics for the network interfaces.
 *
 * @param output_file The file to write the statistics to.
 * @return True on success, false otherwise.
 */
static int cdPrintNetworkInterfaces(FILE *output_file)
{
    struct ifaddrs *ifa;
    int retval = 1;
    
    if( getifaddrs(&ifa) < 0 )
    {
	lerror("getifaddrs failed");
	retval = 0;
    }
    else
    {
	struct ifaddrs *curr;

	/* Walk the list of interfaces. */
	curr = ifa;
	while( curr != NULL )
	{
	    struct if_data *id;
	    
	    if( (id = curr->ifa_data) != NULL ) // Link level only.
	    {
		char ifname[64];

		/* Print out the stats. */
		fprintf(output_file,
			" iface=%s,%ld,%ld,%ld,%ld,%ld",
			niFormatMACAddress(ifname, curr->ifa_addr),
			id->ifi_ipackets,
			id->ifi_ibytes,
			id->ifi_opackets,
			id->ifi_obytes,
			id->ifi_iqdrops);
	    }
	    curr = curr->ifa_next;
	}
	
	freeifaddrs(ifa);
	ifa = NULL;
    }
    return( retval );
}

static int cdPrintDummynetPipes(FILE *output_file)
{
    static int nalloc = 0, nbytes = sizeof(struct dn_pipe);
    static void *data = NULL;
    
    int retval = 1;

    if( (data != NULL) &&
	(getsockopt(canaryd_data.cd_Socket,
		    IPPROTO_IP,
		    IP_DUMMYNET_GET,
		    data,
		    &nbytes) == -1) )
    {
	lerror("getsockopt(IP_DUMMYNET_GET) 1");
	retval = 0;
    }
    else
    {
	while( (nbytes >= nalloc) && (retval == 1) )
	{
	    nalloc = nalloc * 2 + 200;
	    nbytes = nalloc;
	    if( (data = realloc(data, nbytes)) == NULL )
	    {
		lerror("Could not allocate memory.");
		retval = 0;
	    }
	    else if( getsockopt(canaryd_data.cd_Socket,
				IPPROTO_IP,
				IP_DUMMYNET_GET,
				data,
				&nbytes) == -1 )
	    {
		lerror("getsockopt(IP_DUMMYNET_GET)");
		retval = 0;
	    }
	}
    }

    if( retval )
    {
	struct dn_pipe *dp = (struct dn_pipe *)data;
	struct dn_flow_queue *dfq;
	void *next = data;

	for( ; nbytes >= sizeof(struct dn_pipe); dp = (struct dn_pipe *)next )
	{
	    if( dp->next != (struct dn_pipe *)DN_IS_PIPE )
	    {
		nbytes = 0;
	    }
	    else
	    {
		int len;

		len = sizeof(struct dn_pipe) +
		    (dp->fs.rq_elements * sizeof(struct dn_flow_queue));
		next = (void *)dp + len;
		nbytes -= len;

		dfq = (struct dn_flow_queue *)(dp + 1);
		fprintf(output_file,
			" pipe=%d,%d,%d,%d",
			dp->pipe_nr,
			dfq->len,
			dfq->len_bytes,
			dfq->drops);
	    }
	}
	
	nbytes = nalloc;
    }
    
    return( retval );
}

int main(int argc, char *argv[])
{
    int rc, retval = EXIT_SUCCESS;
    struct sigaction sa;
    struct rtprio rtp;
    sigset_t sigmask;

    /* Initialize our globals. */
    canaryd_data.cd_Flags |= CDF_LOOPING;

    /* Run for forever. */
    canaryd_data.cd_StopTime.tv_sec = LONG_MAX;
    canaryd_data.cd_StopTime.tv_usec = LONG_MAX;
    
    canaryd_data.cd_IntervalTime.it_interval.tv_sec = 1;
    canaryd_data.cd_IntervalTime.it_interval.tv_usec = 0;
    canaryd_data.cd_IntervalTime.it_value.tv_sec = 1;
    canaryd_data.cd_IntervalTime.it_value.tv_usec = 0;

    /* Prime the sigaction for capturing SIGALRM. */
    sigemptyset(&sigmask);
    sigaddset(&sigmask, SIGALRM);
    sigaddset(&sigmask, SIGINT);
    sigaddset(&sigmask, SIGTERM);
    
    sa.sa_mask = sigmask;
    sa.sa_flags = 0;
#if defined(SA_RESTART)
    sa.sa_flags |= SA_RESTART;
#endif

    sa.sa_handler = sigalrm;
    sigaction(SIGALRM, &sa, NULL);

    sa.sa_handler = sigexit;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    rtp.type = RTP_PRIO_REALTIME;
    rtp.prio = 0;

    getdrivedata((char *[]){ "ad0", NULL });
    
    openlog("canaryd", LOG_NDELAY, LOG_TESTBED);
    
    setenv("PATH", CANARYD_PATH_ENV, 1);
    
    switch( cdProcessOptions(argc, argv) )
    {
    case 1:
	/* There was an error processing the arguments. */
	cdUsage(stderr, argv[0]);
	retval = EXIT_FAILURE;
	break;
    case -1:
	/* No errors, but nothing else to do either. */
	break;
    case 0:
	if( (canaryd_data.cd_Socket =
	     socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) == -1 )
	{
	    lerror("Could not allocate raw socket.");
	    retval = EXIT_FAILURE;
	}
	/* Optionally daemonize, */
	else if( !(canaryd_data.cd_Flags & CDF_DEBUG) &&
		 ((rc = daemon(0, 0)) < 0) )
	{
	    lerror("Could not daemonize.");
	    retval = EXIT_FAILURE;
	}
	/* ... dump our PID file, */
	else if( !(canaryd_data.cd_Flags & CDF_DEBUG) &&
		 !cdWritePIDFile(PIDFILE, getpid()) )
	{
	    lerror("Could not write PID file.");
	    retval = EXIT_FAILURE;
	}
	/* ... optionally start tracing immediately, */
	else if( (canaryd_data.cd_Flags & CDF_TRACE_IMMEDIATELY) &&
		 !cdStartTracing(CANARYD_LOG) )
	{
	    retval = EXIT_FAILURE;
	}
	/* ... bump our priority, */
	else if( rtprio(RTP_SET, getpid(), &rtp) < 0 )
	{
	    lerror("Could not set real-time priority!");
	    retval = EXIT_FAILURE;
	}
	/* ... initialize internals, */
	else if( !cpInitChildProcessData() )
	{
	    lerror("Could not initialize internal state");
	    retval = EXIT_FAILURE;
	}
	/* ... connect to the event system, */
	else if( !ceInitCanarydEvents("elvin://localhost") )
	{
	    lerror("Could not initialize events");
	    retval = EXIT_FAILURE;
	}
	/* ... setup interval timer, and */
	else if( setitimer(ITIMER_REAL,
			   &canaryd_data.cd_IntervalTime,
			   NULL) < 0 )
	{
	    lerror("Could not register itimer.");
	    retval = EXIT_FAILURE;
	}
	/* ... start looping. */
	else
	{
	    int pagesize, mempages;
	    double physmem;
	    sigset_t ss;

	    pagesize = getpagesize();
	    mempages = getmempages();
	    physmem = pagesize * mempages;
	    sigfillset(&ss);
	    sigdelset(&ss, SIGALRM);
	    sigdelset(&ss, SIGINT);
	    sigdelset(&ss, SIGTERM);
	    /* Main loop, waits for events and optionally prints stats. */
	    while( canaryd_data.cd_Flags & CDF_LOOPING )
	    {
		gettimeofday(&canaryd_data.cd_CurrentTime, NULL);

		/* Make sure we're not past the stop time and */
		if( timercmp(&canaryd_data.cd_CurrentTime,
			     &canaryd_data.cd_StopTime,
			     >) )
		{
		    cdStopTracing();
		}
		/* ... try outputting trace data. */
		else if( canaryd_data.cd_Flags & CDF_TRACING )
		{
		    int *cpu_pct, *mem_busy, *netmem_busy;
		    struct cpVNode *vn;

		    /* We're tracing now, update stats, and */
		    cpu_pct = getcpustates();
		    
		    mem_busy = getmembusy(mempages);

		    netmem_busy = getnetmembusy();
		    
		    cdScanProcFS();

		    /* ... print them out. */
		    fprintf(canaryd_data.cd_OutputFile,
			    "vers=2 stamp=%ld,%ld cpu=%f intr=%f mem=%d,%d "
			    "disk=%u netmem=%u,%u,%u",
			    canaryd_data.cd_CurrentTime.tv_sec,
			    canaryd_data.cd_CurrentTime.tv_usec,
			    (1000.0 - cpu_pct[CP_IDLE]) / 10.0,
			    cpu_pct[CP_INTR] / 10.0,
			    mem_busy[1],
			    mem_busy[0],
			    getdiskbusy(),
			    netmem_busy[0],
			    netmem_busy[1],
			    netmem_busy[2]);
		    
		    cdPrintNetworkInterfaces(canaryd_data.cd_OutputFile);
		    cdPrintDummynetPipes(canaryd_data.cd_OutputFile);

		    /*
		     * Dump the vnodes list and clear their counters for the
		     * next round.
		     */
		    vn = child_process_data.cpd_VNodes;
		    while( vn != NULL )
		    {
			double cpu_pct, mem_pct;
			
			cpu_pct = (vn->vn_Usage / 1000000.0) * 100.0;
			mem_pct = ((double)vn->vn_MemoryUsage / physmem) *
			    100.0;
			fprintf(canaryd_data.cd_OutputFile,
				" vnode=%s,%f,%f",
				vn->vn_Name,
				cpu_pct,
				mem_pct);
			vn->vn_Usage = 0;
			vn->vn_MemoryUsage = 0;
			vn = vn->vn_Next;
		    }
		    
		    fprintf(canaryd_data.cd_OutputFile, "\n");
		    fflush(canaryd_data.cd_OutputFile);
		}

		/* Check for events and */
		event_poll(canaryd_events_data.ced_Handle);
		
		/* ... wait for the next signal. */
		if( canaryd_data.cd_Flags & CDF_LOOPING )
		{
		    sigsuspend(&ss);
		    /*
		     * XXX We should probably make sure it was a SIGALRM that
		     * woke us up, but eh...
		     */
		}
	    }
	}

	/* Clean up our mess. */
	cdStopTracing();
	if( !(canaryd_data.cd_Flags & CDF_DEBUG) )
	{
	    unlink(PIDFILE);
	}
	break;
    }
    return( retval );
}

enum {
  SITEIDX = 0,
  EXPTIDX,
  GROUPIDX,
  HOSTIDX,
  OBJTYPEIDX,
  OBJNAMEIDX,
  EVTYPEIDX,
  ARGSIDX,

  MAXIDX
};

void ceEventCallback(event_handle_t handle,
		     event_notification_t en, 
		     void *data)
{
    char buf[MAXIDX][128]; /* XXX: bad magic */
    int len = 128;
    struct timeval now;
    static int ecount;
    
    bzero(buf, sizeof(buf));
    
    event_notification_get_site(handle, en, buf[SITEIDX], len);
    event_notification_get_expt(handle, en, buf[EXPTIDX], len);
    event_notification_get_group(handle, en, buf[GROUPIDX], len);
    event_notification_get_host(handle, en, buf[HOSTIDX], len);
    event_notification_get_objtype(handle, en, buf[OBJTYPEIDX], len);
    event_notification_get_objname(handle, en, buf[OBJNAMEIDX], len);
    event_notification_get_eventtype(handle, en, buf[EVTYPEIDX], len);
    event_notification_get_arguments(handle, en, buf[ARGSIDX], len);
    
    if( canaryd_data.cd_Flags & CDF_DEBUG )
    {
	gettimeofday(&now, NULL);
	fprintf(stderr,"Event %d: %lu.%03lu %s %s %s %s %s %s %s %s\n",
		++ecount, now.tv_sec, now.tv_usec / 1000,
		buf[SITEIDX], buf[EXPTIDX], buf[GROUPIDX],
		buf[HOSTIDX], buf[OBJTYPEIDX], buf[OBJNAMEIDX], 
		buf[EVTYPEIDX], buf[ARGSIDX]);
    }
    
    if( strcmp(buf[EVTYPEIDX], TBDB_EVENTTYPE_START) == 0 )
    {
	char *pair, *cursor = buf[ARGSIDX];

	/* XXX Move this initialization elsewhere. */
	canaryd_data.cd_StopTime.tv_sec = LONG_MAX;
	canaryd_data.cd_StopTime.tv_usec = LONG_MAX;

	/* Parse the arguments and */
	while( (pair = strsep(&cursor, " \t")) != NULL )
	{
	    char *key_end;

	    if( strlen(pair) == 0 )
	    {
		/* Empty argument, no reason to complain. */
	    }
	    else if( (key_end = strchr(pair, '=')) == NULL )
	    {
		/* Poorly formed argument, complain. */
		lwarn("Bad START event argument");
	    }
	    else if( strncasecmp(pair, "duration", (key_end - pair)) == 0 )
	    {
		struct timeval duration;

		duration.tv_usec = 0;
		if( sscanf(key_end + 1, "%ld", &duration.tv_sec) == 0 )
		{
		    lwarn("Duration is not an integer.");
		}
		else
		{
		    timeradd(&canaryd_data.cd_CurrentTime,
			     &duration,
			     &canaryd_data.cd_StopTime);
		}
	    }
	    else
	    {
		/* Unknown key... */
		lwarn("Unknown key passed to START event");
	    }
	}

	/* ... start tracing again. */
	cdStartTracing(CANARYD_LOG); // XXX Let the user specify the log file.
    }
    else if( strcmp(buf[EVTYPEIDX], TBDB_EVENTTYPE_STOP) == 0 )
    {
	cdStopTracing();
    }
    else if( strcmp(buf[EVTYPEIDX], TBDB_EVENTTYPE_RESET) == 0 )
    {
	/* set_default_overload_params(); */
    }
    else if( strcmp(buf[EVTYPEIDX], TBDB_EVENTTYPE_REPORT) == 0 )
    {
    }
    else
    {
	lnotice("Unknown event type received.");
    }
}
