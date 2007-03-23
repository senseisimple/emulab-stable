/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2004, 2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h> 

#define EVENTDEBUG
//#define TESTING

/* tg stuff */
#include "config.h"
#include "distribution.h"
#include "protocol.h"

/* event library stuff */
#include "tbdefs.h"
#include "event.h"

static char		*progname;
static event_handle_t	ehandle;
static tg_action	actions[2]; /* large enough for setup/wait sequence */
static char 		*logfile;
static char		*myexp;

extern int		gotevent;
extern protocol		prot;
extern tg_action	*tg_first;
extern int		FlushOutput;

extern double dist_const_gen(distribution *);
extern char *dist_const_init(distribution *, double);


static void
callback(event_handle_t handle,
	 event_notification_t notification, void *data);

static void
usage()
{
	fprintf(stderr,
		"Usage: %s [-s serverip] [-p serverport] [-l logfile] "
		"[ -N name ] [ -S srcip.srcport ] [ -T targetip.targetport ] "
		"[-P proto] [-R role] [ -E pid/eid ] [-k keyfile]\n",
		progname);
	exit(-1);
}

static void
badaddr()
{
	fprintf(stderr,
		"Badly formatted address, should be <ipaddr>.<port>\n"
		"   e.g., 192.168.1.1.1025\n");
	usage();
}

static void
badprot()
{
	fprintf(stderr,
		"Bad protocol, should be \"udp\" or \"tcp\"\n");
	usage();
}

void
tgevent_init(int argc, char *argv[])
{
	address_tuple_t	tuple;
	char *server = "localhost";
	char *port = NULL;
	char *ipaddr = NULL;
	char *myname = NULL;
	char *keyfile = NULL;
	char buf[BUFSIZ], ipbuf[BUFSIZ];
	struct sockaddr saddr, taddr;
	int c, gotsaddr = 0;

	progname = argv[0];
	memset(&saddr, 0, sizeof(saddr));
	memset(&taddr, 0, sizeof(taddr));
	
	while ((c = getopt(argc, argv, "s:p:S:T:P:R:N:l:E:k:")) != -1) {
		switch (c) {
		case 's':
			server = optarg;
			break;
		case 'p':
			port = optarg;
			break;
		case 'S':
			if (!ipport_atoaddr(optarg, &saddr))
				badaddr();
			gotsaddr = 1;
			break;
		case 'T':
			if (!ipport_atoaddr(optarg, &taddr))
				badaddr();
			break;
		case 'P':
			prot.prot = find_protocol(optarg);
			break;
		case 'R':
			if (strcmp(optarg, "server") == 0 ||
			    strcmp(optarg, "sink") == 0)
				prot.qos |= QOS_SERVER;
			break;
		case 'N':
			myname = optarg;
			break;
		case 'E':
			myexp = optarg;
			break;
		case 'l':
			logfile = optarg;
			break;
		case 'k':
			keyfile = optarg;
			break;

		default:
			usage();
		}
	}

	if (prot.prot == NULL)
		badprot();

	if (prot.qos & QOS_SERVER) {
		prot.src = taddr;
		prot.qos |= QOS_SRC;
	} else {
		prot.dst = taddr;
		prot.qos |= QOS_DST;
		/*
		 * Can specify source address to bind local interface
		 * of sender.
		 */
		if (gotsaddr) {
			prot.src = saddr;
			prot.qos |= QOS_SRC;
		}
	}

	argc -= optind;
	argv += optind;

#ifndef TESTING
	/*
	 * Get our IP address. Thats how we name ourselves to the
	 * Testbed Event System. 
	 */
	if (ipaddr == NULL) {
	    struct hostent	*he;
	    struct in_addr	myip;
	    
	    if (gethostname(buf, sizeof(buf)) < 0) {
		fatal("could not get hostname");
	    }

	    if (! (he = gethostbyname(buf))) {
		fatal("could not get IP address from hostname");
	    }
	    memcpy((char *)&myip, he->h_addr, he->h_length);
	    strcpy(ipbuf, inet_ntoa(myip));
	    ipaddr = ipbuf;
	}

	/*
	 * Convert server/port to elvin thing.
	 *
	 * XXX This elvin string stuff should be moved down a layer. 
	 */
	if (server) {
		snprintf(buf, sizeof(buf), "elvin://%s%s%s",
			 server,
			 (port ? ":"  : ""),
			 (port ? port : ""));
		server = buf;
	}

	/*
	 * Construct an address tuple for subscribing to events for
	 * this node.
	 */
	tuple = address_tuple_alloc();
	if (tuple == NULL) {
		fatal("could not allocate an address tuple");
	}
	/*
	 * Do not use the ipaddr if we have a pid/eid. This allows
	 * trafgens to work inside jails that have their own IP.
	 */
	if (myexp) {
		tuple->expt = myexp;
		tuple->host = ADDRESSTUPLE_ANY;
	}
	else {
		tuple->host = ipaddr;
		tuple->expt = ADDRESSTUPLE_ANY;
	}
	tuple->site      = ADDRESSTUPLE_ANY;
	tuple->group     = ADDRESSTUPLE_ANY;
	tuple->objtype   = TBDB_OBJECTTYPE_TRAFGEN;
	tuple->objname   = myname ? myname : ADDRESSTUPLE_ANY;
	tuple->eventtype = ADDRESSTUPLE_ANY;

	/*
	 * Register with the event system. 
	 */
	ehandle = event_register_withkeyfile(server, 0, keyfile);
	if (ehandle == NULL) {
		fatal("could not register with event system");
	}
	
	/*
	 * Subscribe to the event we specified above.
	 */
	if (!event_subscribe(ehandle, callback, tuple, NULL)) {
		fatal("could not subscribe to TRAFGEN events");
	}
	address_tuple_free(tuple);

	/*
	 * Also subscribe to the start time event which we use to
	 * synchronize connection setup.
	 */
	tuple = address_tuple_alloc();
	if (tuple == NULL) {
		fatal("could not allocate an address tuple");
	}
	tuple->host	 = ADDRESSTUPLE_ANY;
	tuple->site      = ADDRESSTUPLE_ANY;
	tuple->group     = ADDRESSTUPLE_ANY;
	tuple->expt      = myexp ? myexp : ADDRESSTUPLE_ANY;
	tuple->objtype	 = TBDB_OBJECTTYPE_TIME;
	tuple->objname   = ADDRESSTUPLE_ANY;
	tuple->eventtype = ADDRESSTUPLE_ANY;
	if (!event_subscribe(ehandle, callback, tuple, NULL)) {
		fatal("could not subscribe to TIME events");
	}
	address_tuple_free(tuple);
#endif
}

