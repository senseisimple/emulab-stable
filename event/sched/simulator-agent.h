/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004, 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file simulator-agent.h
 *
 * Header file for the SIMULATOR agent.
 */

#ifndef _simulator_agent_h
#define _simulator_agent_h

#include "event-sched.h"
#include "local-agent.h"
#include "error-record.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enumeration of the different categories of text in a report.
 */
typedef enum {
	SA_RDK_MESSAGE,	/*< Main message body, this is at the top of the report
			 and is intended for a plain english description of
			 the experiment. */
	SA_RDK_LOG,	/*< Log data for the tail of the report, used for
			  machine generated data mostly. */

	SA_RDK_MAX	/*< The maximum number of message types. */
} sa_report_data_kind_t;

enum {
	SAB_STABLE,
};

enum {
	SAF_STABLE = (1L << SAB_STABLE),
};

/**
 * A local agent structure for the NS Simulator object.
 */
struct _simulator_agent {
	struct _local_agent sa_local_agent;	/*< Local agent base. */
	unsigned long sa_flags;
	struct lnList sa_error_records;		/*< The error records that have
						  been collected over the
						  course of the experiment. */
	char *sa_report_data[SA_RDK_MAX];	/*< Different kinds of text to
						 include in the report. */
};

/**
 * Pointer type for the _simulator_agent structure.
 */
typedef struct _simulator_agent *simulator_agent_t;

/**
 * Create a simulator agent and initialize it with the default values.
 *
 * @return An initialized simulator agent.
 */
simulator_agent_t create_simulator_agent(void);

/**
 * Check a simulator agent object against the following invariants:
 *
 * @li sa_local_agent is sane
 * @li sa_error_records is sane
 *
 * @param sa An initialized simulator agent object.
 * @return True.
 */
int simulator_agent_invariant(simulator_agent_t sa);

#ifdef __cplusplus
}
#endif

#endif
