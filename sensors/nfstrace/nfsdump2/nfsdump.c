/*
 * Copyright (c) 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static const char copyright[] =
    "@(#) Copyright (c) 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997\n\
The Regents of the University of California.  All rights reserved.\n";
static const char rcsid[] =
    "@(#) $Header: /home/cvs_mirrors/cvs-public.flux.utah.edu/CVS/testbed/sensors/nfstrace/nfsdump2/nfsdump.c,v 1.2 2006-02-02 16:16:17 stack Exp $ (LBL)";
#endif

/*
 * nfsdump - monitor nfs traffic on an ethernet.
 *
 * First written (as tcpdump) in 1987 by Van Jacobson, Lawrence
 * Berkeley Laboratory.  Mercilessly hacked and occasionally improved
 * since then via the combined efforts of Van, Steve McCanne and Craig
 * Leres of LBL.
 *
 * Gutted even more mercilessly and changed into nfsdump in 2001 by
 * Dan Ellard, Harvard University.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include <sys/types.h>
#include <sys/time.h>

#include <netinet/in.h>

#include <pcap.h>
#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>


#include "interface.h"
#include "machdep.h"
#include "setsignal.h"
#include "gmt2local.h"

#include "mypcap.h"
#include "nfsrecord.h"

int Oflag = 1;			/* run filter code optimizer */
int pflag;			/* don't go promiscuous */
int sflag = 0;			/* use the libsmi to translate OIDs */
int tflag = 1;			/* print packet arrival time */
int uflag = 0;			/* Print undecoded NFS handles */
int vflag;			/* verbose */


char	*quickSummaryFile	= NULL;

char *program_name;

int32_t thiszone;		/* seconds offset from gmt to local time */

typedef	struct	{
	int active;
	unsigned int secs;
	unsigned int iters;
    	unsigned int curriter;
	unsigned int end_time;
	char *basename;
	char *tmpname;
	char *cur_sumname;
	char *cur_logname;
} intervalDesc_t;

static intervalDesc_t	Interval	= {
	0,		/* Not active */
	120,		/* 2 minutes &&& FOR DEBUGGING ONLY */
	1,		/* One iteration */
	0,
	0,		/* unknown start_time */
	"output",	/* stupid default output basename */
	"TEMP-LOG",	/* something different from the output basename */
	NULL,		/* no summary by default */
	NULL
};

static long		EndOfLife	= -1;

/* Forwards */
static RETSIGTYPE cleanup(int);
static void usage(void) __attribute__((noreturn));
static int updateOutFile (intervalDesc_t *i);
static int printSummary (char *filename);

/* Length of saved portion of packet. */

/* Ignoring the default, we choose a ridiculously large snaplen */
int snaplen = 1500;
int parallel = 0;

struct printer {
	pcap_handler f;
	int type;
};

void packetPrinter (u_char *user, const struct pcap_pkthdr *h, const u_char *pp);

static struct printer printers[] = {
/* 	{ ether_if_print,	DLT_EN10MB }, */
	{ packetPrinter,	DLT_EN10MB },
	{ packetPrinter,	DLT_IEEE802 },	/* Needed on some hosts. */
	{ NULL,			0 },
};

static pcap_handler
lookup_printer(int type)
{
	struct printer *p;

	for (p = printers; p->f; ++p)
		if (type == p->type)
			return p->f;

	error("unknown data link type %d", type);
	/* NOTREACHED */
}

static pcap_t *pd;

extern int optind;
extern int opterr;
extern char *optarg;

