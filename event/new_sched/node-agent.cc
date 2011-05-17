/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004, 2005 University of Utah and the Flux Group.
n * All rights reserved.
 */

#include "config.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>

#include "systemf.h"
#include "rpc.h"
#include "node-agent.h"

using namespace emulab;

#define REBOOT_TIMEOUT (6 * 60)

/**
 * A "looper" function for node agents that dequeues and processes events for
 * a particular node.  This function will be passed to pthread_create when a
 * new thread needs to be created to handle events.
 *
 * @param arg The node agent object to handle events for.
 * @return NULL
 *
 * @see reload_with
 * @see do_reboot
 * @see local_agent_queue
 * @see local_agent_dequeue
 */
static void *node_agent_looper(void *arg);

/**
 * Expand an event into a list of physical node identifiers.  For events
 * targeted at a single node, this function will just return the ID for that
 * node.  Otherwise, it will concatenate all of the node IDs onto a single
 * string.
 *
 * @param se The event being sent to the node(s).
 * @return A space separated string of physical node identifiers or NULL if
 * not enough memory could be allocated.  The caller is responsible for freeing
 * the memory when it is finished.
 */
static char *expand_nodeids(sched_event_t *se);

/**
 * Handler for the RELOAD event of the node object.  First, the function will
 * sync the logholes for the nodes that will be reloaded, then it will start
 * the reload and wait for the disks to be loaded and the nodes to finish
 * rebooting.
 *
 * @param na The node agent the event was sent to, or if multiple nodes are
 * being sent this event, the first one in the list.
 * @param nodeids A space separated list of physical nodes that should have
 * their disks reloaded.
 * @param args Any arguments accompanying the event.  Currently, the only
 * supported event argument is IMAGE, which specifies which image to load onto
 * the nodes.  Otherwise, the nodes will be reloaded with the default image.
 * @return Zero on success, otherwise an error code.
 *
 * @see expand_nodeids
 */
static int reload_with(node_agent_t na, struct agent **agent_array, int aa_len,
		       int token, char *nodeids, char *args);

/**
 * Handler for the REBOOT event of the node object.  First, the function will
 * sync the logholes for the nodes that will be reboot, then it will reboot
 * the nodes and wait for them to report ISUP.
 *
 * @param na The node agent the event was sent to, or if multiple nodes are
 * being sent this event, the first one in the list.
 * @param nodeids A space separated list of physical nodes that should be
 * rebooted.
 * @return Zero on success, otherwise an error code.
 *
 * @see expand_nodeids
 */
static int do_reboot(node_agent_t na, struct agent **agent_array, int aa_len,
		     int token, char *nodeids);

node_agent_t create_node_agent(void)
{
	node_agent_t na, retval;

	if ((na = (node_agent_t)malloc(sizeof(struct _node_agent))) == NULL) {
		retval = NULL;
		errno = ENOMEM;
	}
	else if (local_agent_init(&na->na_local_agent) != 0) {
		retval = NULL;
	}
	else {
		na->na_local_agent.la_flags |= LAF_MULTIPLE;
		na->na_local_agent.la_looper = node_agent_looper;
		retval = na;
		na = NULL;
	}

	free(na);
	na = NULL;

	return retval;
}

int node_agent_invariant(node_agent_t na)
{
	assert(na != NULL);
	assert(local_agent_invariant(&na->na_local_agent));
}

static char *expand_nodeids(sched_event_t *se)
{
	char *retval;

	assert(se != NULL);
	
	if (se->length == 1) {
		assert(se->agent.s != NULL);

		/*
		 * Just dup the singleton for now and return that.  We could
		 * return the agent's name field, but I don't feel like special
		 * casing the free() right now.
		 */
		retval = strdup(se->agent.s->nodeid);
	}
	else {
		int lpc, len = 1;

		/* Find the length of all the node IDs and separators, then */
		for (lpc = 1; lpc <= se->length; lpc++) {
			len += strlen(se->agent.m[lpc]->nodeid) + 1;
		}

		/* ... cons up the string. */
		if ((retval = (char *)malloc(len)) != NULL) {
			char *cursor = retval;

			for (lpc = 1; lpc <= se->length; lpc++) {
				assert(se->agent.m[lpc] != NULL);
				
				strcpy(cursor, se->agent.m[lpc]->nodeid);
				cursor += strlen(cursor);
				
				cursor[0] = ' ';
				cursor += 1;
			}
			cursor[0] = '\0';

			assert(strlen(retval) > 0);
		}
	}

	return retval;
}