void
tgevent_shutdown(void)
{
	/*
	 * Unregister with the event system:
	 */
	if (event_unregister(ehandle) == 0) {
		fatal("could not unregister with event system");
	}
}


#ifndef TESTING
#define STARTEVENT	TBDB_EVENTTYPE_START
#define STOPEVENT	TBDB_EVENTTYPE_STOP
#define MODIFYEVENT	TBDB_EVENTTYPE_MODIFY
#define TIMESTARTS	TBDB_EVENTTYPE_START
#define RESETEVENT	TBDB_EVENTTYPE_RESET

#define DEFAULT_PKT_SIZE	1024

#define MAX_RATE		1000000			/* 1Gbps in Kbps */
#define MAX_INTERVAL		(double)(24*60*60)	/* 1 day in sec */
#define DEFAULT_INTERVAL	0.1

/*
 * Handle Testbed Control events.
 *
 * Type:	Args:
 *
 * START	PACKETSIZE=NNN RATE=KKK INTERVAL=FFFF
 * STOP
 * MODIFY	PACKETSIZE=NNN or RATE=KKK or INTERVAL=FFFF
 * RESET
 */
static int
parse_args(char *buf, tg_action *tg)
{
	static int currate = -1;
	static int curpsize = DEFAULT_PKT_SIZE;
	static double curinterval = DEFAULT_INTERVAL;
	static int curqos = -1;
	int psize, rate, qos;
	double interval;
	char *cp;

	psize = rate = qos = -1;
	interval = -1.0;

	/*
	 * Parse out any new values
	 */
	cp = buf;
	if (cp != NULL) {
		while ((cp = strsep(&buf, " ")) != NULL) {
			if (sscanf(cp, "PACKETSIZE=%i", &psize) == 1)
				continue; 

			if (sscanf(cp, "INTERVAL=%lf", &interval) == 1)
				continue; 

			if (sscanf(cp, "RATE=%i", &rate) == 1)
				continue; 

			if (sscanf(cp, "IPTOS=%i", &qos) == 1)
				continue; 
		}
	}

	/*
	 * Determine and validate packet size.
	 * If a packet size was not explicitly specified, use existing size.
	 */
	if (psize < 0)
		psize = curpsize;
	if (psize > MAX_PKT_SIZE) {
		fprintf(stderr, "%s: invalid packet size %d, ignored\n",
			progname, psize);
		return(EINVAL);
	}

	/*
	 * Determine packet interval.
	 * If packet interval was not explicitly specified and a rate was
	 * specified, compute the interval from that.  Otherwise use the
	 * existing interval.
	 */
	if (interval < 0.0) {
		if (rate < 0) {
			interval = curinterval;
			rate = currate;
		}
	} else
		rate = -1;

	/*
	 * If we are using an explicit rate value specified by the user
	 * (either from this time or previously), we need to maintain that
	 * rate in the face of packet length changes.  Thus we recompute
	 * the interval to ensure a proper value.
	 */
	if (rate >= 0)
		interval = 1.0 / ((rate * 1000) / (8.0 * psize));

	/*
	 * Validate the computed interval.
	 */
	if (interval > MAX_INTERVAL) {
		fprintf(stderr, "%s: invalid packet interval %.9f\n",
			progname, interval);
		return(EINVAL);
	}

	/*
	 * Check QOS value
	 */
	if (qos == -1)
		qos = curqos;

	/*
	 * Finally, record the new values.
	 */
	dist_const_init(&tg->arrival, interval);
	dist_const_init(&tg->length, (double)psize);
	tg->tg_flags |= (TG_ARRIVAL|TG_LENGTH);
	if (qos != -1) {
		prot.tos = qos;
		prot.qos |= QOS_TOS;
	} else
		prot.qos &= ~QOS_TOS;
	curinterval = interval;
	curpsize = psize;
	currate = rate;
	curqos = qos;

#if 0
	fprintf(stderr, "parse_args: new psize=%d, interval=%.9f, qos=%d\n",
		psize, interval, qos);
#endif

	return(0);
}

