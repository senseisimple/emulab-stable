/*
 * This ia program agent to manage programs from the event system.
 *
 * You can start, stop, and kill (signal) programs. 
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
#include <time.h>
#include "tbdefs.h"
#include "log.h"
#include "event.h"

static char 		*logfile;
static char		debug;

struct proginfo {
	char		name[TBDB_FLEN_EVOBJNAME];
	char	       *cmdline;
	int		pid;
	struct timeval  started;
	struct proginfo *next;
};
static struct proginfo *proginfos;

static void	callback(event_handle_t handle,
			 event_notification_t notification, void *data);
static void	start_program(char *objname, char *args);
static void	stop_program(char *objname, char *args);
static void	signal_program(char *objname, char *args);

void
usage(char *progname)
{
	fprintf(stderr,
		"Usage: %s [-s server] [-p port] [-l logfile]\n", progname);
	exit(-1);
}

int
main(int argc, char **argv)
{
	event_handle_t handle;
	address_tuple_t	tuple;
	char *progname;
	char *server = NULL;
	char *port = NULL;
	char *ipaddr = NULL;
	char buf[BUFSIZ], ipbuf[BUFSIZ];
	int c;

	progname = argv[0];
	
	while ((c = getopt(argc, argv, "s:p:l:d")) != -1) {
		switch (c) {
		case 's':
			server = optarg;
			break;
		case 'p':
			port = optarg;
			break;
		case 'l':
			logfile = optarg;
			break;
		case 'd':
			debug++;
			break;
		default:
			usage(progname);
		}
	}
	argc -= optind;
	argv += optind;

	if (debug) 
		loginit(0, 0);
	else {
		/* Become a daemon */
		daemon(0, 0);

		if (logfile)
			loginit(0, logfile);
		else
			loginit(1, "program-agent");
	}

	/*
	 * Get our IP address. Thats how we name ourselves to the
	 * Testbed Event System. 
	 */
	if (ipaddr == NULL) {
	    struct hostent	*he;
	    struct in_addr	myip;
	    
	    if (gethostname(buf, sizeof(buf)) < 0) {
		fatal("could not get hostname");
	    }

	    if (! (he = gethostbyname(buf))) {
		fatal("could not get IP address from hostname: %s", buf);
	    }
	    memcpy((char *)&myip, he->h_addr, he->h_length);
	    strcpy(ipbuf, inet_ntoa(myip));
	    ipaddr = ipbuf;
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
	 * We want to get all program events for this node. We will
	 * suck out the objname from the notification, and use that
	 * as the handle while it is running.
	 */
	tuple->host	 = ipaddr;
	tuple->objtype   = TBDB_OBJECTTYPE_PROGRAM;
	tuple->objname   = ADDRESSTUPLE_ANY;
	tuple->eventtype = ADDRESSTUPLE_ANY;

	/*
	 * Register with the event system. 
	 */
	handle = event_register(server, 0);
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

	info("Event: %lu:%d %s %s %s\n", now.tv_sec, now.tv_usec,
	     objname, event, args);

	/*
	 * Dispatch the event. 
	 */
	if (strcmp(event, TBDB_EVENTTYPE_START) == 0)
		start_program(objname, args);
	else if (strcmp(event, TBDB_EVENTTYPE_STOP) == 0)
		stop_program(objname, args);
	else if (strcmp(event, TBDB_EVENTTYPE_KILL) == 0)
		signal_program(objname, args);
	else {
		error("Invalid event: %s\n", event);
		return;
	}
}

/*
 * There are three commands:
 *
 * START	args string is the command to invoke.
 * STOP		no args string.
 * KILL		args string holds the signal *name* to send.
 */

/*
 * Start a program.
 */
