/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004, 2005, 2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <unistd.h>
#include <string.h>

#include "console-agent.h"

#ifndef min
#define min(x, y) ((x) < (y)) ? (x) : (y)
#endif

/**
 * A "looper" function for console agents that dequeues and processes events
 * for a particular console.  This function will be passed to pthread_create
 * when a new thread needs to be created to handle events.
 *
 * @param arg The console agent object to handle events for.
 * @return NULL
 *
 * @see reload_with
 * @see do_reboot
 * @see local_agent_queue
 * @see local_agent_dequeue
 */
static void *console_agent_looper(void *arg);

console_agent_t create_console_agent(void)
{
	console_agent_t ca, retval;

	if ((ca = (console_agent_t)
	     malloc(sizeof(struct _console_agent))) == NULL) {
		retval = NULL;
		errno = ENOMEM;
	}
	else if (local_agent_init(&ca->ca_local_agent) != 0) {
		retval = NULL;
	}
	else {
		ca->ca_local_agent.la_flags |= LAF_MULTIPLE;
		ca->ca_local_agent.la_looper = console_agent_looper;
		ca->ca_mark = -1;
		retval = ca;
		ca = NULL;
	}

	free(ca);
	ca = NULL;

	return retval;
}

int console_agent_invariant(console_agent_t ca)
{
	assert(ca != NULL);
	assert(local_agent_invariant(&ca->ca_local_agent));
}

static void do_start(console_agent_t ca, sched_event_t *se)
{
	int lpc;
	
	assert(ca != NULL);
	assert(console_agent_invariant(ca));
	assert(se != NULL);

	for (lpc = 0; lpc < se->length; lpc++) {
		char path[MAXPATHLEN];
		struct agent *agent;
		struct stat st;

		if (se->length == 1)
			agent = se->agent.s;
		else
			agent = se->agent.m[lpc];
		
		snprintf(path, sizeof(path),
			 "%s/%s.run",
			 TIPLOGDIR, agent->nodeid);
		if (stat(path, &st) < 0) {
			error("could not stat %s\n", path);
		}
		else {
			ca = (console_agent_t)agent->handler;
			ca->ca_mark = st.st_size;
		}
	}
}

static void do_stop(console_agent_t ca, sched_event_t *se, char *args)
{
	char *filename;
	int rc, lpc;
	
	assert(ca != NULL);
	assert(console_agent_invariant(ca));
	assert(se != NULL);
	assert(args != NULL);

	if (ca->ca_mark == -1) {
		error("CONSOLE STOP event without a START");
		return;
	}

	if ((rc = event_arg_get(args, "FILE", &filename)) < 0) {
		error("no filename given in CONSOLE STOP event");
		return;
	}
	filename[rc] = '\0';
	
	for (lpc = 0; lpc < se->length; lpc++) {
		char path[MAXPATHLEN], outpath[MAXPATHLEN];
		int infd = -1, outfd = -1;
		struct agent *agent;
		size_t end_mark;
		struct stat st;
		
		if (se->length == 1)
			agent = se->agent.s;
		else
			agent = se->agent.m[lpc];
		
		snprintf(path, sizeof(path),
			 "%s/%s.run",
			 TIPLOGDIR, agent->nodeid);
		if (stat(path, &st) < 0) {
			error("could not stat %s\n", path);
			ca->ca_mark = -1;
			continue;
		}

		snprintf(outpath, sizeof(outpath),
			 "logs/%s-%s.log",
			 agent->name,
			 filename);
		
		ca = (console_agent_t)agent->handler;
		end_mark = st.st_size;
		if ((infd = open(path, O_RDONLY)) < 0) {
			error("could not open %s\n", path);
		}
		else if (lseek(infd, ca->ca_mark, SEEK_SET) < 0) {
			error("could not seek to right place\n");
		}
		else if ((outfd = open(outpath,
				       O_WRONLY|O_CREAT|O_TRUNC,
				       0640)) < 0) {
			error("could not create output file\n");
		}
		else {
			size_t remaining = end_mark - ca->ca_mark;
			char buffer[4096];

			while ((rc = read(infd,
					  buffer,
					  min(remaining,
					      sizeof(buffer)))) > 0) {
				write(outfd, buffer, rc);
				
				remaining -= rc;
			}
		}
		
		if (infd != -1)
			close(infd);
		if (outfd != -1)
			close(outfd);

		ca->ca_mark = -1;
	}
}

static void *console_agent_looper(void *arg)
{
	console_agent_t ca = (console_agent_t)arg;
	event_handle_t handle;
	void *retval = NULL;
	sched_event_t se;

	assert(arg != NULL);
	
	handle = ca->ca_local_agent.la_handle;

	while (local_agent_dequeue(&ca->ca_local_agent, 0, &se) == 0) {
		char evtype[TBDB_FLEN_EVEVENTTYPE];
		event_notification_t en;
		char argsbuf[BUFSIZ];

		en = se.notification;
		
		if (!event_notification_get_eventtype(
			handle, en, evtype, sizeof(evtype))) {
			error("couldn't get event type from notification %p\n",
			      en);
		}
		else {
			struct agent **agent_array, *agent_singleton[1];
			int rc, lpc, token = ~0;
			
			event_notification_get_arguments(handle,
							 en,
							 argsbuf,
							 sizeof(argsbuf));
			event_notification_get_int32(handle,
						     en,
						     "TOKEN",
						     (int32_t *)&token);
			argsbuf[sizeof(argsbuf) - 1] = '\0';

			if (strcmp(evtype, TBDB_EVENTTYPE_START) == 0) {
				do_start(ca, &se);
			}
			else if (strcmp(evtype, TBDB_EVENTTYPE_STOP) == 0) {
				do_stop(ca, &se, argsbuf);
			}
			else {
				error("cannot handle CONSOLE event %s.",
				      evtype);
				rc = -1;
			}
		}
		sched_event_free(handle, &se);
	}

	return retval;
}
