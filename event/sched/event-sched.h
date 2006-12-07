/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * event-sched.h --
 *
 *      This file contains definitions for the testbed event
 *      scheduler.
 *
 */

#ifndef __SCHED_H__
#define __SCHED_H__

#include <stdio.h>
#include <sys/time.h>
#include "event.h"
#include "log.h"
#include "tbdefs.h"

#include "listNode.h"

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif /* MAXHOSTNAMELEN */

#define LOGDIR		"/local/logs"

#ifdef __cplusplus
extern "C" {
#endif

struct _local_agent;

struct agent {
	struct  lnNode link;
	char    name[TBDB_FLEN_EVOBJNAME];
	char    nodeid[TBDB_FLEN_NODEID];
	char    vnode[TBDB_FLEN_VNAME];
	char	objtype[TBDB_FLEN_EVOBJTYPE];
	char	ipaddr[32];
	struct _local_agent *handler;
};

extern struct lnList agents;

enum {
	SEB_COMPLETE_EVENT,
	SEB_TIME_START,
	SEB_SENDS_COMPLETE,
	SEB_SINGLE_HANDLER,
};

enum {
	/** Flag for events that are COMPLETEs and should not be forwarded. */
	SEF_COMPLETE_EVENT = (1L << SEB_COMPLETE_EVENT),
	SEF_TIME_START = (1L << SEB_TIME_START),
	/** Flag for events that will send back a COMPLETE. */
	SEF_SENDS_COMPLETE = (1L << SEB_SENDS_COMPLETE),
	SEF_SINGLE_HANDLER = (1L << SEB_SINGLE_HANDLER),
};

/* Scheduler-internal representation of an event. */
typedef struct sched_event {
	union {
		struct agent *s;
		struct agent **m;
	} agent;
	event_notification_t notification;
	struct timeval time;			/* event firing time */
	unsigned short length;
	unsigned short flags;
} sched_event_t;

extern char	pideid[BUFSIZ];
extern char	*pid, *eid;

extern int debug;
extern unsigned long next_token;

/*
 * Function prototypes:
 */

int agent_invariant(struct agent *agent);
int sends_complete(struct agent *agent, const char *evtype);

void sched_event_free(event_handle_t handle, sched_event_t *se);
int sched_event_prepare(event_handle_t handle, sched_event_t *se);
int sched_event_enqueue_copy(event_handle_t handle,
			     sched_event_t *se,
			     struct timeval *new_time);
void make_timestamp(char * buf, const struct timeval * t_timeval);

/* queue.c */
void sched_event_init(void);
int sched_event_enqueue(sched_event_t event);
int sched_event_dequeue(sched_event_t *event, int wait);
void sched_event_queue_dump(FILE *fp);

extern char build_info[];

#ifdef __cplusplus
}
#endif

#endif /* __SCHED_H__ */
