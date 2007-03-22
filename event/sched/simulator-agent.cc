/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004, 2005, 2006, 2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file simulator-agent.cc
 *
 * Implementation file for the SIMULATOR agent.
 */

#include "config.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "popenf.h"
#include "systemf.h"
#include "rpc.h"
#include "log.h"
#include "simulator-agent.h"

using namespace emulab;

/**
 * A "looper" function for the simulator agent that dequeues and processes
 * events destined for the Simulator object.  This function will be passed to
 * pthread_create when a new thread needs to be created to handle events.
 *
 * @param arg The simulator agent object to handle events for.
 * @return NULL
 *
 * @see send_report
 * @see add_report_data
 * @see local_agent_queue
 * @see local_agent_dequeue
 */
static void *simulator_agent_looper(void *arg);

simulator_agent_t create_simulator_agent(void)
{
	simulator_agent_t sa, retval;

	if ((sa = (simulator_agent_t)
	     malloc(sizeof(struct _simulator_agent))) == NULL) {
		retval = NULL;
		errno = ENOMEM;
	}
	else if (local_agent_init(&sa->sa_local_agent) != 0) {
		retval = NULL;
	}
	else {
		int lpc;
		
		sa->sa_local_agent.la_looper = simulator_agent_looper;
		lnNewList(&sa->sa_error_records);
		for (lpc = 0; lpc < SA_RDK_MAX; lpc++)
			sa->sa_report_data[lpc] = NULL;
		
		retval = sa;
		sa = NULL;
	}

	free(sa);
	sa = NULL;

	return retval;
}

int simulator_agent_invariant(simulator_agent_t sa)
{
	assert(sa != NULL);
	assert(local_agent_invariant(&sa->sa_local_agent));
	lnCheck(&sa->sa_error_records);
	
	return 1;
}

int add_report_data(simulator_agent_t sa,
		    sa_report_data_kind_t rdk,
		    char *data,
		    unsigned long flags)
{
	char *new_data;
	int retval;

	assert(sa != NULL);
	assert(rdk >= 0);
	assert(rdk < SA_RDK_MAX);
	assert(data != NULL);

	if ((new_data = (char *)realloc(sa->sa_report_data[rdk],
					((sa->sa_report_data[rdk] != NULL) ?
					 strlen(sa->sa_report_data[rdk]) : 0) +
					strlen(data) +
					1 +
					1)) == NULL) {
		retval = -1;
		errno = ENOMEM;
	}
	else {
		if (sa->sa_report_data[rdk] == NULL)
			new_data[0] = '\0'; // Need to clear the fresh malloc.
		sa->sa_report_data[rdk] = new_data;
		
		strcat(sa->sa_report_data[rdk], data);
		if ((flags & SA_RDF_NEWLINE) &&
		    (data[strlen(data) - 1] != '\n')) {
			strcat(sa->sa_report_data[rdk], "\n");
		}

		retval = 0;
	}
	
	return retval;
}

static int remap_experiment(simulator_agent_t sa, int token)
{
	char nsfile[BUFSIZ];
	EmulabResponse er;
	int retval;

	snprintf(nsfile, sizeof(nsfile),
		 "/proj/%s/exp/%s/tbdata/%s-modify.ns",
		 pid, eid, eid);
	if (access(nsfile, R_OK) == -1) {
		snprintf(nsfile, sizeof(nsfile),
			 "/proj/%s/exp/%s/tbdata/%s.ns",
			 pid, eid, eid);
	}
	RPC_grab();
	retval = RPC_invoke("experiment.modify",
			    &er,
			    SPA_String, "proj", pid,
			    SPA_String, "exp", eid,
			    SPA_Boolean, "wait", true,
			    SPA_Boolean, "reboot", true,
			    SPA_Boolean, "restart_eventsys", true,
			    SPA_String, "nsfilepath", nsfile,
			    SPA_TAG_DONE);
	RPC_drop();

	if (retval != 0) {
		rename("tbdata/feedback_data.tcl",
		       "tbdata/feedback_data_failed.tcl");
		rename("tbdata/feedback_data_old.tcl",
		       "tbdata/feedback_data.tcl");
	}

	return retval;
}

