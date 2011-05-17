/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004, 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef _event_sched_rpc_h
#define _event_sched_rpc_h

#include "event-sched.h"

#ifdef __cplusplus

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/girerr.hpp>
#include <xmlrpc-c/client.hpp>
#include <iostream>
#include "emulab_proxy.h"

int RPC_invoke(char *method,
	       emulab::EmulabResponse *er_out,
	       emulab::spa_attr_t tag,
	       ...);

struct r_rpc_data {
	const char *certpath;
	const char *host;
	unsigned short port;
	int refcount;
	xmlrpc_c::clientXmlTransport *transport;
	pthread_mutex_t mutex;
};

extern struct r_rpc_data rpc_data;

#endif

#define DEFAULT_RPC_PORT 3069

#define ROBOT_TIMEOUT 10 * 60 /* seconds */

#ifdef __cplusplus
extern "C" {
#endif

extern const char *topography_name; // XXX temporary

typedef enum {
	ES_UNKNOWN,
	ES_ACTIVATING,
	ES_ACTIVE,
	ES_MODIFY_RESWAP,
} expt_state_t;
	
int RPC_init(const char *certpath, const char *host, unsigned short port);
int RPC_grab(void);
void RPC_drop(void);

int RPC_metadata(char *pid, char *eid);
int RPC_waitforrobots(event_handle_t handle, char *pid, char *eid);
expt_state_t RPC_expt_state(char *pid, char *eid);
int RPC_waitforactive(char *pid, char *eid);
int RPC_notifystart(char *pid, char *eid, char *timeline, int set_or_clear);
int RPC_agentlist(event_handle_t handle, char *pid, char *eid);
int RPC_grouplist(event_handle_t handle, char *pid, char *eid);
int RPC_eventlist(char *pid, char *eid,
		  event_handle_t handle, address_tuple_t tuple);

extern int SetExpPath(const char *path);
extern int AddUserEnv(const char *name, const char *value);

extern int AddAgent(event_handle_t handle,
		    const char *vname, const char *vnode, const char *nodeid,
		    const char *ipaddr, const char *type);

extern int AddGroup(event_handle_t handle, const char *groupname,
                    const char *agentname);

extern int AddEvent(event_handle_t handle, address_tuple_t tuple,
		    const char *exidx, const char *ftime, const char *objname,
                    const char *exargs, const char *objtype, const char *evttype,
                    const char *parent, const char *triggertype);

extern int AddRobot(event_handle_t handle,
		    struct agent *agent,
		    double init_x,
		    double init_y,
		    double init_orientation);

extern const char *XMLRPC_ROOT;
#ifdef __cplusplus
}
#endif

#endif
