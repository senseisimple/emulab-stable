/* AGENT-MAIN-DUMMY.H*/

#ifndef _AGENT_MAIN_DUMMY_H_
#define _AGENT_MAIN_DUMMY_H_


#if 0
#include <ctype.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include "event.h"
#include <stdio.h>
#include <unistd.h>
#endif


#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
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

#define MAX_LINE_LENGTH 256
#define MAX_LINKS       4 /* 4 simplex or 2 duplex, since we have 4 interfaces
			     on delay nodes*/
/**************************DEFINES******************************************/


/*************************USER DEFINED TYPES********************************/

#define PIPE_QSIZE_IN_BYTES     0x0001
#define PIPE_Q_IS_RED           0x0002
#define PIPE_Q_IS_GRED          0x0004
#define PIPE_HAS_FLOW_MASK      0x0008


/* Parameters for RED and gentle RED*/
typedef struct {
  /* from ip_dummynet.h*/
  int w_q ;               /* queue weight (scaled) */
  int max_th ;            /* maximum threshold for queue (scaled) */
  int min_th ;            /* minimum threshold for queue (scaled) */
  int max_p ;             /* maximum value for p_b (scaled) */
} structRed_params;


/* Pipe parameter structures*/
typedef struct {
  int delay;  /* pipe delay*/
  int bw;  /* pipe bw*/
  int plr; /* queue loss rate*/
  int q_size; /* queuq size in slots/bytes*/
  structRed_params red_gred_params; /* red/gred params*/
#if REAL_WORLD
  struct ipfw_flow_id id ; /* flow mask of the pipe*/
#endif  
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
char              *interfaces[2];/* fxp0, fxp1 etc*/
char              *linktype; /*simplex, duplex */
int               pipes[2]; /* array of pipe numbers*/
structpipe_params params[2]; /* params for the two pipes*/
}structlink_map, * structlink_map_t;


/*************************USER DEFINED TYPES********************************/

/******************************Function prototypes******************************/
void usage(char *);
void fill_tuple(address_tuple_t);
void agent_callback(event_handle_t handle,
		    event_notification_t notification, void *data);
void handle_event(char* , char* , event_notification_t ,event_handle_t);
void handle_link_modify(char *objname, event_handle_t handle,
			event_notification_t notification);
void handle_link_up (char* linkname);
void handle_link_down (char* linkname);
int  check_object(char* objname);
int  search(char * objname);
void dump_link_map();
/******************************Function prototypes******************************/

#endif /*__AGENT_MAIN_DUMMY_H*/
	 
