/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003 University of Utah and the Flux Group.
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

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif /* MAXHOSTNAMELEN */

/* Scheduler-internal representation of an event. */
typedef struct sched_event {
    event_notification_t notification; /* event notification */
    struct timeval time;        /* event firing time */
    int simevent;		/* A simulator event, dummy */
} sched_event_t;

/*
 * Debugging and tracing definitions:
 */
#define ERROR(fmt,...) error(__FUNCTION__ ": " fmt, ## __VA_ARGS__)
#ifdef DEBUG
#define TRACE(fmt,...) info(__FUNCTION__ ": " fmt, ## __VA_ARGS__)
#else
#define TRACE(fmt,...)
#endif /* DEBUG */
extern int debug;

/*
 * Function prototypes:
 */

/* queue.c */
void sched_event_init(void);
int sched_event_enqueue(sched_event_t event);
int sched_event_dequeue(sched_event_t *event, int wait);
void sched_event_queue_dump(FILE *fp);

#endif /* __SCHED_H__ */
