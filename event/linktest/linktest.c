/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <stdio.h>
#include <ctype.h>
#include <netdb.h>
#include <unistd.h>
#include <paths.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pwd.h>
#include <time.h>
#include "tbdefs.h"
#include "log.h"
#include "event.h"
#include "linktest.h"

#define TRUE    1;
#define FALSE   0;

static int	      debug;
static volatile int   locked;
static pid_t          linktest_pid;
static char           *pideid;
static event_handle_t handle;

static void	      callback(event_handle_t handle,
			       event_notification_t notification, void *data);
static void           exec_linktest(char *args, int);
static void           sigchld_handler(int sig);
static void           send_group_kill();
static void           send_kill_event();
     
void
usage(char *progname)
{
	fprintf(stderr,
		"Usage: %s [-d] "
		"[-s server] [-p port] [-k keyfile] [-l logfile] -e pid/eid\n",
		progname);
	exit(-1);
}

int
main(int argc, char **argv) {

	address_tuple_t	tuple;
	char *server = NULL;
	char *port = NULL;
	char *keyfile = NULL;
	char *pidfile = NULL;
	char *logfile = NULL;
	char *progname;
	char c;
	char buf[BUFSIZ];
	pideid = NULL;
	
	progname = argv[0];

	while ((c = getopt(argc, argv, "s:p:e:l:dk:i:")) != -1) {
	  switch (c) {
	  case 'd':
	    debug++;
	    break;
	  case 's':
	    server = optarg;
	    break;
	  case 'p':
	    port = optarg;
	    break;
	  case 'e':
	    pideid = optarg;
	    break;
	  case 'i':
	    pidfile = optarg;
	    break;
	  case 'l':
	    logfile = optarg;
	    break;
	  case 'k':
	    keyfile = optarg;
	    break;
	  default:
	    usage(progname);
	  }
	}

	if (!pideid)
	  usage(progname);

	if (debug)
		loginit(0, 0);
	else {
		if (logfile)
			loginit(0, logfile);
		else
			loginit(1, "linktest");
		/* See below for daemonization */
	}

	/*
	 * Convert server/port to elvin thing.
	 *
	 * XXX This elvin string stuff should be moved down a layer. 
	 */
	if (server) {
		snprintf(buf, sizeof(buf), "elvin://%s%s%s",
			 server,
			 (port ? ":"  : ""),
			 (port ? port : ""));
		server = buf;
	}

	/*
	 * Construct an address tuple for subscribing to events for
	 * this node.
	 */
	tuple = address_tuple_alloc();
	if (tuple == NULL) {
		fatal("could not allocate an address tuple");
	}
	/*
	 * Ask for just the events we care about. 
	 */
	tuple->expt      = pideid;
	tuple->objtype   = TBDB_OBJECTTYPE_LINKTEST;
	tuple->eventtype = ADDRESSTUPLE_ANY;

	/*
	 * Register with the event system. 
	 */
	handle = event_register_withkeyfile(server, 0, keyfile);
	if (handle == NULL) {
	        fatal("could not register with event system");
	}
	
	/*
	 * Subscribe to the event we specified above.
	 */
	if (! event_subscribe(handle, callback, tuple, NULL)) {
		fatal("could not subscribe to event");
	}

	/*
	 * Do this now, once we have had a chance to fail on the above
	 * event system calls.
	 */
	if (!debug)
		daemon(0, 1);

	/*
	 * Write out a pidfile if root (after we daemonize).
	 */
	if (!getuid()) {
		FILE *fp;
		
		if (pidfile)
			strcpy(buf, pidfile);
		else
			sprintf(buf, "%s/linktest.pid", _PATH_VARRUN);
		fp = fopen(buf, "w");
		if (fp != NULL) {
			fprintf(fp, "%d\n", getpid());
			(void) fclose(fp);
		}
	}

	/*
	 * Initialize variables used to control child execution
	 */
	locked = FALSE;
	if(signal(SIGCHLD,sigchld_handler) == SIG_ERR) {
	        fatal("could not install child handler");
	}
	
	/*
	 * Begin the event loop, waiting to receive event notifications:
	 */
	event_main(handle);

	/*
	 * Unregister with the event system:
	 */
	if (event_unregister(handle) == 0) {
		fatal("could not unregister with event system");
	}

	return 0;
}
/*
 * Handle the events.
 */
