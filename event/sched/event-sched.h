/*
 * sched.h --
 *
 *      This file contains definitions for the testbed event
 *      scheduler.
 *
 * @COPYRIGHT@
 *
 * $Id: event-sched.h,v 1.1 2001-12-04 15:15:32 imurdock Exp $
 */

#ifndef __SCHED_H__
#define __SCHED_H__

#include <stdio.h>
#include <sys/time.h>
#include <event.h>

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif /* MAXHOSTNAMELEN */

/* Scheduler-internal representation of an event. */
typedef struct sched_event {
    struct timeval time;        /* event firing time */
    char host[MAXHOSTNAMELEN];  /* host to send the event to, or "*"
                                 * for all hosts */
    event_type_t type;          /* event type */
} sched_event_t;

/*
 * Function prototypes:
 */

/* queue.c */
int sched_event_enqueue(sched_event_t event);
int sched_event_dequeue(sched_event_t *event);
void sched_event_queue_dump(FILE *fp);
void sched_event_queue_verify(void);

#endif /* __SCHED_H__ */