static void dump_node_logs(int token, struct agent **agent_array, int aa_len,
			   EmulabResponse *er)
{
	const char *output;
	FILE *file;
	int lpc;
	
	output = ((std::string)er->getOutput()).c_str();

	for (lpc = 0; lpc < aa_len; lpc++) {
		char filename[1024];
		FILE *file;
		
		snprintf(filename, sizeof(filename),
			 NODE_DUMP_DIR,
			 agent_array[lpc]->name);
		mkdir(filename, 0664);
		snprintf(filename, sizeof(filename),
			 NODE_DUMP_FILE,
			 agent_array[lpc]->name, token);
		if ((file = fopen(filename, "w")) != NULL) {
			fprintf(file, "%s", output);
			fclose(file);
		}
	}
}

static int reload_with(node_agent_t na, struct agent **agent_array, int aa_len,
		       int token, char *nodeids, char *args)
{
	event_handle_t handle;
	EmulabResponse er;
	char *image_name;
	int rc, retval;

	assert(na != NULL);
	assert(node_agent_invariant(na));
	assert(nodeids != NULL);
	assert(strlen(nodeids) > 0);
	assert(args != NULL);

	handle = na->na_local_agent.la_handle;

	/*
	 * Get any logs off the node(s) before we destroy them with the disk
	 * reload, then
	 */
	if (systemf("loghole --port=%d --quiet sync %s",
		    DEFAULT_RPC_PORT,
		    nodeids) != 0) {
		warning("failed to sync log hole for node %s\n", nodeids);
	}

	/* ... reload the default image, or */
	if ((rc = event_arg_get(args, "IMAGE", &image_name)) < 0) {
		if ((retval = RPC_invoke("node.reload",
					 &er,
					 SPA_String, "nodes", nodeids,
					 SPA_Boolean, "wait", true,
					 SPA_Boolean, "bootwait", true,
					 SPA_TAG_DONE)) != 0) {
			warning("could not reload: %s\n", nodeids);
			dump_node_logs(token, agent_array, aa_len, &er);
		}
	}
	/* ... a user-specified image. */
	else {
		image_name[rc] = '\0';
		if ((retval = RPC_invoke("node.reload",
					 &er,
					 SPA_String, "nodes", nodeids,
					 SPA_String, "imageproj", pid,
					 SPA_String, "imagename", image_name,
					 SPA_Boolean, "wait", true,
					 SPA_Boolean, "bootwait", true,
					 SPA_TAG_DONE)) != 0) {
			warning("could not reload: %s\n", nodeids);
			dump_node_logs(token, agent_array, aa_len, &er);
		}
	}

	/* XXX dump output to a file. */
	
	return retval;
}

static int do_reboot(node_agent_t na, struct agent **agent_array, int aa_len,
		     int token, char *nodeids)
{
	event_handle_t handle;
	EmulabResponse er;
	int retval;

	assert(na != NULL);
	assert(nodeids != NULL);
	assert(strlen(nodeids) > 0);

	handle = na->na_local_agent.la_handle;

	/* Sync the logholes in case the OS clears out /tmp, then */
	if (systemf("loghole --port=%d --quiet sync %s",
		    DEFAULT_RPC_PORT,
		    nodeids) != 0) {
		warning("failed to sync log hole for node %s\n", nodeids);
	}

	info("rebooting; %s\n", nodeids);

	/* ... start the reboot. */
	if ((retval = RPC_invoke("node.reboot",
				 &er,
				 SPA_String, "nodes", nodeids,
				 SPA_Boolean, "wait", true,
				 SPA_TAG_DONE)) != 0) {
		warning("could not reboot: %s\n", nodeids);
		dump_node_logs(token, agent_array, aa_len, &er);
	}
	
	return retval;
}

