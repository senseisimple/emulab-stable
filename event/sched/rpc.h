/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifdef __cplusplus
#define CD "C"
#else
#define CD
#endif

/*
 * When included from the C++ side, we do not want to even try to
 * pass these headers through.
 */
#ifdef __cplusplus
struct event_handle;
typedef struct event_handle * event_handle_t;

struct address_tuple;
typedef struct address_tuple * address_tuple_t;
#endif

extern CD int RPC_init(char *certpath, char *host, int port);
extern CD int RPC_waitforactive(char *pid, char *eid);
extern CD int RPC_agentlist(char *pid, char *eid);
extern CD int RPC_grouplist(char *pid, char *eid);
extern CD int RPC_eventlist(char *pid, char *eid,
			    event_handle_t handle, address_tuple_t tuple,
			    long basetime);

extern CD int AddAgent(char *vname, char *vnode, char *nodeid,
			char *ipaddr, char *type);

extern CD int AddGroup(char *groupname, char *agentname);

extern CD int AddEvent(event_handle_t handle, address_tuple_t tuple,
		       long basetime,
		       char *exidx, char *ftime, char *objname, char *exargs,
		       char *objtype, char *evttype);