static void
callback(event_handle_t handle, event_notification_t notification, void *data)
{
	char		objname[TBDB_FLEN_EVOBJTYPE];
	char		event[TBDB_FLEN_EVEVENTTYPE];
	char		args[BUFSIZ];
	struct timeval	now;

	gettimeofday(&now, NULL);
	
	if (! event_notification_get_objname(handle, notification,
					     objname, sizeof(objname))) {
		error("Could not get objname from notification!\n");
		return;
	}

	if (! event_notification_get_eventtype(handle, notification,
					       event, sizeof(event))) {
		error("Could not get event from notification!\n");
		return;
	}

	event_notification_get_arguments(handle,
					 notification, args, sizeof(args));

	/*
	 * Dispatch the event. 
	 */
	if(!strcmp(event, TBDB_EVENTTYPE_START)) {
	  if(!locked) {

	    /*
	     * Set locked bit. The bit is not set to false
	     * until a SIGCHLD signal is received
	     */
	    locked = TRUE;

	    /*
	     * The Linktest script is not running, so
	     * fork a process and execute it.
	     */
	    linktest_pid = fork();
	    if(linktest_pid < 0) {
	         error("Could not fork a process to run linktest script!\n");
	         return;
	    }

	    /*
	     * Changes the process group of the child to itself so
	     * a sigkill to the child process group will not kill
	     * the Linktest daemon.
	     */
	    if(!linktest_pid) {
  	         pid_t mypid = getpid();
	         setpgid(mypid,mypid);

		 /* Finally, execute the linktest script. */
		 exec_linktest(args, sizeof(args));
	    }
	  }
	} else if(!strcmp(event, TBDB_EVENTTYPE_STOP)) {
	  /*
	   * STOP is informational and may be ignored.
	   */

	} else if (!strcmp(event, TBDB_EVENTTYPE_KILL)) {

	  /*
	   * Ignore unless we are running.
	   */
	  if(locked) {

	    /*
	     * If KILL is received, there is a problem on this
	     * or some other node. So, kill off linktest
	     * and its children.
	     */
	    send_group_kill();
	    
	  }
	}
}

/*
 * Executes Linktest with arguments received from the Linktest
 * start event. Does not return.
 */ 
static void
exec_linktest(char *args, int buflen) {
	char	   *word, *argv[MAX_ARGS];
	int	   i,res;

	/*
	 * Set up arguments for execv call by parsing contents
	 * of the event string.
	 */
	word = strtok(args," \t");
	i=1;
	do {
	  argv[i++] = word;
	} while ((word = strtok(NULL," \t"))
		 && (i<MAX_ARGS));
	argv[i] = NULL;
	argv[0] = LINKTEST_SCRIPT;

	/*
	 * Execute the script with the arguments from the event
	 */
	res = execv( LINKTEST_SCRIPT,argv);
	if(res < 0) {
	    error("Could not execute the Linktest script.");
	    return;
	}
}

static
void sigchld_handler(int sig) {
        pid_t pid;
	int status;
	int exit_code;

	/*
	 * If the exit_code is nonzero after a normal exit,
	 * the daemon sends a KILL to let other nodes know
	 * that something has failed.
	 *
	 * However, ignore the case of a non-normal exit,
	 * since that is likely the result of a KILL signal.
	 */
	exit_code = 0; 
	while((pid = waitpid(-1, &status, 0)) > 0) {

	  /*
	   * If Linktest died due to an error, Perl will exit
	   * the script normally with an error code. 
	   */
	  if(WIFEXITED(status)) {
	    exit_code = WEXITSTATUS(status);
	    info("Linktest exit code: %d\n",exit_code);
	    
	    /*
	     * If this was an abnormal exit (exit status != 0)
	     * then we must send KILL to the process group of
	     * Linktest to kill its subchildren. However,
	     * it doesn't seem to hurt (cause kill errors) to
	     * send it anyway.
	     */
	    send_group_kill();
	    
	  } else if (WIFSIGNALED(status)) {
	    /*
	     * Linktest exited due to a signal, likely from
	     * this daemon. If that's the case, group_kill
	     * has already been sent.
	     */
	    info("Linktest killed by signal %d.\n", WTERMSIG(status));
	  } else {
	    /*
	     * Linktest is stopped unexpectedly.
	     */
	    error("unexpected SIGCHLD received\n");
	  }
	  
	}
	if(errno != ECHILD) {
	  error("waitpid error\n");
	}

	/*
	 * Go ahead and unlock since Linktest and its children
	 * should all be killed and reaped now.
	 */
	locked = FALSE;

	/*
	 * Now let other nodes know about the problem, if any.
	 */
	if(exit_code) {
	  info("Posting KILL event\n");
	  send_kill_event();
	}
	return;
}

static
void send_group_kill() {
        int res;
	
        /*
	 * Kill off all processes in the process group of the
	 * Linktest run. This may include the linktest script
	 * itself, and any children it forked.
	 */
	res = kill(-linktest_pid,SIGKILL);
	if(res < 0) {
	  /*
	   * Not a serious error, likely the process group
	   * has already exited.
	   */
	  return;
	}
}

static
void send_kill_event() {
	event_notification_t notification;
	address_tuple_t	tuple;

	/*
	 * Construct an address tuple for generating the event.
	 */
	tuple = address_tuple_alloc();
	if (tuple == NULL) {
	  error("could not allocate an address tuple");
	}

	/*
	 * Set up event to send a kill...
	 */
	tuple->objtype  = TBDB_OBJECTTYPE_LINKTEST;
	tuple->eventtype= TBDB_EVENTTYPE_KILL;
	tuple->host	= ADDRESSTUPLE_ALL;
	tuple->expt     = pideid;
	  
	/* Generate the event */
	notification = event_notification_alloc(handle, tuple);
	if (notification == NULL) {
	  error("could not allocate notification");
	}

	/* Send the event */
	if (event_notify(handle, notification) == 0) {
	  error("could not send event notification");
	}

	/* Clean up */
	event_notification_free(handle, notification);

}
