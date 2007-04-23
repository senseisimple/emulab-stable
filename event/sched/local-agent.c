/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004, 2005, 2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file local-agent.c
 *
 * Implementation file for event agents that are managed internally by the
 * event scheduler.
 */

#include "config.h"

#include <errno.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include "local-agent.h"

/**
 * Structure for events placed on an agent's queue.
 */
struct _local_agent_event {
	struct lnNode lae_link;		/*< Linked list node. */
	sched_event_t lae_event;	/*< Copy of the event structure. */
};

/**
 * Pointer type for the _local_agent_event structure.
 */
typedef struct _local_agent_event *local_agent_event_t;

int local_agent_invariant(local_agent_t la)
{
	assert(la != NULL);
	assert(!(la->la_flags & ~(LAF_IMMEDIATE|LAF_LOOPING|LAF_MULTIPLE)));
	lnCheck(&la->la_queue);
	assert(la->la_handle != NULL);
	assert(la->la_looper != NULL);
	assert(la->la_agent != NULL);

	return 1;
}

int local_agent_init(local_agent_t la)
{
	int retval;

	assert(la != NULL);

	if ((retval = pthread_mutex_init(&la->la_mutex, NULL)) != 0) {
	}
	else if ((retval = pthread_cond_init(&la->la_cond, NULL)) != 0) {
		if (pthread_mutex_destroy(&la->la_mutex) != 0)
			assert(0);
	}
	else {
		la->la_link.ln_Name = NULL;
		la->la_flags = 0;
		lnNewList(&la->la_queue);
		la->la_handle = NULL;
		la->la_looper = NULL;
		la->la_agent = NULL;
		la->la_expand = NULL;
		
		retval = 0;
	}
	
	return retval;
}

int local_agent_queue(local_agent_t la, sched_event_t *se)
{
	local_agent_event_t lae;
	int retval;

	assert(la != NULL);
	assert(local_agent_invariant(la));
	assert(se != NULL);
	assert(se->length >= 1);

	if (la->la_flags & LAF_IMMEDIATE) {
		retval = la->la_immediate(la, se);
	}
	else if ((lae = malloc(sizeof(struct _local_agent_event))) == NULL) {
		retval = ENOMEM;
	}
	else
	{
		pthread_t pt;
		
		lae->lae_event = *se;
		se->agent.s = NULL;
		se->agent.m = NULL;
		se->notification = NULL;
		se->length = 0;
		
		if (pthread_mutex_lock(&la->la_mutex) != 0)
			assert(0);

		lnAddTail(&la->la_queue, &lae->lae_link);
		if (la->la_flags & LAF_LOOPING) {
			// thread is already running, nothing to do...
		}
		else if ((retval = pthread_create(&pt,
						  NULL,
						  la->la_looper,
						  la)) == 0) {
			la->la_flags |= LAF_LOOPING;
			pthread_detach(pt);
		}

		if (pthread_mutex_unlock(&la->la_mutex) != 0)
			assert(0);

		if (pthread_cond_signal(&la->la_cond) != 0)
			assert(0);

		retval = 0;
	}

	return retval;
}

int local_agent_dequeue(local_agent_t la,
			time_t timeout,
			sched_event_t *se_out)
{
	local_agent_event_t lae = NULL;
	struct timespec ts;
	int retval = 0;

	assert(local_agent_invariant(la));
	assert(timeout >= 0);
	assert(timeout < (60 * 60 * 24));
	assert(se_out != NULL);

	if (timeout == 0)
		timeout = DEFAULT_TIMEOUT;

	if (pthread_mutex_lock(&la->la_mutex) != 0)
		assert(0);

	gettimeofday((struct timeval *)&ts, NULL);
	ts.tv_sec += timeout;
	
	while ((retval == 0) &&
	       (lae = (local_agent_event_t)lnRemHead(&la->la_queue)) == NULL) {
		if ((retval = pthread_cond_timedwait(&la->la_cond,
						     &la->la_mutex,
						     &ts)) != 0) {
			switch (retval)
			{
			case ETIMEDOUT:
				la->la_flags &= ~LAF_LOOPING;
				break;
			default:
				perror("pthread_cond_timedwait");
				assert(0);
			}
		}
	}
	
	if (pthread_mutex_unlock(&la->la_mutex) != 0)
		assert(0);

	if (lae != NULL) {
		*se_out = lae->lae_event;
		
		free(lae);
		lae = NULL;

		assert(se_out->length > 0);
	}

	return retval;
}