static void
callback(event_handle_t handle, event_notification_t notification, void *data)
{
	char		buf[8][64];
	int		len = 64;
	static int	setupdone;
	struct timeval	now;

	buf[0][0] = buf[1][0] = buf[2][0] = buf[3][0] = 0;
	buf[4][0] = buf[5][0] = buf[6][0] = buf[7][0] = 0;
	event_notification_get_site(handle, notification, buf[0], len);
	event_notification_get_expt(handle, notification, buf[1], len);
	event_notification_get_group(handle, notification, buf[2], len);
	event_notification_get_host(handle, notification, buf[3], len);
	event_notification_get_objtype(handle, notification, buf[4], len);
	event_notification_get_objname(handle, notification, buf[5], len);
	event_notification_get_eventtype(handle, notification, buf[6], len);
	event_notification_get_arguments(handle, notification, buf[7], len);

#ifdef EVENTDEBUG
	{
		static int ecount;

		gettimeofday(&now, NULL);
		fprintf(stderr, "Event %d: %lu.%03lu %s %s %s %s %s %s %s %s\n",
			++ecount, now.tv_sec, now.tv_usec / 1000,
			buf[0], buf[1], buf[2],
			buf[3], buf[4], buf[5], buf[6], buf[7]);
	}
#endif

	if (tg_first != &actions[0])
		fatal("global action list corrupted!");

	/*
	 * Perform a setup when we get the "start of time" event.
	 */
	if (strcmp(buf[4], TBDB_OBJECTTYPE_TIME) == 0) {
		if (strcmp(buf[6], TIMESTARTS) == 0) {
			/*
			 * We may receive TIME START events for any experiment
			 * (if an experiment ID was not specified on the
			 * command line).  If we get a redundant start event,
			 * check the experiment ID.  If the start is for some
			 * other experiment, ignore it.  If it is another start
			 * for us, assume the event daemon has been restarted
			 * and just go back to WAITing.
			 */
			if (setupdone) {
				if (strcmp(buf[1], myexp) == 0)
					goto dowait;
				return;
			}

			if (logfile) {
				FlushOutput = 1;
				gettimeofday(&now, NULL);
				start_log(now, NULL);
			}

			/*
			 * Delay startup of source relative to sink
			 * by 0.5 seconds.
			 */
			if ((prot.qos & QOS_SERVER) == 0) {
				now.tv_sec = 0;
				now.tv_usec = 1000000 / 2;
				select(0, NULL, NULL, NULL, &now);
			}
#ifdef EVENTDEBUG
			gettimeofday(&now, NULL);
			fprintf(stderr, "%lu.%03lu: SETUP/WAIT exp=%s\n",
				now.tv_sec, now.tv_usec / 1000,
				myexp ? myexp : buf[1]);
#endif
			actions[0].tg_flags = TG_SETUP;
			actions[0].next = &actions[1];
			actions[1].tg_flags = TG_WAIT;
			actions[1].next = NULL;

			/*
			 * XXX if they didn't specify an experiment,
			 * use the value from the first time start we see.
			 */
			if (myexp == 0)
				myexp = strdup(buf[1]);

			setupdone = 1;
		}
	}

	/*
	 * If eventtype is STOP, execute an infinite wait
	 */
	else if (strcmp(buf[6], STOPEVENT) == 0) {
	dowait:
#ifdef EVENTDEBUG
		gettimeofday(&now, NULL);
		fprintf(stderr, "%lu.%03lu: WAIT\n",
			now.tv_sec, now.tv_usec / 1000);
#endif
		tg_first->tg_flags = TG_WAIT;
		tg_first->next = NULL;
	}

	/*
	 * If eventtype is START, parse any explicitly provided parameters
	 * and load the tg_action structure.  If there are errors, go back
	 * to the STOPed state.
	 */
	else if (strcmp(buf[6], STARTEVENT) == 0) {
		memset(tg_first, 0, sizeof(tg_action));
		if (parse_args(buf[7], tg_first) != 0) {
			fprintf(stderr, "START invalid params\n");
			goto dowait;
		}

		/*
		 * XXX if no time start event was seen, do an implied setup.
		 * This will happen if the node reboots after the scheduler
		 * has already sent the time start.
		 */
		if (!setupdone) {
#ifdef EVENTDEBUG
			gettimeofday(&now, NULL);
			fprintf(stderr, "%lu.%03lu: SETUP/START exp=%s\n",
				now.tv_sec, now.tv_usec / 1000,
				myexp ? myexp : buf[1]);
#endif
			if (logfile) {
				FlushOutput = 1;
				gettimeofday(&now, NULL);
				start_log(now, NULL);
			}
			actions[1] = actions[0];
			actions[0].tg_flags = TG_SETUP;
			actions[0].next = &actions[1];
			actions[1].next = NULL;
			if (actions[1].tg_flags != (TG_ARRIVAL|TG_LENGTH))
				fatal("START1: list corrupted!");
			if (myexp == 0)
				myexp = strdup(buf[1]);
			setupdone = 1;
		} else {
#ifdef EVENTDEBUG
			gettimeofday(&now, NULL);
			fprintf(stderr, "%lu.%03lu: START\n",
				now.tv_sec, now.tv_usec / 1000);
#endif
			if (tg_first->tg_flags != (TG_ARRIVAL|TG_LENGTH) ||
			    tg_first->next != NULL)
				fatal("START2: list corrupted!");
		}
	}

	/*
	 * If eventtype is MODIFY, update the params but remain in the
	 * current state.  Invalid params are ignored.
	 */
	else if (strcmp(buf[6], MODIFYEVENT) == 0) {
		if (parse_args(buf[7], tg_first) != 0)
			fprintf(stderr, "MODIFY invalid params, ignored\n");
#ifdef EVENTDEBUG
		else {
			int plen;
			double interval, bps;

			plen = (int)dist_const_gen(&tg_first->length);
			interval = dist_const_gen(&tg_first->arrival);
			bps = (plen * 8) * (1.0 / interval);

			gettimeofday(&now, NULL);
			fprintf(stderr, "%lu.%03lu: MODIFY length=%d bytes, "
				"interval=%.9f sec (rate=%f Kbps), iptos=%x\n",
				now.tv_sec, now.tv_usec / 1000, plen,
				interval, bps / 1000, prot.tos);
		}
#endif
		/*
		 * XXX Normally we return and reprocess the current event
		 * list after a MODIFY event, but if the head of the event
		 * list is a SETUP event (part of a SETUP/WAIT or SETUP/START
		 * combo) we can't do that.  So in that case, we change the
		 * head of the list to reflect the WAIT or START and continue
		 * with that action.
		 */
		if (tg_first->tg_flags & TG_SETUP) {
			if (tg_first->next == NULL)
				fatal("MODIFY: list corrupted!");
			if (tg_first->next->tg_flags & TG_WAIT)
				goto dowait;
			tg_first->tg_flags &= ~TG_SETUP;
			tg_first->next = NULL;
			if (tg_first->tg_flags != (TG_ARRIVAL|TG_LENGTH))
				fatal("MODIFY2: list corrupted!");
		}
	}
	/*
	 * If eventtype is RESET, zero out the event list so that TG
	 * falls out of its service loop and does a teardown.
	 * We regain control in tgevent_loop below and reissue a
	 * SETUP/WAIT combo.
	 */
	else if (strcmp(buf[6], RESETEVENT) == 0) {
#ifdef EVENTDEBUG
		gettimeofday(&now, NULL);
		fprintf(stderr, "%lu.%03lu: RESET\n",
			now.tv_sec, now.tv_usec / 1000);
#endif
		tg_first = NULL;
	}
	gotevent = 1;
}
#endif

