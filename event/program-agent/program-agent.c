/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * This is a program agent to manage programs from the event system.
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
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include "tbdefs.h"
#include "log.h"
#include "event.h"
#define MAXAGENTS	250

static char		debug;
static int		numagents;
static char		*user;

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
static void	start_program(struct proginfo *pinfo, char *args);
static void	stop_program(struct proginfo *pinfo, char *args);
static void	signal_program(struct proginfo *pinfo, char *args);
static int	parse_configfile(char *filename);

void
usage(char *progname)
{
	fprintf(stderr,
		"Usage: %s [-s server] [-p port] [-l logfile] [-k keyfile] "
		"[-u login] [-i pidfile] -e pid/eid -c configfile\n",
		progname);
	exit(-1);
}

int
main(int argc, char **argv)
{
	FILE *fp;
	event_handle_t handle;
	address_tuple_t	tuple;
	char *progname;
	char *server = "localhost";
	char *port = NULL;
	char *logfile = NULL;
	char *pidfile = NULL;
	char *pideid = NULL;
	char *keyfile = NULL;
	char *configfile = NULL;
	char buf[BUFSIZ], agentlist[BUFSIZ];
	struct proginfo *pinfo;
	int c;

	progname = argv[0];
	bzero(agentlist, sizeof(agentlist));
	
	while ((c = getopt(argc, argv, "s:p:l:du:i:e:c:k:")) != -1) {
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
		case 'c':
			configfile = optarg;
			break;
		case 'd':
			debug++;
			break;
		case 'u':
			user = optarg;
			break;
		case 'i':
			pidfile = optarg;
			break;
		case 'e':
			pideid = optarg;
			break;
		case 'k':
			keyfile = optarg;
			break;
		default:
			usage(progname);
		}
	}
	argc -= optind;
	argv += optind;

	if (!pideid || !configfile)
		usage(progname);
	if (parse_configfile(configfile) != 0)
		exit(1);
	if (!getuid() && !user)
		usage(progname);

	if (debug) 
		loginit(0, logfile);
	else {
		/* Become a daemon */
		daemon(0, 0);

		if (logfile)
			loginit(0, logfile);
		else
			loginit(1, "program-agent");
	}

	/*
	 * Cons up the agentlist for subscription below.
	 */
	pinfo = proginfos;
	while (pinfo) {
		info("AGENT: %s, CMD: %s\n", pinfo->name, pinfo->cmdline);

		if (numagents >= MAXAGENTS)
			fatal("Too many agents listed");

		numagents++;
		if (strlen(agentlist))
			strcat(agentlist, ",");
		strcat(agentlist, pinfo->name);
		
		pinfo = pinfo->next;
	}
	info("agentlist: %s\n", agentlist);
	info("user: %s\n", user);

	/*
	 * Write out a pidfile if root.
	 */
	if (!getuid()) {
		if (pidfile)
			strcpy(buf, pidfile);
		else
			sprintf(buf, "%s/progagent.pid", _PATH_VARRUN);
		fp = fopen(buf, "w");
		if (fp != NULL) {
			fprintf(fp, "%d\n", getpid());
			(void) fclose(fp);
		}
	}

	/*
	 * Flip to the user, but only if we are currently root.
	 */
	if (! getuid()) {
		struct passwd *pw;

		/*
		 * Must be a valid user of course.
		 */
		if ((pw = getpwnam(user)) == NULL)
			fatal("invalid user: %s", user);

		/*
		 * Initialize the group list, and then flip to uid.
		 */
		if (setgid(pw->pw_gid) ||
		    initgroups(user, pw->pw_gid) ||
		    setuid(pw->pw_uid)) {
			fatal("Could not become user: %s", user);
		}
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
	 * Ask for just the program agents we care about. 
	 */
	tuple->expt      = pideid;
	tuple->objtype   = TBDB_OBJECTTYPE_PROGRAM;
	tuple->objname   = agentlist;
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
	struct proginfo *pinfo;
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

	pinfo = proginfos;
	while (pinfo) {
		if (! strcmp(pinfo->name, objname))
			break;
		pinfo = pinfo->next;
	}
	if (!pinfo) {
		error("Invalid program agent: %s\n", objname);
		return;
	}

	/*
	 * Dispatch the event. 
	 */
	if (strcmp(event, TBDB_EVENTTYPE_START) == 0)
		start_program(pinfo, args);
	else if (strcmp(event, TBDB_EVENTTYPE_STOP) == 0)
		stop_program(pinfo, args);
	else if (strcmp(event, TBDB_EVENTTYPE_KILL) == 0)
		signal_program(pinfo, args);
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
start_program(struct proginfo *pinfo, char *args)
{
	char		buf[BUFSIZ];
	int		pid, status;
		
	if (pinfo->pid) {
		/*
		 * We reap children here, rather than using SIGCHLD.
		 * If the program is still running, we are done. 
		 */
		if ((pid = waitpid(pinfo->pid, &status, WNOHANG) <= 0)) {
			if (pid < 0)
				error("start_program: %s waitpid: %s\n",
				      pinfo->name, strerror(errno));
			
			warning("start_program: %s is still running: %d\n",
				pinfo->name, pinfo->pid);
			return;
		}
		info("start_program: "
		     "%s (pid:%d) has exited with status: 0x%x\n",
		     pinfo->name, pinfo->pid, status);
		pinfo->pid = 0;
	}

	/*
	 * The args string holds the command line to execute. We allow
	 * this to be reset in dynamic events, but is optional; the cuurent
	 * command will be used by default, which initially comes from tmcd.
	 */
	if (args && strlen(args) &&
	    !strncmp("COMMAND=", args, strlen("COMMAND="))) {
		if (pinfo->cmdline)
			free(pinfo->cmdline);
		pinfo->cmdline = strdup(&args[strlen("COMMAND=")]);
	}

	/*
	 * Fork a child to run the command in and return to get
	 * more events. 
	 */
	if ((pid = fork()) < 0) {
		error("fork() failed: %s\n", strerror(errno));
		return;
	}
	if (pid) {
		info("start_program: %s (pid:%d) starting\n", pinfo->name,pid);
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
stop_program(struct proginfo *pinfo, char *args)
{
	if (!pinfo->pid) {
		warning("stop_program: %s is not running!\n", pinfo->name);
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
#undef NSIG
#define NSIG 32
const char *const sys_signame[NSIG] = {
	"Signal 0",
	"hup",				/* SIGHUP */
	"int",				/* SIGINT */
	"quit",				/* SIGQUIT */
	"ill",				/* SIGILL */
	"trap",				/* SIGTRAP */
	"abrt",				/* SIGABRT */
	"bus",				/* SIGBUS */
	"fpe",				/* SIGFPE */
	"kill",				/* SIGKILL */
	"usr1",				/* SIGUSR1 */
	"segv",				/* SIGSEGV */
	"usr2",				/* SIGUSR2 */
	"pipe",				/* SIGPIPE */
	"alrm",				/* SIGALRM */
	"term",				/* SIGTERM */
	"stkflt",			/* SIGSTKFLT */
	"chld",				/* SIGCHLD */
	"cont",				/* SIGCONT */
	"stop",				/* SIGSTOP */
	"tstp",				/* SIGTSTP */
	"ttin",				/* SIGTTIN */
	"ttou",				/* SIGTTOU */
	"urg",				/* SIGURG */
	"xcpu",				/* SIGXCPU */
	"xfsz",				/* SIGXFSZ */
	"vtalrm",			/* SIGVTALRM */
	"prof",				/* SIGPROF */
	"winch",			/* SIGWINCH */
	"io",				/* SIGIO */
	"pwr",				/* SIGPWR */
	"sys",				/* SIGSYS */
};
#endif
#ifdef __CYGWIN__
#undef NSIG
#define NSIG 32
const char *const sys_signame[NSIG] = {
	"Signal 0",
	"hup",				/* SIGHUP */
	"int",				/* SIGINT */
	"quit",				/* SIGQUIT */
	"ill",				/* SIGILL */
	"trap",				/* SIGTRAP */
	"abrt",				/* SIGABRT */
	"emt",				/* SIGEMT */
	"fpe",				/* SIGFPE */
	"kill",				/* SIGKILL */
	"bus",				/* SIGBUS */
	"segv",				/* SIGSEGV */
	"sys",				/* SIGSYS */
	"pipe",				/* SIGPIPE */
	"alrm",				/* SIGALRM */
	"term",				/* SIGTERM */
	"urg",				/* SIGURG */
	"stop",				/* SIGSTOP */
	"tstp",				/* SIGTSTP */
	"cont",				/* SIGCONT */
	"chld",				/* SIGCHLD */
	"ttin",				/* SIGTTIN */
	"ttou",				/* SIGTTOU */
	"io",				/* SIGIO */
	"xcpu",				/* SIGXCPU */
	"xfsz",				/* SIGXFSZ */
	"vtalrm",			/* SIGVTALRM */
	"prof",				/* SIGPROF */
	"winch",			/* SIGWINCH */
	"lost",				/* SIGLOST */
	"usr1",				/* SIGUSR1 */
	"usr2",				/* SIGUSR2 */
};
#endif

static void
signal_program(struct proginfo *pinfo, char *args)
{
	char		buf[BUFSIZ], *bp;
	int		i;
	
	if (!pinfo->pid) {
		warning("signal_program: %s is not running!\n", pinfo->name);
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

static int
parse_configfile(char *filename)
{
	FILE	*fp;
	char	buf[BUFSIZ], *bp, *cp;
	struct proginfo *pinfo;
	
	if ((fp = fopen(filename, "r")) == NULL) {
		errorc("could not open configfile %s", filename);
		return -1;
	}
	
	while (fgets(buf, sizeof(buf), fp)) {
		int cc = strlen(buf);
		if (buf[cc-1] == '\n')
			buf[cc-1] = (char) NULL;

		if (!strncmp(buf, "UID=", 4)) {
			if (user) {
				info("User already set; skipping config\n");
				continue;
			}
			user = strdup(&buf[4]);
			if (!user) {
				error("parse_configfile: out of memory\n");
				goto bad;
			}
			continue;
		}
		if (!strncmp(buf, "AGENT=", 6)) {
			pinfo = (struct proginfo *) calloc(1, sizeof(*pinfo));

			if (!pinfo) {
				error("parse_configfile: out of memory\n");
				goto bad;
			}
			bp = cp = buf + 6;
			while (*cp && !isspace(*bp))
				bp++;
			*bp++ = (char) NULL;
			strncpy(pinfo->name, cp, sizeof(pinfo->name));

			if (strncmp(bp, "COMMAND='", 9)) {
				error("parse_configfile: malformed: %s\n", bp);
				goto bad;
			}
			bp += 9;
			
			cc = strlen(bp);
			if (bp[cc-1] != '\'') {
				error("parse_configfile: malformed: %s\n", bp);
				goto bad;
			}
			bp[cc-1] = (char) NULL;
			
			pinfo->cmdline = strdup(bp);
			if (!pinfo->cmdline) {
				error("parse_configfile: out of memory\n");
				goto bad;
			}
			pinfo->next = proginfos;
			proginfos   = pinfo;
			continue;
		}
		error("parse_configfile: malformed: %s\n", buf);
		goto bad;
	}
	fclose(fp);
	return 0;
 bad:
	fclose(fp);
	return -1;
}