int
main(int argc, char **argv)
{
	register int cnt, op, i;
	bpf_u_int32 localnet, netmask;
	register char *cp, *infile, *cmdbuf, *device, *RFileName, *WFileName;
	pcap_handler printer;
	struct bpf_program fcode;
	RETSIGTYPE (*oldhandler)(int);
	u_char *pcap_userdata;
	char ebuf[PCAP_ERRBUF_SIZE];

	cnt = -1;
	device = NULL;
	infile = NULL;
	RFileName = NULL;
	WFileName = NULL;
	if ((cp = strrchr(argv[0], '/')) != NULL)
		program_name = cp + 1;
	else
		program_name = argv[0];

	if (abort_on_misalignment(ebuf, sizeof(ebuf)) < 0)
		error("%s", ebuf);

#ifdef LIBSMI
	smiInit("nfsdump");
#endif
	
	opterr = 0;
	while (
	    (op = getopt(argc, argv, "B:c:F:I:i:L:lm:nN:Opq:r:s:T:tuvw:P")) != -1)
		switch (op) {

		case 'c':
			cnt = atoi(optarg);
			if (cnt <= 0)
				error("invalid packet count %s", optarg);
			break;

		case 'F':
			infile = optarg;
			break;

		case 'i':
			device = optarg;
			break;

		case 'N':
			Interval.active = 1;
			Interval.iters = atoi (optarg);
			if (Interval.iters < 0) {
				fprintf (stderr, "Iterations must be >= 0.\n");
				exit (1);
			}
			break;

			/* The interval is specified on the
			 * commandline in minutes, and converted
			 * immediately to seconds-- although the
			 * actual internal interval is given in
			 * seconds, some other parts of the system get
			 * confused if the interval isn't a round
			 * number of minutes.
			 */

		case 'I':
			Interval.active = 1;
			Interval.secs = 60 * atoi (optarg);
			if (Interval.secs <= 0) {
				fprintf (stderr, "Interval minutes must be > 0.\n");
				exit (1);
			}
			break;

			/*
			 * Specify the tmpname, the temporary file to
			 * use if "interval" logging is turned on.
			 */

		case 'T':
			Interval.active = 1;
			Interval.tmpname = optarg;
			break;

		case 'B':
			Interval.active = 1;
			Interval.basename = optarg;
			break;

		case 'l':
#ifdef HAVE_SETLINEBUF
			setlinebuf(stdout);
#else
			setvbuf(stdout, NULL, _IOLBF, 0);
#endif
			break;

		case 'L': {
			struct timeval now;
			long lifetime = atol (optarg);

			gettimeofday (&now, NULL);

			EndOfLife = now.tv_sec + lifetime;

			printf ("now = %d, eol = %d\n", now.tv_sec,
					EndOfLife);
			break;
		}

		case 'm':
#ifdef LIBSMI
		        if (smiLoadModule(optarg) == 0) {
				error("could not load MIB module %s", optarg);
		        }
			sflag = 1;
#else
			(void)fprintf(stderr, "%s: ignoring option `-m %s' ",
				      program_name, optarg);
			(void)fprintf(stderr, "(no libsmi support)\n");
#endif
			
		case 'O':
			Oflag = 0;
			break;

		case 'p':
			++pflag;
			break;

		case 'r':
			RFileName = optarg;
			break;

		case 's': {
			char *end;

			snaplen = strtol(optarg, &end, 0);
			if (optarg == end || *end != '\0'
			    || snaplen < 0 || snaplen > 65535)
				error("invalid snaplen %s", optarg);
			else if (snaplen == 0)
				snaplen = 65535;
			break;
		}

		case 't':
			--tflag;
			break;

		case 'u':
			++uflag;
			break;
			
		case 'v':
			++vflag;
			break;

		case 'w':
			WFileName = optarg;
			break;

		case 'q':
			quickSummaryFile = optarg;
			break;

		case 'P':
			parallel = 1;
			break;

		default:
			usage();
			/* NOTREACHED */
		}

	if (tflag > 0)
		thiszone = gmt2local(0);

	nfs_v3_stat_init (&v3statsBlock);
	nfs_v2_stat_init (&v2statsBlock);

	if (((readTable = ptCreateTable()) == NULL) ||
	    ((writeTable = ptCreateTable()) == NULL)) {
	    error("cannot allocate packet tables");
	}

	if (RFileName != NULL) {
		/*
		 * We don't need network access, so set it back to the user id.
		 * Also, this prevents the user from reading anyone's
		 * trace file.
		 */
		setuid(getuid());

		pd = pcap_open_offline(RFileName, ebuf);
		if (pd == NULL)
			error("%s", ebuf);
		localnet = 0;
		netmask = 0;
	} else {
		if (device == NULL) {
			device = pcap_lookupdev(ebuf);
			if (device == NULL)
				error("%s", ebuf);
		}
/* 		pd = pcap_open_live(device, snaplen, !pflag, 1000, ebuf); */
		pd = pcap_open_live(device, snaplen, !pflag, 50, ebuf);
		if (pd == NULL)
			error("%s", ebuf);
		i = pcap_snapshot(pd);
		if (snaplen < i) {
			warning("snaplen raised from %d to %d", snaplen, i);
			snaplen = i;
		}
		if (pcap_lookupnet(device, &localnet, &netmask, ebuf) < 0) {
			localnet = 0;
			netmask = 0;
			warning("%s", ebuf);
		}
		/*
		 * Let user own process after socket has been opened.
		 */
		setuid(getuid());
	}
	if (infile)
		cmdbuf = read_infile(infile);
	else
		cmdbuf = copy_argv(&argv[optind]);

	if (pcap_compile(pd, &fcode, cmdbuf, Oflag, netmask) < 0) {
		error("%s", pcap_geterr(pd));
	}

	(void)setsignal(SIGTERM, cleanup);
	(void)setsignal(SIGINT, cleanup);
	/* Cooperate with nohup(1) */
	if ((oldhandler = setsignal(SIGHUP, cleanup)) != SIG_DFL)
		(void)setsignal(SIGHUP, oldhandler);

	if (pcap_setfilter(pd, &fcode) < 0)
		error("%s", pcap_geterr(pd));
	if (WFileName) {
		pcap_dumper_t *p = pcap_dump_open(pd, WFileName);
		if (p == NULL)
			error("%s", pcap_geterr(pd));
		printer = pcap_dump;
		pcap_userdata = (u_char *)p;
	} else {
		printer = lookup_printer(pcap_datalink(pd));
		pcap_userdata = 0;
	}
	if (RFileName == NULL) {
		(void)fprintf(stderr, "%s: listening on %s\n",
		    program_name, device);
		(void)fflush(stderr);
	}
	if ((EndOfLife < 0) && !Interval.active) {
		if (pcap_loop(pd, cnt, printer, pcap_userdata) < 0) {
			(void)fprintf(stderr, "%s: pcap_loop: %s\n",
			    program_name, pcap_geterr(pd));
			exit(1);
		}
	}
	else {
		int total_cnt = 0;
		struct timeval now;

		if (parallel) {
			mypcap_init(pd, printer);
		}
		
		/*
		 * This definition assumes that packets are frequent,
		 * so we won't have to wait very long at all to see
		 * PACKET_POLL_CNT packets.  If the system is not very
		 * busy (or bursty) then consider lowering this value. 
		 * At worst, the most conservative thing to do is to
		 * lower it all the way to 1.
		 */

#define	PACKET_POLL_CNT	200

		for (;;) {
			if ((cnt > 0) && (total_cnt >= cnt)) {
				break;
			}
			gettimeofday (&now, NULL);

			if ((EndOfLife > 0) && (now.tv_sec >= EndOfLife)) {

				/* If we're doing intervals, we need
				 * to push the last output file out,
				 * and rename it.
				 */

				if (Interval.active) {
					updateOutFile (&Interval);
				}

				break;
			}

			/*
			 * If the interval has expired, it's time to
			 * fiddle with where OutFile points.
			 */

			if (Interval.active &&
					now.tv_sec >= Interval.end_time) {
				updateOutFile (&Interval);

				if (Interval.iters > 0 && (Interval.curriter >= (Interval.iters - 1))) {
				    Interval.curriter = 0;
				}
				else {
				    Interval.curriter += 1;
				}
			}

			if (parallel) {
				mypcap_read(pd, pcap_userdata);
			}
			else if (pcap_loop(pd, PACKET_POLL_CNT, printer, pcap_userdata) < 0) {
				(void)fprintf(stderr, "%s: pcap_loop: %s\n",
				    program_name, pcap_geterr(pd));
				exit(1);
			}

			total_cnt += PACKET_POLL_CNT;
		}
	}

	pcap_close(pd);

	if (quickSummaryFile != NULL) {
		printSummary (quickSummaryFile);
	}

	exit(0);
}

