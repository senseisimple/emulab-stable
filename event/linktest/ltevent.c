/*
 * Event helper for Linktest events.
 */

#include <stdio.h>
#include <ctype.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include "log.h"
#include "tbdefs.h"

#include "event.h"

static char	*progname;
static void
callback(event_handle_t handle,
	 event_notification_t notification, void *data);

void
usage()
{
	fprintf(stderr,
		"Usage:\t%s -s server [-p port] [-k keyfile] -e pid/eid [-w event | -x event] [ARGS...]\n",
		progname);
	fprintf(stderr, " -w event\twait for Linktest event\n");
	fprintf(stderr, " -x event\ttransmit Linktest event\n");
	exit(-1);
}

void up(char *str) {
  if(str) {
    while (*str) {
      *str = toupper(*str);
      str++;
    }
  }
}

int
main(int argc, char **argv)
{
	event_handle_t handle;
	event_notification_t notification;
	address_tuple_t	tuple;
	char *server = NULL;
	char *port = NULL;
	char *keyfile = NULL;
	char buf[BUFSIZ];
	char *pideid = NULL;
	char *send_event = NULL;
	char *wait_event = NULL;
	char event_args[BUFSIZ];
	int c;

	progname = argv[0];
	
	while ((c = getopt(argc, argv, "s:p:w:x:e:k:")) != -1) {
		switch (c) {
		case 's':
			server = optarg;
			break;
		case 'p':
			port = optarg;
			break;
		case 'w':
			wait_event = optarg;;
			break;
		case 'x':
			send_event = optarg;
			break;
		case 'e':
			pideid = optarg;
			break;
		case 'k':
			keyfile = optarg;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if(!pideid)
	        usage();

	if(!server)
	        usage();

	event_args[0] = '\0';
	while(argc) {
	  strncat(event_args,argv[0],sizeof(event_args));
	  strncat(event_args," ",sizeof(event_args));
	  argv++;
	  argc--;
	}

	loginit(0, 0);
	
	/*
	 * Uppercase event tags for now. Should be wired in list instead.
	 */
	up(send_event);
	up(wait_event);
	
	/*
	 * Convert server/port to elvin thing.
	 *
	 * XXX This elvin string stuff should be moved down a layer. 
	 */
	snprintf(buf, sizeof(buf), "elvin://%s%s%s",
		 server,
		 (port ? ":"  : ""),
		 (port ? port : ""));
	server = buf;

	/* Register with the event system: */
	handle = event_register_withkeyfile(server, 0, keyfile);
	if (handle == NULL) {
		fatal("could not register with event system");
	}

	if(send_event) {
	  /*
	   * Construct an address tuple for generating the event.
	   */
	  tuple = address_tuple_alloc();
	  if (tuple == NULL) {
	    fatal("could not allocate an address tuple");
	  }

	  tuple->objtype  = TBDB_OBJECTTYPE_LINKTEST;
	  tuple->eventtype= send_event;
	  tuple->host	= ADDRESSTUPLE_ALL;
	  tuple->expt = pideid;
	  
	  /* Generate the event */
	  notification = event_notification_alloc(handle, tuple);
	  if (notification == NULL) {
	    fatal("could not allocate notification");
	  }

	  if(strlen(event_args)) {
	    if((event_notification_put_string(handle
					      , notification
					      , "ARGS"
					      , event_args))==0) {
	      fatal("could not set event arguments");
	    }
	  }

	  if (event_notify(handle, notification) == 0) {
	    fatal("could not send test event notification");
	  }
	  
	  /*gettimeofday(&now, NULL);
	  info("Sent %s at time: %lu:%d\n",send_event ,now.tv_sec, now.tv_usec);*/

	  event_notification_free(handle, notification);

	}

	if(wait_event) {

	  /*
	 * Construct an address tuple for subscribing to events for
	 * this node.
	 */
	  tuple = address_tuple_alloc();
	  if (tuple == NULL) {
	    fatal("could not allocate an address tuple");
	  }

	  tuple->host	 = ADDRESSTUPLE_ALL;
	  tuple->site      = ADDRESSTUPLE_ANY;
	  tuple->group     = ADDRESSTUPLE_ANY;
	  tuple->expt      = pideid;
	  tuple->objtype   = TBDB_OBJECTTYPE_LINKTEST;
	  tuple->objname   = ADDRESSTUPLE_ANY;
	  tuple->eventtype = wait_event;
	  
	  /*
	 * Register with the event system. 
	 */
	  handle = event_register_withkeyfile(server, 0, keyfile);
	  if (handle == NULL) {
	    fatal("could not register with event system");
	  }
	
	  /*
	 * Subscribe to the event we specified above.
	 */
	  if (! event_subscribe(handle, callback, tuple, NULL)) {
	    fatal("could not subscribe to event");
	  }
	
	  /*
	 * Begin the event loop, waiting to receive event notifications:
	 */

	  event_main(handle);

	}
	
	/* Unregister with the event system: */
	if (event_unregister(handle) == 0) {
		fatal("could not unregister with event system");
	}
	
	return 0;
}


static void
callback(event_handle_t handle, event_notification_t notification, void *data)
{

	exit(0);
}




