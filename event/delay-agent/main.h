/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * agent-main.h --
 *
 *      Delay node agent data structures
 *
 */

#ifndef __AGENT_MAIN_H
#define __AGENT_MAIN_H


/*************************INCLUDES******************************************/
/* for setsockopt and stuff */
#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <paths.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <grp.h>
#include <limits.h>
#include <netdb.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sysexits.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip_fw.h>
#include <net/route.h> /* def. of struct route */
#include <netinet/ip_dummynet.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
/* for setsockopt and stuff */

#include <stdio.h>
#include <unistd.h>
#include "event.h"
#include "tbdefs.h"
#include "log.h"


#if 0
#include "tbdb.h"
#endif


#if 0
/* these are defined in testbed/lib/libtb/tbdb.h, will include
   that directory later and then throw out these defines
 */
#define TBDB_OBJECTTYPE_LINK	"LINK"
#define TBDB_EVENTTYPE_UP	"UP"
#define TBDB_EVENTTYPE_DOWN	"DOWN"
#define TBDB_EVENTTYPE_MODIFY   "MODIFY"

#endif
/*************************INCLUDES******************************************/

/**************************DEFINES******************************************/

#define MAX_LINE_LENGTH 512
#define MAX_LINKS       256 /* Virtual interfaces */
/**************************DEFINES******************************************/


/*************************USER DEFINED TYPES********************************/

/* flag bits which indicate pipe parameters*/
#define PIPE_QSIZE_IN_BYTES     0x0001
#define PIPE_Q_IS_RED           0x0002
#define PIPE_Q_IS_GRED          0x0004
#define PIPE_HAS_FLOW_MASK      0x0008

/* Parameters for RED and gentle RED*/
typedef struct {
  /* from ip_dummynet.h*/
  double w_q ;               /* queue weight */
  int max_th ;            /* maximum threshold for queue (scaled) */
  int min_th ;            /* minimum threshold for queue (scaled) */
  double max_p ;             /* maximum value for p_b */
} structRed_params;

/* link status*/   
typedef enum {
  LINK_UP = 0,
  LINK_DOWN
}enumlinkstat;

/* Pipe parameter structures*/
typedef struct {
  int delay;  /* pipe delay*/
  int bw;  /* pipe bw*/
  int backfill; /*pramod-CHANGES, -add backfill to the pipe*/
  double plr; /* queue loss rate*/
  int q_size; /* queuq size in slots/bytes*/
  structRed_params red_gred_params; /* red/gred params*/
  struct ipfw_flow_id id ; /* flow mask of the pipe*/
  int buckets;  /* number of buckets*/
  int n_qs; /* number of dynamic queues */
  u_short flags_p; 
}structpipe_params, *structpipe_params_t;

/* This structure maps the linkname (eg. link0 ) to the physical
   interfaces(eg. fxp0) and to the pipe numbers created at the start of
   the experiment.
   Later as events are recd. this table tells which pipe configuration should
   be modified
 */

typedef struct {
char              *linkname; /*link0, link1 etc*/
int		  islan;  /* 1 if a lan, 0 if a duplex link */
int		  numpipes;  /* 1 if a simplex pipe, 2 if a duplex pipe */
char              *interfaces[2];/* fxp0, fxp1 etc*/
char		  *vnodes[2]; /* nodeA, nodeB*/
char		  linkvnodes[2][256]; /* link0-nodeA, link0-nodeB*/
char              *linktype; /*simplex, duplex */
int               pipes[2]; /* array of pipe numbers*/
structpipe_params params[2]; /* params for the two pipes*/
enumlinkstat      stat;      /* link status : UP/DOWN*/
}structlink_map, * structlink_map_t;


/*************************USER DEFINED TYPES********************************/


/******************************Function prototypes******************************/

void usage(char *);
void fill_tuple(address_tuple_t);
void agent_callback(event_handle_t handle,
		    event_notification_t notification, void *data);
void handle_pipes (char *objname, char *eventtype, event_notification_t
		   ,event_handle_t, int);
int  checkevent (char *);
void handle_link_up(char * linkname, int l_index);
void handle_link_down(char * linkname, int l_index);
void handle_link_modify(char * linkname, int l_index,
			event_handle_t handle,
			event_notification_t notification);
int  get_link_params(int l_index);
void get_flowset_params(struct dn_flow_set*, int, int);
void get_queue_params(struct dn_flow_set*,int, int);
void set_link_params(int l_index, int blackhole, int);
int  get_new_link_params(int l_index, event_handle_t handle,
			 event_notification_t notification,
			 int *);
void dump_link_map();
int  get_link_info();
/******************************Function prototypes******************************/

#endif /*__AGENT_MAIN_H*/
	 
