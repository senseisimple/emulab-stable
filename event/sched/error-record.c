/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004, 2005, 2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file error-record.c
 *
 * Implementation of the error-record code.
 */

#include "config.h"

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <sys/param.h>
#include <string.h>

#include "tbdefs.h"
#include "popenf.h"

#include "error-record.h"

/**
 * Dump a digested version of the program-agent's "status" file to the given
 * output.  The status file is a 'key=value' formatted file that describes a
 * particular invocation of a program agent.  The file is written out by the
 * agent after the program finishes, pulled back to ops by a 'loghole sync' and
 * then gets included in the Simulator's report through this function.
 *
 * @param er An error record for a failed program-agent invocation.
 * @param out The file to write any output to.
 * @return Zero on success, -1 otherwise.
 *
 * @see send_report
 */
static int dump_agent_status(error_record_t er, FILE *out);

/**
 * Utility function to pipe the tail of a file to the given output file.  After
 * the program-agent's "status" file is dumped to the output, the tail of the
 * file gets included as well so the user can get an idea of what went wrong.
 *
 * @param path The path of the file whose tail should be written to the output
 * file.
 * @param out The file to write any output to.
 * @return Zero on success, -1 otherwise.
 *
 * @see dump_agent_status
 */
static int tail_file(char *path, FILE *out);

error_record_t create_error_record(void)
{
	error_record_t retval = NULL;

	if ((retval = calloc(1, sizeof(struct _error_record))) == NULL) {
		errno = ENOMEM;
	}
	else {
		retval->er_agent = NULL;
		retval->er_token = -1;
		retval->er_error = 0;
	}
	
	return retval;
}

void delete_error_record(error_record_t er)
{
	if (er != NULL) {
		free(er);
		er = NULL;
	}
}

int error_record_invariant(error_record_t er)
{
	assert(er != NULL);
	assert(er->er_agent != NULL);
	assert(agent_invariant(er->er_agent));
	assert(er->er_token != -1);
	
	return 1;
}

void delete_error_records(struct lnList *list)
{
	error_record_t er;
	
	assert(list != NULL);
	lnCheck(list);

	while ((er = (error_record_t)lnRemHead(list)) != NULL) {
		delete_error_record(er);
		er = NULL;
	}

	lnCheck(list);
	assert(lnEmptyList(list));
}

static int dump_agent_status(error_record_t er, FILE *out)
{
	/*
	 * The format of the path for the status file:
	 *   logs/<node>/<LOGDIR>/<agent>.<token>.status
	 */
	static char *file_format = "logs/%s" LOGDIR "/%s.status.%lu";

	/*
	 * A map of status file 'keys' that people may be interested in and
	 * some human-readable prefix text.
	 */
	static struct {
		char *key;
		char *desc;
	} status_map[] = {
		{ "DIR",		"  Directory:\t" },
		{ "COMMAND",		"  Command:\t" },
		{ "START_TIME",		"  Started at:\t" },
		{ "EXIT_CODE",		"  Exit code:\t" },
		{ "TIMEOUT_FIRED",	"  Timeout Fired:\t" },
		{ NULL, NULL }
	};
	
	char buffer[BUFSIZ];
	int retval;
	FILE *file;

	assert(er != NULL);
	assert(error_record_invariant(er));
	assert(strcmp(er->er_agent->objtype, TBDB_OBJECTTYPE_PROGRAM) == 0);
	assert(out != NULL);

	snprintf(buffer,
		 sizeof(buffer),
		 file_format,
		 er->er_agent->vnode,
		 er->er_agent->name,
		 er->er_token);

	/*
	 * We expect the file to have been brought over already, so just try
	 * to open out.
	 */
	if ((file = fopen(buffer, "r")) == NULL) {
		fprintf(out, "warning: missing status file: '%s'\n", buffer);
		retval = 0;
	}
	else {
		/* Print out a short header, then */
		fprintf(out,
			"Program agent: '%s' located on node '%s'\n",
			er->er_agent->name,
			er->er_agent->vnode);

		/* ... write out the data most people would care about. */
		while (fgets(buffer, sizeof(buffer), file) != NULL) {
			char *value;

			if ((value = strchr(buffer, '=')) == NULL) {
				warning("Bad line in status file: %s\n",
					buffer);
			}
			else {
				int lpc;
				
				*value = '\0';
				value += 1;
				for (lpc = 0; status_map[lpc].key; lpc++) {
					if (strcmp(buffer,
						   status_map[lpc].key) == 0) {
						fprintf(out,
							"%s%s",
							status_map[lpc].desc,
							value);
					}
				}
			}
		}
		
		fclose(file);
		file = NULL;

		retval = 0;
	}
	
	return retval;
}

