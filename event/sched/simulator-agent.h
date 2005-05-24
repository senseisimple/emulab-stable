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
	SA_RDK_CONFIG,	/*< Config data for the tail of the report. */

	SA_RDK_MAX	/*< The maximum number of message types. */
} sa_report_data_kind_t;

enum {
	SA_RDB_NEWLINE,
};

enum {
	SA_RDF_NEWLINE = (1L << SA_RDB_NEWLINE),
};

enum {
	SAB_TIME_STARTED,
	SAB_STABLE,
};

enum {
	SAF_TIME_STARTED = (1L << SAB_TIME_STARTED),
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
	struct timeval sa_time_start;		/*< */
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

/**
 * Add some message/log data that should be included in the generated report
 * for this simulator.  The data is appended to any previous additions along
 * with a newline if it does not have one.
 *
 * @param sa The simulator agent object where the data is kept.
 * @param rdk The type of data being added.
 * @param data The data to add, should just be ASCII text.
 * @return Zero on success, -1 otherwise.
 *
 * @see send_report
 */
int add_report_data(simulator_agent_t sa,
		    sa_report_data_kind_t rdk,
		    char *data,
		    unsigned long flags);

#ifdef __cplusplus
}
#endif

#endif
