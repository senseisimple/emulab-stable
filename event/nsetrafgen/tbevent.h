/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef TBEVENT_H
#define TBEVENT_H

/* NSE includes */
#include "config.h"

/* event library stuff */
#ifdef __cplusplus
extern "C" {
#endif

#include <sys/param.h>
#include "tbdefs.h"
#include "log.h"
#include "event.h"

#ifdef __cplusplus
}
#endif

class TbEventSink : public TclObject {
public:
	TbEventSink() : gotevent(0), ehandle(0) {
	  bzero(ipaddr, sizeof(ipaddr));
	  bzero(server, sizeof(server));
	  bzero(objnamelist, sizeof(objnamelist));
	  bzero(logfile, sizeof(logfile));
	  bzero(nseswap_cmdline, sizeof(nseswap_cmdline));
	}
	~TbEventSink();
	virtual int command(int argc, const char*const* argv);

	void init();
	void subscribe();
	int poll();
	void send_nseswap();


private:
	char  server[BUFSIZ];
	int   gotevent;
	event_handle_t	ehandle;
	char ipaddr[BUFSIZ];
	char  objnamelist[BUFSIZ];
	char  logfile[MAXPATHLEN];
	char  nseswap_cmdline[MAXPATHLEN]; 

	static void
	  callback(event_handle_t handle,
		   event_notification_t notification, void *data);
};

class TbResolver : public TclObject {
 public:
  virtual int command(int argc, const char*const* argv);
};

#endif
