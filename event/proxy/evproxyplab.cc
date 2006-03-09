/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003, 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 *
 */
#include <stdio.h>
#include <ctype.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <paths.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.h"
#include "event.h"
#include "tbdefs.h"
#include "log.h"
#include <map>
#include <string>
#include <iostream>

static int debug = 0;
static event_handle_t localhandle;
static event_handle_t bosshandle;
static std::map<std::string, event_subscription_t> exptmap;
static char nodeidstr[BUFSIZ], ipaddr[32];
;

void
usage(char *progname)
{
    fprintf(stderr, "Usage: %s [-s server] [-p port] -n pnodeid -l local_elvin_port\n", progname);
    exit(-1);
}

static void
callback(event_handle_t handle,
	 event_notification_t notification, void *data);

static void
expt_callback(event_handle_t handle,
	 event_notification_t notification, void *data);

static void
sched_callback(event_handle_t handle,
	       event_notification_t notification, void *data);

static void subscribe_callback(event_handle_t handle,  int result,
			       event_subscription_t es, void *data);

static void async_callback(event_handle_t handle,  int result,
			   event_subscription_t es, void *data);

static void schedule_updateevent();


int
main(int argc, char **argv)
{
	address_tuple_t		tuple;
	char			*progname;
	char			*server = NULL;
	char			*port = NULL, *lport = NULL;
	char			*myeid = NULL;
	char			*pnodeid = NULL;
	char			buf[BUFSIZ];
	char			hostname[MAXHOSTNAMELEN];
	struct hostent		*he;
	int			c;
	struct in_addr		myip;
	FILE			*fp;

	progname = argv[0];
	
	while ((c = getopt(argc, argv, "ds:p:e:n:l:")) != -1) {
		switch (c) {
		case 'd':
			debug++;
			break;
		case 's':
			server = optarg;
			break;
		case 'p':
			port = optarg;
			break;
		case 'l':
			lport = optarg;
			break;
		case 'e':
			myeid = optarg;
			break;
		case 'n':
		        pnodeid = optarg;
                        break;
		default:
			usage(progname);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc)
		usage(progname);

	if ((! pnodeid) || (! lport))
	   fatal("Must provide pnodeid and local elvin port"); 


	if (debug) {
	        loginit(0, 0);
        }

	/*
	 * Get our IP address. Thats how we name this host to the
	 * event System. 
	 */
	if (gethostname(hostname, MAXHOSTNAMELEN) == -1) {
		fatal("could not get hostname: %s\n", strerror(errno));
	}
	if (! (he = gethostbyname(hostname))) {
		fatal("could not get IP address from hostname: %s", hostname);
	}
	memcpy((char *)&myip, he->h_addr, he->h_length);
	strcpy(ipaddr, inet_ntoa(myip));

	/*
	 * If server is not specified, then it defaults to EVENTSERVER.
	 * This allows the client to work on either users.emulab.net
	 * or on a client node. 
	 */
	if (!server)
	  server = EVENTSERVER;
	

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

	/*
	 * Construct an address tuple for generating the event.
	 */
	tuple = address_tuple_alloc();
	if (tuple == NULL) {
		fatal("could not allocate an address tuple");
	}
	
	/* Register with the event system on boss */
	bosshandle = event_register(server, 0);
	if (bosshandle == NULL) {
		fatal("could not register with remote event system");
	}

	snprintf(buf, sizeof(buf), "elvin://localhost:%s",lport);

	/* Register with the event system on the local node */
	localhandle = event_register(buf, 0);
	if (localhandle == NULL) {
		fatal("could not register with local event system");
	}
	
	/*
	 * Create a subscription to pass to the remote server. We want
	 * all events for this node, or all events for the experiment
	 * if the node is unspecified (we want to avoid getting events
	 * that are directed at specific nodes that are not us!). 
	 */
	snprintf(nodeidstr, sizeof(nodeidstr), "__%s_proxy", pnodeid);
	snprintf(buf, sizeof(buf), "%s,%s,%s", 
                 TBDB_EVENTTYPE_UPDATE, TBDB_EVENTTYPE_CLEAR,
                 TBDB_EVENTTYPE_RELOAD);

	tuple->eventtype = buf;
	tuple->host = ipaddr;
	tuple->objname = nodeidstr;
	tuple->objtype = TBDB_OBJECTTYPE_EVPROXY;
	
	/* Subscribe to the test event: */
	if (! event_subscribe(bosshandle, callback, tuple, 
			      (void *)"event received")) {
		fatal("could not subscribe to events on remote server");
        }

        /* Setup the global event passthru */
	address_tuple_free(tuple);
	tuple = address_tuple_alloc();

	snprintf(buf, sizeof(buf), "%s,%s", 
                 ipaddr, ADDRESSTUPLE_ALL);

	tuple->host = buf;
	tuple->expt = TBDB_EVENTEXPT_NONE;

	if (! event_subscribe(bosshandle, expt_callback, tuple, 
			      NULL)) {
		fatal("could not subscribe to events on remote server");
        }

        /*
         * Setup local elvind subscriptions:
         */
	address_tuple_free(tuple);
	tuple = address_tuple_alloc();
        
        tuple->host = ADDRESSTUPLE_ALL;
        tuple->scheduler = 1;
	
	if (! event_subscribe(localhandle, sched_callback, tuple,
			      NULL)) {
		fatal("could not subscribe to events on local server");
	}

        info("Successfully setup initial event subscriptions.\n");

	/*
	 * Stash the pid away.
	 */
	snprintf(buf, sizeof(buf), "%s/evproxy.pid", _PATH_VARRUN);
	fp = fopen(buf, "w");
	if (fp != NULL) {
		fprintf(fp, "%d\n", getpid());
		(void) fclose(fp);
	}

        /* Tell the schedulers on ops that we want a subscription update. */
	schedule_updateevent();

	/*
	 * Do this now, once we have had a chance to fail on the above
	 * event system calls.
	 */
	if (!debug) {
		daemon(0, 0);
		loginit(0, "/var/emulab/logs/evproxy.log");
	}
	
	/* Begin the event loop, waiting to receive event notifications */
        info("Entering main event loop.\n");
	event_main(bosshandle);

	/* Unregister with the remote event system: */
	if (event_unregister(bosshandle) == 0) {
		fatal("could not unregister with remote event system");
	}
	/* Unregister with the local event system: */
	if (event_unregister(localhandle) == 0) {
		fatal("could not unregister with local event system");
	}

	return 0;
}


/*
 * Handle incoming events from the remote server. 
 */
static void
callback(event_handle_t handle, event_notification_t notification, void *data)
{
	char		eventtype[TBDB_FLEN_EVEVENTTYPE];
	char		objecttype[TBDB_FLEN_EVOBJTYPE];
	char		objectname[TBDB_FLEN_EVOBJNAME];
	char            expt[TBDB_FLEN_PID + TBDB_FLEN_EID + 1];

	
	event_notification_get_eventtype(handle,
					 notification, eventtype, sizeof(eventtype));
	event_notification_get_objtype(handle,
				       notification, objecttype, sizeof(objecttype));
	event_notification_get_objname(handle,
				       notification, objectname, sizeof(objectname));
	event_notification_get_expt(handle, notification, expt, sizeof(expt));
	
	if (debug) {
	  info("%s %s %s\n", eventtype, objecttype, objectname);  
	}
	
	if (strcmp(objecttype,TBDB_OBJECTTYPE_EVPROXY) == 0) {
	  
	  /* Add a new subcription request if a new expt is added on
	   * the plab node.
	   */

	  if (strcmp(eventtype,TBDB_EVENTTYPE_UPDATE) == 0) {
	    
	    std::string key(expt);
	    std::map<std::string, event_subscription_t>::iterator member = 
	      exptmap.find(key);
	    
	    if(member == exptmap.end()) {

	      address_tuple_t tuple = address_tuple_alloc();
	      if (tuple == NULL) {
		fatal("could not allocate an address tuple");
	      }
	      
	      tuple->expt = expt;
	      tuple->scheduler = 2;
		  
	      /* malloc data -- to be freed in subscribe_callback */
	      char *data = (char *) xmalloc(sizeof(expt));
	      strcpy(data, expt);

	      int retval = event_async_subscribe(bosshandle, 
					  expt_callback, tuple, NULL,
					  subscribe_callback, 
					  (void *) data, 1);
	      if (! retval) {
		error("could not subscribe to events on remote server.\n");
	      }

	      tuple->scheduler = 0;
		  
	      retval = event_async_subscribe(bosshandle, 
					  expt_callback, tuple, NULL,
					  async_callback, 
					  NULL, 1);
	      if (! retval) {
		error("could not subscribe to events on remote server.\n");
	      } 

              info("Subscribing to experiment: %s\n", expt);

	      address_tuple_free(tuple);
	    }
	  } 

	  
	  /* Remove a old subcription whenever get a CLEAR message */
	  
	  else if (strcmp(eventtype,TBDB_EVENTTYPE_CLEAR) == 0) {
	    std::string key(expt);
	    
	    std::map<std::string, event_subscription_t>::iterator member = 
	      exptmap.find(key);
	    
	    if(member != exptmap.end()) {
	      event_subscription_t tmp = member->second;
	      
	      /* remove the subscription */
	      int success = event_async_unsubscribe(handle, tmp);
	      
	      if (!success) {
		error("not able to delete the subscription.\n");
		elvin_error_fprintf(stderr, handle->status);
	      } else {
		exptmap.erase(key);
                info("Unsubscribing from experiment: %s\n", expt);
	      }
	    }
	  }
	  
	  /* reset all subscriptions if you get a RELOAD event */
	  else if (strcmp(eventtype,TBDB_EVENTTYPE_RELOAD) == 0) {

            info("RELOAD received.  Clearing experiment subscriptions "
                 "and scheduling an UPDATE.\n");
	   
	    std::map<std::string, event_subscription_t>::iterator iter;
	    
	    for(iter = exptmap.begin(); iter != exptmap.end(); iter++) {
	      /* remove the subscription */
	      std::string key = iter->first;
	      event_subscription_t tmp = iter->second;
		
	      int success = event_async_unsubscribe(handle, tmp);
	      
	      if (!success) {
		error("not able to delete the subscription.\n");
		elvin_error_fprintf(stderr, handle->status);
	      }
	    }
	      
	    exptmap.clear();
            schedule_updateevent();
	    
	  }
	  
	  /* pass thru EVPROXY events to plab-scheduler */
	  if (! event_notify(localhandle, notification))
            error("Failed to deliver notification!\n");
	  
	}
}

/*
 * Handle incoming experiment events from the remote server  
 */
static void
expt_callback(event_handle_t handle, event_notification_t notification, void *data)
{
	char		objecttype[TBDB_FLEN_EVOBJTYPE];
        char		objectname[TBDB_FLEN_EVOBJNAME];
	char            expt[TBDB_FLEN_PID + TBDB_FLEN_EID + 1];
	int		plabsched = 0;

	event_notification_get_objtype(handle,
				       notification, objecttype, sizeof(objecttype));
        event_notification_get_objname(handle,
				       notification, objectname, sizeof(objectname));
        event_notification_get_expt(handle, notification, expt, sizeof(expt));

        if (debug) {
          info("Received event for %s: %s %s\n", expt, objecttype, objectname);
	}

	if (strcmp(objecttype,TBDB_OBJECTTYPE_EVPROXY) != 0) {
	  /*
           * Filter <plabsched,1> events, and for rest resend the notification 
	   * to the local elvind server.
           */
	  int ret = event_notification_get_int32(handle, notification,
						 TBDB_PLABSCHED, &plabsched);
	  
	  if ((!ret) || (plabsched != 1)) {
	    
	    if (! event_notify(localhandle, notification))
	      error("Failed to deliver notification!\n");
	  }
	}
}


static void
sched_callback(event_handle_t handle,
	       event_notification_t notification,
	       void *data)
{
        if (! event_notify(bosshandle, notification))
		error("Failed to deliver scheduled notification!\n");
  
}




/* Callback functions for asysn event subscribe */

void subscribe_callback(event_handle_t handle,  int result,
			event_subscription_t es, void *data) {
  if (result) {
    std::string key((char *)data);
    exptmap[key] = es;
    info("Subscription for %s added successfully.\n", (char *)data);
  } else {
    error("not able to add the subscription.\n");
    elvin_error_fprintf(stderr, handle->status);
  }
  
  free(data);

}


/* Callback functions for async event subscribe/unsubscribe */

void async_callback(event_handle_t handle,  int result,
			event_subscription_t es, void *data) {
  if (!result) {
    error("Error in async callback\n");
    elvin_error_fprintf(stderr, handle->status);
  }
  

}


void schedule_updateevent() {
  /* send a message to elvind on ops */
  address_tuple_t tuple = address_tuple_alloc();

  struct timeval now;
  gettimeofday(&now, NULL);

  if (tuple == NULL) {
    fatal("could not allocate an address tuple");
  }
  
  tuple->objtype = TBDB_OBJECTTYPE_EVPROXY;
  tuple->objname = nodeidstr;
  tuple->eventtype = TBDB_EVENTTYPE_UPDATE;
  tuple->host = ipaddr;
  
  event_notification_t notification = 
    event_notification_alloc(bosshandle, tuple);
  
  if (notification == NULL) {
    fatal("could not allocate notification\n");
  }
  
  if (event_schedule(bosshandle, notification, &now) == 0) {
    error("could not schedule update event.\n");
  }

  info("Scheduled remote UPDATE event.\n");

  event_notification_free(bosshandle, notification);
  
  address_tuple_free(tuple);

}
