/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004, 2005, 2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file timeline-agent.c
 */

#include "config.h"

#include <string.h>
#include <errno.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include "rpc.h"
#include "timeline-agent.h"

/**
 *
 */
static int timeline_agent_expand(local_agent_t la, sched_event_t *se);

/**
 *
 */
static int timeline_agent_immediate(local_agent_t la, sched_event_t *se);

/**
 *
 */
static int sequence_agent_immediate(local_agent_t la, sched_event_t *se);

struct _ts_agent *create_timeline_agent(ta_kind_t tk)
{
	struct _ts_agent *ta, *retval;

	if ((ta = malloc(sizeof(struct _ts_agent))) == NULL) {
		retval = NULL;
		errno = ENOMEM;
	}
	else if (local_agent_init(&ta->ta_local_agent) != 0) {
		retval = NULL;
	}
	else {
		ta->ta_local_agent.la_flags |= LAF_IMMEDIATE;
		ta->ta_local_agent.la_expand = timeline_agent_expand;
		switch (tk) {
		case TA_TIMELINE:
			ta->ta_local_agent.la_immediate =
				timeline_agent_immediate;
			break;
		case TA_SEQUENCE:
			ta->ta_local_agent.la_immediate =
				sequence_agent_immediate;
			break;
		default:
			assert(0);
			break;
		}
		ta->ta_events = NULL;
		ta->ta_count = 0;
		ta->ta_current_event = -1;

		retval = ta;
		ta = NULL;
	}

	free(ta);
	ta = NULL;
	
	return retval;
}

int timeline_agent_invariant(struct _ts_agent *ta)
{
	assert(ta != NULL);
	assert(local_agent_invariant(&ta->ta_local_agent));
	if (ta->ta_count > 0) {
		assert(ta->ta_events != NULL);
	}
	else {
		assert(ta->ta_events == NULL);
		assert(ta->ta_current_event == -1);
	}
	
	return 1;
}

static int timeline_agent_expand(local_agent_t la, sched_event_t *se)
{
	struct _ts_agent *ta = (struct _ts_agent *)la;
	event_handle_t handle;
	int lpc, retval = 0;

	assert(la != NULL);
	assert(timeline_agent_invariant(ta));
	assert(se != NULL);

	handle = la->la_handle;

	// XXX only do this when not active.
	for (lpc = 0; lpc < ta->ta_count; lpc++) {
		sched_event_t *sub_se = &ta->ta_events[lpc];

		event_notification_remove(handle,
					  sub_se->notification,
					  "TOKEN");
		event_notification_put_int32(handle,
					     sub_se->notification,
					     "TOKEN",
					     next_token++);
		event_notification_insert_hmac(handle, sub_se->notification);
	}

	event_notification_clear_objtype(handle, se->notification);
	event_notification_set_objtype(handle,
				       se->notification,
				       la->la_agent->objtype);
	event_notification_insert_hmac(handle, se->notification);
	
	return retval;
}

int timeline_agent_append(struct _ts_agent *ta, sched_event_t *se)
{
	sched_event_t *new_events;
	int retval;

	assert(ta != NULL);
	assert(timeline_agent_invariant(ta));
	assert(se != NULL);
	assert(se->notification != NULL);
	
	if ((new_events = realloc(ta->ta_events,
				  (ta->ta_count + 1) *
				  sizeof(sched_event_t))) == NULL) {
		retval = -1;
		errno = ENOMEM;
	}
	else {
		ta->ta_events = new_events;
		ta->ta_events[ta->ta_count] = *se;
		ta->ta_count += 1;
		
		retval = 0;
	}
	
	return retval;
}