static int do_modify(simulator_agent_t sa, int token, char *args)
{
	int rc, retval = 0;
	char *mode;

	assert(sa != NULL);
	assert(args != NULL);
	
	if ((rc = event_arg_get(args, "MODE", &mode)) <= 0) {
		error("no mode specified\n");
	}
	else if (strncasecmp("stabilize", mode, rc) == 0) {
		rename("tbdata/feedback_data.tcl",
		       "tbdata/feedback_data_old.tcl");
		if (systemf("loghole --port=%d --quiet sync",
			    DEFAULT_RPC_PORT) != 0) {
			error("failed to sync log holes\n");
		}
		else if (systemf("feedbacklogs %s %s", pid, eid) != 0) {
			if (sa->sa_flags & SAF_STABLE) {
				/* XXX log error */
				warning("unstabilized!\n");
			}
			else {
				retval = remap_experiment(sa, token);
			}
		}
		else {
			info("stabilized\n");
			sa->sa_flags |= SAF_STABLE;
		}
	}
	else {
		warning("unknown mode %s\n", mode);
	}

	return retval;
}

static void dump_report_data(FILE *file,
			     simulator_agent_t sa,
			     sa_report_data_kind_t srdk,
			     int clear)
{
	assert(file != NULL);
	assert(sa != NULL);
	assert(srdk >= 0);
	assert(srdk < SA_RDK_MAX);

	if ((sa->sa_report_data[srdk] != NULL) &&
	    (strlen(sa->sa_report_data[srdk]) > 0)) {
		fprintf(file, "\n%s\n", sa->sa_report_data[srdk]);
		if (clear) {
			free(sa->sa_report_data[srdk]);
			sa->sa_report_data[srdk] = NULL;
		}
	}
}

int send_report(simulator_agent_t sa, char *args)
{
	struct lnList error_records;
	char loghole_name[BUFSIZ];
	int rc, retval;
	FILE *file;
	bool archive = true;
	char * tmp;

	assert(sa != NULL);
	assert(args != NULL);
	
	/*
	 * Atomically move the error records from the agent object onto our
	 * local list and make the agent's list empty.
	 */
	if (pthread_mutex_lock(&sa->sa_local_agent.la_mutex) != 0)
		assert(0);
	lnMoveList(&error_records, &sa->sa_error_records);

	assert(lnEmptyList(&sa->sa_error_records));
	if (pthread_mutex_unlock(&sa->sa_local_agent.la_mutex) != 0)
		assert(0);
	
	if (debug) {
		char time_buf[24];
		struct timeval now;
		gettimeofday(&now, NULL);
		make_timestamp(time_buf, &now);
		info("Sending Report at %s with args \"%s\"\n",
		     time_buf, args);
	}
	     

	/*
	 * Get the logs off the nodes so we can generate summaries from the
	 * error records.
	 */
	if ((rc = systemf("loghole --port=%d -vvv sync", DEFAULT_RPC_PORT))
	    != 0) {
		errorc("failed to sync log holes %d\n", rc);
	}

	if ((file = fopen("logs/report.mail", "w")) == NULL) {
		errorc("could not create report.mail\n");
		retval = -1;
	}
	else {
		int rc, lpc, error_count;
		char *digester;
		FILE *vfile;
		
		retval = 0;

		error_count = lnCountNodes(&error_records);
		if (error_count > 0) {
			fprintf(file,
				"\n"
				"  *** %d error(s) were detected!\n"
				"      Details should be below in the logs.\n"
				"\n",
				error_count);
		}

		if ((vfile = popenf("loghole --port=%d validate",
				    "r",
				    DEFAULT_RPC_PORT)) == NULL) {
			fprintf(file, "[unable to validate logs]\n");
		}
		else {
			char buf[BUFSIZ];
			int total = 0;
			
			while ((rc = fread(buf,
					   1,
					   sizeof(buf),
					   vfile)) > 0) {
				fwrite(buf, 1, rc, file);
				total += rc;
			}
			pclose(vfile);
			vfile = NULL;

			if (total > 0)
				fprintf(file, "\n");
		}
		
		/* Dump user supplied stuff first, */
		dump_report_data(file, sa, SA_RDK_MESSAGE, 1);

		/* ... run the user-specified log digester, then */
		if ((rc = event_arg_get(args, "DIGESTER", &digester)) > 0) {
			FILE *dfile;
			
			digester[rc] = '\0';

			if ((dfile = popenf("%s | tee logs/digest.out",
					    "r",
					    digester)) == NULL) {
				fprintf(file,
					"[failed to run digester %s]\n",
					digester);
			}
			else {
				char buf[BUFSIZ];
				
				while ((rc = fread(buf,
						   1,
						   sizeof(buf),
						   dfile)) > 0) {
					fwrite(buf, 1, rc, file);
				}
				pclose(dfile);
				dfile = NULL;
			}

			fprintf(file, "\n");
		}
		
		fprintf(file, "Configuration:\n");
		dump_report_data(file, sa, SA_RDK_CONFIG, 0);

		fprintf(file, "\nLog:\n");
		dump_report_data(file, sa, SA_RDK_LOG, 1);
		
		/* ... dump the error records. */
		if (dump_error_records(&error_records, file) != 0) {
			errorc("dump_error_records failed");
			retval = -1;
		}
		
		fclose(file);
		file = NULL;
	}
	
	if ((rc = event_arg_get(args, "ARCHIVE", &tmp)) > 0) {
		if (rc == 4 && strncmp(tmp, "true", 4) == 0) {
			archive = true;
		} else if (rc == 5 && strncmp(tmp, "false", 5) == 0) {
			archive = false;
		} else {
			error("ARCHIVE must be \"true\" or \"false\"\n");
		}
	} 

	if (archive) {
		if ((file = popenf("loghole --port=%d --quiet archive --delete", 
				   "r", DEFAULT_RPC_PORT)) == NULL) {
			strcpy(loghole_name, eid);
			error("failed to archive log holes\n");
		}
		else {
			int len;
			
			fgets(loghole_name, sizeof(loghole_name), file);
			pclose(file);
			file = NULL;
			
			len = strlen(loghole_name);
			if (loghole_name[len - 1] == '\n')
				loghole_name[len - 1] = '\0';
		}
	}
	else {
		loghole_name[0] = '\0';
	}

	if (systemf("mail -s \"%s: %s/%s experiment report\" %s "
		    "< logs/report.mail",
		    OURDOMAIN,
		    pid,
		    loghole_name[0] ? loghole_name : eid,
		    getenv("USER")) != 0) {
		errorc("could not execute send report\n");
		retval = -1;
	}
	
	delete_error_records(&error_records);

	assert(lnEmptyList(&error_records));
	
	return retval;
}