void
tgevent_loop(void)
{
	int err;
	struct timeval now;

	if (logfile) {
		extern char prefix[], suffix[];
		extern char *ofile;
		char *cp = logfile;

		ofile = logfile;
		strcpy(prefix, strsep(&cp, "."));
		if (cp)
			strcpy(suffix, cp);
		else
			strcpy(suffix, "log");
#ifdef EVENTDEBUG
		fprintf(stderr, "trafgen: logfile=%s.%s\n", prefix, suffix);
#endif
	}

	/*
	 * XXX hand build a wait forever command.
	 * Setup happens when the first event arrives.
	 */
	memset(actions, 0, sizeof(actions));
	actions[0].tg_flags = TG_WAIT;

	while (1) {
		gettimeofday(&now, NULL);
#ifdef EVENTDEBUG
		fprintf(stderr, "%lu.%03lu: trafgen (re)started\n",
			now.tv_sec, now.tv_usec / 1000);
#endif

		/*
		 * We loop in here til we get a reset event.
		 */
		tg_first = &actions[0];
		do_actions();

		/*
		 * Got a reset.  Build a SETUP/WAIT combo and jump back
		 * into the fray.
		 */
		actions[0].tg_flags = TG_SETUP;
		actions[0].next = &actions[1];
		actions[1].tg_flags = TG_WAIT;

		/*
		 * Restart the log
		 */
		if (logfile) {
			FlushOutput = 1;
			log_close();
			start_log(now, "%s");
		}
	}

	log_close();
	tgevent_shutdown();
}

