/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004, 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file local-agent.h
 *
 * Header file for event agents that are managed internally by the event
 * scheduler.
 */

#ifndef _local_agent_h
#define _local_agent_h

#include <pthread.h>

#include "listNode.h"
#include "event-sched.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	LAB_IMMEDIATE,	/*< Flag number used to indicate that the agent uses
			  an "immediate" event handler function instead of
			  a separate thread. */
	LAB_LOOPING,	/*< Flag number used to indicate that the agent is
                          alive and in its event loop. */
	LAB_MULTIPLE,	/*< Flag number used to indicate that the agent can
			  handle a single event for multiple agents in a
			  group. */

	LAB_MAX
};

enum {
	LAF_IMMEDIATE = (1L << LAB_IMMEDIATE),
	LAF_LOOPING = (1L << LAB_LOOPING),
	LAF_MULTIPLE = (1L << LAB_MULTIPLE),
};

/**
 * Base structure for local event agents.
 */
struct _local_agent {
	struct lnNode la_link;
	unsigned long la_flags;		/*< Storage for LAF_ flags. */
	struct lnList la_queue;		/*< Queue for events to be handled. */
	pthread_mutex_t la_mutex;	/*< Mutex for this object. */
	pthread_cond_t la_cond;		/*< Condition variable for the queue */
	event_handle_t la_handle;	/*< Event system handle. */
	struct agent *la_agent;		/*< Reference to the agent handle. */
	int (*la_expand)(struct _local_agent *la, sched_event_t *se);
	union {
		/**
		 *
		 */
		void *(*looper)(void *arg);

		/**
		 *
		 */
		int (*immediate)(struct _local_agent *la, sched_event_t *se);
	} la_funcs;
};

#define la_looper la_funcs.looper
#define la_immediate la_funcs.immediate

/**
 * Pointer type for the _local_agent structure.
 */
typedef struct _local_agent *local_agent_t;

/**
 * Check a local agent object against the following invariants:
 *
 * @li la_flags only contains valid flags.
 * @li la_queue is a valid list header.
 * @li la_handle != NULL
 * @li la_looper != NULL
 *
 * If the object does not pass these tests an assert will be blown.
 *
 * @param la The initialized local agent object to check.
 * @return True
 */
int local_agent_invariant(local_agent_t la);

/**
 * Initialize a local agent object.  This function only does a partial
 * initialization of the object, the caller must still fill out the la_handle
 * and la_looper fields of the object.
 *
 * @param la The uninitialized local agent object that should be initialized.
 * @return Zero on success, an error number otherwise.
 */
int local_agent_init(local_agent_t la);

/**
 * Queue an event on a local agent's event queue.  This will also start the
 * looper thread if it is not already going.
 *
 * @param la The local agent that can handle the given event.
 * @param se Pointer to the event to be handled.  If the event is successfully
 * queued, the notification field will be set to NULL since the notification is
 * now owned by the local agent thread.
 * @return Zero on success, an error number otherwise.
 */
int local_agent_queue(local_agent_t la, sched_event_t *se);

/**
 * The default timeout (1 min.) for the local_agent_dequeue function.
 */
#define DEFAULT_TIMEOUT 60 /* seconds */

/**
 * Dequeue an event so that it can be handled or wait for a timeout.
 *
 * @param la The local agent object where events should be dequeued from.
 * @param timeout The number of seconds to wait for an event on the queue
 * before returning, or zero to wait for the default time.
 * @param se_out A pointer to a sched_event_t object.
 * @return Zero on success, an error number otherwise.  If the error is
 * ETIMEDOUT, the caller is expected to cleanup and exit the thread.
 */
int local_agent_dequeue(local_agent_t la,
			time_t timeout,
			sched_event_t *se_out);

#ifdef __cplusplus
}
#endif

#endif
