
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

#include <netdb.h>
#include <netinet/in.h>

class TbEventSink : public TclObject {
public:
	TbEventSink() : gotevent(0), ehandle(0) {
	  bzero(ipaddr, sizeof(ipaddr));
	  bzero(server, sizeof(server));
	  bzero(objnamelist, sizeof(objnamelist));
	  bzero(logfile, sizeof(logfile));
	}
	~TbEventSink();
	virtual int command(int argc, const char*const* argv);

	void init();
	void subscribe();
	int poll();


private:
	char  server[BUFSIZ];
	int   gotevent;
	event_handle_t	ehandle;
	char ipaddr[BUFSIZ];
	char  objnamelist[BUFSIZ];
	char  logfile[MAXPATHLEN];

	static void
	  callback(event_handle_t handle,
		   event_notification_t notification, void *data);
};

#endif