static int tail_file(char *path, FILE *out)
{
	FILE *tail_in;
	int retval;
	
	assert(path != NULL);
	assert(strlen(path) > 0);
	assert(out != NULL);
	
	if (access(path, R_OK) != 0) {
		fprintf(out, "warning: missing log file: '%s'\n", path);
		retval = 0; // Non-fatal error...
	}
	else if ((tail_in = popenf("tail %s", "r", path)) == NULL) {
		fprintf(out, "error: cannot tail log '%s'\n", path);
		retval = -1;
	}
	else {
		char buffer[1024];
		int rc;

		/* Print a separator header, */
		fprintf(out, ">> Tail of log '%s' <<\n", path);

		/* ... the tail of the file, and */
		while ((rc = fread(buffer,
				   1,
				   sizeof(buffer),
				   tail_in)) > 0) {
			fwrite(buffer, 1, rc, out);
		}
		if (pclose(tail_in) == -1) {
			warning("pclose for tail %s failed\n", path);
		}
		tail_in = NULL;

		/* ... a separator footer. */
		memset(buffer, '=', 79);
		buffer[79] = '\0';
		fprintf(out, "%s\n\n", buffer);

		retval = 0;
	}
	
	return retval;
}

int dump_error_record(error_record_t er, FILE *out)
{
	char path[PATH_MAX];
	int lpc, retval = 0;

	assert(er != NULL);
	assert(error_record_invariant(er));
	assert(out != NULL);
	
	if (strcmp(er->er_agent->objtype, TBDB_OBJECTTYPE_PROGRAM) == 0) {
		/**
		 * NULL-terminated array of log file name formats that should
		 * be sent back to the user.
		 */
		static char *filename_formats[] = {
			"logs/%s" LOGDIR "/%s.out.%lu",
			"logs/%s" LOGDIR "/%s.err.%lu",
			NULL
		};

		/* First, write out any status information, then */
		retval = dump_agent_status(er, out);
		/* ... pass the logs through. */
		for (lpc = 0; filename_formats[lpc] && (retval == 0); lpc++) {
			snprintf(path,
				 sizeof(path),
				 filename_formats[lpc],
				 er->er_agent->vnode,
				 er->er_agent->name,
				 er->er_token);

			retval = tail_file(path, out);
		}
	}
	else if (strcmp(er->er_agent->objtype, TBDB_OBJECTTYPE_NODE) == 0) {
		/**
		 * NULL-terminated array of log file name formats that should
		 * be sent back to the user.
		 */
		static char *filename_formats[] = {
			"logs/%s/node-control.%lu",
			NULL
		};

		/* Pass the logs through. */
		for (lpc = 0; filename_formats[lpc] && (retval == 0); lpc++) {
			snprintf(path,
				 sizeof(path),
				 filename_formats[lpc],
				 er->er_agent->vnode,
				 er->er_token);

			retval = tail_file(path, out);
		}
	}
	else if (strcmp(er->er_agent->objtype,
			TBDB_OBJECTTYPE_LINKTEST) == 0) {
		FILE *pfile;

		if ((pfile = popen("cat tbdata/linktest/*.error",
				   "r")) == NULL) {
			fprintf(out,
				"warning: unable to read linktest error "
				"files\n");
		}
		else {
			char buffer[1024];
			int rc;

			fprintf(out, ">> Linktest Errors <<\n");
			/* ... the tail of the file, and */
			while ((rc = fread(buffer,
					   1,
					   sizeof(buffer),
					   pfile)) > 0) {
				fwrite(buffer, 1, rc, out);
			}
			if (pclose(pfile) == -1) {
				warning("linktest pclose failed\n");
			}
			pfile = NULL;
			
			/* ... a separator footer. */
			memset(buffer, '=', 79);
			buffer[79] = '\0';
			fprintf(out, "%s\n\n", buffer);
		}
	}
	
	return retval;
}

int dump_error_records(struct lnList *list, FILE *out)
{
	error_record_t er;
	int retval = 0;

	assert(list != NULL);
	lnCheck(list);
	assert(out != NULL);

	for (er = (error_record_t)list->lh_Head;
	     (er->er_link.ln_Succ != NULL) && (retval == 0);
	     er = (error_record_t)er->er_link.ln_Succ) {
		assert(error_record_invariant(er));
		
		retval = dump_error_record(er, out);
	}
	
	return retval;
}