static int do_snapshot(node_agent_t na, struct agent **agent_array, int aa_len,
		       int token, char *nodeids, char *args)
{
	event_handle_t handle;
	EmulabResponse er;
	char *image_name;
	int rc, retval;

	assert(na != NULL);
	assert(node_agent_invariant(na));
	assert(nodeids != NULL);
	assert(strlen(nodeids) > 0);
	assert(args != NULL);

	handle = na->na_local_agent.la_handle;

	if (systemf("loghole --port=%d -vvv sync %s",
		    DEFAULT_RPC_PORT,
		    nodeids) != 0) {
		warning("failed to sync log hole for node %s\n", nodeids);
	}
	else if (systemf("loghole --port=%d --quiet clean -fn %s",
			 DEFAULT_RPC_PORT,
			 nodeids) != 0) {
		warning("failed to clean log hole on node(s): %s\n", nodeids);
	}

	if ((rc = event_arg_get(args, "IMAGE", &image_name)) < 0) {
		warning("no image name given: %s\n", nodeids);

		retval = -1;
	}
	else {
		image_name[rc] = '\0';
		if ((retval = RPC_invoke("node.create_image",
					 &er,
					 SPA_String, "node", nodeids,
					 SPA_String, "imageproj", pid,
					 SPA_String, "imagename", image_name,
					 SPA_Boolean, "wait", true,
					 SPA_Boolean, "bootwait", true,
					 SPA_TAG_DONE)) != 0) {
			warning("could not snapshot: %s\n", nodeids);
			dump_node_logs(token, agent_array, aa_len, &er);
			
		}
		/* XXX Kinda hacky way to wait for the node to come up. */
		else if ((retval =
			  RPC_invoke("node.statewait",
				     &er,
				     SPA_String, "node", nodeids,
				     SPA_Integer, "timeout", REBOOT_TIMEOUT,
				     SPA_String, "state", "ISUP",
				     SPA_TAG_DONE)) != 0) {
			warning("timeout waiting for node: %s\n", nodeids);
			dump_node_logs(token, agent_array, aa_len, &er);
		}
	}

	/* XXX dump output to a file. */
	
	return retval;
}

static void *node_agent_looper(void *arg)
{
	node_agent_t na = (node_agent_t)arg;
	event_handle_t handle;
	void *retval = NULL;
	sched_event_t se;

	assert(arg != NULL);
	
	handle = na->na_local_agent.la_handle;

	RPC_grab();
	while (local_agent_dequeue(&na->na_local_agent, 0, &se) == 0) {
		char evtype[TBDB_FLEN_EVEVENTTYPE];
		event_notification_t en;
		char argsbuf[BUFSIZ];
		char *nodeids = NULL;

		en = se.notification;
		
		if (!event_notification_get_eventtype(
			handle, en, evtype, sizeof(evtype))) {
			error("couldn't get event type from notification %p\n",
			      en);
		}
		else if ((nodeids = expand_nodeids(&se)) == NULL) {
			error("could not expand nodeids!\n");
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

			if (se.length == 0) {
			}
			else if (se.length == 1) {
				agent_singleton[0] = se.agent.s;
				agent_array = agent_singleton;
			}
			else {
				agent_array = &se.agent.m[1];
			}
			
			if (strcmp(evtype, TBDB_EVENTTYPE_REBOOT) == 0) {
				rc = do_reboot(na, agent_array, se.length,
					       token, nodeids);
			}
			else if (strcmp(evtype, TBDB_EVENTTYPE_RELOAD) == 0) {
				rc = reload_with(na, agent_array, se.length,
						 token, nodeids, argsbuf);
			}
			else if (strcmp(evtype,
					TBDB_EVENTTYPE_SNAPSHOT) == 0) {
				rc = do_snapshot(na, agent_array, se.length,
						 token, nodeids, argsbuf);
			}
			else if (strcmp(evtype, TBDB_EVENTTYPE_SETDEST) == 0) {
				event_notify(handle, en);
				se.length = 0;
			}
			else if (strcmp(evtype, TBDB_EVENTTYPE_MODIFY) == 0) {
				event_notify(handle, en);
				se.length = 0;
			}
			else {
				error("cannot handle NODE event %s.", evtype);
				rc = -1;
			}

			for (lpc = 0; lpc < se.length; lpc++) {
				event_do(handle,
					 EA_Experiment, pideid,
					 EA_Type, TBDB_OBJECTTYPE_NODE,
					 EA_Name, agent_array[lpc]->name,
					 EA_Event, TBDB_EVENTTYPE_COMPLETE,
					 EA_ArgInteger, "ERROR", rc,
					 EA_ArgInteger, "CTOKEN", token,
					 EA_TAG_DONE);
			}
		}
		
		free(nodeids);
		nodeids = NULL;

		sched_event_free(handle, &se);
	}
	RPC_drop();

	return retval;
}