static int do_reset(simulator_agent_t sa, char *args)
{
	int retval = 0;

	assert(sa != NULL);
	assert(args != NULL);

	if (systemf("loghole --port=%d --quiet clean --force",
		    DEFAULT_RPC_PORT) != 0) {
		error("failed to clean log holes\n");
	}

	return retval;
}

static int do_snapshot(simulator_agent_t sa, char *args)
{
	char *loghole_args;
	int retval = 0;

	assert(sa != NULL);
	assert(args != NULL);
	
	if (event_arg_get(args, "LOGHOLE_ARGS", &loghole_args) <= 0) {
		loghole_args = "";
	}
	
	if (systemf("loghole --port=%d sync %s",
		    DEFAULT_RPC_PORT, loghole_args) != 0) {
		error("failed to sync log holes\n");
	}

	return retval;
}

static int do_stoprun(simulator_agent_t sa, int token, char *args)
{
	int retval = 0;

	assert(sa != NULL);
	assert(args != NULL);

	/*
	 * Not allowed to use waitmode; will deadlock the event system!
	 */
	if (systemf("template_stoprun -t %d -p %s -e %s",
		    token, pid, eid) != 0) {
		error("failed to stop current run\n");
		retval = -1;
	}

	return retval;
}

static int strreltime(char *buf, size_t buflen, time_t secs)
{
    int hours, mins, retval = 0;
    char *signage = "";
    
    assert(buf != NULL);
    
    if (secs < 0) {
	secs = abs(secs);
	signage = "-";
    }
    
    hours = secs / (60 * 60);
    mins = ((secs - (hours * 60 * 60)) / 60) % 60;
    secs = secs % 60;

    if (hours)
	snprintf(buf, buflen, "%s%dh%dm%ds", signage, hours, mins, secs);
    else if (mins)
	snprintf(buf, buflen, "%s%dm%ds", signage, mins, secs);
    else
	snprintf(buf, buflen, "%s%ds", signage, secs);
    return retval;
}

static int do_log(simulator_agent_t sa, char *message)
{
	time_t current_time;
	int retval = -1;
	char date[256];
	
	current_time = time(NULL);
	if (sa->sa_flags & SAF_TIME_STARTED) {
		strreltime(date,
			   sizeof(date),
			   current_time - sa->sa_time_start.tv_sec);
	}
	else {
		ctime_r(&current_time, date);
		date[strlen(date) - 1] = '\0';
	}
	strcat(date, " - ");
	fprintf(stderr, "log: %s%s\n", date, message);
	add_report_data(sa, SA_RDK_LOG, date, 0);
	add_report_data(sa, SA_RDK_LOG, message, SA_RDF_NEWLINE);

	return retval;
}