int sequence_agent_enqueue_next(sequence_agent_t sa)
{
	int done = 0, retval = 0;
	event_handle_t handle;
	struct timeval now;

	assert(sa != NULL);
	assert(timeline_agent_invariant(sa));
	assert(sa->ta_count > 0);
	assert(sa->ta_current_event >= 0);
	assert(sa->ta_current_event <= sa->ta_count);
	
	handle = sa->ta_local_agent.la_handle;
	
	gettimeofday(&now, NULL);

	if (sa->ta_current_event == sa->ta_count)
		done = 1;
	
	while (!done) {
		struct sched_event *se;
		struct timeval then;

		se = &sa->ta_events[sa->ta_current_event];
		timeradd(&now, &se->time, &then);
		retval = sched_event_enqueue_copy(handle, se, &then);
		now = then;

		if (se->flags & SEF_SENDS_COMPLETE) {
			done = 1;
		}
		else if ((sa->ta_current_event + 1) < sa->ta_count) {
			sa->ta_current_event += 1;
		}
		else {
			sa->ta_current_event += 1;
			done = 1;
		}
	}

	if (done && (sa->ta_current_event == sa->ta_count)) {
		event_do(handle,
			 EA_Experiment, pideid,
			 EA_Type, TBDB_OBJECTTYPE_SEQUENCE,
			 EA_Name, sa->ta_local_agent.la_link.ln_Name,
			 EA_Event, TBDB_EVENTTYPE_COMPLETE,
			 EA_ArgInteger, "ERROR", 0,
			 EA_ArgInteger, "CTOKEN", sa->ta_token,
			 EA_TAG_DONE);
		sa->ta_current_event = -1;
		sa->ta_token = ~0;
	}
	
	return retval;
}

int sequence_agent_handle_complete(event_handle_t handle,
				   struct lnList *list,
				   struct agent *agent,
				   int ctoken,
				   int agerror)
{
	sequence_agent_t sa;
	int retval = 0;
	
	assert(handle != NULL);
	assert(list != NULL);
	assert(agent != NULL);

	sa = (sequence_agent_t)list->lh_Head;
	while (sa->ta_local_agent.la_link.ln_Succ != NULL) {
		struct sched_event *seq_se = NULL;
		struct agent *ag = NULL;
		int token = -1;

		if (sa->ta_current_event >= 0) {
			seq_se = &sa->ta_events[sa->ta_current_event];
			if (seq_se->length > 1)
				ag = seq_se->agent.m[0];
			else
				ag = seq_se->agent.s;
//info("Am I failing here?\n");
			event_notification_get_int32(handle,
						     seq_se->notification,
						     "TOKEN",
						     &token);
//info("End question\n");
		}
		
		if ((ag == NULL) || (ag != agent) || (ctoken != token)) {
#if 0
			printf("seq no match %s %d %p %p %d %d\n",
			       sa->ta_local_agent.la_link.ln_Name,
			       sa->ta_current_event,
			       ag, agent,
			       ctoken,
			       token);
#endif
		}
		else if (agerror == 0) {
#if 0
			printf("seq complete %s %d\n",
			       sa->ta_local_agent.la_link.ln_Name,
			       sa->ta_current_event);
#endif
			sa->ta_current_event += 1;
			sequence_agent_enqueue_next(sa);

			retval += 1;
		}
		else {
			event_do(handle,
				 EA_Experiment, pideid,
				 EA_Type, TBDB_OBJECTTYPE_SEQUENCE,
				 EA_Name, sa->ta_local_agent.la_link.ln_Name,
				 EA_Event, TBDB_EVENTTYPE_COMPLETE,
				 EA_ArgInteger, "ERROR", agerror,
				 EA_ArgInteger, "CTOKEN", sa->ta_token,
				 EA_TAG_DONE);
			sa->ta_current_event = -1;

			retval += 1;
		}
		sa = (sequence_agent_t)sa->ta_local_agent.la_link.ln_Succ;
	}
	
	return retval;
}

static void delete_events(struct _ts_agent *ta)
{
	event_handle_t handle;
	int lpc;

	assert(ta != NULL);
	assert(timeline_agent_invariant(ta));

	handle = ta->ta_local_agent.la_handle;
	
	for (lpc = 0; lpc < ta->ta_count; lpc++) {
		sched_event_free(handle, &ta->ta_events[lpc]);
	}
	free(ta->ta_events);
	ta->ta_events = NULL;
	
	ta->ta_count = 0;
	ta->ta_token = ~0;
	ta->ta_current_event = -1;
	
	assert(timeline_agent_invariant(ta));
}