static void
start_program(char *objname, char *args)
{
	struct proginfo	*pinfo;
	char		buf[BUFSIZ];
	int		pid, status;
		
	/*
	 * Search for an existing proginfo. If we have one, and its
	 * running, then ignore the start request. The user has to
	 * be smarter than that. We do not release the proginfo when
	 * it stops; why bother.
	 */
	pinfo = proginfos;
	while (pinfo) {
		if (! strcmp(pinfo->name, objname))
			break;
		pinfo = pinfo->next;
	}
	if (!pinfo) {
		pinfo = (struct proginfo *) calloc(1, sizeof(*pinfo));

		if (!pinfo) {
			error("start_program: out of memory\n");
			return;
		}
		strcpy(pinfo->name, objname);
		gettimeofday(&pinfo->started, NULL);
		pinfo->next = proginfos;
		proginfos   = pinfo;
	}
	if (pinfo->pid) {
		/*
		 * We reap children here, rather than using SIGCHLD.
		 * If the program is still running, we are done. 
		 */
		if ((pid = waitpid(pinfo->pid, &status, WNOHANG) <= 0)) {
			if (pid < 0)
				error("start_program: %s waitpid: %s\n",
				      objname, strerror(errno));
			
			warning("start_program: %s is still running: %d\n",
				objname, pinfo->pid);
			return;
		}
		info("start_program: "
		     "%s (pid:%d) has exited with status: 0x%x\n",
		     objname, pinfo->pid, status);
		pinfo->pid = 0;
	}

	/*
	 * The args string holds the command line to execute. We allow
	 * this to be reset in dynamic events, which may seem a little
	 * odd, but its easy to support, so why not.
	 */
	if (strncmp("COMMAND=", args, strlen("COMMAND="))) {
		error("start_program: malformed arguments: %s\n", args);
		return;
	}
	pinfo->cmdline = strdup(&args[strlen("COMMAND=")]);

	/*
	 * Fork a child to run the command in and return to get
	 * more events. 
	 */
	if ((pid = fork()) < 0) {
		error("fork() failed: %s\n", strerror(errno));
		return;
	}
	if (pid) {
		info("start_program: %s (pid:%d) starting\n", objname, pid);
		pinfo->pid = pid;
		return;
	}

	/* Make sure parent and child are fully disconnected */
	if (setsid() == -1) {
		error("setsid() failed: %s\n", strerror(errno));
		return;
	}

	/*
	 * The command is going to be run via the shell. 
	 * We do not know anything about the command line, so we reinit
	 * the log file so that any output goes into a different file. If
	 * the user command line includes a redirection, that will get the
	 * output since the sh will set up the descriptors appropriately.
	 */
	sprintf(buf, "%s%s.debug", _PATH_TMP, pinfo->name);
	loginit(0, buf);
	close(0);

	/*
	 * Exec the shell. We will reap children later by hand, since
	 * I have no idea how SIGCHLD and Elvin interact, and I am not
	 * in the mood to find out!
	 */
	execl(_PATH_CSHELL, "csh", "-f", "-c", pinfo->cmdline, (char *)NULL);

	/* Ug */
	pfatal("start_program: exec failed: %s", pinfo->cmdline);
}

/*
 * Stop a program.
 */
static void
stop_program(char *objname, char *args)
{
	struct proginfo	*pinfo;
		
	/*
	 * Search for an existing proginfo. If there is none, or if
	 * the pid is zero (stopped), then ignore request.
	 */
	pinfo = proginfos;
	while (pinfo) {
		if (! strcmp(pinfo->name, objname))
			break;
		pinfo = pinfo->next;
	}
	if (!pinfo || !pinfo->pid) {
		warning("stop_program: %s is not running!\n", objname);
		return;
	}
	if (killpg(pinfo->pid, SIGTERM) < 0 &&
	    killpg(pinfo->pid, SIGKILL) < 0) {
		error("stop_program: kill() failed: %s!\n", strerror(errno));
	}
}

/*
 * Signal a program.
 */
#ifdef linux
extern const char * const sys_siglist[];
#define sys_signame sys_siglist
#endif

static void
signal_program(char *objname, char *args)
{
	char		buf[BUFSIZ], *bp;
	struct proginfo	*pinfo;
	int		i;
	
	/*
	 * Search for an existing proginfo. If there is none, or if
	 * the pid is zero (stopped), then ignore request.
	 */
	pinfo = proginfos;
	while (pinfo) {
		if (! strcmp(pinfo->name, objname))
			break;
		pinfo = pinfo->next;
	}
	if (!pinfo || !pinfo->pid) {
		warning("signal_program: %s is not running!\n", objname);
		return;
	}

	/*
	 * args string holds the signal number. We can just sccanf it out.
	 */
	if (sscanf(args, "SIGNAL=%s", buf) == 0) {
		error("signal_program: malformed arguments: %s\n", args);
		return;
	}
	bp = buf;

	if (!strncasecmp(buf, "sig", 3))
		bp += 3;
	
	for (i = 1; i < NSIG; i++) {
		if (!strcasecmp(sys_signame[i], bp))
			break;
	}
	if (i == NSIG) {
		error("signal_program: invalid signal: %s\n", buf);
		return;
	}
	
	if (kill(pinfo->pid, i) < 0) {
		error("signal_program: kill(%d) failed: %s!\n", i,
		      strerror(errno));
	}
}