static void *simulator_agent_looper(void *arg)
{
	simulator_agent_t sa = (simulator_agent_t)arg;
	event_handle_t handle;
	void *retval = NULL;
	sched_event_t se;
	
	assert(arg != NULL);

	handle = sa->sa_local_agent.la_handle;

	while (local_agent_dequeue(&sa->sa_local_agent, 0, &se) == 0) {
		char evtype[TBDB_FLEN_EVEVENTTYPE];
		char argsbuf[BUFSIZ];

		assert(se.length == 1);
		
		if (!event_notification_get_eventtype(
			handle, se.notification, evtype, sizeof(evtype))) {
			error("couldn't get event type from notification %p\n",
			      se.notification);
		}
		else {
			int rc = 0, token = ~0;
			
			event_notification_get_arguments(handle,
							 se.notification,
							 argsbuf,
							 sizeof(argsbuf));
			event_notification_get_int32(handle,
						     se.notification,
						     "TOKEN",
						     (int32_t *)&token);
			argsbuf[sizeof(argsbuf) - 1] = '\0';

			/* Strictly for the event viewer */
			event_notify(handle, se.notification);

			if (strcmp(evtype, TBDB_EVENTTYPE_SWAPOUT) == 0) {
				EmulabResponse er;
				
				RPC_grab();
				RPC_invoke("experiment.swapexp",
					   &er,
					   SPA_String, "proj", pid,
					   SPA_String, "exp", eid,
					   SPA_String, "direction", "out",
					   SPA_TAG_DONE);
				RPC_drop();
			}
			else if (strcmp(evtype, TBDB_EVENTTYPE_MODIFY) == 0) {
				do_modify(sa, token, argsbuf);
			}
			else if (strcmp(evtype, TBDB_EVENTTYPE_HALT) == 0) {
				rc = systemf("endexp %s %s", pid, eid);
			}
			else if (strcmp(evtype, TBDB_EVENTTYPE_DEBUG) == 0) {
				fprintf(stderr, "debug: %s\n", argsbuf);
			}
			else if (strcmp(evtype, TBDB_EVENTTYPE_MESSAGE) == 0) {
				fprintf(stderr, "msg: %s\n", argsbuf);
				add_report_data(sa,
						SA_RDK_MESSAGE,
						argsbuf,
						SA_RDF_NEWLINE);
			}
			else if (strcmp(evtype, TBDB_EVENTTYPE_LOG) == 0) {
				do_log(sa, argsbuf);
			}
			else if (strcmp(evtype, TBDB_EVENTTYPE_REPORT) == 0) {
				do_log(sa, "Sending report\n");
				send_report(sa, argsbuf);
			}
			else if (strcmp(evtype, TBDB_EVENTTYPE_RESET) == 0) {
				do_reset(sa, argsbuf);
			}
			else if (strcmp(evtype, TBDB_EVENTTYPE_SNAPSHOT) == 0){
				do_snapshot(sa, argsbuf);
			}
			else if (strcmp(evtype, TBDB_EVENTTYPE_STOPRUN) == 0){
				do_stoprun(sa, token, argsbuf);
			}
			else {
				error("cannot handle SIMULATOR event %s.",
				      evtype);
			}
			if (strcmp(evtype, TBDB_EVENTTYPE_RESET) == 0 ||
			    strcmp(evtype, TBDB_EVENTTYPE_REPORT) == 0 ||
			    strcmp(evtype, TBDB_EVENTTYPE_LOG) == 0 ||
			    strcmp(evtype, TBDB_EVENTTYPE_SNAPSHOT) == 0 ||
			    strcmp(evtype, TBDB_EVENTTYPE_MODIFY) == 0) {
				event_do(handle,
					 EA_Experiment, pideid,
					 EA_Type, TBDB_OBJECTTYPE_SIMULATOR,
					 EA_Name,
					 sa->sa_local_agent.la_link.ln_Name,
					 EA_Event, TBDB_EVENTTYPE_COMPLETE,
					 EA_ArgInteger, "ERROR", rc != 0,
					 EA_ArgInteger, "CTOKEN", token,
					 EA_TAG_DONE);
			}
		}

		sched_event_free(handle, &se);
	}

	return retval;
}
