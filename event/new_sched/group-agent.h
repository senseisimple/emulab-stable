/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file group-agent.h
 */

#ifndef _group_agent_h
#define _group_agent_h

#include "listNode.h"
#include "local-agent.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A local agent structure for event groups.
 */
struct _group_agent {
	struct _local_agent ga_local_agent;	/*< Local agent base. */
	int ga_token;				/*< The token to return in the
						  complete event for this
						  agent. */
	struct agent **ga_agents;		/*< The list of agents in this
						  group. */
	unsigned int ga_count;			/*< The number of agents in
						  this group. */
	int ga_remaining;			/*< The number of agents that
						  still need to send a
						  complete event, or -1 if the
						  group is inactive. */
	int ga_error;
};

/**
 * Pointer type for the _group_agent structure.
 */
typedef struct _group_agent *group_agent_t;

/**
 * Create a group agent and initialize it with the default values.
 *
 * @return An initialized group agent object.
 */
group_agent_t create_group_agent(struct agent *agent);

/**
 * Check a group agent object against the following invariants:
 *
 * @li ga_local_agent is sane
 * @li if ga_count > 0: ga_agents != NULL && ga_agents[N].ga_agent != NULL
 * @li if ga_count == 0: ga_agents == NULL && ga_remaining == -1
 *
 * @param ga A fully initialized group agent.
 */
int group_agent_invariant(group_agent_t ga);

/**
 * Append a regular agent to a group agent.
 *
 * @param ga The group that the agent should be added to.
 * @param agent The agent to add to the group.
 */
int group_agent_append(group_agent_t ga, struct agent *agent);

int group_agent_handle_complete(event_handle_t handle,
				struct lnList *list,
				struct agent *agent,
				int ctoken,
				int agerror);

#ifdef __cplusplus
}
#endif

#endif
