/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Link Agent: supports up/down, parameterized modifications to interfaces.  
 * Commands are passed to a perl script and interpreted and executed in 
 * OS/device specific ways; this makes life easier for us all.
 */

#include <stdio.h>
#include <ctype.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <paths.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include "event.h"
#include "log.h"
#include "tbdefs.h"

static char	*progname;
static int	debug = 0;
static int	verbose = 0;
static char     *ifdynconfig = "/usr/local/etc/emulab/ifdynconfig";

static void	callback(event_handle_t handle,
			 event_notification_t notification, void *data);
static int	runcommand(char *cmd);
static char    *convertmac(char *mac);

/*
 * Map from the object name to the interface we actually need to change.
 */
typedef struct ifmap {
	char		*objname;
	char		*iface;
	char		*mac;
	struct ifmap	*next;
} ifmap_t;
ifmap_t			*mapping = (ifmap_t *) NULL;

void
usage()
{
	fprintf(stderr,
		"Usage: %s [-s server] [-p port] [-k keyfile] [-l logfile] "
		"[-i pidfile] -e pid/eid [names ...]\n", progname);
	exit(-1);
}

static inline void
upcase(char *str)
{
	if (str) {
		while (*str) {
			*str = toupper(*str);
			str++;
		}
	}
}

int
main(int argc, char **argv)
{
	event_handle_t		handle;
	address_tuple_t		tuple;
	char			buf[BUFSIZ];
	char			*server   = "localhost";
	char			*port     = NULL;
	char			*keyfile  = NULL;
	char			*pideid   = NULL;
	char			*logfile  = NULL;
	char			*pidfile  = NULL;
	int			c;

	progname = argv[0];
	
	while ((c = getopt(argc, argv, "dvs:p:e:k:i:l:")) != -1) {
		switch (c) {
		case 'd':
			debug++;
			break;
		case 'l':
			logfile = optarg;
			break;
		case 'i':
			pidfile = optarg;
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
		case 'k':
			keyfile = optarg;
			break;
		case 'v':
			verbose++;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (!pideid)
	        usage();
	if (!argc)
		usage();

	if (debug) 
		loginit(0, logfile);
	else {
		/* Become a daemon */
		daemon(0, 0);

		if (logfile)
			loginit(0, logfile);
		else
			loginit(1, "link-agent");
	}
	debug = verbose;

	/*
	 * Write out a pidfile if root.
	 */
	if (!getuid()) {
		FILE	*fp;
		
		if (pidfile)
			strcpy(buf, pidfile);
		else
			sprintf(buf, "%s/linkagent.pid", _PATH_VARRUN);
		fp = fopen(buf, "w");
		if (fp != NULL) {
			fprintf(fp, "%d\n", getpid());
			(void) fclose(fp);
		}
	}

	/*
	 * Convert server/port to elvin thing.
	 *
	 * XXX This elvin string stuff should be moved down a layer. 
	 */
	snprintf(buf, sizeof(buf), "elvin://%s%s%s",
		 server,
		 (port ? ":"  : ""),
		 (port ? port : ""));
	server = buf;

	/* Register with the event system: */
	handle = event_register_withkeyfile(server, 0, keyfile);
	if (handle == NULL) {
		fatal("could not register with event system");
	}

	/*
	 * Construct an address tuple for subscribing to events for
	 * this node.
	 */
	tuple = address_tuple_alloc();
	if (tuple == NULL) {
		fatal("could not allocate an address tuple");
	}

	tuple->host	 = ADDRESSTUPLE_ANY;
	tuple->site      = ADDRESSTUPLE_ANY;
	tuple->group     = ADDRESSTUPLE_ANY;
	tuple->expt      = pideid;
	tuple->objtype   = ADDRESSTUPLE_ANY;
	tuple->eventtype = ADDRESSTUPLE_ANY;
	tuple->objname   = "";	/* See below */

	/*
	 * Process the rest of the arguments. They are of the form:
	 *
	 *	linkname,vnode,iface,mac linkname,vnode,iface,mac ....
	 */
	while (argc) {
		char		*bp, *ap = *argv;
		ifmap_t		*mp;
		char		*link, *vnode, *iface, *mac;

		if ((bp = index(ap, ',')) == NULL)
			usage();
		*bp++ = (char) NULL;
		link  = ap;
		vnode = bp;

		if ((bp = index(bp, ',')) == NULL)
			usage();
		*bp++ = (char) NULL;
		iface = bp;

		if ((bp = index(bp, ',')) == NULL)
			usage();
		*bp++ = (char) NULL;
		mac   = convertmac(bp);
		if (! mac) {
			fatal("Must have a proper MAC!");
		}
		mac = strdup(mac);

		/*
		 * Need two mappings; one for the entire link/lan, and one
		 * for just this member of the link,lan.
		 */
		if ((mp = calloc(1, sizeof(*mp))) == NULL)
			fatal("Out of memory!");
		mp->iface   = iface;
		mp->mac     = mac;
		mp->objname = link;
		mp->next    = mapping;
		mapping     = mp;

		if (debug)
			info("%s %s %s\n", mp->objname, mp->iface, mp->mac);

		if ((mp = calloc(1, sizeof(*mp))) == NULL)
			fatal("Out of memory!");
		strcpy(buf, link);
		strcat(buf, "-");
		strcat(buf, vnode);
		mp->iface   = iface;
		mp->mac     = mac;
		mp->objname = strdup(buf);
		mp->next    = mapping;
		mapping     = mp;

		if (debug)
			info("%s %s %s\n", mp->objname, mp->iface, mp->mac);
		
		/*
		 * Also build up a subscription string.
		 */
		if ((bp = malloc(strlen(tuple->objname) + strlen(link) +
				 strlen(buf) + 3)) == NULL)
			fatal("Out of memory!");
		if (*(tuple->objname)) {
			strcpy(bp, tuple->objname);
			strcat(bp, ",");
		}
		else
			*bp = (char) NULL;
		strcat(bp, link);
		strcat(bp, ",");
		strcat(bp, buf);
		tuple->objname = bp;

		argc--;
		argv++;

	}
	if (debug)
		info("Subscribing to %s\n", tuple->objname);
	  
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

	/* Unregister with the event system: */
	if (event_unregister(handle) == 0) {
		fatal("could not unregister with event system");
	}
	
	return 0;
}

/* Output macro to check for string overflow */
#define OUTPUT(buf, size, format...) \
({ \
	int __count__ = snprintf((buf), (size), ##format); \
        \
        if (__count__ >= (size)) { \
		error("Not enough room in output buffer! line %d.\n", __LINE__);\
		return; \
	} \
	__count__; \
})

static void
callback(event_handle_t handle, event_notification_t notification, void *data)
{
	char		objname[TBDB_FLEN_EVOBJNAME];
	char		eventtype[TBDB_FLEN_EVEVENTTYPE];
	char		args[2 * BUFSIZ];
	char		cmd[2 * BUFSIZ];
	ifmap_t		*mp;

	event_notification_get_objname(handle, notification,
				       objname, sizeof(objname));
	event_notification_get_eventtype(handle, notification,
					 eventtype, sizeof(eventtype));
	event_notification_get_arguments(handle, notification,
					args, sizeof(args));

	if (debug)
		info("%s %s %s\n", objname, eventtype, args);

	/*
	 * Find the mapping for this event so we know what interface to
	 * mess with.
	 */
	mp = mapping;
	while (mp) {
		if (strcmp(mp->objname, objname) == 0)
			break;
		mp = mp->next;
	}
	if (!mp) {
		error("Unknown object name: %s\n", objname);
		return;
	}
	
	/*
	 * The idea is this; there are currently two (or more) link agents
	 * vying for the same LINK events. We just worry about the ones
	 * we care about and ignore the rest. For example, the delay agent
	 * is also listening to these events.
	 */
	if (strcmp(eventtype, TBDB_EVENTTYPE_UP) == 0) {
		/* sprintf(cmd, "iwconfig %s txpower auto", mp->iface); */
	        sprintf(cmd, "%s %s up", ifdynconfig, mp->iface);
		runcommand(cmd);
	}
	else if (strcmp(eventtype, TBDB_EVENTTYPE_DOWN) == 0) {
		/* sprintf(cmd, "iwconfig %s txpower off", mp->iface); */
		sprintf(cmd, "%s %s down", ifdynconfig, mp->iface);
		runcommand(cmd);
	}
	else if (strcmp(eventtype, TBDB_EVENTTYPE_MODIFY) == 0) {
		char	*ap = args;
		char    *cp = cmd, *ecp = &cmd[sizeof(cmd)-1];
		int	cmdlen;
		char    cargs[2 * BUFSIZ];
		char    *cap = cargs, *ecap = &cargs[sizeof(cargs)-1];
		int     found_enable = 0;

		cp += OUTPUT(cp,sizeof(cmd),"%s %s ",ifdynconfig,mp->iface);
		cmdlen = strlen(cmd);

		/*
		 * Perl string matching would be so much nicer!
		 */
		while (*ap) {
			char	*kp, *setting, *value;
			int     quoted = 0;
		
			while (*ap == ' ')
				ap++;
			if (! *ap)
				break;

			if ((kp = index(ap, '=')) == NULL) {
				error("Could(1) not parse arg at '%s'\n", ap);
				return;
			}
			setting = ap;
			*kp++ = '\0';
			ap = kp;

			/*
			 * Setting is either enclosed in quotes, or a single
			 * token with no spaces in it.
			 */
			if (*ap == '\'') {
				quoted = 1;
				ap++;
			}
			value = ap;

			/*
			 * Scan for end of value; up until quote,
			 * space, or null. 
			 */
			while (*ap) {
				if ((quoted && *ap == '\'') || *ap == ' ')
					break;
				ap++;
			}
			if ((quoted && *ap != '\'') ||
			    (!quoted && (*ap != '\0' && *ap != ' '))) {
				error("Could(2) not parse arg at '%s'\n", ap);
				return;
			}
			*ap = '\0';
			/* Skip past close quote. */
			if (quoted)
				ap++;
			ap++;

			/*
			 * Okay, now do something with setting and value.
			 */
			if (! strcasecmp("enable", setting)) {
				/*
				 * Alias for UP/DOWN events above. Note that
				 * we want to run this first/last. 
				 */
				found_enable = 1;
				if (! strcasecmp("yes", value)) {
					cp += OUTPUT(cp,ecp - cp, " up");
				}
				else if (! strcasecmp("no", value)) {
					cp += OUTPUT(cp,ecp - cp, " down");
				}
				else {
					error("Ignoring setting: %s=%s\n",
					      setting, value);
					found_enable = 0;
					continue;
				}
				
			}
			else {
			    cap += OUTPUT(cap,ecap - cap," %s=%s",
					  setting,value);
			}
		}
		if (!found_enable) {
			cp += OUTPUT(cp,ecp - cp," modify");
		}
		cp += OUTPUT(cp,ecp - cp," %s",cargs);
		if (strlen(cmd) > cmdlen)
			runcommand(cmd);
	}
	else if (debug) {
		info("Ignoring event: %s %s %s\n", objname, eventtype, args);
	}
	
}

/*
 * Run a command. We use popen so we capture the output and send it to the
 * the log file.
 */
static int
runcommand(char *cmd)
{
	FILE	*fp;
	char	buf[BUFSIZ];

	info("%s\n", cmd);
	
	if ((fp = popen(cmd, "r")) == NULL) {
		info("popen failed!\n");
		return -1;
	}
	while (fgets(buf, sizeof(buf), fp)) {
		info(buf);
	}
	return pclose(fp);
}

static char *
convertmac(char *from)
{
	int		a,b,c,d,e,f;
	static char	buf[32];

	if (sscanf(from, "%2x%2x%2x%2x%2x%2x", &a, &b, &c, &d, &e, &f) != 6) {
		error("Bad mac: %s\n", from);
		return (char *) NULL;
	}
	sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", a, b, c, d, e, f);
	return buf;
}