/*
 * Trigger a callback if and event is pending.
 * Callback will set gotevent signifying that 
 */
void
tgevent_poll(void)
{
#ifdef TESTING
	static unsigned long count;
	static int state;
	static struct timeval last;
	struct timeval now;

	gettimeofday(&now, NULL);
	if (last.tv_sec == 0) {
		fprintf(stderr,
		    "%lu.%03lu: initial setup..\n",
		    now.tv_sec, now.tv_usec / 1000);
		memset(tg_first, 0, sizeof(tg_action));
		actions[0].tg_flags = TG_SETUP;
		actions[0].next = &actions[1];
		actions[1].tg_flags = TG_WAIT;
		gotevent = 1;
		last = now;
		return;
	}
	if ((prot.qos & QOS_SERVER) || now.tv_sec - last.tv_sec < 5)
		return;

	last = now;
	state = (state + 1) % 5;
	memset(tg_first, 0, sizeof(tg_action));

	switch (state) {
	case 0:
		fprintf(stderr, "%lu.%03lu: "
		    "sending arrival constant 0.001 length constant 64..\n",
		    now.tv_sec, now.tv_usec / 1000);
		dist_const_init(&tg_first->arrival, 0.001);
		dist_const_init(&tg_first->length, 64.0);
		break;
	case 1: case 3:
		fprintf(stderr, "%lu.%03lu: "
		    "idling..\n",
		    now.tv_sec, now.tv_usec / 1000);
		tg_first->tg_flags = TG_WAIT;
		break;
	case 2:
		fprintf(stderr, "%lu.%03lu: "
		    "sending arrival exp 0.03/0/1 length exp 256/64/1024..\n",
		    now.tv_sec, now.tv_usec / 1000);
		dist_exp_init(&tg_first->arrival, 0.03, 0.0, 1.0);
		dist_exp_init(&tg_first->length, 256.0, 64.0, 1024.0);
		break;
	case 4:
		fprintf(stderr, "%lu.%03lu: "
		    "resetting..\n",
		    now.tv_sec, now.tv_usec / 1000);
		tg_first = NULL;
		break;
	}
	gotevent = 1;
#else
	int rv;

	rv = event_poll(ehandle);
	if (rv)
		fprintf(stderr, "event_poll failed, err=%d\n", rv);
#endif
}
