/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004, 2005, 2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file group-agent.c
 */

#include "config.h"

#include <string.h>
#include <errno.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include "group-agent.h"

/**
 * Event expansion callback, this function will expand the given event to the
 * same number of events as agents in this group.
 *
 * @param la This group agent object.
 * @param se The sched_event_t to expand.
 * @return Zero on success, -1 otherwise.
 */
static int group_agent_expand(local_agent_t la, sched_event_t *se);

/**
 * The "immediate" callback for handling group agent events, however, since
 * there are no group agent events, this will blow an assert.
 *
 * @param la This group agent object.
 * @param se The sched_event_t to handle.
 * @return Zero on success, -1 otherwise.
 */
static int group_agent_immediate(local_agent_t la, sched_event_t *se);

group_agent_t create_group_agent(struct agent *agent)
{
	struct agent **agent_array = NULL;
	group_agent_t ga, retval;
	
	if ((ga = malloc(sizeof(struct _group_agent))) == NULL) {
		retval = NULL;
		errno = ENOMEM;
	}
	else if ((agent_array = malloc(sizeof(struct agent *))) == NULL) {
		retval = NULL;
		errno = ENOMEM;
	}
	else if (local_agent_init(&ga->ga_local_agent) != 0) {
		retval = NULL;
	}
	else {
		ga->ga_local_agent.la_link.ln_Name = agent->name;
		ga->ga_local_agent.la_agent = agent;
		ga->ga_local_agent.la_flags |= LAF_IMMEDIATE;
		ga->ga_local_agent.la_expand = group_agent_expand;
		ga->ga_local_agent.la_immediate = group_agent_immediate;
		ga->ga_token = ~0;
		ga->ga_agents = agent_array;
		agent_array = NULL;
		ga->ga_agents[0] = agent;
		ga->ga_count = 0;
		ga->ga_remaining = -1;

		retval = ga;
		ga = NULL;
	}

	free(agent_array);
	agent_array = NULL;

	free(ga);
	ga = NULL;
	
	return retval;
}

int group_agent_invariant(group_agent_t ga)
{
	char *type;
	int lpc;
	
	assert(ga != NULL);
	assert(local_agent_invariant(&ga->ga_local_agent));

	assert(ga->ga_agents != NULL);
	type = ga->ga_agents[1]->objtype;
	for (lpc = 1; lpc <= ga->ga_count; lpc++) {
		assert(ga->ga_agents[lpc] != NULL);
		assert(strcmp(ga->ga_agents[lpc]->objtype, type) == 0);
	}

	return 1;
}

int group_agent_append(group_agent_t ga, struct agent *agent)
{
	struct agent **new_agents;
	int retval;
	
	assert(ga != NULL);
	assert(group_agent_invariant(ga));
	assert(agent != NULL);
	assert(agent_invariant(agent));

	if ((new_agents = realloc(ga->ga_agents,
				  (ga->ga_count + 2) *
				  sizeof(struct agent *))) == NULL) {
		retval = -1;
		errno = ENOMEM;
	}
	else {
		ga->ga_agents = new_agents;
		new_agents = NULL;
		
		ga->ga_agents[1 + ga->ga_count] = agent;
		ga->ga_count += 1;
		
		retval = 0;
	}

	assert(group_agent_invariant(ga));

	return retval;
}

