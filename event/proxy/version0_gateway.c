/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2007-2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <sys/time.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <setjmp.h>
#include <paths.h>
#include <pubsub/pubsub.h>
#include "log.h"

static int	debug   = 0;
static int	verbose = 1;
static int	stop    = 0;

/* Forward decls */
static void     pubsub_callback(pubsub_handle_t *handle,
				pubsub_subscription_t *subscription,
				pubsub_notification_t *notification,
				void *data);

static void     v0_callback(pubsub_handle_t *handle,
			    pubsub_subscription_t *subscription,
			    pubsub_notification_t *notification,
			    void *data);

/* A few things are easier if global */
static pubsub_handle_t		*pubsub_handle;
static pubsub_handle_t		*v0_handle;

char *usagestr = 
 "usage: pubsubv0_gateway [options]\n"
 " -h              Display this message\n"
 " -d              Turn on debugging\n"
 " -v              Turn on verbose logging\n"
 " -p portnum      Specify a pubsub port number to connect to\n"
 "\n";

void
usage()
{
	fprintf(stderr, "%s", usagestr);
	exit(1);
}

static void
setverbose(int sig)
{
	if (sig == SIGUSR1)
		verbose = 1;
	else
		verbose = 0;
}

static void
sigterm(int sig)
{
	stop = 1;
}

int
main(int argc, char **argv)
{
	int			ch;
	int			portnum = PUBSUB_SERVER_PORTNUM;
	char			buf[BUFSIZ];
	pubsub_error_t		pubsub_error;
	
	while ((ch = getopt(argc, argv, "hdvp:")) != -1) {
		switch(ch) {
		case 'd':
			debug++;
			break;
		case 'v':
			verbose++;
			break;
		case 'p':
			if (sscanf(optarg, "%d", &portnum) == 0) {
				fprintf(stderr,
					"Error: -p value is not a number: "
					"%s\n",
					optarg);
				usage();
			}
			else if ((portnum <= 0) || (portnum >= 65536)) {
				fprintf(stderr,
					"Error: -p value is not between "
					"0 and 65536: %d\n",
					portnum);
				usage();
			}
			break;
		case 'h':
			fprintf(stderr, "%s", usagestr);
			exit(0);
			break;
		default:
			usage();
		}
	}

	argv += optind;
	argc -= optind;

	if (argc) {
		error("Unrecognized command line arguments: %s ...\n",argv[0]);
		usage();
	}

	if (!debug) {
		loginit(1, "v0_gateway");
		/* See below for daemonization */
		pubsub_debug = 1; /* XXX for now */
	} else {
		pubsub_debug = debug;
	}

	info("version0<->version1 gateway daemon starting\n");

	/*
	 * XXX Need to daemonize earlier or the threads go away.
	 */
	if (!debug)
		daemon(0, 0);
	
	signal(SIGUSR1, setverbose);
	signal(SIGUSR2, setverbose);
	signal(SIGTERM, sigterm);

	/*
	 * Stash the pid away.
	 */
	if (!geteuid()) {
		FILE	*fp;

		sprintf(buf, "%s/version0_gateway.pid", _PATH_VARRUN);
		fp = fopen(buf, "w");
		if (fp != NULL) {
			fprintf(fp, "%d\n", getpid());
			(void) fclose(fp);
		}
	}

	if (pubsub_connect("localhost",
			   PUBSUB_SERVER_PORTNUM, &pubsub_handle) < 0) {
		error("Could not connect to pubsub server on localhost!\n");
		exit(-1);
	}
	info("connected to pubsubd server\n");

	if (pubsub_connect("localhost", portnum, &v0_handle) < 0) {
		error("Could not connect to v0 pubsub on localhost!\n");
		exit(-1);
	}
	info("connected to pubsubd server\n");

	/* Subscribe to *all* events */
	if (!pubsub_add_subscription(pubsub_handle, "",
				     pubsub_callback, NULL,
				     &pubsub_error)) {
		error("subscription to pubsub server failed\n");
		exit(-1);
	}
	info("pubsub subscription added\n");

	/* Subscribe to *all* events */
	if (!pubsub_add_subscription(v0_handle, "",
				     v0_callback, NULL,
				     &pubsub_error)) {
		error("subscription to v0 server failed\n");
		exit(-1);
	}
	info("v0 subscription added\n");

	/*
	 * Threaded mode; stuff just happens. 
	 */
	while (!stop) {
		struct timeval  tv = { 5, 0 };

		select(0, NULL, NULL, NULL, &tv);
	}

	pubsub_disconnect(pubsub_handle);
	pubsub_disconnect(v0_handle);
	unlink(buf);
	return 0;
}

static char	notify_debug_string[2*BUFSIZ];

#if 0
static int
pubsub_notify_traverse_debug(void *arg, char *name,
			     pubsub_type_t type, pubsub_value_t value,
			     pubsub_error_t *error)
{
	char		**bp = (char **) arg;
	char		buf[BUFSIZ], *tp = buf;
	unsigned char   *up;
	int		i, cc;

	switch (type) {
	case STRING_TYPE:
		snprintf(*bp,
			 &notify_debug_string[sizeof(notify_debug_string)-1]-
			 *bp,
			 "%s=%s,", name, value.pv_string);
		break;
		
	case INT32_TYPE:
		snprintf(*bp,
			 &notify_debug_string[sizeof(notify_debug_string)-1]-
			 *bp,
			 "%s=%d,", name, value.pv_int32);
		break;
		
	case OPAQUE_TYPE:
		up = (unsigned char *) value.pv_opaque.data;
		for (i = 0; i < value.pv_opaque.length; i++, up++) {
			cc = snprintf(tp,
				      &buf[sizeof(buf)-1]-tp, "%02hhx", *up);
			tp += cc;
			if (tp >= &buf[sizeof(buf)-1])
				break;
		}		
		snprintf(*bp,
			 &notify_debug_string[sizeof(notify_debug_string)-1]-
			 *bp,
			 "%s=%s,", name, buf);
		break;
		
	default:
		snprintf(*bp,
			 &notify_debug_string[sizeof(notify_debug_string)-1]-
			 *bp,
			 "%s=...,", name);
		break;
	}
	*bp = *bp + strlen(*bp);
	
	return 1;
}
#endif

static void
pubsub_callback(pubsub_handle_t *handle,
		pubsub_subscription_t *subscription,
		pubsub_notification_t *notification,
		void *arg)
{
	pubsub_error_t		pubsub_error;

	/*
	 * These are events coming from the version 1 server. We need to
	 * remove the ___elvin_ordered___ ordered flag since the client
	 * will not understand it (no elvin compat on them).
	 */
	pubsub_notification_remove(notification,
				   "___elvin_ordered___", &pubsub_error);
	
	if (pubsub_notify(v0_handle, notification, &pubsub_error) < 0) {
		error("Failed to send pubsub notification to v0!\n");
	}
}

static void
v0_callback(pubsub_handle_t *handle,
	    pubsub_subscription_t *subscription,
	    pubsub_notification_t *notification,
	    void *arg)
{
	pubsub_error_t		pubsub_error;

	/*
	 * These are events coming from version 0 clients. We do not
	 * need to do anything except forward them.
	 */
	if (pubsub_notify(pubsub_handle, notification, &pubsub_error) < 0) {
		error("Failed to send v0 notification to pubsub!\n");
	}
}

