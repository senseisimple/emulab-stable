#include "tbevent.h"
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

static class TbEventSinkClass : public TclClass {
public:
	TbEventSinkClass() : TclClass("TbEventSink") {}
	TclObject* create(int /* argc */, const char*const* /* argv */) {
		return (new TbEventSink);
	}
} class_tbevent_sink;


void
TbEventSink::callback(event_handle_t handle,
		      event_notification_t notification, void *data) {

  TbEventSink *obj = (TbEventSink *)data;
  obj->gotevent = 1;

  char		buf[7][64];
  int		len = 64;
  char          argsbuf[8192];
  
  buf[0][0] = buf[1][0] = buf[2][0] = buf[3][0] = 0;
  buf[4][0] = buf[5][0] = buf[6][0] = buf[7][0] = 0;
  event_notification_get_site(handle, notification, buf[0], len);
  event_notification_get_expt(handle, notification, buf[1], len);
  event_notification_get_group(handle, notification, buf[2], len);
  event_notification_get_host(handle, notification, buf[3], len);
  event_notification_get_objtype(handle, notification, buf[4], len);
  event_notification_get_objname(handle, notification, buf[5], len);
  event_notification_get_eventtype(handle, notification, buf[6], len);
  event_notification_get_arguments(handle, notification, argsbuf, sizeof(argsbuf));
  
  struct timeval now;
  gettimeofday(&now, NULL);
  
  info("Received Event: %lu:%ld \"%s\"\n", now.tv_sec, now.tv_usec, argsbuf);

  // All events coming in to NSE should be MODIFY events and we just need to
  // evaluate the string in args. If the user sent code with wrong syntax,
  // we just print a warning in the log. Also we need a mechanism
  // to report the error back to the experimenter
  // XXX: fix needed
  
  if( Tcl_GlobalEval( Tcl::instance().interp(), argsbuf ) != TCL_OK ) {
    error( "Tcl Eval error in code: \"%s\"\n", argsbuf );
  }
  
}


TbEventSink::~TbEventSink() {
  if( ehandle != 0 ) {
    /*
     * Unregister with the event system:
     */
    if (event_unregister(ehandle) == 0) {
      fatal("could not unregister with event system");
    }    
  }
}

void
TbEventSink::init() {

  char buf[BUFSIZ] ;

  if( logfile[0] != 0 ) {
    loginit(0, logfile);
  } else {
    loginit(1, "Testbed-NSE");
  }
  
  if( server[0] == 0 ) {
    fatal("event system server unknown\n");
  }
  
  /*
   * Get our IP address. Thats how we name ourselves to the
   * Testbed Event System. 
   */
  struct hostent	*he;
  struct in_addr	myip;
  
  if (gethostname(buf, sizeof(buf)) < 0) {
    fatal("could not get hostname");
  }
  
  if (! (he = gethostbyname(buf))) {
    fatal("could not get IP address from hostname");
  }
  memcpy((char *)&myip, he->h_addr, he->h_length);
  strncpy(ipaddr, inet_ntoa(myip), sizeof(ipaddr));

  /*
   * Register with the event system. 
   */
  ehandle = event_register(server, 0);
  if (ehandle == NULL) {
    fatal("could not register with event system");
  }

}

int
TbEventSink::poll() {
  int rv;
  
  rv = event_poll(ehandle);
  if (rv)
    fatal("event_poll failed, err=%d\n", rv);

  if(gotevent) {
    gotevent = 0 ;
    return 1;
  } else {
    return 0;
  }
}

int TbEventSink::command(int argc, const char*const* argv)
{
  
  if( argc == 3 ) {
    if(strcmp(argv[1], "event-server") == 0) {
      strncpy(server, argv[2], sizeof(server));
      return(TCL_OK);
    }
    if(strcmp(argv[1], "objnamelist") == 0) {
      strncpy(objnamelist, argv[2], sizeof(objnamelist));
      return(TCL_OK);
    }
    if(strcmp(argv[1], "logfile") == 0) {
      strncpy(logfile, argv[2], sizeof(logfile));
      return(TCL_OK);
    }
  }

  return (TclObject::command(argc, argv));
}  

void
TbEventSink::subscribe() {
  /*
   * Construct an address tuple for subscribing to events for
   * this node.
   */
  address_tuple_t  tuple = address_tuple_alloc();
  if (tuple == NULL) {
    fatal("could not allocate an address tuple");
  }
  tuple->host	 = ipaddr[0] ? ipaddr : ADDRESSTUPLE_ANY;
  tuple->site      = ADDRESSTUPLE_ANY;
  tuple->group     = ADDRESSTUPLE_ANY;
  tuple->expt      = ADDRESSTUPLE_ANY;	/* pid/eid */
  tuple->objtype   = TBDB_OBJECTTYPE_TRAFGEN;

  if( objnamelist[0] != 0 ) {
    tuple->objname = objnamelist;
  } else {
    tuple->objname   = ADDRESSTUPLE_ANY;
  }
  tuple->eventtype = ADDRESSTUPLE_ANY;

  info( "TRAFGEN event subscription for object list: \"%s\" for node %s\n", tuple->objname, tuple->host);
  
  if (!event_subscribe(ehandle, callback, tuple, this)) {
    fatal("could not subscribe to TRAFGEN events for objects %s", tuple->objname );
  }
  address_tuple_free(tuple);

}

static class TbResolverClass : public TclClass {
public:
	TbResolverClass() : TclClass("TbResolver") {}
	TclObject* create(int /* argc */, const char*const* /* argv */) {
		return (new TbResolver);
	}
} class_tbresolver;

int TbResolver::command(int argc, const char*const* argv)
{
  
  if( argc == 3 ) {
    if(strcmp(argv[1], "lookup") == 0) {
      struct hostent *he = gethostbyname(argv[2]);
      struct in_addr	myip;
      char   ipaddr[BUFSIZ];
      Tcl &tcl = Tcl::instance();
      if( he ) {
	memcpy((char *)&myip, he->h_addr, he->h_length);
	strncpy(ipaddr, inet_ntoa(myip), sizeof(ipaddr));
	sprintf(tcl.buffer(), "%s", ipaddr );
	tcl.result(tcl.buffer());
      } else {
	tcl.result("");
      }
      return(TCL_OK);
    }
  }
    
  return (TclObject::command(argc, argv));
}  
