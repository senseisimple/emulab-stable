#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
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

static void
callback(event_handle_t handle,
	 event_notification_t notification, void *data);

static void
usage()
{
	fprintf(stderr,
		"Usage: %s [-s serverip] [-p serverport] [-l logfile] "
		"[ -N name ] [ -T targetip.targetport ] [-P proto] [-R role] "
		"[ -E pid/eid ]\n",
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
	char *server = NULL;
	char *port = NULL;
	char *ipaddr = NULL;
	char *myname = NULL;
	char buf[BUFSIZ], ipbuf[BUFSIZ];
	struct sockaddr tmp;
	int c;

	progname = argv[0];
	memset(&tmp, 0, sizeof(tmp));
	
	while ((c = getopt(argc, argv, "s:p:T:P:R:N:l:E:")) != -1) {
		switch (c) {
		case 's':
			server = optarg;
			break;
		case 'p':
			port = optarg;
			break;
		case 'T':
			if (!ipport_atoaddr(optarg, &tmp))
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

		default:
			usage();
		}
	}

	if (prot.prot == NULL)
		badprot();

	if (prot.qos & QOS_SERVER) {
		prot.src = tmp;
		prot.qos |= QOS_SRC;
	} else {
		prot.dst = tmp;
		prot.qos |= QOS_DST;
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
	tuple->host	 = ipaddr;
	tuple->site      = ADDRESSTUPLE_ANY;
	tuple->group     = ADDRESSTUPLE_ANY;
	tuple->expt      = myexp ? myexp : ADDRESSTUPLE_ANY;
	tuple->objtype   = TBDB_OBJECTTYPE_TRAFGEN;
	tuple->objname   = myname ? myname : ADDRESSTUPLE_ANY;
	tuple->eventtype = ADDRESSTUPLE_ANY;

	/*
	 * Register with the event system. 
	 */
	ehandle = event_register(server, 0);
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
	tuple->expt      = ADDRESSTUPLE_ANY;	/* pid/eid */
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

#define DEFAULT_PKT_SIZE	64

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
 */
static int
parse_args(char *buf, tg_action *tg)
{
	extern double dist_const_gen(distribution *);
	extern char *dist_const_init(distribution *, double);
	int psize, rate;
	double interval;
	char *cp;

	psize = rate = -1;
	interval = -1.0;

	/*
	 * Parse out any new values
	 */
	cp = buf;
	while ((cp = strsep(&buf, " ")) != NULL) {
		if (sscanf(cp, "PACKETSIZE=%d", &psize) == 1)
			continue; 

		if (sscanf(cp, "INTERVAL=%lf", &interval) == 1)
			continue; 

		if (sscanf(cp, "RATE=%d", &rate) == 1)
			continue; 
	}

	/*
	 * Get current/default values for unspecified fields
	 */
	if (psize < 0) {
		if (tg->tg_flags & TG_LENGTH)
			psize = (int)dist_const_gen(&tg->length);
		else
			psize = DEFAULT_PKT_SIZE;
	}
	if (interval < 0.0) {
		if (rate > 0)
			interval = 1.0 / ((rate * 1000) / (8.0 * psize));
		else if (tg->tg_flags & TG_ARRIVAL)
			interval = dist_const_gen(&tg->arrival);
		else
			interval = DEFAULT_INTERVAL;
	}

	/*
	 * Verify and change values
	 */
	if (psize > MAX_PKT_SIZE) {
		fprintf(stderr, "%s: invalid packet size %d, ignored\n",
			progname, psize);
		return(EINVAL);
	}
	if (interval > MAX_INTERVAL) {
		fprintf(stderr, "%s: invalid packet interval %f\n",
			progname, interval);
		return(EINVAL);
	}
	dist_const_init(&tg->arrival, interval);
	dist_const_init(&tg->length, psize);
	tg->tg_flags |= (TG_ARRIVAL|TG_LENGTH);

#if 0
	fprintf(stderr, "parse_args: new psize=%d, interval=%f\n",
		psize, interval);
#endif

	return(0);
}

static void
callback(event_handle_t handle, event_notification_t notification, void *data)
{
	char		buf[8][64];
	int		len = 64;
	static int	startdone;
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
	gettimeofday(&now, NULL);
	fprintf(stderr, "Event: %lu.%03lu %s %s %s %s %s %s %s %s\n",
		now.tv_sec, now.tv_usec / 1000,
		buf[0], buf[1], buf[2], 
		buf[3], buf[4], buf[5], buf[6], buf[7]);
#endif

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
			if (startdone) {
				if (strcmp(buf[1], myexp) == 0)
					goto dowait;
				return;
			}

			if (logfile) {
				extern int FlushOutput;
				FlushOutput = 1;
				gettimeofday(&now, NULL);
				start_log(now, NULL);
			}

			/*
			 * Delay startup of source relative to sink
			 */
			if ((prot.qos & QOS_SERVER) == 0) {
				now.tv_sec = 0;
				now.tv_usec = 10000;
				select(0, NULL, NULL, NULL, &now);
			}
#ifdef EVENTDEBUG
			gettimeofday(&now, NULL);
			fprintf(stderr, "%lu.%03lu: SETUP\n",
				now.tv_sec, now.tv_usec / 1000);
#endif
			if (tg_first != &actions[0])
				fatal("global action list corrupted!");
			actions[0].tg_flags = TG_SETUP;
			actions[0].next = &actions[1];
			actions[1].tg_flags = TG_WAIT;
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
		tg_first->tg_flags &= ~TG_SETUP;
		tg_first->tg_flags |= TG_WAIT;
	}

	/*
	 * If eventtype is START, load params (first call only) and 
	 * let it rip.  If there are errors in the initial params,
	 * go back to the STOPed state.
	 */
	else if (strcmp(buf[6], STARTEVENT) == 0) {
		if (!startdone) {
			memset(tg_first, 0, sizeof(tg_action));
			if (parse_args(buf[7], tg_first) != 0) {
				fprintf(stderr, "START invalid params\n");
				goto dowait;
			}
			startdone = 1;

			/*
			 * XXX if they didn't specify an experiment,
			 * use the value of the first start event we see.
			 */
			if (myexp == 0)
				myexp = strdup(buf[1]);
		}
#ifdef EVENTDEBUG
		gettimeofday(&now, NULL);
		fprintf(stderr, "%lu.%03lu: START\n",
			now.tv_sec, now.tv_usec / 1000);
#endif
		tg_first->tg_flags &= ~(TG_SETUP|TG_WAIT);
	}

	/*
	 * If eventtype is MODIFY, update the params but remain in the
	 * current state.  Invalid params are ignored.
	 */
	else if (strcmp(buf[6], MODIFYEVENT) == 0) {
		if (!startdone)
			fprintf(stderr, "MODIFY without START event\n");
		else if (parse_args(buf[7], tg_first) != 0)
			fprintf(stderr, "MODIFY invalid params\n");
#ifdef EVENTDEBUG
		gettimeofday(&now, NULL);
		fprintf(stderr, "%lu.%03lu: MODIFY\n",
			now.tv_sec, now.tv_usec / 1000);
#endif
	}
	gotevent = 1;
}
#endif

void
tgevent_loop(void)
{
	int err;
	struct timeval now;

	gettimeofday(&now, NULL);
#ifdef EVENTDEBUG
	fprintf(stderr, "%lu.%03lu: trafgen started\n",
		now.tv_sec, now.tv_usec / 1000);
#endif

	/*
	 * XXX hand build a wait forever command.
	 * Setup happens when the first event arrives.
	 */
	memset(actions, 0, sizeof(actions));
	actions[0].tg_flags = TG_WAIT;

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
	 * We loop in here til done
	 */
	tg_first = &actions[0];
	do_actions();

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

	if ((++count % 500) != 0) return;
	state = (state + 1) % 3;

	memset(tg_first, 0, sizeof(tg_action));
	switch (state) {
	case 0:
		fprintf(stderr,
		  "sending arrival constant 0.001 length constant 64..\n");
		dist_const_init(&tg_first->arrival, 0.001);
		dist_const_init(&tg_first->length, 64.0);
		break;
	case 1:
		fprintf(stderr,
		  "idling..\n");
		tg_first->tg_flags = TG_WAIT;
		break;
	case 2:
		fprintf(stderr,
		  "sending arrival exp 0.03/0/1 length exp 256/64/1024..\n");
		dist_exp_init(&tg_first->arrival, 0.03, 0.0, 1.0);
		dist_exp_init(&tg_first->length, 256.0, 64.0, 1024.0);
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