int group_agent_handle_complete(event_handle_t handle,
				struct lnList *list,
				struct agent *agent,
				int ctoken,
				int agerror)
{
	group_agent_t ga;
	int retval = 0;

	assert(handle != NULL);
	assert(list != NULL);
	lnCheck(list);
	assert(agent != NULL);
	assert(agent_invariant(agent));

	ga = (group_agent_t)list->lh_Head;
	while (ga->ga_local_agent.la_link.ln_Succ != NULL) {
		int lpc, found = 0;

		assert(group_agent_invariant(ga));

		if (ga->ga_remaining > 0) {
			for (lpc = 1; (lpc <= ga->ga_count) && !found; lpc++) {
#if 0
				printf("cmp %s %s  %d %d\n",
				       agent->name,
				       ga->ga_agents[lpc]->name,
				       ctoken,
				       ga->ga_token);
#endif
				if ((agent == ga->ga_agents[lpc]) &&
				    (ga->ga_token == ctoken)) {
					ga->ga_remaining -= 1;
					if (agerror > ga->ga_error) {
						ga->ga_error = agerror;
					}
					
					found = 1;
				}
			}
		}

		if (ga->ga_remaining == 0) {
#if 0
			printf("group complete %s\n",
			       ga->ga_local_agent.la_link.ln_Name);
#endif
			event_do(handle,
				 EA_Experiment, pideid,
				 EA_Type, TBDB_OBJECTTYPE_GROUP,
				 EA_Name, ga->ga_local_agent.la_link.ln_Name,
				 EA_Event, TBDB_EVENTTYPE_COMPLETE,
				 EA_ArgInteger, "ERROR", ga->ga_error,
				 EA_ArgInteger, "CTOKEN", ga->ga_token,
				 EA_TAG_DONE);
			
			ga->ga_token = ~0;
			ga->ga_remaining = -1;

			retval += 1;
		}
		else {
#if 0
			printf("group remains %d\n", ga->ga_remaining);
#endif
		}
		
		ga = (group_agent_t)ga->ga_local_agent.la_link.ln_Succ;
	}

	return retval;
}

static int group_agent_expand(local_agent_t la, sched_event_t *se)
{
	group_agent_t ga = (group_agent_t)la;
	event_notification_t en;
	event_handle_t handle;
	int retval;
	
	assert(la != NULL);
	assert(group_agent_invariant(ga));
	assert(ga->ga_count > 0);
	assert(se != NULL);
	assert(se->length == 1);
	assert(se->flags & SEF_SINGLE_HANDLER);
	assert(se->notification != NULL);

	handle = la->la_handle;
	en = se->notification;

	/*
	 * The agents must be the same type, so we update the object type once
	 * for all the agents instead of individually.
	 */
	event_notification_clear_objtype(handle, en);
	event_notification_set_objtype(handle,
				       en,
				       ga->ga_agents[1]->objtype);
	event_notification_insert_hmac(handle, en);

	if (ga->ga_remaining != -1) {
		error("group %s is already active and waiting for COMPLETEs\n",
		      la->la_agent->name);
		errno = EBUSY;
		retval = -1;
	}
	else if (ga->ga_count == 1) {
		retval = 0;
	}
	else {
		se->agent.m = ga->ga_agents;
		se->length = ga->ga_count;
		
		retval = 0;
	}
	
	return retval;
}

static int group_agent_immediate(local_agent_t la, sched_event_t *se)
{
	group_agent_t ga = (group_agent_t)la;
	event_handle_t handle;
	int retval;

	assert(la != NULL);
	assert(group_agent_invariant(ga));
	assert(se != NULL);

	handle = la->la_handle;

	if (ga->ga_token != ~0) {
		warning("group already active\n");
		// XXX what to do? need to let STOPs go through...
	}
	else if (se->flags & SEF_SENDS_COMPLETE) {
		/*
		 * Sending this type of event will elicit a COMPLETE
		 * event from the agents, so we need to update the
		 * group object so it will wait for all the COMPLETEs
		 * before sending its own.
		 */
		event_notification_get_int32(handle,
					     se->notification,
					     "TOKEN",
					     &ga->ga_token);
		ga->ga_error = 0;
		ga->ga_remaining = ga->ga_count;
	}

	if (ga->ga_agents[1]->handler != NULL) {
		if (ga->ga_agents[1]->handler->la_flags & LAF_MULTIPLE) {
			local_agent_queue(ga->ga_agents[1]->handler, se);
		}
		else {
			int lpc;

			for (lpc = 1; lpc <= ga->ga_count; lpc++) {
				local_agent_queue(ga->ga_agents[lpc]->handler,
						  se);
			}
		}
		
		retval = 0;
	}
	else {
		event_notification_t en;
		int lpc;

		en = se->notification;
		for (lpc = 1; lpc <= ga->ga_count; lpc++) {
			event_notification_clear_objname(handle, en);
			event_notification_set_objname(
				handle, en, ga->ga_agents[lpc]->name);
			event_notification_insert_hmac(handle, en);

			event_notify(handle, en);
		}
		
		retval = 0;
	}
	
	return retval;
}