/* make a clean exit on interrupts */
static RETSIGTYPE
cleanup(int signo)
{
	struct pcap_stat stat;

	/* Can't print the summary if reading from a savefile */
	if (pd != NULL && pcap_file(pd) == NULL) {
		ptDumpTable(OutFile, readTable, "read");
		ptDumpTable(OutFile, writeTable, "write");
		(void)fflush(stdout);
		putc('\n', stderr);
		if (pcap_stats(pd, &stat) < 0)
			(void)fprintf(stderr, "pcap_stats: %s\n",
			    pcap_geterr(pd));
		else {
			(void)fprintf(stderr, "%d packets received by filter\n",
			    stat.ps_recv);
			(void)fprintf(stderr, "%d packets dropped by kernel\n",
			    stat.ps_drop);
		}
	}
	exit(0);
}

static void
usage(void)
{
	extern char version[];
	extern char pcap_version[];

	(void)fprintf(stderr, "%s version %s\n", program_name, version);
	(void)fprintf(stderr, "libpcap version %s\n", pcap_version);
	(void)fprintf(stderr,
"Usage: %s [-lnOptuv] [-c count] [ -F file ]\n", program_name);
	(void)fprintf(stderr,
"\t\t[ -i interface ] [ -r file ] [ -s snaplen ]\n");
	(void)fprintf(stderr,
"\t\t[ -w file ] [ expression ]\n");
	exit(-1);
}

