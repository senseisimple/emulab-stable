/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004, 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file node-agent.h
 */

#ifndef _node_agent_h
#define _node_agent_h

#include "event-sched.h"
#include "local-agent.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NODE_DUMP_DIR "logs/%s"
#define NODE_DUMP_FILE "logs/%s/node-control.%d"

/**
 * A local agent structure for Node objects.
 */
struct _node_agent {
	struct _local_agent na_local_agent;	/*< Local agent base. */
};

/**
 * Pointer type for the _node_agent structure.
 */
typedef struct _node_agent *node_agent_t;

/**
 * Create a node agent and intialize it with the default values.
 *
 * @return An initialized node agent object.
 */
node_agent_t create_node_agent(void);

/**
 * Check a node agent object against the following invariants:
 *
 * @li na_local_agent is sane
 *
 * @param na An initialized node agent object.
 * @return True.
 */
int node_agent_invariant(node_agent_t na);

#ifdef __cplusplus
}
#endif

#endif