static int timeline_agent_immediate(local_agent_t la, sched_event_t *se)
{
	timeline_agent_t ta = (timeline_agent_t)la;
	char evtype[TBDB_FLEN_EVEVENTTYPE];
	int retval;

	assert(la != NULL);
	assert(timeline_agent_invariant(ta));
	assert(se != NULL);
	assert(se->notification != NULL);

	if (! event_notification_get_eventtype(la->la_handle,
					       se->notification,
					       evtype, sizeof(evtype))) {
		error("could not get event type from notification %p\n",
		      se->notification);
		retval = -1;
	}
	else if (strcmp(evtype, TBDB_EVENTTYPE_START) == 0 ||
		 strcmp(evtype, TBDB_EVENTTYPE_RUN) == 0) {
		struct timeval now, then;
		int token, lpc;

		if (strncmp(la->la_agent->name, "__", 2) != 0) {
			RPC_grab();
			RPC_notifystart(pid, eid, la->la_agent->name, 1);
			RPC_drop();
		}
		
		event_notification_get_int32(la->la_handle,
					     se->notification,
					     "TOKEN",
					     (int32_t *)&token);
		gettimeofday(&now, NULL);
		then = now;
		for (lpc = 0; lpc < ta->ta_count; lpc++) {
			timeradd(&now,
				 &ta->ta_events[lpc].time,
				 &then);
			sched_event_enqueue_copy(la->la_handle,
						 &ta->ta_events[lpc],
						 &then);
		}
		
		event_do(la->la_handle,
			 EA_Experiment, pideid,
			 EA_When, &then,
			 EA_Type, TBDB_OBJECTTYPE_TIMELINE,
			 EA_Name, la->la_link.ln_Name,
			 EA_Event, TBDB_EVENTTYPE_COMPLETE,
			 EA_ArgInteger, "ERROR", 0,
			 EA_ArgInteger, "CTOKEN", token,
			 EA_TAG_DONE);
		
		retval = 0;
	}
	else if (strcmp(evtype, TBDB_EVENTTYPE_CLEAR) == 0) {
		delete_events(ta);
		
		retval = 0;
	}
	else {
		error("unknown timeline event %s\n", evtype);
		retval = -1;
	}

	return retval;
}

static int sequence_agent_immediate(local_agent_t la, sched_event_t *se)
{
	sequence_agent_t sa = (sequence_agent_t)la;
	char evtype[TBDB_FLEN_EVEVENTTYPE];
	event_handle_t handle;
	int retval;
	
	assert(la != NULL);
	assert(timeline_agent_invariant(sa));
	assert(se != NULL);
	assert(se->notification != NULL);

	handle = la->la_handle;
	
	if (! event_notification_get_eventtype(la->la_handle,
					       se->notification,
					       evtype, sizeof(evtype))) {
		error("could not get event type from notification %p\n",
		      se->notification);
		retval = -1;
	}
	else if (strcmp(evtype, TBDB_EVENTTYPE_START) == 0 ||
		 strcmp(evtype, TBDB_EVENTTYPE_RUN) == 0) {
		if (sa->ta_current_event != -1) {
			error("sequence %s is already active\n",
			      sa->ta_local_agent.la_link.ln_Name);
		}
		else if (sa->ta_count > 0) {
			if (strncmp(la->la_agent->name, "__", 2) != 0) {
				RPC_grab();
				RPC_notifystart(pid, eid,
						la->la_agent->name, 1);
				RPC_drop();
			}
		
			event_notification_get_int32(handle,
						     se->notification,
						     "TOKEN",
						     &sa->ta_token);
			
			sa->ta_current_event = 0;
			sequence_agent_enqueue_next(sa);
		}
		else {
			warning("sequence, %s, is empty\n",
				sa->ta_local_agent.la_link.ln_Name);
		}

		retval = 0;
	}
	else if (strcmp(evtype, TBDB_EVENTTYPE_RESET) == 0) {
		sa->ta_token = ~0;
		sa->ta_current_event = -1;

		retval = 0;
	}
	else if (strcmp(evtype, TBDB_EVENTTYPE_CLEAR) == 0) {
		delete_events(sa);

		retval = 0;
	}
	else {
		error("unknown sequence event %s\n", evtype);
		retval = -1;
	}

	return retval;
}