/*
 *
 */

static int updateOutFile (intervalDesc_t *i)
{
	extern FILE *OutFile;
	struct tm tm;
	time_t clock;
	int rc;

	ptDumpTable(OutFile, readTable, "read");
	ptDumpTable(OutFile, writeTable, "write");

	if (OutFile != NULL && OutFile != stdout) {
		fclose (OutFile);

		if (i->tmpname != NULL && i->cur_logname != NULL) {

			rc = rename (i->tmpname, i->cur_logname);
			assert (rc == 0);
		}

		if (i->cur_logname != NULL) {
			free (i->cur_logname);
			i->cur_logname = NULL;
		}
		if (i->cur_sumname != NULL) {
			printSummary (i->cur_sumname);
			free (i->cur_sumname);
			i->cur_sumname = NULL;
		}
	}

	/*
	 * Round the current time to the nearest interval.
	 */

	clock = time (NULL);
	clock -= (clock % i->secs);

	i->end_time = clock + i->secs;

	localtime_r (&clock, &tm);

	i->cur_logname = malloc (strlen (i->basename) + 40);
	assert (i->cur_logname != NULL);

	sprintf (i->cur_logname, "%s.txt.%.2d",
		 i->basename, i->curriter);
	
	i->cur_sumname = malloc (strlen (i->basename) + 40);
	assert (i->cur_sumname != NULL);

	sprintf (i->cur_sumname, "%s.sum.%.2d",
		 i->basename, i->curriter);

	/*
	 * Now that we've figure out what the name of the file will
	 * EVENTUALLY be, we actually open the tmp file and dump
	 * everything is there.  The file doesn't get it's real, final
	 * name until it's closed.  This avoids other processes seeing
	 * the file half-written.
	 */

	OutFile = fopen (i->tmpname, "w+");
	assert (OutFile != NULL);

	setvbuf (OutFile, NULL, _IOFBF, 0);

	return (0);
}

/*
 * print the summary statistics to filename (the info is similar to
 * nfsstat) and then reset the NFS protocol counters.  (it would be
 * nice to reset the packet counters as well, but I haven't figured
 * out how to do that)
 *
 * If filename is "-", then uses stdout.
 */

static int printSummary (char *filename)
{
	FILE *qs = NULL;
	struct pcap_stat stat;

	if (filename == NULL) {
		return (0);
	}

	if (strcmp (filename, "-") == 0) {
		qs = stdout;
	}
	else {
		qs = fopen (filename, "w+");
	}

	if (qs != NULL) {
		nfs_v3_stat_print (&v3statsBlock, qs);
		fprintf (qs, "\n");
		nfs_v2_stat_print (&v2statsBlock, qs);

		fprintf (qs, "\n");
		if (pcap_stats(pd, &stat) < 0) {
			fprintf(qs, "pcap_stats: %s\n", pcap_geterr(pd));
		}
		else {
			fprintf(qs, "%d packets received by filter\n",
					stat.ps_recv);
			fprintf(qs, "%d packets dropped by kernel\n",
					stat.ps_drop);
		}


		{
			extern int xidHashElemCnt;

			fprintf (qs, "%d elems in xidHashTable.\n",
					xidHashElemCnt);
		}

		if (qs != stdout) {
			fclose (qs);
		}
	}

	nfs_v3_stat_init (&v3statsBlock);
	nfs_v2_stat_init (&v2statsBlock);

	return (0);
}
