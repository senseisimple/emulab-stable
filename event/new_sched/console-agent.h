/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file console-agent.h
 */

#ifndef _console_agent_h
#define _console_agent_h

#include "event-sched.h"
#include "local-agent.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TIPLOGDIR "/var/log/tiplogs"

/**
 * A local agent structure for Console objects.
 */
struct _console_agent {
	struct _local_agent ca_local_agent;	/*< Local agent base. */
	off_t ca_mark;
};

/**
 * Pointer type for the _console_agent structure.
 */
typedef struct _console_agent *console_agent_t;

/**
 * Create a console agent and intialize it with the default values.
 *
 * @return An initialized console agent object.
 */
console_agent_t create_console_agent(void);

/**
 * Check a console agent object against the following invariants:
 *
 * @li na_local_agent is sane
 *
 * @param na An initialized console agent object.
 * @return True.
 */
int console_agent_invariant(console_agent_t na);

#ifdef __cplusplus
}
#endif

#endif
