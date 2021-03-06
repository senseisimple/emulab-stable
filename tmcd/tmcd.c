/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2011 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/fcntl.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <paths.h>
#include <setjmp.h>
#include <pwd.h>
#include <grp.h>
#include <mysql/mysql.h>
#include "tmcd.h"
#include "config.h"
#include "ssl.h"
#include "log.h"
#include "tbdefs.h"
#include "bootwhat.h"
#include "bootinfo.h"

#ifdef EVENTSYS
#include "event.h"
#endif

/* XXX: Not sure this is okay! */
#include "tpm.h"

/*
 * XXX This needs to be localized!
 */
#define FSPROJDIR	FSNODE ":" FSDIR_PROJ
#define FSGROUPDIR	FSNODE ":" FSDIR_GROUPS
#define FSUSERDIR	FSNODE ":" FSDIR_USERS
#ifdef  FSDIR_SHARE
#define FSSHAREDIR	FSNODE ":" FSDIR_SHARE
#endif
#ifdef  FSDIR_SCRATCH
#define FSSCRATCHDIR	FSNODE ":" FSDIR_SCRATCH
#endif
#define PROJDIR		PROJROOT_DIR
#define GROUPDIR	GROUPSROOT_DIR
#define USERDIR		USERSROOT_DIR
#define SCRATCHDIR	SCRATCHROOT_DIR
#define SHAREDIR	SHAREROOT_DIR
#define NETBEDDIR	"/netbed"
#define PLISALIVELOGDIR "/usr/testbed/log/plabisalive"
#define RELOADPID	"emulab-ops"
#define RELOADEID	"reloading"
#define FSHOSTID	"/usr/testbed/etc/fshostid"
#define DOTSFS		".sfs"
#define RUNASUSER	"nobody"
#define RUNASGROUP	"nobody"
#define NTPSERVER       "ntp1"
#define PROTOUSER	"elabman"
#define PRIVKEY_LEN	128
#define URN_LEN		128
#define XSTRINGIFY(s)   STRINGIFY(s)
#define STRINGIFY(s)	#s

/* XXX backward compat */
#ifndef TBCOREDIR
#define	TBCOREDIR	TBROOT "/tmp"
#endif

/* socket read/write timeouts in ms */
#define READTIMO	3000
#define WRITETIMO	3000

#define TESTMODE
#define DEFAULTNETMASK	"255.255.255.0"
/* This can be tossed once all the changes are in place */
static char *
CHECKMASK(char *arg)
{
	if (arg && arg[0])
		return arg;

	error("No netmask defined!\n");
	return DEFAULTNETMASK;
}
/* #define CHECKMASK(arg)  ((arg) && (arg[0]) ? (arg) : DEFAULTNETMASK) */

#define DISKTYPE	"ad"
#define DISKNUM		0

/* Compiled in slothd parameters
 *
 * 1 - reg_interval  2 - agg_interval  3 - load_thresh
 * 4 - expt_thresh   5 - ctl_thresh
 */
#define SDPARAMS        "reg=300 agg=5 load=1 expt=5 ctl=1000"

/* Defined in configure and passed in via the makefile */
#define DBNAME_SIZE	64
#define HOSTID_SIZE	(32+64)
#define DEFAULT_DBNAME	TBDBNAME

/* For secure disk loading */
#define SECURELOAD_OPMODE "SECURELOAD"
#define SECURELOAD_STATE  "RELOADSETUP"

int		debug = 0;
static int	verbose = 0;
static int	insecure = 0;
static int	byteswritten = 0;
static char	pidfile[MAXPATHLEN];
static char     dbname[DBNAME_SIZE];
static struct in_addr myipaddr;
static char	fshostid[HOSTID_SIZE];
static int	nodeidtoexp(char *nodeid, char *pid, char *eid, char *gid);
static void	tcpserver(int sock, int portnum);
static void	udpserver(int sock, int portnum);
static int      handle_request(int, struct sockaddr_in *, char *, int);
static int      checkcerts(char*);
static int	makesockets(int portnum, int *udpsockp, int *tcpsockp);
int		client_writeback(int sock, void *buf, int len, int tcp);
void		client_writeback_done(int sock, struct sockaddr_in *client);
MYSQL_RES *	mydb_query(char *query, int ncols, ...);
int		mydb_update(char *query, ...);
static int	safesymlink(char *name1, char *name2);

/* socket timeouts */
static int	readtimo = READTIMO;
static int	writetimo = WRITETIMO;

/* thread support */
#define MAXCHILDREN	20
#define MINCHILDREN	8
static int	numchildren;
static int	maxchildren       = 13;
static int      num_udpservers    = 3;
static int      num_altudpservers = 1;
static int      num_alttcpservers = 1;
static int	mypid;
static volatile int killme;

/* Output macro to check for string overflow */
#define OUTPUT(buf, size, format...) \
({ \
	int __count__ = snprintf((buf), (size), ##format); \
        \
        if (__count__ >= (size)) { \
		error("Not enough room in output buffer! line %d.\n", __LINE__);\
		return 1; \
	} \
	__count__; \
})

/*
 * This structure is passed to each request function. The intent is to
 * reduce the number of DB queries per request to a minimum.
 */
typedef struct {
	struct in_addr  client;
	int		allocated;
	int		jailflag;
	int		isvnode;
	int		issubnode;
	int		islocal;
	int		isdedicatedwa;
	int		iscontrol;
	int		isplabdslice;
	int		isplabsvc;
	int		elab_in_elab;
        int		singlenet;	  /* Modifier for elab_in_elab */
	int		update_accounts;
	int		exptidx;
	int		creator_idx;
	int		swapper_idx;
	int		swapper_isadmin;
        int		genisliver_idx;
        int		geniflags;
	char		nodeid[TBDB_FLEN_NODEID];
	char		vnodeid[TBDB_FLEN_NODEID];
	char		pnodeid[TBDB_FLEN_NODEID]; /* XXX */
	char		pid[TBDB_FLEN_PID];
	char		eid[TBDB_FLEN_EID];
	char		gid[TBDB_FLEN_GID];
	char		nickname[TBDB_FLEN_VNAME];
	char		type[TBDB_FLEN_NODETYPE];
	char		class[TBDB_FLEN_NODECLASS];
        char		ptype[TBDB_FLEN_NODETYPE];	/* Of physnode */
	char		pclass[TBDB_FLEN_NODECLASS];	/* Of physnode */
	char		creator[TBDB_FLEN_UID];
	char		swapper[TBDB_FLEN_UID];
	char		syncserver[TBDB_FLEN_VNAME];	/* The vname */
	char		keyhash[TBDB_FLEN_PRIVKEY];
	char		eventkey[TBDB_FLEN_PRIVKEY];
	char		sfshostid[TBDB_FLEN_SFSHOSTID];
	char		testdb[TBDB_FLEN_TINYTEXT];
	char		sharing_mode[TBDB_FLEN_TINYTEXT];
	char            privkey[PRIVKEY_LEN+1];
        /* This key is a replacement for privkey, on protogeni resources */
	char            external_key[PRIVKEY_LEN+1];
} tmcdreq_t;
static int	iptonodeid(struct in_addr, tmcdreq_t *, char*);
static int	checkdbredirect(tmcdreq_t *);

#ifdef EVENTSYS
int			myevent_send(address_tuple_t address);
static event_handle_t	event_handle = NULL;
#endif

/*
 * Commands we support.
 */
#define COMMAND_PROTOTYPE(x) \
	static int \
	x(int sock, tmcdreq_t *reqp, char *rdata, int tcp, int vers)

COMMAND_PROTOTYPE(doreboot);
COMMAND_PROTOTYPE(donodeid);
COMMAND_PROTOTYPE(domanifest);
COMMAND_PROTOTYPE(dostatus);
COMMAND_PROTOTYPE(doifconfig);
COMMAND_PROTOTYPE(doaccounts);
COMMAND_PROTOTYPE(dobridges);
COMMAND_PROTOTYPE(dodelay);
COMMAND_PROTOTYPE(dolinkdelay);
COMMAND_PROTOTYPE(dohosts);
COMMAND_PROTOTYPE(dorpms);
COMMAND_PROTOTYPE(dodeltas);
COMMAND_PROTOTYPE(dotarballs);
COMMAND_PROTOTYPE(doblobs);
COMMAND_PROTOTYPE(dostartcmd);
COMMAND_PROTOTYPE(dostartstat);
COMMAND_PROTOTYPE(doready);
COMMAND_PROTOTYPE(doreadycount);
COMMAND_PROTOTYPE(domounts);
COMMAND_PROTOTYPE(dosfshostid);
COMMAND_PROTOTYPE(doloadinfo);
COMMAND_PROTOTYPE(doreset);
COMMAND_PROTOTYPE(dorouting);
COMMAND_PROTOTYPE(dotrafgens);
COMMAND_PROTOTYPE(donseconfigs);
COMMAND_PROTOTYPE(dostate);
COMMAND_PROTOTYPE(docreator);
COMMAND_PROTOTYPE(dotunnels);
COMMAND_PROTOTYPE(dovnodelist);
COMMAND_PROTOTYPE(dosubnodelist);
COMMAND_PROTOTYPE(doisalive);
COMMAND_PROTOTYPE(doipodinfo);
COMMAND_PROTOTYPE(dontpinfo);
COMMAND_PROTOTYPE(dontpdrift);
COMMAND_PROTOTYPE(dojailconfig);
COMMAND_PROTOTYPE(doplabconfig);
COMMAND_PROTOTYPE(dosubconfig);
COMMAND_PROTOTYPE(doixpconfig);
COMMAND_PROTOTYPE(doslothdparams);
COMMAND_PROTOTYPE(doprogagents);
COMMAND_PROTOTYPE(dosyncserver);
COMMAND_PROTOTYPE(dokeyhash);
COMMAND_PROTOTYPE(doeventkey);
COMMAND_PROTOTYPE(dofullconfig);
COMMAND_PROTOTYPE(doroutelist);
COMMAND_PROTOTYPE(dorole);
COMMAND_PROTOTYPE(dorusage);
COMMAND_PROTOTYPE(dodoginfo);
COMMAND_PROTOTYPE(dohostkeys);
COMMAND_PROTOTYPE(dotmcctest);
COMMAND_PROTOTYPE(dofwinfo);
COMMAND_PROTOTYPE(dohostinfo);
COMMAND_PROTOTYPE(doemulabconfig);
COMMAND_PROTOTYPE(doeplabconfig);
COMMAND_PROTOTYPE(dolocalize);
COMMAND_PROTOTYPE(dorootpswd);
COMMAND_PROTOTYPE(dobooterrno);
COMMAND_PROTOTYPE(dobootlog);
COMMAND_PROTOTYPE(dobattery);
COMMAND_PROTOTYPE(dotopomap);
COMMAND_PROTOTYPE(douserenv);
COMMAND_PROTOTYPE(dotiptunnels);
COMMAND_PROTOTYPE(dorelayconfig);
COMMAND_PROTOTYPE(dotraceconfig);
COMMAND_PROTOTYPE(doltmap);
COMMAND_PROTOTYPE(doltpmap);
COMMAND_PROTOTYPE(doelvindport);
COMMAND_PROTOTYPE(doplabeventkeys);
COMMAND_PROTOTYPE(dointfcmap);
COMMAND_PROTOTYPE(domotelog);
COMMAND_PROTOTYPE(doportregister);
COMMAND_PROTOTYPE(dobootwhat);
COMMAND_PROTOTYPE(dotpmblob);
COMMAND_PROTOTYPE(dotpmpubkey);
COMMAND_PROTOTYPE(dotpmdummy);
COMMAND_PROTOTYPE(dodhcpdconf);
COMMAND_PROTOTYPE(dosecurestate);
COMMAND_PROTOTYPE(doquoteprep);
COMMAND_PROTOTYPE(doimagekey);

/*
 * The fullconfig slot determines what routines get called when pushing
 * out a full configuration. Physnodes get slightly different
 * than vnodes, and at some point we might want to distinguish different
 * types of vnodes (jailed, plab).
 */
#define FULLCONFIG_NONE		0x0
#define FULLCONFIG_PHYS		0x1
#define FULLCONFIG_VIRT		0x2
#define FULLCONFIG_ALL		(FULLCONFIG_PHYS|FULLCONFIG_VIRT)

/*
 * Flags encode a few other random properties of commands
 */
#define F_REMUDP	0x01	/* remote nodes can request using UDP */
#define F_MINLOG	0x02	/* record minimal logging info normally */
#define F_MAXLOG	0x04	/* record maximal logging info normally */
#define F_ALLOCATED	0x08	/* node must be allocated to make call */
#define F_REMNOSSL	0x10	/* remote nodes can request without SSL */
#define F_REMREQSSL	0x20	/* remote nodes must connect with SSL */
#define F_REQTPM	0x40	/* require TPM on client */

struct command {
	char	*cmdname;
	int	fullconfig;
	int	flags;
	int    (*func)(int, tmcdreq_t *, char *, int, int);
} command_array[] = {
	{ "reboot",	  FULLCONFIG_NONE, 0, doreboot },
	{ "nodeid",	  FULLCONFIG_ALL,  0, donodeid },
	{ "manifest",	  FULLCONFIG_ALL,  0, domanifest },
	{ "status",	  FULLCONFIG_NONE, 0, dostatus },
	{ "ifconfig",	  FULLCONFIG_ALL,  F_ALLOCATED, doifconfig },
	{ "accounts",	  FULLCONFIG_ALL,  F_REMREQSSL, doaccounts },
	{ "delay",	  FULLCONFIG_ALL,  F_ALLOCATED, dodelay },
	{ "bridges",	  FULLCONFIG_ALL,  F_ALLOCATED, dobridges },
	{ "linkdelay",	  FULLCONFIG_ALL,  F_ALLOCATED, dolinkdelay },
	{ "hostnames",	  FULLCONFIG_NONE, F_ALLOCATED, dohosts },
	{ "rpms",	  FULLCONFIG_ALL,  F_ALLOCATED, dorpms },
	{ "deltas",	  FULLCONFIG_NONE, F_ALLOCATED, dodeltas },
	{ "tarballs",	  FULLCONFIG_ALL,  F_ALLOCATED, dotarballs },
	{ "blobs",	  FULLCONFIG_ALL,  F_ALLOCATED, doblobs },
	{ "startupcmd",	  FULLCONFIG_ALL,  F_ALLOCATED, dostartcmd },
	{ "startstatus",  FULLCONFIG_NONE, F_ALLOCATED, dostartstat }, /* Before startstat*/
	{ "startstat",	  FULLCONFIG_NONE, 0, dostartstat },
	{ "readycount",   FULLCONFIG_NONE, F_ALLOCATED, doreadycount },
	{ "ready",	  FULLCONFIG_NONE, F_ALLOCATED, doready },
	{ "mounts",	  FULLCONFIG_ALL,  F_ALLOCATED, domounts },
	{ "sfshostid",	  FULLCONFIG_NONE, F_ALLOCATED, dosfshostid },
	{ "loadinfo",	  FULLCONFIG_NONE, 0, doloadinfo},
	{ "reset",	  FULLCONFIG_NONE, 0, doreset},
	{ "routing",	  FULLCONFIG_ALL,  F_ALLOCATED, dorouting},
	{ "trafgens",	  FULLCONFIG_ALL,  F_ALLOCATED, dotrafgens},
	{ "nseconfigs",	  FULLCONFIG_ALL,  F_ALLOCATED, donseconfigs},
	{ "creator",	  FULLCONFIG_ALL,  F_ALLOCATED, docreator},
	{ "state",	  FULLCONFIG_NONE, 0, dostate},
	{ "tunnels",	  FULLCONFIG_ALL,  F_ALLOCATED, dotunnels},
	{ "vnodelist",	  FULLCONFIG_PHYS, 0, dovnodelist},
	{ "subnodelist",  FULLCONFIG_PHYS, 0, dosubnodelist},
	{ "isalive",	  FULLCONFIG_NONE, F_REMUDP|F_MINLOG, doisalive},
	{ "ipodinfo",	  FULLCONFIG_PHYS, 0, doipodinfo},
	{ "ntpinfo",	  FULLCONFIG_PHYS, 0, dontpinfo},
	{ "ntpdrift",	  FULLCONFIG_NONE, 0, dontpdrift},
	{ "jailconfig",	  FULLCONFIG_VIRT, F_ALLOCATED, dojailconfig},
	{ "plabconfig",	  FULLCONFIG_VIRT, F_ALLOCATED, doplabconfig},
	{ "subconfig",	  FULLCONFIG_NONE, 0, dosubconfig},
        { "sdparams",     FULLCONFIG_PHYS, 0, doslothdparams},
        { "programs",     FULLCONFIG_ALL,  F_ALLOCATED, doprogagents},
        { "syncserver",   FULLCONFIG_ALL,  F_ALLOCATED, dosyncserver},
        { "keyhash",      FULLCONFIG_ALL,  F_ALLOCATED|F_REMREQSSL, dokeyhash},
        { "eventkey",     FULLCONFIG_ALL,  F_ALLOCATED|F_REMREQSSL, doeventkey},
        { "fullconfig",   FULLCONFIG_NONE, F_ALLOCATED, dofullconfig},
        { "routelist",	  FULLCONFIG_PHYS, F_ALLOCATED, doroutelist},
        { "role",	  FULLCONFIG_PHYS, F_ALLOCATED, dorole},
        { "rusage",	  FULLCONFIG_NONE, F_REMUDP|F_MINLOG, dorusage},
        { "watchdoginfo", FULLCONFIG_ALL,  F_REMUDP|F_MINLOG, dodoginfo},
        { "hostkeys",     FULLCONFIG_NONE, 0, dohostkeys},
        { "tmcctest",     FULLCONFIG_NONE, F_MINLOG, dotmcctest},
        { "firewallinfo", FULLCONFIG_ALL,  0, dofwinfo},
        { "hostinfo",     FULLCONFIG_NONE, 0, dohostinfo},
	{ "emulabconfig", FULLCONFIG_NONE, F_ALLOCATED, doemulabconfig},
	{ "eplabconfig",  FULLCONFIG_NONE, F_ALLOCATED, doeplabconfig},
	{ "localization", FULLCONFIG_PHYS, 0, dolocalize},
	{ "rootpswd",     FULLCONFIG_NONE, F_REMREQSSL, dorootpswd},
	{ "booterrno",    FULLCONFIG_NONE, 0, dobooterrno},
	{ "bootlog",      FULLCONFIG_NONE, 0, dobootlog},
	{ "battery",      FULLCONFIG_NONE, F_REMUDP|F_MINLOG, dobattery},
	{ "topomap",      FULLCONFIG_NONE, F_MINLOG|F_ALLOCATED, dotopomap},
	{ "userenv",      FULLCONFIG_ALL,  F_ALLOCATED, douserenv},
	{ "tiptunnels",	  FULLCONFIG_ALL,  F_ALLOCATED, dotiptunnels},
	{ "traceinfo",	  FULLCONFIG_ALL,  F_ALLOCATED, dotraceconfig },
	{ "ltmap",        FULLCONFIG_NONE, F_MINLOG|F_ALLOCATED, doltmap},
	{ "ltpmap",       FULLCONFIG_NONE, F_MINLOG|F_ALLOCATED, doltpmap},
	{ "elvindport",   FULLCONFIG_NONE, 0, doelvindport},
	{ "plabeventkeys",FULLCONFIG_NONE, F_REMREQSSL, doplabeventkeys},
	{ "intfcmap",     FULLCONFIG_NONE, 0, dointfcmap},
	{ "motelog",      FULLCONFIG_ALL,  F_ALLOCATED, domotelog},
	{ "portregister", FULLCONFIG_NONE, F_REMNOSSL, doportregister},
	{ "bootwhat",	  FULLCONFIG_NONE, 0, dobootwhat },
	{ "tpmblob",	  FULLCONFIG_ALL, 0, dotpmblob },
	{ "tpmpubkey",	  FULLCONFIG_ALL, 0, dotpmpubkey },
	{ "tpmdummy",	  FULLCONFIG_ALL, F_REQTPM, dotpmdummy },
	{ "dhcpdconf",	  FULLCONFIG_ALL, 0, dodhcpdconf },
	{ "securestate",  FULLCONFIG_NONE, F_REMREQSSL, dosecurestate},
	{ "quoteprep",    FULLCONFIG_NONE, F_REMREQSSL, doquoteprep},
	{ "imagekey",     FULLCONFIG_NONE, F_REQTPM, doimagekey},
};
static int numcommands = sizeof(command_array)/sizeof(struct command);

char *usagestr =
 "usage: tmcd [-d] [-p #]\n"
 " -d              Turn on debugging. Multiple -d options increase output\n"
 " -p portnum	   Specify a port number to listen on\n"
 " -c num	   Specify number of servers (must be %d <= x <= %d)\n"
 " -v              More verbose logging\n"
 " -i ipaddr       Sets the boss IP addr to return (for multi-homed servers)\n"
 "\n";

void
usage()
{
	fprintf(stderr, usagestr, MINCHILDREN, MAXCHILDREN);
	exit(1);
}

static void
cleanup()
{
	signal(SIGHUP, SIG_IGN);
	killme = 1;
	killpg(0, SIGHUP);
	unlink(pidfile);
}

static void
setverbose(int sig)
{
	signal(sig, SIG_IGN);

	if (sig == SIGUSR1)
		verbose = 1;
	else
		verbose = 0;
	info("verbose logging turned %s\n", verbose ? "on" : "off");

	/* Just the parent sends this */
	if (numchildren)
		killpg(0, sig);
	signal(sig, setverbose);
}

int
main(int argc, char **argv)
{
	int			tcpsock, udpsock, i, ch;
	int			alttcpsock, altudpsock;
	int			status, pid;
	int			portnum = TBSERVER_PORT;
	FILE			*fp;
	char			buf[BUFSIZ];
	struct hostent		*he;
	extern char		build_info[];
	int			server_counts[4]; /* udp,tcp,altudp,alttcp */
	struct {
		int	pid;
		int	which;
	} servers[MAXCHILDREN];

	while ((ch = getopt(argc, argv, "dp:c:Xvi:")) != -1)
		switch(ch) {
		case 'p':
			portnum = atoi(optarg);
			break;
		case 'd':
			debug++;
			break;
		case 'c':
			maxchildren = atoi(optarg);
			break;
		case 'X':
			insecure = 1;
			break;
		case 'v':
			verbose++;
			break;
		case 'i':
			if (inet_aton(optarg, &myipaddr) == 0) {
				fprintf(stderr, "invalid IP address %s\n",
					optarg);
				usage();
			}
			break;
		case 'h':
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc)
		usage();
	if (maxchildren < MINCHILDREN || maxchildren > MAXCHILDREN)
		usage();

#ifdef  WITHSSL
	if (tmcd_server_sslinit()) {
		error("SSL init failed!\n");
		exit(1);
	}
#endif
	if (debug)
		loginit(0, 0);
	else {
		/* Become a daemon */
		if (chdir(TBCOREDIR)) {
			daemon(0, 0);
		} else {
			daemon(1, 0);
		}
		loginit(1, "tmcd");
	}
	info("daemon starting (version %d)\n", CURRENT_VERSION);
	info("%s\n", build_info);

	/*
	 * Get FS's SFS hostid
	 * XXX This approach is somewhat kludgy
	 */
	strcpy(fshostid, "");
	if (access(FSHOSTID,R_OK) == 0) {
		fp = fopen(FSHOSTID, "r");
		if (!fp) {
			error("Failed to get FS's hostid");
		}
		else {
			fgets(fshostid, HOSTID_SIZE, fp);
			if (rindex(fshostid, '\n')) {
				*rindex(fshostid, '\n') = 0;
				if (debug) {
				    info("fshostid: %s\n", fshostid);
				}
			}
			else {
				error("fshostid from %s may be corrupt: %s",
				      FSHOSTID, fshostid);
			}
			fclose(fp);
		}
	}

	/*
	 * Grab our IP for security check below.
	 */
	if (myipaddr.s_addr == 0) {
#ifdef	LBS
		strcpy(buf, BOSSNODE);
#else
		if (gethostname(buf, sizeof(buf)) < 0)
			pfatal("getting hostname");
#endif
		if ((he = gethostbyname(buf)) == NULL) {
			error("Could not get IP (%s) - %s\n",
			      buf, hstrerror(h_errno));
			exit(1);
		}
		memcpy((char *)&myipaddr, he->h_addr, he->h_length);
	}

	/*
	 * If we were given a port on the command line, don't open the
	 * alternate ports
	 */
	if (portnum != TBSERVER_PORT) {
	    if (makesockets(portnum, &udpsock, &tcpsock) < 0) {
		error("Could not make sockets!");
		exit(1);
	    }
	    num_alttcpservers = num_altudpservers = 0;
	} else {
	    if (makesockets(portnum, &udpsock, &tcpsock) < 0 ||
		makesockets(TBSERVER_PORT2, &altudpsock, &alttcpsock) < 0) {
		    error("Could not make sockets!");
		    exit(1);
	    }
	}

	signal(SIGTERM, cleanup);
	signal(SIGINT, cleanup);
	signal(SIGHUP, cleanup);
	signal(SIGUSR1, setverbose);
	signal(SIGUSR2, setverbose);

	/*
	 * Stash the pid away.
	 */
	mypid = getpid();
	sprintf(pidfile, "%s/tmcd.pid", _PATH_VARRUN);
	fp = fopen(pidfile, "w");
	if (fp != NULL) {
		fprintf(fp, "%d\n", mypid);
		(void) fclose(fp);
	}

	/*
	 * Change to non-root user!
	 */
	if (geteuid() == 0) {
		struct passwd	*pw;
		uid_t		uid;
		gid_t		gid;

		/*
		 * Must be a valid user of course.
		 */
		if ((pw = getpwnam(RUNASUSER)) == NULL) {
			error("invalid user: %s", RUNASUSER);
			exit(1);
		}
		uid = pw->pw_uid;
		gid = pw->pw_gid;

		if (setgroups(1, &gid)) {
			errorc("setgroups");
			exit(1);
		}
		if (setgid(gid)) {
			errorc("setgid");
			exit(1);
		}
		if (setuid(uid)) {
			errorc("setuid");
			exit(1);
		}
		info("Flipped to user/group %d/%d\n", uid, gid);
	}

	/*
	 * Now fork a set of children to handle requests. We keep the
	 * pool at a set level. There are 4 types of servers, each getting
	 * a different number of servers. We do it this cause otherwise
	 * we have to deal with the select storm problem; a bunch of processes
	 * select on the same set of file descriptors, and all get woken up
	 * when something comes in, then all read from the socket but only
	 * one gets it and the others go back to sleep. There are various ways
	 * to deal with this problem, but all of them are a lot more code!
	 */
	server_counts[0] = num_udpservers;
	server_counts[1] = num_altudpservers;
	server_counts[2] = num_alttcpservers;
	server_counts[3] = maxchildren -
		(num_udpservers + num_altudpservers + num_altudpservers);
	bzero(servers, sizeof(servers));

	while (1) {
		while (!killme && numchildren < maxchildren) {
			int which = 3;

			/*
			 * Find which kind of server is short one.
			 */
			for (i = 0; i < 4; i++) {
				if (server_counts[i]) {
					which = i;
					break;
				}
			}

			if ((pid = fork()) < 0) {
				errorc("forking server");
				goto done;
			}
			if (pid) {
				server_counts[which]--;
				/*
				 * Find free slot
				 */
				for (i = 0; i < maxchildren; i++) {
					if (!servers[i].pid)
						break;
				}
				servers[i].pid   = pid;
				servers[i].which = which;
				numchildren++;
				continue;
			}
			/* Poor way of knowing parent/child */
			numchildren = 0;
			mypid = getpid();

			/* Child does useful work! Never Returns! */
			signal(SIGTERM, SIG_DFL);
			signal(SIGINT, SIG_DFL);
			signal(SIGHUP, SIG_DFL);

			switch (which) {
			case 0: udpserver(udpsock, portnum);
				break;
			case 1: udpserver(altudpsock, TBSERVER_PORT2);
				break;
			case 2: tcpserver(alttcpsock, TBSERVER_PORT2);
				break;
			case 3: tcpserver(tcpsock, portnum);
				break;
			}
			exit(-1);
		}

		/*
		 * Parent waits.
		 */
		pid = waitpid(-1, &status, 0);
		if (pid < 0) {
			errorc("waitpid failed");
			continue;
		}
		if (WIFSIGNALED(status)) {
			error("server %d exited with signal %d!\n",
			      pid, WTERMSIG(status));
		}
		else if (WIFEXITED(status)) {
			error("server %d exited with status %d!\n",
			      pid, WEXITSTATUS(status));
		}
		numchildren--;

		/*
		 * Figure out which and what kind of server it was that died.
		 */
		for (i = 0; i < maxchildren; i++) {
			if (servers[i].pid == pid) {
				servers[i].pid = 0;
				server_counts[servers[i].which]++;
				break;
			}
		}
		if (killme && !numchildren)
			break;
	}
 done:
	CLOSE(tcpsock);
	close(udpsock);
	info("daemon terminating\n");
	exit(0);
}

/*
 * Create sockets on specified port.
 */
static int
makesockets(int portnum, int *udpsockp, int *tcpsockp)
{
	struct sockaddr_in	name;
	socklen_t		length;
	int			i, udpsock, tcpsock;

	/*
	 * Setup TCP socket for incoming connections.
	 */

	/* Create socket from which to read. */
	tcpsock = socket(AF_INET, SOCK_STREAM, 0);
	if (tcpsock < 0) {
		pfatal("opening stream socket");
	}

	i = 1;
	if (setsockopt(tcpsock, SOL_SOCKET, SO_REUSEADDR,
		       (char *)&i, sizeof(i)) < 0)
		pwarning("setsockopt(SO_REUSEADDR)");;

	/* Create name. */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons((u_short) portnum);
	if (bind(tcpsock, (struct sockaddr *) &name, sizeof(name))) {
		pfatal("binding stream socket");
	}
	/* Find assigned port value and print it out. */
	length = sizeof(name);
	if (getsockname(tcpsock, (struct sockaddr *) &name, &length)) {
		pfatal("getsockname");
	}
	if (listen(tcpsock, 128) < 0) {
		pfatal("listen");
	}
	info("listening on TCP port %d\n", ntohs(name.sin_port));

	/*
	 * Setup UDP socket
	 */

	/* Create socket from which to read. */
	udpsock = socket(AF_INET, SOCK_DGRAM, 0);
	if (udpsock < 0) {
		pfatal("opening dgram socket");
	}

	i = 1;
	if (setsockopt(udpsock, SOL_SOCKET, SO_REUSEADDR,
		       (char *)&i, sizeof(i)) < 0)
		pwarning("setsockopt(SO_REUSEADDR)");;

	i = 128 * 1024;
	if (setsockopt(udpsock, SOL_SOCKET, SO_RCVBUF, &i, sizeof(i)) < 0)
		pwarning("setsockopt(SO_RCVBUF)");

	/* Create name. */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons((u_short) portnum);
	if (bind(udpsock, (struct sockaddr *) &name, sizeof(name))) {
		pfatal("binding dgram socket");
	}

	/* Find assigned port value and print it out. */
	length = sizeof(name);
	if (getsockname(udpsock, (struct sockaddr *) &name, &length)) {
		pfatal("getsockname");
	}
	info("listening on UDP port %d\n", ntohs(name.sin_port));

	*tcpsockp = tcpsock;
	*udpsockp = udpsock;
	return 0;
}

/*
 * Listen for UDP requests. This is not a secure channel, and so this should
 * eventually be killed off.
 */
static void
udpserver(int sock, int portnum)
{
	char			buf[MYBUFSIZE];
	struct sockaddr_in	client;
	socklen_t		length;
	int			cc;
	unsigned int		nreq = 0;

	info("udpserver starting: pid=%d sock=%d portnum=%d\n",
	     mypid, sock, portnum);

	/*
	 * Wait for udp connections.
	 */
	while (1) {
		setproctitle("UDP %d: %u done", portnum, nreq);
		length = sizeof(client);
		cc = recvfrom(sock, buf, sizeof(buf) - 1,
			      0, (struct sockaddr *)&client, &length);
		if (cc <= 0) {
			if (cc < 0)
				errorc("Reading UDP request");
			error("UDP Connection aborted\n");
			continue;
		}
		buf[cc] = '\0';
		handle_request(sock, &client, buf, 0);
		nreq++;
	}
	exit(1);
}

int
tmcd_accept(int sock, struct sockaddr *addr, socklen_t *addrlen, int ms)
{
	int	newsock;

	if ((newsock = accept(sock, addr, addrlen)) < 0)
		return -1;

	/*
	 * Set timeout value to keep us from hanging due to a
	 * malfunctioning or malicious client.
	 */
	if (ms > 0) {
		struct timeval tv;

		tv.tv_sec = ms / 1000;
		tv.tv_usec = (ms % 1000) * 1000;
		if (setsockopt(newsock, SOL_SOCKET, SO_RCVTIMEO,
			       &tv, sizeof(tv)) < 0) {
			errorc("setting SO_RCVTIMEO");
		}
	}

	return newsock;
}

/*
 * Listen for TCP requests.
 */
static void
tcpserver(int sock, int portnum)
{
	char			buf[MAXTMCDPACKET];
	struct sockaddr_in	client;
	socklen_t		length;
	int			cc, newsock;
	unsigned int		nreq = 0;
	struct timeval		tv;

	info("tcpserver starting: pid=%d sock=%d portnum=%d\n",
	     mypid, sock, portnum);

	/*
	 * Wait for TCP connections.
	 */
	while (1) {
		setproctitle("TCP %d: %u done", portnum, nreq);
		length  = sizeof(client);
		newsock = ACCEPT(sock, (struct sockaddr *)&client, &length,
				 readtimo);
		if (newsock < 0) {
			errorc("accepting TCP connection");
			continue;
		}

		/*
		 * Set write timeout value to keep us from hanging due to a
		 * malfunctioning or malicious client.
		 * NOTE: ACCEPT function sets read timeout.
		 */
		tv.tv_sec = writetimo / 1000;
		tv.tv_usec = (writetimo % 1000) * 1000;
		if (setsockopt(newsock, SOL_SOCKET, SO_SNDTIMEO,
			       &tv, sizeof(tv)) < 0) {
			errorc("setting SO_SNDTIMEO");
			CLOSE(newsock);
			continue;
		}

		/*
		 * Read in the command request.
		 */
		if ((cc = READ(newsock, buf, sizeof(buf) - 1)) <= 0) {
			if (cc < 0) {
				if (errno == EWOULDBLOCK)
					errorc("Timeout reading TCP request");
				else
					errorc("Error reading TCP request");
			}
			error("TCP connection aborted\n");
			CLOSE(newsock);
			continue;
		}
		buf[cc] = '\0';
		handle_request(newsock, &client, buf, 1);
		CLOSE(newsock);
		nreq++;
	}
	exit(1);
}

//#define error(x...)	fprintf(stderr, ##x)
//#define info(x...)	fprintf(stderr, ##x)

static int
handle_request(int sock, struct sockaddr_in *client, char *rdata, int istcp)
{
	struct sockaddr_in redirect_client;
	int		   redirect = 0;
	char		   buf[BUFSIZ], *bp, *cp;
	char		   privkeybuf[PRIVKEY_LEN];
	char		   *privkey = (char *) NULL;
	int		   i, overbose = 0, err = 0;
	int		   version = DEFAULT_VERSION;
	tmcdreq_t	   tmcdreq, *reqp = &tmcdreq;

	byteswritten = 0;
#ifdef	WITHSSL
	cp = (istcp ? (isssl ? "SSL" : "TCP") : "UDP");
#else
	cp = (istcp ? "TCP" : "UDP");
#endif
	setproctitle("%s: %s", inet_ntoa(client->sin_addr), cp);

	/*
	 * Init the req structure.
	 */
	bzero(reqp, sizeof(*reqp));

	/*
	 * Look for special tags.
	 */
	bp = rdata;
	while ((bp = strsep(&rdata, " ")) != NULL) {
		/*
		 * Look for PRIVKEY.
		 */
		if (sscanf(bp, "PRIVKEY=%" XSTRINGIFY(PRIVKEY_LEN) "s", buf)) {
			if (strlen(buf) < 16) {
				info("tmcd client provided short privkey");
				goto skipit;
			}
			for (i = 0; i < strlen(buf); i++){
				if (! isxdigit(buf[i])) {
					info("tmcd client provided invalid "
					     "characters in privkey");
					goto skipit;
				}
			}
			strncpy(privkeybuf, buf, sizeof(privkeybuf));
			privkey = privkeybuf;

			if (debug) {
				info("%s: PRIVKEY %s\n", reqp->nodeid, buf);
			}
			continue;
		}


		/*
		 * Look for VERSION.
		 * Check for clients that are newer than the server
		 * and complain.
		 */
		if (sscanf(bp, "VERSION=%d", &i) == 1) {
			version = i;
			if (version > CURRENT_VERSION) {
				error("version skew on request from %s: "
				      "server=%d, request=%d, "
				      "old TMCD installed?\n",
				      inet_ntoa(client->sin_addr),
				      CURRENT_VERSION, version);
			}
			continue;
		}

		/*
		 * Look for REDIRECT, which is a proxy request for a
		 * client other than the one making the request. Good
		 * for testing. Might become a general tmcd redirect at
		 * some point, so that we can test new tmcds.
		 */
		if (sscanf(bp, "REDIRECT=%30s", buf)) {
			redirect_client = *client;
			redirect        = 1;
			inet_aton(buf, &client->sin_addr);

			info("REDIRECTED from %s to %s\n",
			     inet_ntoa(redirect_client.sin_addr), buf);

			continue;
		}

		/*
		 * Look for VNODE. This is used for virtual nodes.
		 * It indicates which of the virtual nodes (on the physical
		 * node) is talking to us. Currently no perm checking.
		 */
		if (sscanf(bp, "VNODEID=%30s", buf)) {
			for (i = 0; i < strlen(buf); i++){
				if (! (isalnum(buf[i]) ||
				       buf[i] == '_' || buf[i] == '-')) {
					info("tmcd client provided invalid "
					     "characters in vnodeid");
					goto skipit;
				}
			}
			reqp->isvnode = 1;
			strncpy(reqp->vnodeid, buf, sizeof(reqp->vnodeid));

			if (debug) {
				info("VNODEID %s\n", buf);
			}
			continue;
		}

		/*
		 * Look for external key
		 */
		if (sscanf(bp, "IDKEY=%" XSTRINGIFY(PRIVKEY_LEN) "s", buf)) {
			for (i = 0; i < strlen(buf); i++){
				if (! isalnum(buf[i])) {
					info("tmcd client provided invalid "
					     "characters in IDKEY");
					goto skipit;
				}
			}
			strncpy(reqp->external_key,
				buf, sizeof(reqp->external_key));

			if (debug) {
				info("IDKEY %s\n", buf);
			}
			continue;
		}

		/*
		 * An empty token (two delimiters next to each other)
		 * is indicated by a null string. If nothing matched,
		 * and its not an empty token, it must be the actual
		 * command and arguments. Break out.
		 *
		 * Note that rdata will point to any text after the command.
		 *
		 */
		if (*bp) {
			break;
		}
	}

	/* Start with default DB */
	strcpy(dbname, DEFAULT_DBNAME);

	/*
	 * Map the ip to a nodeid.
	 */
	if ((err = iptonodeid(client->sin_addr, reqp, privkey))) {
		if (privkey) {
			error("No such node with wanode_key [%s]\n", privkey);
		}
		else if (reqp->external_key[0]) {
			if (reqp->isvnode)
				error("No such vnode %s with key %s\n",
				      reqp->vnodeid, reqp->external_key);
			else
				error("No such node with key %s\n",
				      reqp->external_key);
		}
		else if (reqp->isvnode) {
			error("No such vnode %s associated with %s\n",
			      reqp->vnodeid, inet_ntoa(client->sin_addr));
		}
		else {
			error("No such node: %s\n",
			      inet_ntoa(client->sin_addr));
		}
		goto skipit;
	}

	/*
	 * Redirect is allowed from the local host only.
	 */
	if (redirect &&
	    redirect_client.sin_addr.s_addr != myipaddr.s_addr) {
		char	buf1[32], buf2[32];

		strcpy(buf1, inet_ntoa(redirect_client.sin_addr));
		strcpy(buf2, inet_ntoa(client->sin_addr));

		if (verbose)
			info("%s INVALID REDIRECT: %s\n", buf1, buf2);
		goto skipit;
	}

#ifdef  WITHSSL
	/*
	 * We verify UDP requests below based on the particular request
	 */
	if (!istcp)
		goto execute;

	/*
	 * If the connection is not SSL, then it must be a local node.
	 */
	if (isssl) {
		/*
		 * LBS: I took this test out. This client verification has
		 * always been a pain, and offers very little since since
		 * the private key is not encrypted anyway. Besides, we
		 * do not return any sensitive data via tmcd, just a lot of
		 * goo that would not be of interest to anyone. I will
		 * kill this code at some point.
		 */
		if (0 &&
		    tmcd_sslverify_client(reqp->nodeid, reqp->pclass,
					  reqp->ptype,  reqp->islocal)) {
			error("%s: SSL verification failure\n", reqp->nodeid);
			if (! redirect)
				goto skipit;
		}
	}
	else if (reqp->iscontrol) {
		error("%s: Control node connection without SSL!\n",
		      reqp->nodeid);
		if (!insecure)
			goto skipit;
	}
#else
	/*
	 * When not compiled for ssl, do not allow remote connections.
	 */
	if (!reqp->islocal) {
		error("%s: Remote node connection not allowed (Define SSL)!\n",
		      reqp->nodeid);
		if (!insecure)
			goto skipit;
	}
	if (reqp->iscontrol) {
		error("%s: Control node connection not allowed "
		      "(Define SSL)!\n", reqp->nodeid);
		if (!insecure)
			goto skipit;
	}
#endif
	/*
	 * Check for a redirect using the default DB. This allows
	 * for a simple redirect to a secondary DB for testing.
	 * Upon return, the dbname has been changed if redirected.
	 */
	if (checkdbredirect(reqp)) {
		/* Something went wrong */
		goto skipit;
	}

	/*
	 * Figure out what command was given.
	 */
 execute:
	for (i = 0; i < numcommands; i++)
		if (strncmp(bp, command_array[i].cmdname,
			    strlen(command_array[i].cmdname)) == 0)
			break;

	if (i == numcommands) {
		info("%s: INVALID REQUEST: %.8s\n", reqp->nodeid, bp);
		goto skipit;
	}

	/*
	 * If this is a UDP request from a remote node,
	 * make sure it is allowed.
	 */
	if (!istcp && !reqp->islocal &&
	    (command_array[i].flags & F_REMUDP) == 0) {
		error("%s: %s: Invalid UDP request from remote node\n",
		      reqp->nodeid, command_array[i].cmdname);
		goto skipit;
	}

	/*
	 * Ditto for remote node connection without SSL.
	 */
	if (istcp && !isssl && !reqp->islocal &&
	    (command_array[i].flags & F_REMREQSSL) != 0) {
		error("%s: %s: Invalid NO-SSL request from remote node\n",
		      reqp->nodeid, command_array[i].cmdname);
		goto skipit;
	}

	if (!reqp->allocated && (command_array[i].flags & F_ALLOCATED) != 0) {
		if (verbose || (command_array[i].flags & F_MINLOG) == 0)
			error("%s: %s: Invalid request from free node\n",
			      reqp->nodeid, command_array[i].cmdname);
		goto skipit;
	}

	/*
	 * Enforce TPM use with an iron fist!
	 */
	if ((command_array[i].flags & F_REQTPM)) {
		if (!isssl) {
			/* Should at least be TLS encrypted */
			error("%s: %s: Invalid non-SSL/TPM request\n",
			      reqp->nodeid, command_array[i].cmdname);
			goto skipit;
		}

		/*
		 * Make sure they are using the TPM certificate that we have in
		 * the database for this TLS sesion.
		 */
		if (checkcerts(reqp->nodeid)) {
			error("%s: %s: TPM certificate mismatch\n",
			      reqp->nodeid, command_array[i].cmdname);
			goto skipit;
		}
	}

	/*
	 * Execute it.
	 */
	if ((command_array[i].flags & F_MAXLOG) != 0) {
		overbose = verbose;
		verbose = 1;
	}
	if (verbose || (command_array[i].flags & F_MINLOG) == 0)
		info("%s: vers:%d %s %s\n", reqp->nodeid,
		     version, cp, command_array[i].cmdname);
	setproctitle("%s: %s %s", reqp->nodeid, cp, command_array[i].cmdname);

	err = command_array[i].func(sock, reqp, rdata, istcp, version);

	if (err)
		info("%s: %s: returned %d\n",
		     reqp->nodeid, command_array[i].cmdname, err);
	if ((command_array[i].flags & F_MAXLOG) != 0)
		verbose = overbose;

 skipit:
	if (!istcp)
		client_writeback_done(sock,
				      redirect ? &redirect_client : client);

	if (byteswritten &&
	    (verbose || (command_array[i].flags & F_MINLOG) == 0))
		info("%s: %s wrote %d bytes\n",
		     reqp->nodeid, command_array[i].cmdname,
		     byteswritten);

	return 0;
}

static int checkcerts(char *nid)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	int		nrows, ret;

	X509		*local, *remote;

	res = mydb_query("select tpmx509 "
			"from node_hostkeys "
			"where node_id='%s' ",
			1, nid);

	/* Treat errors as failure */
	if (!res) {
		error("Couldn't get tpmx509 from database for node %s\n", nid);
		return 1;
	}

	nrows = mysql_num_rows(res);
	if (!nrows) {
		error("No tpmx509 in the database for node %s\n", nid);
		mysql_free_result(res);
		return 1;
	}

	row = mysql_fetch_row(res);

	remote = tmcd_sslgetpeercert();
	if (!remote) {
		error("SSL_get_peer_certificate() returned NULL for node %s\n",
		    nid);
		mysql_free_result(res);
		return 1;
	}

	local = tmcd_sslrowtocert(row[0], nid);
	if (!local) {
		error("Failure converting row to X509 for node %s\n",
		    nid);
		mysql_free_result(res);
		X509_free(remote);
		return 1;
	}

	ret = X509_cmp(local, remote);

	mysql_free_result(res);
	X509_free(local);
	X509_free(remote);

	return ret;
}

/*
 * Accept notification of reboot.
 */
COMMAND_PROTOTYPE(doreboot)
{
	/*
	 * This is now a no-op. The things this used to do are now
	 * done by stated when we hit RELOAD/RELOADDONE state
	 */
	return 0;
}

/*
 * Return emulab nodeid (not the experimental name).
 */
COMMAND_PROTOTYPE(donodeid)
{
	char		buf[MYBUFSIZE];

	OUTPUT(buf, sizeof(buf), "%s\n", reqp->nodeid);
	client_writeback(sock, buf, strlen(buf), tcp);
	return 0;
}

/*
 * Return a boot manifest for the node's boot scripts.
 */
COMMAND_PROTOTYPE(domanifest)
{
	MYSQL_RES	*res = NULL;
	MYSQL_ROW	row;
	char		buf[2*MYBUFSIZE];
	int		nrows = 0;
	int		disable_type = 0, disable_osid = 0, disable_node = 0;

	res = mydb_query("select opt_name,opt_value"
			 " from virt_client_service_opts"
			 " where exptidx=%d or vnode='%s'",
			 2, reqp->exptidx, reqp->nickname);
	if (!res) {
		info("MANIFEST: %s: DB Error getting expt client service opts!\n",
		      reqp->nodeid);
	}
	else if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		res = NULL;
	}
	while (nrows) {
		row = mysql_fetch_row(res);

		if (strcmp(row[0],"disable_type") == 0 
		    && strcmp(row[1],"1") == 0) {
			disable_type = 1;
		}
		else if (strcmp(row[0],"disable_osid") == 0 
			 && strcmp(row[1],"1") == 0) {
			disable_osid = 1;
		}
		else if (strcmp(row[0],"disable_node") == 0 
			 && strcmp(row[1],"1") == 0) {
			disable_node = 1;
		}
		else {
			info("MANIFEST: %s: unknown expt client service opt %s!\n",
			     reqp->nodeid,row[0]);
		}
	}
	if (res) {
		mysql_free_result(res);
		res = NULL;
		nrows = 0;
	}

	/*
	 * This is a messy query.  The rules for the manifest are
	 * 0) only service and hook entries whose service (service,env,whence)
	 *    tuples exist in the client_services table can be returned;
	 * 1) only one SERVICE line foreach (service,env) tuple can be 
	 *    returned;
	 * 2) the user can only override (service,env) tuples that have
	 *    user_can_override set to 1;
	 *    (and if there are multiple admin tuples, but the highest-prio
	 *    one allows override, even if less prio ones don't, we still
	 *    allow override!)
	 * 3) multiple HOOK lines can be returned, and the order in
	 *    which hook lines are generated is by querying the
	 *    client_service_hooks table for node types matching this node;
	 *    the client_service_hooks table for osids matching this node;
	 *    the client_service_hooks table for nodes matching this node;
	 *    the virt_client_service_hooks table for '' nodes
	 *      (wildcard, experiment wide);
	 *    the virt_client_service_hooks table for nodes matching
	 *      this node.
	 * 4) the user can disable all admin hooks with
	 *    user_can_override set to 1
	 * 5) the virt_client_service_opts table controls which types of 
	 *    admin hooks are disabled, across the experiment, or per-node.
	 */
	res = mydb_query("select cs.service,cs.env,cs.whence,"
			 /* 3 */
			 "  vcsnodeblobs.uuid,vcsnode.alt_vblob_id,vcsnode.enable,vcsnode.enable_hooks,vcsnode.fatal,"
			 /* 8 */
			 "  vcsexptblobs.uuid,vcsexpt.alt_vblob_id,vcsexpt.enable,vcsexpt.enable_hooks,vcsexpt.fatal,"
			 /* 13 */
			 "  csnode.alt_blob_id,csnode.enable,csnode.enable_hooks,"
			 /* 16 */
			 "  csnode.fatal,csnode.user_can_override,"
			 /* 18 */
			 "  csos.alt_blob_id,csos.enable,csos.enable_hooks,"
			 /* 21 */
			 "  csos.fatal,csos.user_can_override,"
			 /* 23 */
			 "  cstype.alt_blob_id,cstype.enable,cstype.enable_hooks,"
			 /* 26 */
			 "  cstype.fatal,cstype.user_can_override,"
			 /* 28 */
			 "  cs.hooks_only"
			 " from reserved as r"
			 " left join nodes as n on r.node_id=n.node_id"
			 " straight_join client_services as cs"
			 " left join virt_client_service_ctl as vcsexpt on"
			 "   (r.exptidx=vcsexpt.exptidx and vcsexpt.vnode=''"
			 "    and cs.idx=vcsexpt.service_idx"
			 "    and cs.env=vcsexpt.env"
			 "    and cs.whence=vcsexpt.whence)"
			 " left join blobs as vcsexptblobs on"
			 "   (vcsexpt.exptidx=vcsexptblobs.exptidx"
			 "    and vcsexpt.alt_vblob_id=vcsexptblobs.vblob_id)"
			 " left join virt_client_service_ctl as vcsnode on"
			 "   (r.exptidx=vcsnode.exptidx"
			 "    and r.vname=vcsnode.vnode"
			 "    and cs.idx=vcsnode.service_idx"
			 "    and cs.env=vcsnode.env"
			 "    and cs.whence=vcsnode.whence)"
			 " left join blobs as vcsnodeblobs on"
			 "   (vcsnode.exptidx=vcsnodeblobs.exptidx"
			 "    and vcsnode.alt_vblob_id=vcsnodeblobs.vblob_id)"
			 " left join client_service_ctl as csnode on"
			 "   (csnode.obj_type='node'"
			 "    and r.node_id=csnode.obj_name"
			 "    and cs.idx=csnode.service_idx"
			 "    and cs.env=csnode.env"
			 "    and cs.whence=csnode.whence)"
			 " left join client_service_ctl as csos on"
			 "   (csos.obj_type='osid' and n.def_boot_osid=csos.obj_name"
			 "    and cs.idx=csos.service_idx and cs.env=csos.env"
			 "    and cs.whence=csos.whence)"
			 " left join client_service_ctl as cstype on"
			 "   (cstype.obj_type='node_type'"
			 "    and n.type=cstype.obj_name"
			 "    and cs.idx=cstype.service_idx"
			 "    and cs.env=cstype.env and cs.whence=cstype.whence)"
			 " where r.exptidx=%d and r.node_id='%s'"
			 "   and (vcsnode.enable is not NULL"
			 "        or vcsexpt.enable is not NULL"
			 "        or csnode.enable is not NULL"
			 "        or csos.enable is not NULL"
			 "        or cstype.enable is not NULL)",
			 29, reqp->exptidx, reqp->nodeid);
	if (!res) {
		error("MANIFEST: %s: DB Error getting manifest info!\n",
		      reqp->nodeid);
		nrows = 0;
	}
	else if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		res = NULL;
	}
	while (nrows) {
		char *enabled = NULL;
		char *hooks_enabled = NULL;
		char *fatal = NULL;
		char *blobid = NULL;
		int admin_service_not_overrideable = 0;
		int admin_idx = 0;
		int hooks_only;
		int disable_admin = 0;

		row = mysql_fetch_row(res);
		hooks_only = (strcmp(row[28],"1") == 0) ? 1 : 0;

		/* figure out which service control entry to use! */
		/* start by choosing the per-node or per-experiment stuff */
		if (row[5] != NULL && !hooks_only) {
		    enabled = row[5];
		    hooks_enabled = row[6];
		    fatal = row[7];
		    blobid = row[3];
		    /* 
		     * If there was nothing in blob_blobs for this blob, 
		     * just return the vblob and hope it was a real,
		     * hardcoded blob in the blob store.
		     */
		    if (blobid == NULL)
			blobid = row[4];
		}
		else if (row[9] != NULL && !hooks_only) {
		    enabled = row[10];
		    hooks_enabled = row[11];
		    fatal = row[12];
		    blobid = row[7];
		    if (blobid == NULL)
			blobid = row[8];
		}

		if (row[17] != NULL) {
		    admin_idx = 13;
		    if (strcmp(row[17],"0") == 0) {
			admin_service_not_overrideable = 1;
		    }
		    if (disable_node) 
			disable_admin = 1;
		}
		else if (row[22] != NULL) {
		    admin_idx = 18;
		    if (strcmp(row[22],"0") == 0) {
			admin_service_not_overrideable = 1;
		    }
		    if (disable_osid) 
			disable_admin = 1;
		}
		else if (row[27] != NULL) {
		    admin_idx = 23;
		    if (strcmp(row[27],"0") == 0) {
			admin_service_not_overrideable = 1;
		    }
		    if (disable_type) 
			disable_admin = 1;
		}

		/* If the user wants to ignore the admin setting, and
		 * the admin allows it to be overridden, AND the user
		 * didn't specify a control for this service... skip! */
		if (disable_admin && !admin_service_not_overrideable 
		    && enabled == NULL) {
		    --nrows;
		    continue;
		}

		/* If the admin set hooks_only on a service, and didn't
		 * specify a service entry, bail! */
		if (hooks_only && admin_idx == 0) {
		    --nrows;
		    continue;
		}

		/* If the admin seting can't be overridden, or if the
		 * user didn't specify a control for this node or
		 * experiment-wide, send the admin setting */
		if (admin_service_not_overrideable || enabled == NULL) {
		    enabled = row[admin_idx+1];
		    hooks_enabled = row[admin_idx+2];
		    fatal = row[admin_idx+3];
		    blobid = row[admin_idx+0];
		}

		/* the query should prevent against this, but... */
		if (enabled == NULL) {
		    error("MANIFEST: %s: got info from DB for %s, but no enabled!\n",
			  reqp->nodeid,row[0]);
		    --nrows;
		    continue;
		}

		if (blobid == NULL)
		    blobid = "";

		OUTPUT(buf, sizeof(buf),
		       "SERVICE NAME=%s ENV=%s WHENCE=%s"
		       " ENABLED=%s HOOKS_ENABLED=%s FATAL=%s"
		       " BLOBID=%s\n",
		       row[0],row[1],row[2],
		       enabled,hooks_enabled,fatal,blobid);

		client_writeback(sock, buf, strlen(buf), tcp);
		nrows--;
		if (verbose)
			info("MANIFEST: %s", buf);
	}
	if (res) {
		mysql_free_result(res);
		res = NULL;
		nrows = 0;
	}

	/* grab the admin client side hooks */
	res = mydb_query("select cs.service,cs.env,cs.whence,csh.obj_type,"
			 "  csh.hook_blob_id,csh.hook_op,csh.hook_point,"
			 "  csh.argv,csh.fatal,csh.user_can_override"
			 " from reserved as r"
			 " left join nodes as n on r.node_id=n.node_id"
			 " straight_join client_services as cs"
			 " left join client_service_hooks as csh on"
			 "   (cs.idx=csh.service_idx"
			 "    and cs.env=csh.env"
			 "    and cs.whence=csh.whence)"
			 " where r.exptidx=%d and r.node_id='%s'"
			 "   and ((csh.obj_type='node'"
			 "         and r.node_id=csh.obj_name)"
			 "        or (csh.obj_type='osid'"
			 "            and n.def_boot_osid=csh.obj_name)"
			 "        or (csh.obj_type='node_type'"
			 "            and n.type=csh.obj_name))",
			 10,reqp->exptidx,reqp->nodeid);
	if (!res) {
		error("MANIFEST: %s: DB Error getting manifest admin hook info!\n",
		      reqp->nodeid);
	}
	else if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		res = NULL;
	}
	while (nrows) {
		row = mysql_fetch_row(res);

		/*
		 * skip this admin hook if it can be overridden, and if
		 * the user turned off this type of admin hooks
		 */
		if (strcmp(row[9],"1") == 0
		    && ((strcmp(row[3],"node") == 0 && disable_node)
			|| (strcmp(row[3],"osid") == 0 && disable_osid)
			|| (strcmp(row[3],"type") == 0 && disable_type))) {
			--nrows;
			continue;
		}

		OUTPUT(buf, sizeof(buf),
		       "HOOK SERVICE=%s ENV=%s WHENCE=%s"
		       " OP=%s POINT=%s FATAL=%s BLOBID=%s ARGV=\"%s\"\n",
		       row[0],row[1],row[2],
		       row[5],row[6],row[8],row[4],row[7]);

		client_writeback(sock, buf, strlen(buf), tcp);
		nrows--;
		if (verbose)
			info("MANIFEST: %s", buf);
	}
	if (res) {
		mysql_free_result(res);
		res = NULL;
		nrows = 0;
	}

	/* grab the user-specified client side hooks */
	res = mydb_query("select cs.service,cs.env,cs.whence,"
			 "  cshblobs.uuid,csh.hook_vblob_id,csh.hook_op,csh.hook_point,"
			 "  csh.argv,csh.fatal"
			 " from reserved as r"
			 " left join nodes as n on r.node_id=n.node_id"
			 " straight_join client_services as cs"
			 " left join virt_client_service_hooks as csh on"
			 "   (r.exptidx=csh.exptidx"
			 "    and (csh.vnode='' or r.vname=csh.vnode)"
			 "    and cs.idx=csh.service_idx"
			 "    and cs.env=csh.env"
			 "    and cs.whence=csh.whence)"
			 " left join blobs as cshblobs on"
			 "   (csh.exptidx=cshblobs.exptidx"
			 "    and csh.hook_vblob_id=cshblobs.vblob_id)"
			 " where r.exptidx=%d and r.node_id='%s'"
			 "   and csh.hook_vblob_id is not NULL",
			 9,reqp->exptidx,reqp->nodeid);
	if (!res) {
		error("MANIFEST: %s: DB Error getting manifest user hook info!\n",
		      reqp->nodeid);
	}
	else if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		res = NULL;
	}
	while (nrows) {
		row = mysql_fetch_row(res);

		OUTPUT(buf, sizeof(buf),
		       "HOOK SERVICE=%s ENV=%s WHENCE=%s"
		       " OP=%s POINT=%s FATAL=%s BLOBID=%s ARGV=\"%s\"\n",
		       row[0],row[1],row[2],
		       row[5],row[6],row[8],(row[3]) ? row[3] : row[4],row[7]);

		client_writeback(sock, buf, strlen(buf), tcp);
		nrows--;
		if (verbose)
			info("MANIFEST: %s", buf);
	}
	if (res) {
		mysql_free_result(res);
		res = NULL;
		nrows = 0;
	}

	return 0;
}

/*
 * Return status of node. Is it allocated to an experiment, or free.
 */
COMMAND_PROTOTYPE(dostatus)
{
	char		buf[MYBUFSIZE];

	/*
	 * Now check reserved table
	 */
	if (! reqp->allocated) {
		info("%s: STATUS: FREE\n", reqp->nodeid);
		strcpy(buf, "FREE\n");
		client_writeback(sock, buf, strlen(buf), tcp);
		return 0;
	}

	OUTPUT(buf, sizeof(buf), "ALLOCATED=%s/%s NICKNAME=%s\n",
	       reqp->pid, reqp->eid, reqp->nickname);
	client_writeback(sock, buf, strlen(buf), tcp);

	if (verbose)
		info("%s: STATUS: %s", reqp->nodeid, buf);
	return 0;
}

/*
 * Return ifconfig information to client.
 */
COMMAND_PROTOTYPE(doifconfig)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		clause[BUFSIZ];
	char		buf[MYBUFSIZE], *ebufp = &buf[MYBUFSIZE];
	int		nrows;
	int		num_interfaces=0;
	int		cookedgeninode = (reqp->geniflags & 0x2);

	if (cookedgeninode)
		goto skipphys;

	/*
	 * For Virtual Nodes, we return interfaces that belong to it.
	 */
	if (reqp->isvnode && !reqp->issubnode)
		sprintf(clause, "i.vnode_id='%s'", reqp->vnodeid);
	else
		strcpy(clause, "i.vnode_id is NULL");

	/*
	 * Find all the interfaces.
	 */
	res = mydb_query("select i.card,i.IP,i.MAC,i.current_speed,"
			 "       i.duplex,i.IPaliases,i.iface,i.role,i.mask,"
			 "       i.rtabid,i.interface_type,vl.vname "
			 "  from interfaces as i "
			 "left join virt_lans as vl on "
			 "  vl.pid='%s' and vl.eid='%s' and "
			 "  vl.vnode='%s' and vl.ip=i.IP "
			 "where i.node_id='%s' and %s",
			 12, reqp->pid, reqp->eid, reqp->nickname,
			 reqp->issubnode ? reqp->nodeid : reqp->pnodeid,
			 clause);

	/*
	 * We need pnodeid in the query. But error reporting is done
	 * by nodeid. For vnodes, nodeid is pcvmXX-XX and for the rest
	 * it is the same as pnodeid
	 */
	if (!res) {
		error("%s: IFCONFIG: DB Error getting interfaces!\n",
		      reqp->nodeid);
		return 1;
	}

	nrows = (int)mysql_num_rows(res);
	while (nrows) {
		row = mysql_fetch_row(res);
		if (row[1] && row[1][0]) {
			int  card    = atoi(row[0]);
			char *iface  = row[6];
			char *role   = row[7];
			char *type   = row[10];
			char *lan    = row[11];
			char *speed  = "100";
			char *unit   = "Mbps";
			char *duplex = "full";
			char *bufp   = buf;
			char *mask;

			/* Never for the control net; sharks are dead */
			if (strcmp(role, TBDB_IFACEROLE_EXPERIMENT))
				goto skipit;

			/* Do this after above test to avoid error in log */
			mask = CHECKMASK(row[8]);

			/*
			 * Speed and duplex if not the default.
			 */
			if (row[3] && row[3][0])
				speed = row[3];
			if (row[4] && row[4][0])
				duplex = row[4];

			/*
			 * We now use the MAC to determine the interface, but
			 * older images still want that tag at the front.
			 */
			if (vers < 10)
				bufp += OUTPUT(bufp, ebufp - bufp,
					       "INTERFACE=%d ", card);
			else if (vers <= 15)
				bufp += OUTPUT(bufp, ebufp - bufp,
					       "IFACETYPE=eth ");
			else
				bufp += OUTPUT(bufp, ebufp - bufp,
					       "INTERFACE IFACETYPE=%s ", type);

			bufp += OUTPUT(bufp, ebufp - bufp,
				"INET=%s MASK=%s MAC=%s SPEED=%s%s DUPLEX=%s",
				row[1], mask, row[2], speed, unit, duplex);

			/*
			 * For older clients, we tack on IPaliases.
			 * This used to be in the interfaces table as a
			 * comma separated list, now we have to extract
			 * it from the vinterfaces table.
			 */
			if (vers >= 8 && vers < 27) {
				MYSQL_RES *res2;
				MYSQL_ROW row2;
				int nrows2;

				res2 = mydb_query("select IP "
						  "from vinterfaces "
						  "where type='alias' "
						  "and node_id='%s'",
						  1, reqp->nodeid);
				if (res2 == NULL)
					goto adone;

				nrows2 = (int)mysql_num_rows(res2);
				bufp += OUTPUT(bufp, ebufp - bufp,
					       " IPALIASES=\"");
				while (nrows2 > 0) {
					nrows2--;
					row2 = mysql_fetch_row(res2);
					if (!row2 || !row2[0])
						continue;
					bufp += OUTPUT(bufp, ebufp - bufp,
						       "%s", row2[0]);
					if (nrows2 > 0)
						bufp += OUTPUT(bufp,
							       ebufp - bufp,
							       ",");
				}
				bufp += OUTPUT(bufp, ebufp - bufp, "\"");
				mysql_free_result(res2);
			adone: ;
			}

			/*
			 * Tack on iface for IXPs. This should be a flag on
			 * the interface instead of a match against type.
			 */
			if (vers >= 11) {
				bufp += OUTPUT(bufp, ebufp - bufp,
					       " IFACE=%s",
					       (strcmp(reqp->class, "ixp") ?
						"" : iface));
			}
			if (vers >= 14) {
				bufp += OUTPUT(bufp, ebufp - bufp,
					       " RTABID=%s", row[9]);
			}
			if (vers >= 17) {
				bufp += OUTPUT(bufp, ebufp - bufp,
					       " LAN=%s", lan);
			}

			OUTPUT(bufp, ebufp - bufp, "\n");
			client_writeback(sock, buf, strlen(buf), tcp);
			num_interfaces++;
			if (verbose)
				info("%s: IFCONFIG: %s", reqp->nodeid, buf);
		}
	skipit:
		nrows--;
	}
	mysql_free_result(res);

	/*
	 * Interface settings.
	 */
	if (vers >= 16) {
		res = mydb_query("select i.MAC,s.capkey,s.capval "
				 "from interface_settings as s "
				 "left join interfaces as i on "
				 "     s.node_id=i.node_id and s.iface=i.iface "
				 "where s.node_id='%s' and %s ",
				 3,
				 reqp->issubnode ? reqp->nodeid : reqp->pnodeid,
				 clause);

		if (!res) {
			error("%s: IFCONFIG: "
			      "DB Error getting interface_settings!\n",
			      reqp->nodeid);
			return 1;
		}
		nrows = (int)mysql_num_rows(res);
		while (nrows) {
			row = mysql_fetch_row(res);

			sprintf(buf, "INTERFACE_SETTING MAC=%s "
				"KEY='%s' VAL='%s'\n",
				row[0], row[1], row[2]);
			client_writeback(sock, buf, strlen(buf), tcp);
			if (verbose)
				info("%s: IFCONFIG: %s", reqp->nodeid, buf);
			nrows--;
		}
		mysql_free_result(res);
	}

	/*
	 * Handle virtual interfaces for both physical nodes (multiplexed
	 * links) and virtual nodes.  Veths (the first virtual interface type)
	 * were added in rev 10.
	 */
	if (vers < 10)
		return 0;

 skipphys:
	/*
	 * First, return config info for physical interfaces underlying
	 * the virtual interfaces or delay interfaces. These are marked
	 * with a current_speed!=0 but no IP address.
	 */
	if (vers >= 18 && !reqp->isvnode) {
		char *aliasstr;

		res = mydb_query("select i.interface_type,i.mac, "
				 "       i.current_speed,i.duplex "
				 "  from interfaces as i "
				 "where i.current_speed!='0' and "
				 "      (i.IP='' or i.IP is null) and "
				 "      i.role='expt' and i.node_id='%s'",
				 4, reqp->pnodeid);
		if (!res) {
			error("%s: IFCONFIG: "
			     "DB Error getting active physical interfaces!\n",
			      reqp->nodeid);
			return 1;
		}

		if (vers < 27)
			aliasstr = "IPALIASES=\"\" ";
		else
			aliasstr = "";

		nrows = (int)mysql_num_rows(res);
		while (nrows) {
			char *bufp   = buf;
			row = mysql_fetch_row(res);

			bufp += OUTPUT(bufp, ebufp - bufp,
				       "INTERFACE IFACETYPE=%s "
				       "INET= MASK= MAC=%s "
				       "SPEED=%sMbps DUPLEX=%s "
				       "%sIFACE= RTABID= LAN=\n",
				       row[0], row[1], row[2], row[3],
				       aliasstr);

			client_writeback(sock, buf, strlen(buf), tcp);
			if (verbose)
				info("%s: IFCONFIG: %s", reqp->nodeid, buf);
			nrows--;
		}
		mysql_free_result(res);
	}

	/*
	 * Outside a vnode, return only those virtual devices that have
	 * vnode=NULL, which indicates its an emulated interface on a
	 * physical node. When inside a vnode, only return virtual devices
	 * for which vnode=curvnode, which are the interfaces that correspond
	 * to a jail node.
	 */
	if (reqp->isvnode)
		sprintf(buf, "v.vnode_id='%s'", reqp->vnodeid);
	else
		strcpy(buf, "v.vnode_id is NULL");

	/*
	 * Find all the virtual interfaces.
	 */
	res = mydb_query("select v.unit,v.IP,v.mac,i.mac,v.mask,v.rtabid, "
			 "       v.type,vll.vname,v.virtlanidx,vlans.tag, "
			 "       l.lanid "
			 "  from vinterfaces as v "
			 "left join interfaces as i on "
			 "  i.node_id=v.node_id and i.iface=v.iface "
			 "left join virt_lan_lans as vll on "
			 "  vll.idx=v.virtlanidx and vll.exptidx=v.exptidx "
			 "left join lans as l on "
			 "  l.exptidx=vll.exptidx and l.vname=vll.vname "
			 "left join vlans on "
			 "  vlans.id=v.vlanid "
			 "left join lan_attributes as la2 on "
			 "  la2.lanid=v.vlanid and la2.attrkey='stack' "
			 "where v.exptidx='%d' and v.node_id='%s' and "
			 "      (la2.attrvalue='Experimental' or "
			 "       la2.attrvalue is null) "
			 "      and %s",
			 11, reqp->exptidx, reqp->pnodeid, buf);
	if (!res) {
		error("%s: IFCONFIG: DB Error getting veth interfaces!\n",
		      reqp->nodeid);
		return 1;
	}
	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		/* XXX not sure why this is ever an error? */
		if (!reqp->isplabdslice && num_interfaces == 0) {
			error("%s: IFCONFIG: No interfaces!\n", reqp->nodeid);
			return 1;
		}
		return 0;
	}
	while (nrows) {
		char *bufp   = buf;
		char *ifacetype;
		int isveth, doencap;

		row = mysql_fetch_row(res);
		nrows--;

		if (strcmp(row[6], "vlan") == 0 && !row[3]) {
			/*
			 * Convert to a loopback lan, however the client
			 * is able to do it.
			 */
			isveth    = 0;
			doencap   = 0;
			ifacetype = "loop";
		} else if (strcmp(row[6], "veth") == 0) {
			isveth = 1;
			doencap = 1;
			ifacetype = "veth";
		} else if (strcmp(row[6], "veth-ne") == 0) {
			isveth = 1;
			doencap = 0;
			ifacetype = "veth";
		} else {
			isveth = 0;
			doencap = 0;
			ifacetype = row[6];
		}

		/*
		 * Older clients only know how to deal with "veth" here.
		 * "alias" is handled via IPALIASES= and "vlan" is unknown.
		 * So skip all but isveth cases.
		 */
		if (vers < 27 && !isveth)
			continue;

		if (vers >= 16) {
			bufp += OUTPUT(bufp, ebufp - bufp, "INTERFACE ");
		}

		/*
		 * Note that PMAC might be NULL, which happens if there is
		 * no underlying phys interface (say, colocated nodes in a
		 * link).
		 */
		bufp += OUTPUT(bufp, ebufp - bufp,
			       "IFACETYPE=%s "
			       "INET=%s MASK=%s ID=%s VMAC=%s PMAC=%s",
			       ifacetype,
			       row[1], CHECKMASK(row[4]), row[0], row[2],
			       row[3] ? row[3] : "none");

		if (vers >= 14) {
			bufp += OUTPUT( bufp, ebufp - bufp,
					" RTABID=%s", row[5]);
		}
		if (vers >= 15) {
			bufp += OUTPUT(bufp, ebufp - bufp,
				       " ENCAPSULATE=%d", doencap);
		}
		if (vers >= 17) {
			bufp += OUTPUT(bufp, ebufp - bufp, " LAN=%s", row[7]);
		}
		/*
		 * Return a VLAN tag.
		 *
		 * XXX for veth devices it comes out of the virt_lan_lans
		 * table, for vlan devices it comes out of the vlans table,
		 * for anything else it is zero.
		 */
		if (vers >= 20) {
			char *tag = "0";
			if (isveth)
				tag = row[8];
			else if (strcmp(ifacetype, "loop") == 0)
				tag = row[10];
			else if (strcmp(ifacetype, "vlan") == 0)
				tag = row[9] ? row[9] : "0";

			/* sanity check the tag */
			if (!isdigit(tag[0])) {
				error("IFCONFIG: bogus encap tag '%s'\n", tag);
				tag = "0";
			}

			bufp += OUTPUT(bufp, ebufp - bufp, " VTAG=%s", tag);
		}
		OUTPUT(bufp, ebufp - bufp, "\n");
		client_writeback(sock, buf, strlen(buf), tcp);
		if (verbose)
			info("%s: IFCONFIG: %s", reqp->nodeid, buf);
	}
	mysql_free_result(res);

	return 0;
}

/*
 * Return account stuff.
 */
COMMAND_PROTOTYPE(doaccounts)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
	int		nrows, gidint;
	int		tbadmin, didwidearea = 0, nodetypeprojects = 0;
	int		didnonlocal = 0;

	if (! tcp) {
		error("ACCOUNTS: %s: Cannot give account info out over UDP!\n",
		      reqp->nodeid);
		return 1;
	}

	/*
	 * Now check reserved table
	 */
	if ((reqp->islocal || reqp->isvnode) && !reqp->allocated) {
		error("%s: accounts: Invalid request from free node\n",
		      reqp->nodeid);
		return 1;
	}

	/*
	 * See if a per-node-type set of projects is specified for accounts.
	 */
	res = mydb_query("select na.attrvalue from nodes as n "
			 "left join node_type_attributes as na on "
			 "  n.type=na.type "
			 "where n.node_id='%s' and "
			 "na.attrkey='project_accounts'", 1, reqp->nodeid);
	if (res) {
		if ((int)mysql_num_rows(res) != 0) {
			nodetypeprojects = 1;
		}
		mysql_free_result(res);
	}

        /*
	 * We need the unix GID and unix name for each group in the project.
	 */
	if (reqp->iscontrol) {
		/*
		 * All groups!
		 */
		res = mydb_query("select unix_name,unix_gid from groups", 2);
	}
	else if (nodetypeprojects) {
		/*
		 * The projects/groups are specified as a comma separated
		 * list in the node_type_attributes table. Return this
		 * set of groups, plus those in emulab-ops since we want
		 * to include admin people too.
		 */
		res = mydb_query("select g.unix_name,g.unix_gid "
				 " from projects as p "
				 "left join groups as g on "
				 "     p.pid_idx=g.pid_idx "
				 "where p.approved!=0 and "
				 "  (FIND_IN_SET(g.gid_idx, "
				 "   (select na.attrvalue from nodes as n "
				 "    left join node_type_attributes as na on "
				 "         n.type=na.type "
				 "    where n.node_id='%s' and "
				 "    na.attrkey='project_accounts')) > 0 or "
				 "   p.pid='%s')",
				 2, reqp->nodeid, RELOADPID);
	}
	else if (reqp->isvnode || reqp->islocal) {
		res = mydb_query("select unix_name,unix_gid from groups "
				 "where pid='%s'",
				 2, reqp->pid);
	}
	else if (reqp->jailflag && !reqp->islocal) {
		/*
		 * A remote node, doing jails. We still want to return
		 * a group for the admin people who get accounts outside
		 * the jails. Lets use the same query as above for now,
		 * but switch over to emulab-ops.
		 */
		res = mydb_query("select unix_name,unix_gid from groups "
				 "where pid='%s'",
				 2, RELOADPID);
	}
	else if (!reqp->islocal && reqp->isdedicatedwa) {
	        /*
		 * Catch widearea nodes that are dedicated to a single pid/eid.
		 * Same as local case.
		 */
		res = mydb_query("select unix_name,unix_gid from groups "
				 "where pid='%s'",
				 2, reqp->pid);
	}
	else if (!reqp->islocal) {
		/*
		 * XXX - Old style node, not doing jails.
		 *
		 * Added this for Dave. I love subqueries!
		 */
		res = mydb_query("select g.unix_name,g.unix_gid "
				 " from projects as p "
				 "left join groups as g on p.pid=g.pid "
				 "where p.approved!=0 and "
				 "  FIND_IN_SET(g.gid_idx, "
				 "   (select attrvalue from node_attributes "
				 "      where node_id='%s' and "
				 "            attrkey='dp_projects')) > 0",
				 2, reqp->pnodeid);

		if (!res || (int)mysql_num_rows(res) == 0) {
		 /*
		  * Temporary hack until we figure out the right model for
		  * remote nodes. For now, we use the pcremote-ok slot in
		  * in the project table to determine what remote nodes are
		  * okay'ed for the project. If connecting node type is in
		  * that list, then return all of the project groups, for
		  * each project that is allowed to get accounts on the type.
		  */
		  res = mydb_query("select g.unix_name,g.unix_gid "
				   "  from projects as p "
				   "join groups as g on p.pid=g.pid "
				   "where p.approved!=0 and "
				   "      FIND_IN_SET('%s',pcremote_ok)>0",
				   2, reqp->type);
		}
	}
	else {
		error("ACCOUNTS: %s: GIDs Fell off the bottom!\n", reqp->pid);
		return 1;
	}
	if (!res) {
		error("ACCOUNTS: %s: DB Error getting gids!\n", reqp->pid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		error("ACCOUNTS: %s: No Project!\n", reqp->pid);
		mysql_free_result(res);
		return 1;
	}

	while (nrows) {
		row = mysql_fetch_row(res);
		if (!row[1] || !row[1][1]) {
			error("ACCOUNTS: %s: No Project GID!\n", reqp->pid);
			mysql_free_result(res);
			return 1;
		}

		gidint = atoi(row[1]);
		OUTPUT(buf, sizeof(buf),
		       "ADDGROUP NAME=%s GID=%d\n", row[0], gidint);
		client_writeback(sock, buf, strlen(buf), tcp);
		if (verbose)
			info("%s: ACCOUNTS: %s", reqp->nodeid, buf);

		nrows--;
	}
	mysql_free_result(res);

	/*
	 * Each time a node picks up accounts, decrement the update
	 * counter. This ensures that if someone kicks off another
	 * update after this point, the node will end up getting to
	 * do it again in case it missed something.
	 */
	if (mydb_update("update nodes set update_accounts=update_accounts-1 "
			"where node_id='%s' and update_accounts!=0",
			reqp->nodeid)) {
		error("ACCOUNTS: %s: DB Error setting exit update_accounts!\n",
		      reqp->nodeid);
	}

	/*
	 * Now onto the users in the project.
	 */
	if (reqp->iscontrol) {
		/*
		 * All users! This is not currently used. The problem
		 * is that returning a list of hundreds of users whenever
		 * any single change is required is bad. Works fine for
		 * experimental nodes where the number of accounts is small,
		 * but is not scalable.
		 */
		res = mydb_query("select distinct "
				 "  u.uid,u.usr_pswd,u.unix_uid,u.usr_name, "
				 "  p.trust,g.pid,g.gid,g.unix_gid,u.admin, "
				 "  u.emulab_pubkey,u.home_pubkey, "
				 "  UNIX_TIMESTAMP(u.usr_modified), "
				 "  u.usr_email,u.usr_shell,u.uid_idx "
				 "from group_membership as p "
				 "join users as u on p.uid_idx=u.uid_idx "
				 "join groups as g on p.pid=g.pid "
				 "where p.trust!='none' "
				 "      and u.webonly=0 "
                                 "      and g.unix_id is not NULL "
				 "      and u.status='active' order by u.uid",
				 15, reqp->pid, reqp->gid);
	}
	else if (nodetypeprojects) {
		/*
		 * The projects/groups are specified as a comma separated
		 * list in the node_type_attributes table. Return this
		 * set of users.
		 */
		res = mydb_query("select distinct  "
				 "u.uid,'*',u.unix_uid,u.usr_name, "
				 "m.trust,g.pid,g.gid,g.unix_gid,u.admin, "
				 "u.emulab_pubkey,u.home_pubkey, "
				 "UNIX_TIMESTAMP(u.usr_modified), "
				 "u.usr_email,u.usr_shell, "
				 "u.widearearoot,u.wideareajailroot, "
				 "u.usr_w_pswd,u.uid_idx "
				 "from projects as p "
				 "join group_membership as m on "
				 "     m.pid_idx=p.pid_idx "
				 "join groups as g on "
				 "     g.gid_idx=m.gid_idx "
				 "join users as u on u.uid_idx=m.uid_idx "
				 "where p.approved!=0 "
				 "      and m.trust!='none' "
				 "      and u.webonly=0 "
                                 "      and g.unix_gid is not NULL "
				 "      and u.status='active' "
				 "      and u.admin=0 and "
				 "  (FIND_IN_SET(g.gid_idx, "
				 "   (select na.attrvalue from nodes as n "
				 "    left join node_type_attributes as na on "
				 "         n.type=na.type "
				 "    where n.node_id='%s' and "
				 "    na.attrkey='project_accounts')) > 0) "
				 "order by u.uid",
				 18, reqp->nodeid);
	}
	else if (reqp->isvnode ||
		 (reqp->islocal && !reqp->sharing_mode[0])) {
		/*
		 * This crazy join is going to give us multiple lines for
		 * each user that is allowed on the node, where each line
		 * (for each user) differs by the project PID and it unix
		 * GID. The intent is to build up a list of GIDs for each
		 * user to return. Well, a primary group and a list of aux
		 * groups for that user.
		 */
	  	char adminclause[MYBUFSIZE];
		strcpy(adminclause, "");
#ifdef ISOLATEADMINS
		sprintf(adminclause, "and u.admin=%d", reqp->swapper_isadmin);
#endif
		res = mydb_query("select distinct "
				 "  u.uid,u.usr_pswd,u.unix_uid,u.usr_name, "
				 "  p.trust,g.pid,g.gid,g.unix_gid,u.admin, "
				 "  u.emulab_pubkey,u.home_pubkey, "
				 "  UNIX_TIMESTAMP(u.usr_modified), "
				 "  u.usr_email,u.usr_shell, "
				 "  u.widearearoot,u.wideareajailroot, "
				 "  u.usr_w_pswd,u.uid_idx "
				 "from group_membership as p "
				 "join users as u on p.uid_idx=u.uid_idx "
				 "join groups as g on "
				 "     p.pid=g.pid and p.gid=g.gid "
				 "where ((p.pid='%s')) and p.trust!='none' "
				 "      and u.status='active' "
				 "      and u.webonly=0 "
				 "      %s "
                                 "      and g.unix_gid is not NULL "
				 "order by u.uid",
				 18, reqp->pid, adminclause);
	}
	else if ((reqp->jailflag && !reqp->islocal) ||
		 (reqp->islocal && reqp->sharing_mode[0])) {
		/*
		 * A remote node, doing jails or a local node being
		 * shared.  We still want to return accounts for the
		 * admin people outside the jails. Note that remote jail
		 * case is effectively deprecated at this point.
		 */
		res = mydb_query("select distinct "
			     "  u.uid,'*',u.unix_uid,u.usr_name, "
			     "  p.trust,g.pid,g.gid,g.unix_gid,u.admin, "
			     "  u.emulab_pubkey,u.home_pubkey, "
			     "  UNIX_TIMESTAMP(u.usr_modified), "
			     "  u.usr_email,u.usr_shell, "
			     "  u.widearearoot,u.wideareajailroot, "
			     "  u.usr_w_pswd,u.uid_idx "
			     "from group_membership as p "
			     "join users as u on p.uid_idx=u.uid_idx "
			     "join groups as g on "
			     "     p.pid=g.pid and p.gid=g.gid "
			     "where (p.pid='%s') and p.trust!='none' "
			     "      and u.status='active' and "
			     "      (u.admin=1 or u.uid='%s') "
			     "      order by u.uid",
			     18, RELOADPID, PROTOUSER);
	}
	else if (!reqp->islocal && reqp->isdedicatedwa) {
		/*
		 * Wonder why this code is a copy of the islocal || vnode
		 * case above?  It's because I don't want to prevent us from
		 * from the above case where !islocal && jailflag
		 * for dedicated widearea nodes.
		 */

		/*
		 * This crazy join is going to give us multiple lines for
		 * each user that is allowed on the node, where each line
		 * (for each user) differs by the project PID and it unix
		 * GID. The intent is to build up a list of GIDs for each
		 * user to return. Well, a primary group and a list of aux
		 * groups for that user.
		 */
	  	char adminclause[MYBUFSIZE];
		strcpy(adminclause, "");
#ifdef ISOLATEADMINS
		sprintf(adminclause, "and u.admin=%d", reqp->swapper_isadmin);
#endif
		res = mydb_query("select distinct "
				 "  u.uid,'*',u.unix_uid,u.usr_name, "
				 "  p.trust,g.pid,g.gid,g.unix_gid,u.admin, "
				 "  u.emulab_pubkey,u.home_pubkey, "
				 "  UNIX_TIMESTAMP(u.usr_modified), "
				 "  u.usr_email,u.usr_shell, "
				 "  u.widearearoot,u.wideareajailroot, "
				 "  u.usr_w_pswd,u.uid_idx "
				 "from group_membership as p "
				 "join users as u on p.uid_idx=u.uid_idx "
				 "join groups as g on "
				 "     p.pid=g.pid and p.gid=g.gid "
				 "where ((p.pid='%s')) and p.trust!='none' "
				 "      and u.status='active' "
				 "      and u.webonly=0 "
				 "      %s "
                                 "      and g.unix_gid is not NULL "
				 "order by u.uid",
				 18, reqp->pid, adminclause);
	}
	else if (! reqp->islocal) {
		/*
		 * XXX - Old style node, not doing jails.
		 *
		 * Temporary hack until we figure out the right model for
		 * remote nodes. For now, we use the pcremote-ok slot in
		 * in the project table to determine what remote nodes are
		 * okay'ed for the project. If connecting node type is in
		 * that list, then return user info for all of the users
		 * in those projects (crossed with group in the project).
		 */
		char subclause[MYBUFSIZE];
		int  count = 0;

		res = mydb_query("select attrvalue from node_attributes "
				 " where node_id='%s' and "
				 "       attrkey='dp_projects'",
				 1, reqp->pnodeid);

		if (res) {
			if ((int)mysql_num_rows(res) > 0) {
				row = mysql_fetch_row(res);

				count = snprintf(subclause,
					     sizeof(subclause) - 1,
					     "FIND_IN_SET(g.gid_idx,'%s')>0",
					     row[0]);
			}
			else {
				count = snprintf(subclause,
					     sizeof(subclause) - 1,
					     "FIND_IN_SET('%s',pcremote_ok)>0",
					     reqp->type);
			}
			mysql_free_result(res);

			if (count >= sizeof(subclause)) {
				error("ACCOUNTS: %s: Subclause too long!\n",
				      reqp->nodeid);
				return 1;
			}
		}

		res = mydb_query("select distinct  "
				 "u.uid,'*',u.unix_uid,u.usr_name, "
				 "m.trust,g.pid,g.gid,g.unix_gid,u.admin, "
				 "u.emulab_pubkey,u.home_pubkey, "
				 "UNIX_TIMESTAMP(u.usr_modified), "
				 "u.usr_email,u.usr_shell, "
				 "u.widearearoot,u.wideareajailroot, "
				 "u.usr_w_pswd,u.uid_idx "
				 "from projects as p "
				 "join group_membership as m "
				 "  on m.pid=p.pid "
				 "join groups as g on "
				 "  g.pid=m.pid and g.gid=m.gid "
				 "join users as u on u.uid_idx=m.uid_idx "
				 "where p.approved!=0 "
				 "      and %s "
				 "      and m.trust!='none' "
				 "      and u.webonly=0 "
				 "      and u.admin=0 "
                                 "      and g.unix_gid is not NULL "
				 "      and u.status='active' "
				 "order by u.uid",
				 18, subclause);
	}
	else {
		error("ACCOUNTS: %s: UIDs Fell off the bottom!\n", reqp->pid);
		return 1;
	}
	if (!res) {
		error("ACCOUNTS: %s: DB Error getting users!\n", reqp->pid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		error("ACCOUNTS: %s: No Users!\n", reqp->pid);
		mysql_free_result(res);
		return 0;
	}

 again:
	row = mysql_fetch_row(res);
	while (nrows) {
		MYSQL_ROW	nextrow = 0;
		MYSQL_RES	*pubkeys_res;
		MYSQL_RES	*sfskeys_res;
		int		pubkeys_nrows, sfskeys_nrows, i, root = 0;
		int		auxgids[128], gcount = 0;
		char		glist[BUFSIZ];
		char		*bufp = buf, *ebufp = &buf[sizeof(buf)];
		char		*pswd, *wpswd, wpswd_buf[9];

		gidint     = -1;
		tbadmin    = root = atoi(row[8]);
		gcount     = 0;

		while (1) {

			/*
			 * The whole point of this mess. Figure out the
			 * main GID and the aux GIDs. Perhaps trying to make
			 * distinction between main and aux is unecessary, as
			 * long as the entire set is represented.
			 */
			if (strcmp(row[5], reqp->pid) == 0 &&
			    strcmp(row[6], reqp->gid) == 0) {
				gidint = atoi(row[7]);

				/*
				 * Only people in the main pid can get root
				 * at this time, so do this test here.
				 */
				if ((strcmp(row[4], "local_root") == 0) ||
				    (strcmp(row[4], "group_root") == 0) ||
				    (strcmp(row[4], "project_root") == 0))
					root = 1;
			}
			else {
				int k, newgid = atoi(row[7]);

				/*
				 * Avoid dups, which can happen because of
				 * different trust levels in the join.
				 */
				for (k = 0; k < gcount; k++) {
				    if (auxgids[k] == newgid)
					goto skipit;
				}
				auxgids[gcount++] = newgid;
			skipit:
				;
			}
			nrows--;

			if (!nrows)
				break;

			/*
			 * See if the next row is the same UID. If so,
			 * we go around the inner loop again.
			 */
			nextrow = mysql_fetch_row(res);
			if (strcmp(row[0], nextrow[0]))
				break;
			row = nextrow;
		}

		/*
		 * widearearoot and wideareajailroot override trust values
		 * from the project (above) (IF the node is not isdedicatedwa,
		 * since these must behave like local). Of course, tbadmin
		 * overrides everthing!
		 */
		if (!reqp->islocal && !reqp->isdedicatedwa && !reqp->isplabdslice) {
			if (!reqp->isvnode)
				root = atoi(row[14]);
			else
				root = atoi(row[15]);

			if (tbadmin)
				root = 1;
		}

		/* There is an optional Windows password column. */
		pswd = row[1];
		wpswd = row[16];
		if (strncmp(rdata, "windows", 7) == 0) {
			if (wpswd != NULL && strlen(wpswd) > 0) {
				row[1] = wpswd;
			}
			else {

				/* The initial random default for the Windows Password
				 * is based on the Unix encrypted password hash, in
				 * particular the random salt when it's an MD5 crypt.
				 * THis is the 8 characters after an initial "$1$" and
				 * followed by a "$".  Just use the first 8 chars if
				 * the hash is not an MD5 crypt.
				 */
				strncpy(wpswd_buf,
					(strncmp(pswd,"$1$",3)==0) ? pswd + 3 : pswd,
					8);
				wpswd_buf[8]='\0';
				row[1] = wpswd_buf;
			}
		}

		/*
		 * Okay, process the UID. If there is no primary gid,
		 * then use one from the list. Then convert the rest of
		 * the list for the GLIST argument below.
		 */
		if (gidint == -1) {
			gidint = auxgids[--gcount];
		}
		glist[0] = '\0';
		for (i = 0; i < gcount; i++) {
			sprintf(&glist[strlen(glist)], "%d", auxgids[i]);

			if (i < gcount-1)
				strcat(glist, ",");
		}

		if (vers < 4) {
			bufp += OUTPUT(buf, sizeof(buf),
				"ADDUSER LOGIN=%s "
				"PSWD=%s UID=%s GID=%d ROOT=%d NAME=\"%s\" "
				"HOMEDIR=%s/%s GLIST=%s\n",
				row[0], row[1], row[2], gidint, root, row[3],
				USERDIR, row[0], glist);
		}
		else if (vers == 4) {
			bufp += OUTPUT(buf, sizeof(buf),
				"ADDUSER LOGIN=%s "
				"PSWD=%s UID=%s GID=%d ROOT=%d NAME=\"%s\" "
				"HOMEDIR=%s/%s GLIST=\"%s\" "
				"EMULABPUBKEY=\"%s\" HOMEPUBKEY=\"%s\"\n",
				row[0], row[1], row[2], gidint, root, row[3],
				USERDIR, row[0], glist,
				row[9] ? row[9] : "",
				row[10] ? row[10] : "");
		}
		else {
			if (!reqp->islocal) {
				if (vers == 5)
					row[1] = "'*'";
				else
					row[1] = "*";
			}
			bufp += OUTPUT(buf, sizeof(buf),
				"ADDUSER LOGIN=%s "
				"PSWD=%s UID=%s GID=%d ROOT=%d NAME=\"%s\" "
				"HOMEDIR=%s/%s GLIST=\"%s\" SERIAL=%s",
				row[0], row[1], row[2], gidint, root, row[3],
				USERDIR, row[0], glist, row[11]);

			if (vers >= 9) {
				bufp += OUTPUT(bufp, ebufp - bufp,
					       " EMAIL=\"%s\"", row[12]);
			}
			if (vers >= 10) {
				bufp += OUTPUT(bufp, ebufp - bufp,
					       " SHELL=%s", row[13]);
			}
			OUTPUT(bufp, ebufp - bufp, "\n");
		}

		client_writeback(sock, buf, strlen(buf), tcp);

		if (verbose)
			info("ACCOUNTS: "
			     "ADDUSER LOGIN=%s "
			     "UID=%s GID=%d ROOT=%d GLIST=%s\n",
			     row[0], row[2], gidint, root, glist);

		if (vers < 5)
			goto skipkeys;

		/*
		 * Locally, everything is NFS mounted so no point in
		 * sending back pubkey stuff; it's never used except on CygWin.
		 * Add an argument of "pubkeys" to get the PUBKEY data.
		 * An "windows" argument also returns a user's Windows Password.
		 */
#ifndef NOSHAREDFS
		if (reqp->islocal &&
		    ! reqp->genisliver_idx &&
		    ! reqp->sharing_mode[0] &&
		    ! (strncmp(rdata, "pubkeys", 7) == 0
		       || strncmp(rdata, "windows", 7) == 0))
			goto skipsshkeys;
#endif

		/*
		 * Need a list of keys for this user.
		 */
		if (didnonlocal) {
			pubkeys_res = mydb_query("select idx,pubkey "
						 " from nonlocal_user_pubkeys "
						 "where uid_idx='%s'",
						 2, row[17]);
		}
		else {
			pubkeys_res = mydb_query("select idx,pubkey "
						 " from user_pubkeys "
						 "where uid_idx='%s'",
						 2, row[17]);
		}
		if (!pubkeys_res) {
			error("ACCOUNTS: %s: DB Error getting keys\n", row[0]);
			goto skipkeys;
		}
		if ((pubkeys_nrows = (int)mysql_num_rows(pubkeys_res))) {
			while (pubkeys_nrows) {
				MYSQL_ROW	pubkey_row;
				int		klen;
				char		*kbuf;

				pubkey_row = mysql_fetch_row(pubkeys_res);

#define _KHDR	"PUBKEY LOGIN=%s KEY=\"%s\"\n"
				/*
				 * Pubkeys can be large, so we may have to
				 * malloc a special buffer.
				 */
				klen = strlen(_KHDR)
					+ strlen(row[0])
					+ strlen(pubkey_row[1])
					- 4 /* 2 x %s */
					+ 1;
				if (klen > sizeof(buf) - 1) {
					kbuf = malloc(klen);
					if (kbuf == 0) {
						info("ACCOUNT: WARNING "
						     "pubkey for %s too large, "
						     "skipped\n", row[0]);
						pubkeys_nrows--;
						continue;
					}
				} else {
					kbuf = buf;
					klen = sizeof(buf);
				}
				OUTPUT(kbuf, klen, _KHDR,
				       row[0], pubkey_row[1]);
				client_writeback(sock, kbuf, strlen(kbuf), tcp);
				if (kbuf != buf)
					free(kbuf);
				pubkeys_nrows--;
#undef _KHDR

				if (verbose)
					info("ACCOUNTS: PUBKEY LOGIN=%s "
					     "IDX=%s\n",
					     row[0], pubkey_row[0]);
			}
		}
		mysql_free_result(pubkeys_res);

#ifndef NOSHAREDFS
	skipsshkeys:
#endif
		/*
		 * Do not bother to send back SFS keys if the node is not
		 * running SFS.
		 */
		if (vers < 6 || !strlen(reqp->sfshostid))
			goto skipkeys;

		/*
		 * Need a list of SFS keys for this user.
		 */
		sfskeys_res = mydb_query("select comment,pubkey "
					 " from user_sfskeys "
					 "where uid_idx='%s'",
					 2, row[17]);

		if (!sfskeys_res) {
			error("ACCOUNTS: %s: DB Error getting SFS keys\n", row[0]);
			goto skipkeys;
		}
		if ((sfskeys_nrows = (int)mysql_num_rows(sfskeys_res))) {
			while (sfskeys_nrows) {
				MYSQL_ROW	sfskey_row;

				sfskey_row = mysql_fetch_row(sfskeys_res);

				OUTPUT(buf, sizeof(buf),
				       "SFSKEY KEY=\"%s\"\n", sfskey_row[1]);

				client_writeback(sock, buf, strlen(buf), tcp);
				sfskeys_nrows--;

				if (verbose)
					info("ACCOUNTS: SFSKEY LOGIN=%s "
					     "COMMENT=%s\n",
					     row[0], sfskey_row[0]);
			}
		}
		mysql_free_result(sfskeys_res);

	skipkeys:
		row = nextrow;
	}
	mysql_free_result(res);

	if (!(reqp->islocal || reqp->isvnode) && !didwidearea) {
		didwidearea = 1;

		/*
		 * Sleazy. The only real downside though is that
		 * we could get some duplicate entries, which won't
		 * really harm anything on the client.
		 */
		res = mydb_query("select distinct "
				 "u.uid,'*',u.unix_uid,u.usr_name, "
				 "w.trust,'guest','guest',31,u.admin, "
				 "u.emulab_pubkey,u.home_pubkey, "
				 "UNIX_TIMESTAMP(u.usr_modified), "
				 "u.usr_email,u.usr_shell, "
				 "u.widearearoot,u.wideareajailroot "
				 "from widearea_accounts as w "
				 "join users as u on u.uid_idx=w.uid_idx "
				 "where w.trust!='none' and "
				 "      u.status='active' and "
				 "      node_id='%s' "
				 "order by u.uid",
				 16, reqp->nodeid);

		if (res) {
			if ((nrows = mysql_num_rows(res)))
				goto again;
			else
				mysql_free_result(res);
		}
	}

	/*
	 * When sharing mode is on, do not return these accounts to pnodes.
	 * Note that sharing_mode and genisliver_idx should not both be set
	 * on a pnode, but lets be careful.
	 * but
	 */
	if (reqp->genisliver_idx && !didnonlocal &&
	    (reqp->isvnode || !reqp->sharing_mode[0])) {
	        didnonlocal = 1;
		
		/*
		 * Within the nonlocal_user_accounts table, we do not
		 * maintain globally unique unix_uid numbers, since these
		 * accounts are per slice (experiment). Instead, just
		 * use an auto_increment field, which always starts at
		 * 1, and so to create a unix_gid, we just bump the
		 * number into a typically unused area of the space.
		 */
		res = mydb_query("select distinct "
				 "  u.uid,'*', "
				 "  u.unix_uid+20000,"
				 "  u.name, "
				 "  'local_root',g.pid,g.gid,g.unix_gid,0, "
				 "  NULL,NULL, "
				 "  UNIX_TIMESTAMP(now()), "
				 "  u.email,'csh', "
				 "  0,0, "
				 "  NULL,u.uid_idx "
				 "from nonlocal_user_accounts as u "
				 "join groups as g on "
				 "     g.pid='%s' and "
				 "     (g.pid=g.gid or g.gid='%s') "
				 "where (u.exptidx='%d') "
				 "order by u.uid",
				 18, reqp->pid, reqp->gid,
				 reqp->exptidx);

		if (res) {
			if ((nrows = mysql_num_rows(res)))
				goto again;
			else
				mysql_free_result(res);
		}
	}
	return 0;
}

/*
 * Return delay config stuff.
 */
COMMAND_PROTOTYPE(dodelay)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[2*MYBUFSIZE], *ebufp = &buf[sizeof(buf)];
	int		nrows;

	/*
	 * Get delay parameters for the machine. The point of this silly
	 * join is to get the type out so that we can pass it back. Of
	 * course, this assumes that the type is the BSD name, not linux.
	 */
	res = mydb_query("select i.MAC,j.MAC, "
		 "pipe0,delay0,bandwidth0,lossrate0,q0_red, "
		 "pipe1,delay1,bandwidth1,lossrate1,q1_red, "
		 "vname, "
		 "q0_limit,q0_maxthresh,q0_minthresh,q0_weight,q0_linterm, "
		 "q0_qinbytes,q0_bytes,q0_meanpsize,q0_wait,q0_setbit, "
		 "q0_droptail,q0_gentle, "
		 "q1_limit,q1_maxthresh,q1_minthresh,q1_weight,q1_linterm, "
		 "q1_qinbytes,q1_bytes,q1_meanpsize,q1_wait,q1_setbit, "
		 "q1_droptail,q1_gentle,vnode0,vnode1,noshaping, "
		 "backfill0,backfill1,isbridge,vlan0,vlan1"
                 " from delays as d "
		 "left join interfaces as i on "
		 " i.node_id=d.node_id and i.iface=iface0 "
		 "left join interfaces as j on "
		 " j.node_id=d.node_id and j.iface=iface1 "
		 " where d.node_id='%s'",
		 45, reqp->nodeid);
	if (!res) {
		error("DELAY: %s: DB Error getting delays!\n", reqp->nodeid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}
	while (nrows) {
		char	*bufp = buf;
		int     isbridge;
		char    *vnode0, *vnode1;

		row = mysql_fetch_row(res);

		/*
		 * Yikes, this is ugly! Sanity check though, since I saw
		 * some bogus values in the DB.
		 */
		if (!row[0] || !row[1] || !row[2] || !row[3]) {
			error("DELAY: %s: DB values are bogus!\n",
			      reqp->nodeid);
			mysql_free_result(res);
			return 1;
		}
		isbridge = atoi(row[42]);
		if (isbridge) {
			vnode0 = row[43];
			vnode1 = row[44];
		}
		else {
			vnode0 = (row[37] ? row[37] : "foo");
			vnode1 = (row[38] ? row[38] : "bar");
		}
		bufp += OUTPUT(bufp, ebufp - bufp,
			"DELAY INT0=%s INT1=%s "
			"PIPE0=%s DELAY0=%s BW0=%s PLR0=%s "
			"PIPE1=%s DELAY1=%s BW1=%s PLR1=%s "
			"LINKNAME=%s "
			"RED0=%s RED1=%s "
			"LIMIT0=%s MAXTHRESH0=%s MINTHRESH0=%s WEIGHT0=%s "
			"LINTERM0=%s QINBYTES0=%s BYTES0=%s "
			"MEANPSIZE0=%s WAIT0=%s SETBIT0=%s "
			"DROPTAIL0=%s GENTLE0=%s "
			"LIMIT1=%s MAXTHRESH1=%s MINTHRESH1=%s WEIGHT1=%s "
			"LINTERM1=%s QINBYTES1=%s BYTES1=%s "
			"MEANPSIZE1=%s WAIT1=%s SETBIT1=%s "
			"DROPTAIL1=%s GENTLE1=%s "
			"VNODE0=%s VNODE1=%s "
			"NOSHAPING=%s "
			"BACKFILL0=%s BACKFILL1=%s\n",
			row[0], row[1],
			row[2], row[3], row[4], row[5],
			row[7], row[8], row[9], row[10],
			row[12],
			row[6], row[11],
			row[13], row[14], row[15], row[16],
			row[17], row[18], row[19],
			row[20], row[21], row[22],
			row[23], row[24],
			row[25], row[26], row[27], row[28],
			row[29], row[30], row[31],
			row[32], row[33], row[34],
			row[35], row[36], vnode0, vnode1,
			row[39],
			row[40], row[41]);

		client_writeback(sock, buf, strlen(buf), tcp);
		nrows--;
		if (verbose)
			info("DELAY: %s", buf);
	}
	mysql_free_result(res);

	return 0;
}

/*
 * Return link delay config stuff.
 */
COMMAND_PROTOTYPE(dolinkdelay)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[2*MYBUFSIZE];
	int		nrows;

	/*
	 * Get link delay parameters for the machine. We store veth
	 * interfaces in another dynamic table, so must join with both
	 * interfaces and vinterfaces to see which iface this link
	 * delay corresponds to. If there is a veth entry use that, else
	 * use the normal interfaces entry. I do not like this much.
	 * Maybe we should use the regular interfaces table, with type veth,
	 * entries added/deleted on the fly. I avoided that cause I view
	 * the interfaces table as static and pertaining to physical
	 * interfaces.
	 *
	 * Outside a vnode, return only those linkdelays for veths that have
	 * vnode=NULL, which indicates its an emulated interface on a
	 * physical node. When inside a vnode, only return veths for which
	 * vnode=curvnode, which are the interfaces that correspond to a
	 * jail node.
	 */
	if (reqp->isvnode)
		sprintf(buf, "and v.vnode_id='%s'", reqp->vnodeid);
	else
		strcpy(buf, "and v.vnode_id is NULL");

	res = mydb_query("select i.MAC,d.type,vlan,vnode,d.ip,netmask, "
		 "pipe,delay,d.bandwidth,lossrate, "
		 "rpipe,rdelay,rbandwidth,rlossrate, "
		 "q_red,q_limit,q_maxthresh,q_minthresh,q_weight,q_linterm, "
		 "q_qinbytes,q_bytes,q_meanpsize,q_wait,q_setbit, "
		 "q_droptail,q_gentle,v.mac "
                 " from linkdelays as d "
		 "left join interfaces as i on "
		 " i.node_id=d.node_id and i.iface=d.iface "
		 "left join vinterfaces as v on "
		 " v.node_id=d.node_id and v.IP=d.ip "
		 "where d.node_id='%s' and d.exptidx='%d' %s",
		 28, reqp->pnodeid, reqp->exptidx, buf);
	if (!res) {
		error("LINKDELAY: %s: DB Error getting link delays!\n",
		      reqp->nodeid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}
	while (nrows) {
		row = mysql_fetch_row(res);

		OUTPUT(buf, sizeof(buf),
		        "LINKDELAY IFACE=%s TYPE=%s "
			"LINKNAME=%s VNODE=%s INET=%s MASK=%s "
			"PIPE=%s DELAY=%s BW=%s PLR=%s "
			"RPIPE=%s RDELAY=%s RBW=%s RPLR=%s "
			"RED=%s LIMIT=%s MAXTHRESH=%s MINTHRESH=%s WEIGHT=%s "
			"LINTERM=%s QINBYTES=%s BYTES=%s "
			"MEANPSIZE=%s WAIT=%s SETBIT=%s "
			"DROPTAIL=%s GENTLE=%s\n",
			(row[27] ? row[27] : row[0]), row[1],
			row[2],  row[3],  row[4],  CHECKMASK(row[5]),
			row[6],	 row[7],  row[8],  row[9],
			row[10], row[11], row[12], row[14],
			row[14], row[15], row[16], row[17], row[18],
			row[19], row[20], row[21],
			row[22], row[23], row[24],
			row[25], row[26]);

		client_writeback(sock, buf, strlen(buf), tcp);
		nrows--;
		if (verbose)
			info("LINKDELAY: %s", buf);
	}
	mysql_free_result(res);

	return 0;
}

/*
 * Return bridge config stuff.
 */
COMMAND_PROTOTYPE(dobridges)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[2*MYBUFSIZE], *ebufp = &buf[sizeof(buf)];
	int		nrows;

	/*
	 * Get bridge parameters for the machine. 
	 */
	res = mydb_query("select b.bridx,i.MAC,b.vnode,b.vname "
			 " from bridges as b "
			 "left join interfaces as i on "
			 " i.node_id=b.node_id and i.iface=b.iface "
			 " where b.node_id='%s' order by bridx",
			 4, reqp->nodeid);
	if (!res) {
		error("BRIDGES: %s: DB Error getting bridges!\n", reqp->nodeid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}
	while (nrows) {
		char	*bufp = buf;

		row = mysql_fetch_row(res);

		/*
		 * Sanity check.
		 */
		if (!row[0] || !row[1] || !row[2] || !row[3]) {
			error("BRIDGES: %s: DB values are bogus!\n",
			      reqp->nodeid);
			mysql_free_result(res);
			return 1;
		}

		bufp += OUTPUT(bufp, ebufp - bufp,
			       "BRIDGE IDX=%s IFACE=%s VNODE=%s LINKNAME=%s\n",
			       row[0], row[1], row[2], row[3]);

		client_writeback(sock, buf, strlen(buf), tcp);
		nrows--;
		if (verbose)
			info("BRIDGES: %s", buf);
	}
	mysql_free_result(res);

	return 0;
}

COMMAND_PROTOTYPE(dohosts)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
	int		hostcount, nrows;
	int		rv = 0;
	char		*thisvnode = (char *) NULL;

	/*
	 * We build up a canonical host table using this data structure.
	 * There is one item per node/iface. We need a shared structure
	 * though for each node, to allow us to compute the aliases.
	 */
	struct shareditem {
	    	int	hasalias;
		char	*firstvlan;	/* The first vlan to another node */
		int	is_me;          /* 1 if this node is the tmcc client */
	};
	struct shareditem *shareditem = (struct shareditem *) NULL;

	struct hostentry {
		char	nodeid[TBDB_FLEN_NODEID];
		char	vname[TBDB_FLEN_VNAME];
		char	vlan[TBDB_FLEN_VNAME];
		int	virtiface;
		struct in_addr	  ipaddr;
		struct shareditem *shared;
		struct hostentry  *next;
	} *hosts = 0, *host;

	/*
	 * Now use the virt_nodes table to get a list of all of the
	 * nodes and all of the IPs on those nodes. This is the basis
	 * for the canonical host table. Join it with the reserved
	 * table to get the node_id at the same time (saves a step).
	 *
	 * XXX NSE hack: Using the v2pmap table instead of reserved because
	 * of multiple simulated to one physical node mapping. Currently,
	 * reserved table contains a vname which is generated in the case of
	 * nse
	 *
	 * XXX PELAB hack: If virt_lans.protocol is 'ipv4' then this is a
	 * "LAN" of real internet nodes and we should return the control
	 * network IP address.  The intent is to give plab nodes some
	 * convenient aliases for refering to each other.
	 */
	res = mydb_query("select v.vname,v.vnode,v.ip,v.vport,v2p.node_id,"
			 "v.protocol,i.IP "
			 "    from virt_lans as v "
			 "left join v2pmap as v2p on "
			 "     v.vnode=v2p.vname and v.pid=v2p.pid and "
			 "     v.eid=v2p.eid "
			 "left join nodes as n on "
			 "     v2p.node_id=n.node_id "
			 "left join interfaces as i on "
			 "     n.phys_nodeid=i.node_id and i.role='ctrl' "
			 "where v.pid='%s' and v.eid='%s' and "
			 "      v2p.node_id is not null "
			 "      order by v.vnode,v.vname",
			 7, reqp->pid, reqp->eid);

	if (!res) {
		error("HOSTNAMES: %s: DB Error getting virt_lans!\n",
		      reqp->nodeid);
		return 1;
	}
	if (! (nrows = mysql_num_rows(res))) {
		mysql_free_result(res);
		return 0;
	}

	/*
	 * Parse the list, creating an entry for each node/IP pair.
	 */
	while (nrows--) {
		row = mysql_fetch_row(res);
		if (!row[0] || !row[0][0] ||
		    !row[1] || !row[1][0])
			continue;

		if (!thisvnode || strcmp(thisvnode, row[1])) {
			if (! (shareditem = (struct shareditem *)
			       calloc(1, sizeof(*shareditem)))) {
				error("HOSTNAMES: "
				      "Out of memory for shareditem!\n");
				exit(1);
			}
			thisvnode = row[1];
		}

		/*
		 * Check to see if this is the node we're talking to
		 */
		if (!strcmp(row[1], reqp->nickname)) {
		    shareditem->is_me = 1;
		}

		/*
		 * Alloc the per-link struct and fill in.
		 */
		if (! (host = (struct hostentry *) calloc(1, sizeof(*host)))) {
			error("HOSTNAMES: Out of memory!\n");
			exit(1);
		}

		strcpy(host->vlan, row[0]);
		strcpy(host->vname, row[1]);
		strcpy(host->nodeid, row[4]);
		host->virtiface = atoi(row[3]);
		host->shared = shareditem;

		/*
		 * As mentioned above, links with protocol 'ipv4'
		 * use the control net addresses of connected nodes.
		 */
		if (row[5] && strcmp("ipv4", row[5]) == 0 && row[6])
			inet_aton(row[6], &host->ipaddr);
		else
			inet_aton(row[2], &host->ipaddr);

		host->next = hosts;
		hosts = host;
	}
	mysql_free_result(res);

	/*
	 * The last part of the puzzle is to determine who is directly
	 * connected to this node so that we can add an alias for the
	 * first link to each connected node (could be more than one link
	 * to another node). Since we have the vlan names for all the nodes,
	 * its a simple matter of looking in the table for all of the nodes
	 * that are in the same vlan as the node that we are making the
	 * host table for.
	 */
	host = hosts;
	while (host) {
		/*
		 * Only care about this nodes vlans.
		 */
		if (strcmp(host->nodeid, reqp->nodeid) == 0 && host->vlan) {
			struct hostentry *tmphost = hosts;

			while (tmphost) {
				if (strlen(tmphost->vlan) &&
				    strcmp(host->vlan, tmphost->vlan) == 0 &&
				    strcmp(host->nodeid, tmphost->nodeid) &&
				    (!tmphost->shared->firstvlan ||
				     !strcmp(tmphost->vlan,
					     tmphost->shared->firstvlan))) {

					/*
					 * Use as flag to ensure only first
					 * link flagged as connected (gets
					 * an alias), but since a vlan could
					 * be a real lan with more than one
					 * other member, must tag all the
					 * members.
					 */
					tmphost->shared->firstvlan =
						tmphost->vlan;
				}
				tmphost = tmphost->next;
			}
		}
		host = host->next;
	}
#if 0
	host = hosts;
	while (host) {
		printf("%s %s %s %d %s %d\n", host->vname, host->nodeid,
		       host->vlan, host->virtiface, inet_ntoa(host->ipaddr),
		       host->connected);
		host = host->next;
	}
#endif

	/*
	 * Okay, spit the entries out!
	 */
	hostcount = 0;
	host = hosts;
	while (host) {
		char	*alias = " ";

		if ((host->shared->firstvlan &&
		     !strcmp(host->shared->firstvlan, host->vlan)) ||
		    /* Directly connected, first interface on this node gets an
		       alias */
		    (!strcmp(host->nodeid, reqp->nodeid) && !host->virtiface)){
			alias = host->vname;
		} else if (!host->shared->firstvlan &&
			   !host->shared->hasalias &&
			   !host->shared->is_me) {
		    /* Not directly connected, but we'll give it an alias
		       anyway */
		    alias = host->vname;
		    host->shared->hasalias = 1;
		}

		/* Old format */
		if (vers == 2) {
			OUTPUT(buf, sizeof(buf),
			       "NAME=%s LINK=%i IP=%s ALIAS=%s\n",
			       host->vname, host->virtiface,
			       inet_ntoa(host->ipaddr), alias);
		}
		else {
			OUTPUT(buf, sizeof(buf),
			       "NAME=%s-%s IP=%s ALIASES='%s-%i %s'\n",
			       host->vname, host->vlan,
			       inet_ntoa(host->ipaddr),
			       host->vname, host->virtiface, alias);
		}
		client_writeback(sock, buf, strlen(buf), tcp);
		host = host->next;
		hostcount++;
	}

	/*
	 * For plab slices, lets include boss and ops IPs as well
	 * in case of flaky name service on the nodes.
	 *
	 * XXX we only do this if we were going to return something
	 * otherwise.  In the event there are no other hosts, we would
	 * not overwrite the existing hosts file which already has boss/ops.
	 */
	if (reqp->isplabdslice && hostcount > 0) {
		OUTPUT(buf, sizeof(buf),
		       "NAME=%s IP=%s ALIASES=''\nNAME=%s IP=%s ALIASES=''\n",
		       BOSSNODE, EXTERNAL_BOSSNODE_IP,
		       USERNODE, EXTERNAL_USERNODE_IP);
		client_writeback(sock, buf, strlen(buf), tcp);
		hostcount += 2;
	}

#if 0
	/*
	 * List of control net addresses for jailed nodes.
	 * Temporary.
	 */
	res = mydb_query("select r.node_id,r.vname,n.jailip "
			 " from reserved as r "
			 "left join nodes as n on n.node_id=r.node_id "
			 "where r.pid='%s' and r.eid='%s' "
			 "      and jailflag=1 and jailip is not null",
			 3, reqp->pid, reqp->eid);
	if (res) {
	    if ((nrows = mysql_num_rows(res))) {
		while (nrows--) {
		    row = mysql_fetch_row(res);

		    OUTPUT(buf, sizeof(buf),
			   "NAME=%s IP=%s ALIASES='%s.%s.%s.%s'\n",
			   row[0], row[2], row[1], reqp->eid, reqp->pid,
			   OURDOMAIN);
		    client_writeback(sock, buf, strlen(buf), tcp);
		    hostcount++;
		}
	    }
	    mysql_free_result(res);
	}
#endif
	info("HOSTNAMES: %d hosts in list\n", hostcount);
	host = hosts;
	while (host) {
		struct hostentry *tmphost = host->next;
		free(host);
		host = tmphost;
	}
	return rv;
}

/*
 * Return RPM stuff.
 */
COMMAND_PROTOTYPE(dorpms)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE], *bp, *sp;

	/*
	 * Get RPM list for the node.
	 */
	res = mydb_query("select rpms from nodes where node_id='%s' ",
			 1, reqp->nodeid);

	if (!res) {
		error("RPMS: %s: DB Error getting RPMS!\n", reqp->nodeid);
		return 1;
	}

	if ((int)mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return 0;
	}

	/*
	 * Text string is a colon separated list.
	 */
	row = mysql_fetch_row(res);
	if (! row[0] || !row[0][0]) {
		mysql_free_result(res);
		return 0;
	}

	bp  = row[0];
	sp  = bp;
	do {
		bp = strsep(&sp, ":");

		OUTPUT(buf, sizeof(buf), "RPM=%s\n", bp);
		client_writeback(sock, buf, strlen(buf), tcp);
		if (verbose)
			info("RPM: %s", buf);

	} while ((bp = sp));

	mysql_free_result(res);
	return 0;
}

/*
 * Return Tarball stuff.
 */
COMMAND_PROTOTYPE(dotarballs)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE], *bp, *sp, *tp;

	/*
	 * Get Tarball list for the node.
	 */
	res = mydb_query("select tarballs from nodes where node_id='%s' ",
			 1, reqp->nodeid);

	if (!res) {
		error("TARBALLS: %s: DB Error getting tarballs!\n",
		      reqp->nodeid);
		return 1;
	}

	if ((int)mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return 0;
	}

	/*
	 * Text string is a colon separated list of "dir filename".
	 */
	row = mysql_fetch_row(res);
	if (! row[0] || !row[0][0]) {
		mysql_free_result(res);
		return 0;
	}

	bp  = row[0];
	sp  = bp;
	do {
		bp = strsep(&sp, ":");
		if ((tp = strchr(bp, ' ')) == NULL)
			continue;
		*tp++ = '\0';

		OUTPUT(buf, sizeof(buf), "DIR=%s TARBALL=%s\n", bp, tp);
		client_writeback(sock, buf, strlen(buf), tcp);
		if (verbose)
			info("TARBALLS: %s", buf);

	} while ((bp = sp));

	mysql_free_result(res);
	return 0;
}

/* 
 *
 */
COMMAND_PROTOTYPE(doblobs)
{
	MYSQL_RES       *res;
	MYSQL_ROW       row;
	int             nrows;
	char            buf[MYBUFSIZE];
	char            *bufp = buf, *ebufp = &buf[sizeof(buf)];
	
	res = mydb_query("select path,action from experiment_blobs "
			 " where exptidx=%d order by idx",
			 2, reqp->exptidx);
	
	if (!res) {
		error("BLOBS: %s: DB Error getting blobs for %s/%s!\n",
		      reqp->nodeid, reqp->pid, reqp->eid);
		return 1;
	}
	
	nrows = (int)mysql_num_rows(res);
	if (nrows <= 0) {
		mysql_free_result(res);
		return 0;
	}

	/*
	 * Frisbee blobs did exist prior to version 33, but the infrastructure
	 * was not deployed on more than a couple of images and only at Utah.
	 * So we are not going to bother with frisbee backward compat changes
	 * and just require that all images have the new frisbee master server
	 * infrastructure.
	 */
	if (vers < 33) {
		error("BLOBS: %s: requires new frisbee, rebuild image!\n",
		      reqp->nodeid);
		mysql_free_result(res);
		return 1;
	}

	while (nrows > 0) {
		row = mysql_fetch_row(res);
		if (row[0] == NULL || row[0][0] == '\0' ||
		    row[1] == NULL || row[1][0] == '\0') {
			error("BLOBS: %s: bogus path/action for %s/%s in DB\n",
			      reqp->nodeid, reqp->pid, reqp->eid);
			continue;
		}

		bufp += OUTPUT(bufp, ebufp - bufp,
			       "URL=frisbee://%s ACTION=%s\n",
			       row[0], row[1]);
		nrows--;
	}

	client_writeback(sock, buf, strlen(buf), tcp);

	if (verbose)
		info("doblobs: %s", buf);

	mysql_free_result(res);
	return 0;
}


/*
 * This is deprecated, but left in case old images reference it.
 */
COMMAND_PROTOTYPE(dodeltas)
{
	return 0;
}

/*
 * Return node run command. We return the command to run, plus the UID
 * of the experiment creator to run it as!
 */
COMMAND_PROTOTYPE(dostartcmd)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];

	/*
	 * Get run command for the node.
	 */
	res = mydb_query("select startupcmd from nodes where node_id='%s'",
			 1, reqp->nodeid);

	if (!res) {
		error("STARTUPCMD: %s: DB Error getting startup command!\n",
		       reqp->nodeid);
		return 1;
	}

	if ((int)mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return 0;
	}

	/*
	 * Simple text string.
	 */
	row = mysql_fetch_row(res);
	if (! row[0] || !row[0][0]) {
		mysql_free_result(res);
		return 0;
	}
	OUTPUT(buf, sizeof(buf), "CMD='%s' UID=%s\n", row[0], reqp->swapper);
	mysql_free_result(res);
	client_writeback(sock, buf, strlen(buf), tcp);
	if (verbose)
		info("STARTUPCMD: %s", buf);

	return 0;
}

/*
 * Accept notification of start command exit status.
 */
COMMAND_PROTOTYPE(dostartstat)
{
	int		exitstatus;

	/*
	 * Dig out the exit status
	 */
	if (! sscanf(rdata, "%d", &exitstatus)) {
		error("STARTSTAT: %s: Invalid status: %s\n",
		      reqp->nodeid, rdata);
		return 1;
	}

	if (verbose)
		info("STARTSTAT: "
		     "%s is reporting startup command exit status: %d\n",
		     reqp->nodeid, exitstatus);

	/*
	 * Update the node table record with the exit status. Setting the
	 * field to a non-null string value is enough to tell whoever is
	 * watching it that the node is done.
	 */
	if (mydb_update("update nodes set startstatus='%d' "
			"where node_id='%s'", exitstatus, reqp->nodeid)) {
		error("STARTSTAT: %s: DB Error setting exit status!\n",
		      reqp->nodeid);
		return 1;
	}
	return 0;
}

/*
 * Accept notification of ready for action
 */
COMMAND_PROTOTYPE(doready)
{
	/*
	 * Vnodes not allowed!
	 */
	if (reqp->isvnode)
		return 0;

	/*
	 * Update the ready_bits table.
	 */
	if (mydb_update("update nodes set ready=1 "
			"where node_id='%s'", reqp->nodeid)) {
		error("%s: READY: DB Error setting ready bit!\n",
		      reqp->nodeid);
		return 1;
	}

	if (verbose)
		info("%s: READY: Node is reporting ready\n", reqp->nodeid);

	/*
	 * Nothing is written back
	 */
	return 0;
}

/*
 * Return ready bits count (NofM)
 */
COMMAND_PROTOTYPE(doreadycount)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
	int		total, ready, i;

	/*
	 * Vnodes not allowed!
	 */
	if (reqp->isvnode)
		return 0;

	/*
	 * See how many are ready. This is a non sync protocol. Clients
	 * keep asking until N and M are equal. Can only be used once
	 * of course, after experiment creation.
	 */
	res = mydb_query("select ready from reserved "
			 "left join nodes on nodes.node_id=reserved.node_id "
			 "where reserved.eid='%s' and reserved.pid='%s'",
			 1, reqp->eid, reqp->pid);

	if (!res) {
		error("READYCOUNT: %s: DB Error getting ready bits.\n",
		      reqp->nodeid);
		return 1;
	}

	ready = 0;
	total = (int) mysql_num_rows(res);
	if (total) {
		for (i = 0; i < total; i++) {
			row = mysql_fetch_row(res);

			if (atoi(row[0]))
				ready++;
		}
	}
	mysql_free_result(res);

	OUTPUT(buf, sizeof(buf), "READY=%d TOTAL=%d\n", ready, total);
	client_writeback(sock, buf, strlen(buf), tcp);

	if (verbose)
		info("READYCOUNT: %s: %s", reqp->nodeid, buf);

	return 0;
}

/*
 * Return mount stuff.
 */
COMMAND_PROTOTYPE(domounts)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
	char		*bufp, *ebufp = &buf[sizeof(buf)];
	int		nrows, usesfs;
	int		nomounts = 0;
#ifdef  ISOLATEADMINS
	int		isadmin;
#endif

	/*
	 * Do we export filesystems at all?
	 * XXX this could be made per-experiment/project/whatever.
	 */
#ifdef	NOSHAREDFS
	nomounts = 1;
#endif

	/*
	 * Older clients will not properly handle the new format mount
	 * lines, so just return nothing and hope for the best.
	 */
	if (vers < 32 && nomounts)
		return 0;

	/*
	 * Should SFS mounts be served?
	 */
	usesfs = 0;
	if (vers >= 6 && strlen(fshostid) && !nomounts) {
		if (strlen(reqp->sfshostid))
			usesfs = 1;
		else {
			while (*rdata && isspace(*rdata))
				rdata++;

			if (!strncmp(rdata, "USESFS=1", strlen("USESFS=1")))
				usesfs = 1;
		}

		if (verbose) {
			if (usesfs) {
				info("Using SFS\n");
			}
			else {
				info("Not using SFS\n");
			}
		}
	}

	/*
	 * Remote nodes must use SFS.
	 */
	if (!reqp->islocal && !usesfs)
		return 0;

	/*
	 * Return info about the file server
	 */
	if (vers >= 32) {
		char *fstype = "";

		/* XXX sanity for code below */
		if (reqp->sharing_mode[0] && !reqp->isvnode)
			usesfs = 0;

		if (nomounts) {
			fstype = "LOCAL";
		} else if (usesfs) {
			fstype = "SFS";
		} else {
#ifdef NFSRACY
			fstype = "NFS-RACY";
#else
			fstype = "NFS";
#endif
		}

		OUTPUT(buf, sizeof(buf), "FSTYPE=%s\n", fstype);
		client_writeback(sock, buf, strlen(buf), tcp);
	}


	/*
	 * A local phys node acting as a shared host gets toplevel mounts only.
	 */
	if (reqp->sharing_mode[0] && !reqp->isvnode) {
		bufp = buf;
		if (!nomounts)
			bufp += OUTPUT(bufp, ebufp-bufp,
				       "REMOTE=%s ", FSUSERDIR);
		OUTPUT(bufp, ebufp-bufp, "LOCAL=%s\n", USERDIR);
		client_writeback(sock, buf, strlen(buf), tcp);
		/* Leave this logging on all the time for now. */
		info("MOUNTS: %s", buf);

#ifdef FSSCRATCHDIR
		bufp = buf;
		if (!nomounts)
			bufp += OUTPUT(bufp, ebufp-bufp,
				       "REMOTE=%s ", FSSCRATCHDIR);
		OUTPUT(bufp, ebufp-bufp, "LOCAL=%s\n", SCRATCHDIR);
		client_writeback(sock, buf, strlen(buf), tcp);
		/* Leave this logging on all the time for now. */
		info("MOUNTS: %s", buf);
#endif
#ifdef FSSHAREDIR
		bufp = buf;
		if (!nomounts)
			bufp += OUTPUT(bufp, ebufp-bufp,
				       "REMOTE=%s ", FSSHAREDIR);
		OUTPUT(bufp, ebufp-bufp, "LOCAL=%s\n", SHAREDIR);
		client_writeback(sock, buf, strlen(buf), tcp);
		/* Leave this logging on all the time for now. */
		info("MOUNTS: %s", buf);
#endif
		bufp = buf;
		if (!nomounts)
			bufp += OUTPUT(bufp, ebufp-bufp,
				       "REMOTE=%s ", FSPROJDIR);
		OUTPUT(bufp, ebufp-bufp, "LOCAL=%s\n", PROJDIR);
		client_writeback(sock, buf, strlen(buf), tcp);
		/* Leave this logging on all the time for now. */
		info("MOUNTS: %s", buf);

		bufp = buf;
		if (!nomounts)
			bufp += OUTPUT(bufp, ebufp-bufp,
				       "REMOTE=%s ", FSGROUPDIR);
		OUTPUT(bufp, ebufp-bufp, "LOCAL=%s\n", GROUPDIR);
		client_writeback(sock, buf, strlen(buf), tcp);
		/* Leave this logging on all the time for now. */
		info("MOUNTS: %s", buf);
		return 0;
	}
	else if (!usesfs) {
		/*
		 * Return project mount first.
		 */
		bufp = buf;
		if (!nomounts)
			bufp += OUTPUT(bufp, ebufp-bufp,
				       "REMOTE=%s/%s ", FSPROJDIR, reqp->pid);
		OUTPUT(bufp, ebufp-bufp, "LOCAL=%s/%s\n",
		       PROJDIR, reqp->pid);
		client_writeback(sock, buf, strlen(buf), tcp);
		/* Leave this logging on all the time for now. */
		info("MOUNTS: %s", buf);

		/*
		 * If pid!=gid, then this is group experiment, and we return
		 * a mount for the group directory too.
		 */
		if (strcmp(reqp->pid, reqp->gid)) {
			bufp = buf;
			if (!nomounts)
				bufp += OUTPUT(bufp, ebufp-bufp,
					       "REMOTE=%s/%s/%s ", FSGROUPDIR,
					       reqp->pid, reqp->gid);
			OUTPUT(bufp, ebufp-bufp, "LOCAL=%s/%s/%s\n",
			       GROUPDIR, reqp->pid, reqp->gid);
			client_writeback(sock, buf, strlen(buf), tcp);
			/* Leave this logging on all the time for now. */
			info("MOUNTS: %s", buf);
		}

		/*
		 * Skip all this for a vnode; client does not ask.
		 */
		if (reqp->isvnode)
			goto dousers;

#ifdef FSSCRATCHDIR
		/*
		 * Return scratch mount if its defined.
		 */
		bufp = buf;
		if (!nomounts)
			bufp += OUTPUT(bufp, ebufp-bufp,
				       "REMOTE=%s/%s ",
				       FSSCRATCHDIR, reqp->pid);
		OUTPUT(bufp, ebufp-bufp, "LOCAL=%s/%s\n",
		       SCRATCHDIR, reqp->pid);
		client_writeback(sock, buf, strlen(buf), tcp);
		/* Leave this logging on all the time for now. */
		info("MOUNTS: %s", buf);
#endif
#ifdef FSSHAREDIR
		/*
		 * Return share mount if its defined.
		 */
		bufp = buf;
		if (!nomounts)
			bufp += OUTPUT(bufp, ebufp-bufp,
				       "REMOTE=%s ", FSSHAREDIR);
		OUTPUT(bufp, ebufp-bufp, "LOCAL=%s\n", SHAREDIR);
		client_writeback(sock, buf, strlen(buf), tcp);
		/* Leave this logging on all the time for now. */
		info("MOUNTS: %s", buf);
#endif
	}
	else if (usesfs) {
		/*
		 * Return SFS-based mounts. Locally, we send back per
		 * project/group mounts (really symlinks) cause thats the
		 * local convention. For remote nodes, no point. Just send
		 * back mounts for the top level directories.
		 */
		if (reqp->islocal) {
			OUTPUT(buf, sizeof(buf),
			       "SFS REMOTE=%s%s/%s LOCAL=%s/%s\n",
			       fshostid, FSDIR_PROJ, reqp->pid,
			       PROJDIR, reqp->pid);
			client_writeback(sock, buf, strlen(buf), tcp);
			if (verbose)
				info("MOUNTS: %s", buf);

			/*
			 * Return SFS-based group mount.
			 */
			if (strcmp(reqp->pid, reqp->gid)) {
				OUTPUT(buf, sizeof(buf),
				       "SFS REMOTE=%s%s/%s/%s LOCAL=%s/%s/%s\n",
				       fshostid,
				       FSDIR_GROUPS, reqp->pid, reqp->gid,
				       GROUPDIR, reqp->pid, reqp->gid);
				client_writeback(sock, buf, strlen(buf), tcp);
				info("MOUNTS: %s", buf);
			}
#ifdef FSSCRATCHDIR
			/*
			 * Pointer to per-project scratch directory.
			 */
			OUTPUT(buf, sizeof(buf),
			       "SFS REMOTE=%s%s/%s LOCAL=%s/%s\n",
			       fshostid, FSDIR_SCRATCH, reqp->pid,
			       SCRATCHDIR, reqp->pid);
			client_writeback(sock, buf, strlen(buf), tcp);
			if (verbose)
				info("MOUNTS: %s", buf);
#endif
#ifdef FSSHAREDIR
			/*
			 * Pointer to /share.
			 */
			OUTPUT(buf, sizeof(buf), "SFS REMOTE=%s%s LOCAL=%s\n",
				fshostid, FSDIR_SHARE, SHAREDIR);
			client_writeback(sock, buf, strlen(buf), tcp);
			if (verbose)
				info("MOUNTS: %s", buf);
#endif
			/*
			 * Return a mount for "certprog dirsearch"
			 * that matches the local convention. This
			 * allows the same paths to work on remote
			 * nodes.
			 */
			OUTPUT(buf, sizeof(buf),
			       "SFS REMOTE=%s%s/%s LOCAL=%s%s\n",
			       fshostid, FSDIR_PROJ, DOTSFS, PROJDIR, DOTSFS);
			client_writeback(sock, buf, strlen(buf), tcp);
		}
		else {
			/*
			 * Remote nodes get slightly different mounts.
			 * in /netbed.
			 *
			 * Pointer to /proj.
			 */
			OUTPUT(buf, sizeof(buf),
			       "SFS REMOTE=%s%s LOCAL=%s/%s\n",
			       fshostid, FSDIR_PROJ, NETBEDDIR, PROJDIR);
			client_writeback(sock, buf, strlen(buf), tcp);
			if (verbose)
				info("MOUNTS: %s", buf);

			/*
			 * Pointer to /groups
			 */
			OUTPUT(buf, sizeof(buf),
			       "SFS REMOTE=%s%s LOCAL=%s%s\n",
			       fshostid, FSDIR_GROUPS, NETBEDDIR, GROUPDIR);
			client_writeback(sock, buf, strlen(buf), tcp);
			if (verbose)
				info("MOUNTS: %s", buf);

			/*
			 * Pointer to /users
			 */
			OUTPUT(buf, sizeof(buf),
			       "SFS REMOTE=%s%s LOCAL=%s%s\n",
			       fshostid, FSDIR_USERS, NETBEDDIR, USERDIR);
			client_writeback(sock, buf, strlen(buf), tcp);
			if (verbose)
				info("MOUNTS: %s", buf);
#ifdef FSSCRATCHDIR
			/*
			 * Pointer to per-project scratch directory.
			 */
			OUTPUT(buf, sizeof(buf),
			       "SFS REMOTE=%s%s LOCAL=%s/%s\n",
			       fshostid, FSDIR_SCRATCH, NETBEDDIR, SCRATCHDIR);
			client_writeback(sock, buf, strlen(buf), tcp);
			if (verbose)
				info("MOUNTS: %s", buf);
#endif
#ifdef FSSHAREDIR
			/*
			 * Pointer to /share.
			 */
			OUTPUT(buf, sizeof(buf),
			       "SFS REMOTE=%s%s LOCAL=%s%s\n",
			       fshostid, FSDIR_SHARE, NETBEDDIR, SHAREDIR);
			client_writeback(sock, buf, strlen(buf), tcp);
			if (verbose)
				info("MOUNTS: %s", buf);
#endif
		}
	}
 dousers:
	/*
	 * Remote nodes do not get per-user mounts.
	 * ProtoGeni nodes do not get them either.
	 */
	if (!reqp->islocal || reqp->genisliver_idx)
	        return 0;

	/*
	 * Now a list of user directories. These include the members of the
	 * experiments projects if its a regular experimental node.
	 */
	res = mydb_query("select u.uid,u.admin from users as u "
			 "left join group_membership as p on "
			 "     p.uid_idx=u.uid_idx "
			 "where p.pid='%s' and p.gid='%s' and "
			 "      u.status='active' and "
			 "      u.webonly=0 and "
			 "      p.trust!='none'",
			 2, reqp->pid, reqp->gid);
	if (!res) {
		error("MOUNTS: %s: DB Error getting users!\n", reqp->pid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		error("MOUNTS: %s: No Users!\n", reqp->pid);
		mysql_free_result(res);
		return 0;
	}

	while (nrows--) {
		row = mysql_fetch_row(res);
#ifdef ISOLATEADMINS
		isadmin = atoi(row[1]);
		if (isadmin != reqp->swapper_isadmin) {
			continue;
		}
#endif
		bufp = buf;
		if (!nomounts)
			bufp += OUTPUT(bufp, ebufp-bufp, "REMOTE=%s/%s ",
				       FSUSERDIR, row[0]);
		OUTPUT(bufp, ebufp-bufp, "LOCAL=%s/%s\n", USERDIR, row[0]);
		client_writeback(sock, buf, strlen(buf), tcp);

		if (verbose)
		    info("MOUNTS: %s", buf);
	}
	mysql_free_result(res);

	return 0;
}

/*
 * Used by dosfshostid to make sure NFS doesn't give us problems.
 * (This code really unnerves me)
 */
int sfshostiddeadfl;
jmp_buf sfshostiddeadline;
static void
dosfshostiddead()
{
	sfshostiddeadfl = 1;
	longjmp(sfshostiddeadline, 1);
}

static int
safesymlink(char *name1, char *name2)
{
	/*
	 * Really, there should be a cleaner way of doing this, but
	 * this works, at least for now.  Perhaps using the DB and a
	 * symlinking deamon alone would be better.
	 */
	if (setjmp(sfshostiddeadline) == 0) {
		sfshostiddeadfl = 0;
		signal(SIGALRM, dosfshostiddead);
		alarm(1);

		unlink(name2);
		if (symlink(name1, name2) < 0) {
			sfshostiddeadfl = 1;
		}
	}
	alarm(0);
	if (sfshostiddeadfl) {
		errorc("symlinking %s to %s", name2, name1);
		return -1;
	}
	return 0;
}

/*
 * Create dirsearch entry for node.
 */
COMMAND_PROTOTYPE(dosfshostid)
{
	char	nodehostid[HOSTID_SIZE], buf[BUFSIZ];
	char	sfspath[BUFSIZ], dspath[BUFSIZ];

	if (!strlen(fshostid)) {
		/* SFS not being used */
		info("dosfshostid: Called while SFS is not in use\n");
		return 0;
	}

	/*
	 * Dig out the hostid. Need to be careful about not overflowing
	 * the buffer.
	 */
	sprintf(buf, "%%%ds", (int)sizeof(nodehostid));
	if (sscanf(rdata, buf, nodehostid) != 1) {
		error("dosfshostid: No hostid reported!\n");
		return 1;
	}

	/*
	 * No slashes allowed! This path is going into a symlink below.
	 */
	if (index(nodehostid, '/')) {
		error("dosfshostid: %s Invalid hostid: %s!\n",
		      reqp->nodeid, nodehostid);
		return 1;
	}

	/*
	 * Create symlink names
	 */
	OUTPUT(sfspath, sizeof(sfspath), "/sfs/%s", nodehostid);
	OUTPUT(dspath, sizeof(dspath), "%s/%s/%s.%s.%s", PROJDIR, DOTSFS,
	       reqp->nickname, reqp->eid, reqp->pid);

	/*
	 * Create the symlink. The directory in which this is done has to be
	 * either owned by the same uid used to run tmcd, or in the same group.
	 */
	if (safesymlink(sfspath, dspath) < 0) {
		return 1;
	}

	/*
	 * Stash into the DB too.
	 */
	mysql_escape_string(buf, nodehostid, strlen(nodehostid));

	if (mydb_update("update node_hostkeys set sfshostid='%s' "
			"where node_id='%s'", buf, reqp->nodeid)) {
		error("SFSHOSTID: %s: DB Error setting sfshostid!\n",
		      reqp->nodeid);
		return 1;
	}
	if (verbose)
		info("SFSHOSTID: %s: %s\n", reqp->nodeid, nodehostid);
	return 0;
}

/*
 * Return routing stuff.
 */
COMMAND_PROTOTYPE(dorouting)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
	int		n, nrows, isstatic = 0;

	/*
	 * Get the routing type from the nodes table.
	 */
	res = mydb_query("select routertype from nodes where node_id='%s'",
			 1, reqp->nodeid);

	if (!res) {
		error("ROUTES: %s: DB Error getting router type!\n",
		      reqp->nodeid);
		return 1;
	}

	if ((int)mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return 0;
	}

	/*
	 * Return type. At some point we might have to return a list of
	 * routes too, if we support static routes specified by the user
	 * in the NS file.
	 */
	row = mysql_fetch_row(res);
	if (! row[0] || !row[0][0]) {
		mysql_free_result(res);
		return 0;
	}
	if (!strcmp(row[0], "static")) {
		isstatic = 1;
	}
	OUTPUT(buf, sizeof(buf), "ROUTERTYPE=%s\n", row[0]);
	mysql_free_result(res);

	client_writeback(sock, buf, strlen(buf), tcp);
	if (verbose)
		info("ROUTES: %s", buf);

	/*
	 * New images treat "static" as "static-ddijk", so even if there
	 * are routes in the DB, we do not want to return them to the node
	 * since that would be a waste of bandwidth.
	 */
	if (vers >= 19 && isstatic) {
		return 0;
	}

	/*
	 * Get the routing type from the nodes table.
	 */
	res = mydb_query("select dst,dst_type,dst_mask,nexthop,cost,src "
			 "from virt_routes as vi "
			 "where vi.vname='%s' and "
			 " vi.pid='%s' and vi.eid='%s'",
			 6, reqp->nickname, reqp->pid, reqp->eid);

	if (!res) {
		error("ROUTES: %s: DB Error getting manual routes!\n",
		      reqp->nodeid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}
	n = nrows;

	while (n) {
		char dstip[32];
		char *bufp = buf, *ebufp = &buf[sizeof(buf)];

		row = mysql_fetch_row(res);

		/*
		 * OMG, the Linux route command is too stupid to accept a
		 * host-on-a-subnet as the subnet address, so we gotta mask
		 * off the bits manually for network routes.
		 *
		 * Eventually we'll perform this operation in the NS parser
		 * so it appears in the DB correctly.
		 */
		if (strcmp(row[1], "net") == 0) {
			struct in_addr tip, tmask;

			inet_aton(row[0], &tip);
			inet_aton(row[2], &tmask);
			tip.s_addr &= tmask.s_addr;
			strncpy(dstip, inet_ntoa(tip), sizeof(dstip));
		} else
			strncpy(dstip, row[0], sizeof(dstip));

		bufp += OUTPUT(bufp, ebufp - bufp,
			       "ROUTE DEST=%s DESTTYPE=%s DESTMASK=%s "
			       "NEXTHOP=%s COST=%s",
			       dstip, row[1], row[2], row[3], row[4]);

		if (vers >= 12) {
			bufp += OUTPUT(bufp, ebufp - bufp, " SRC=%s", row[5]);
		}
		OUTPUT(bufp, ebufp - bufp, "\n");
		client_writeback(sock, buf, strlen(buf), tcp);

		n--;
	}
	mysql_free_result(res);
	if (verbose)
	    info("ROUTES: %d routes in list\n", nrows);

	return 0;
}

/*
 * Return address from which to load an image, along with the partition that
 * it should be written to and the OS type in that partition.
 */
COMMAND_PROTOTYPE(doloadinfo)
{
	MYSQL_RES	*res, *res2;
	MYSQL_ROW	row, row2;
	char		buf[MYBUFSIZE];
	char		*bufp = buf, *ebufp = &buf[sizeof(buf)];
	char		*disktype, *useacpi, *useasf, *noclflush;
	char		address[MYBUFSIZE];
	char            server_address[MYBUFSIZE];
	char		mbrvers[51];
	char            *loadpart, *OS, *prepare;
	int		disknum, nrows, zfill;

	/*
	 * Get the address the node should contact to load its image
	 */
	res = mydb_query("select loadpart,OS,mustwipe,mbr_version,access_key,"
			 "   i.imageid,prepare,i.imagename,p.pid,g.gid,i.path "
			 "from current_reloads as r "
			 "left join images as i on i.imageid=r.image_id "
			 "left join frisbee_blobs as f on f.imageid=i.imageid "
			 "left join os_info as o on i.default_osid=o.osid "
			 "left join projects as p on i.pid_idx=p.pid_idx "
			 "left join groups as g on i.gid_idx=g.gid_idx "
			 "where node_id='%s' order by r.idx",
			 11, reqp->nodeid);

	if (!res) {
		error("doloadinfo: %s: DB Error getting loading address!\n",
		       reqp->nodeid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}

	if (nrows > 1 && vers <= 29) {
	updatemfs:
		bufp += OUTPUT(bufp, ebufp - bufp,
			       "ADDR=/NEWER-MFS-NEEDED PART=0 PARTOS=Bogus\n");

		error("doloadinfo: %s: Old MFS Version found, need version 33\n",
		      reqp->nodeid);

#ifdef EVENTSYS
		address_tuple_t tuple;
		/*
		 * Send the state out via an event
		 */
		/* XXX: Maybe we don't need to alloc a new tuple every time through */
		tuple = address_tuple_alloc();
		if (tuple == NULL) {
			error("doreset: Unable to allocate address tuple!\n");
			return 1;
		}

		tuple->host      = BOSSNODE;
		tuple->objtype   = "TBNODESTATE";
		tuple->objname	 = reqp->nodeid;
		tuple->eventtype = "RELOADOLDMFS";

		if (myevent_send(tuple)) {
			error("doloadinfo: %s: "
			      "Unable to set state to RELOADOLDMFS",
			      reqp->nodeid);
		}

		address_tuple_free(tuple);
#endif
	}
	else while (nrows) {

		row = mysql_fetch_row(res);
		loadpart = row[0];
		OS = row[1];
		prepare = row[6];

		res2 = mydb_query("select IP "
				  " from interfaces as i, subbosses as s "
				  " where i.node_id=s.subboss_id and "
				  " i.role='ctrl' and "
				  " s.node_id='%s' and s.service='frisbee'",
				  1, reqp->isvnode ? reqp->pnodeid : reqp->nodeid);
		if (!res2) {
			error("doloadinfo: %s: DB Error getting subboss info!\n",
			       reqp->nodeid);
			mysql_free_result(res);
			return 1;
		}

		if (mysql_num_rows(res2)) {
			row2 = mysql_fetch_row(res2);
			strcpy(server_address, row2[0]);
		} else {
			strcpy(server_address, BOSSNODE_IP);
		}
		mysql_free_result(res2);

		/*
		 * Remote nodes get a URL for the address.
		 */
		if (!reqp->islocal) {
			if (!row[4] || !row[4][0]) {
				error("doloadinfo: %s: "
				      "No access key associated with imageid %s\n",
				      reqp->nodeid, row[5]);
				mysql_free_result(res);
				return 1;
			}
			OUTPUT(address, sizeof(address),
			       "%s/spewimage.php?imageid=%s&access_key=%s",
			       TBBASE, row[5], row[4]);
			
			server_address[0] = 0;
		}
		/*
		 * As of version 33, local nodes no longer get an address,
		 * they contact the frisbee master server instead.
		 *
		 * We provide temporary backward compat by firing off
		 * a proxy request to the master server. If that doesn't
		 * work, we draw attention to ourselves via the update MFS
		 * path above.
		 */
		else if (vers >= 33) {
			address[0] = '\0';
		} else {
			char _buf[512];
			int gotit = 0;
			FILE *cfd;

			info("%s LOADINFO compat: starting server for imageid %s",
			     reqp->nodeid, row[5]);
			/*
			 * XXX for vnodes we use the pnode name since the
			 * master server wants to validate a node_id by
			 * looking up its control net IP address in the
			 * interfaces table.  Vnodes have no interfaces
			 * table entries so that won't work.
			 */
			snprintf(_buf, sizeof _buf,
				 "%s/sbin/frisbeehelper -n %s %s",
				 TBROOT,
				 reqp->isvnode ? reqp->pnodeid : reqp->nodeid,
				 row[5]);
			if ((cfd = popen(_buf, "r")) == NULL)
				goto updatemfs;
			while (fgets(_buf, sizeof _buf, cfd) != NULL) {
#if 0
				if (debug > 1)
					info("got: '%s'\n", _buf);
#endif
				if (strncmp(_buf, "Address is ", 11) == 0) {
					gotit = 1;
					break;
				}
			}
			pclose(cfd);
			if (!gotit ||
			    sscanf(_buf, "Address is %32s", address) != 1)
				goto updatemfs;

			/* XXX address info is for boss, not a subboss */
			strcpy(server_address, BOSSNODE_IP);
		}

		bufp += OUTPUT(bufp, ebufp - bufp,
			       "ADDR=%s PART=%s PARTOS=%s", address, loadpart, OS);

		if (server_address[0] && (vers >= 31)) {
			bufp += OUTPUT(bufp, ebufp - bufp,
			               " SERVER=%s", server_address);
		}

		/*
		 * Add zero-fill free space, MBR version fields, and access_key
		 */
		zfill = 0;
		if (row[2] && row[2][0])
			zfill = atoi(row[2]);
		strcpy(mbrvers, "1");
		if (row[3] && row[3][0])
			strcpy(mbrvers, row[3]);

		/*
		 * Get disk type and number
		 */
		disktype = DISKTYPE;
		disknum = DISKNUM;
		useacpi = "unknown";
		useasf = "unknown";
		noclflush = "unknown";

		res2 = mydb_query("select a.attrkey,a.attrvalue,na.attrvalue "
				  "from nodes as n "
				  "left join node_type_attributes as a on "
				  "     n.type=a.type "
				  "left join node_attributes as na on "
				  "     a.attrkey=na.attrkey and "
				  "     na.node_id=n.node_id "
				  "where (a.attrkey='bootdisk_unit' or "
				  "       a.attrkey='disktype' or "
				  "       a.attrkey='use_acpi' or "
				  "       a.attrkey='use_asf' or "
				  "       a.attrkey='no_clflush') and "
				  "      n.node_id='%s'", 3, reqp->nodeid);

		if (!res2) {
			error("doloadinfo: %s: DB Error getting disktype!\n",
			      reqp->nodeid);
			return 1;
		}

		if ((int)mysql_num_rows(res2) > 0) {
			int nrows2 = (int)mysql_num_rows(res2);

			while (nrows2) {
				char *attrstr;

				row2 = mysql_fetch_row(res2);

				/* node_attribute overrides node_type_attribute */
				if (row2[2] && row2[2][0])
					attrstr = row2[2];
				else if (row2[1] && row2[1][0])
					attrstr = row2[1];
				else
					attrstr = NULL;

				if (attrstr) {
					if (strcmp(row2[0], "bootdisk_unit") == 0) {
						disknum = atoi(attrstr);
					}
					else if (strcmp(row2[0], "disktype") == 0) {
						disktype = attrstr;
					}
					else if (strcmp(row2[0], "use_acpi") == 0) {
						useacpi = attrstr;
					}
					else if (strcmp(row2[0], "use_asf") == 0) {
						useasf = attrstr;
					}
					else if (strcmp(row2[0], "no_clflush") == 0) {
						noclflush = attrstr;
					}
				}
				nrows2--;
			}
		}

		mysql_free_result(res2);

		bufp += OUTPUT(bufp, ebufp - bufp,
			       " DISK=%s%d ZFILL=%d ACPI=%s MBRVERS=%s ASF=%s PREPARE=%s NOCLFLUSH=%s",
			       disktype, disknum, zfill, useacpi, mbrvers, useasf, prepare, noclflush);

		/*
		 * Vnodes (and post v32 local nodes) get additional image
		 * metadata fields so that they can uniquely identify the
		 * image.
		 */
		if (reqp->isvnode || (reqp->islocal && vers >= 33)) {
			struct stat sb;

 			if (!row[7] || !row[7][0]) {
				error("doloadinfo: %s: No imagename"
				      " associated with imageid %s\n",
				      reqp->nodeid, row[5]);
				mysql_free_result(res);
				return 1;
			}
			if (!row[8] || !row[8][0]) {
				error("doloadinfo: %s: No pid"
				      " associated with imageid %s\n",
				      reqp->nodeid, row[5]);
				mysql_free_result(res);
				return 1;
			}
			if (!row[9] || !row[9][0]) {
				error("doloadinfo: %s: No gid"
				      " associated with imageid %s\n",
				      reqp->nodeid, row[5]);
				mysql_free_result(res);
				return 1;
			}

			bufp += OUTPUT(bufp, ebufp - bufp, " IMAGEID=%s,%s,%s",
				       row[8], row[9], row[7]);

			/*
			 * Vnodes also get a time stamp for the imagepath.
			 * This is not strictly necessary since the master
			 * frisbee server will return this info, but vnodes
			 * use this to create a file name before calling
			 * the server.
			 */
			if (reqp->isvnode) {
				if (!row[10] || !row[10][0]) {
					error("doloadinfo: %s: No path"
					      " associated with imageid %s\n",
					      reqp->nodeid, row[5]);
					mysql_free_result(res);
					return 1;
				}
				else if (stat(row[10], &sb)) {
					error("doloadinfo: %s: Could not stat path %s"
					      " associated with imageid %s: %s\n",
					      reqp->nodeid, row[10], row[5],
					      strerror(errno));
					mysql_free_result(res);
					return 1;
				}
				bufp += OUTPUT(bufp, ebufp - bufp,
					       " IMAGEMTIME=%u\n",
					       sb.st_mtime);
			}

		}

		/* Tack on the newline, finally */
		bufp += OUTPUT(bufp, ebufp - bufp, "\n");

		nrows--;
	}

	if (res)
		mysql_free_result(res);

	client_writeback(sock, buf, strlen(buf), tcp);
	if (verbose)
		info("doloadinfo: %s", buf);

	return 0;
}

/*
 * Have stated reset any next_pxe_boot_* and next_boot_* fields.
 * Produces no output to the client.
 */
COMMAND_PROTOTYPE(doreset)
{
#ifdef EVENTSYS
	address_tuple_t tuple;
	/*
	 * Send the state out via an event
	 */
	/* XXX: Maybe we don't need to alloc a new tuple every time through */
	tuple = address_tuple_alloc();
	if (tuple == NULL) {
		error("doreset: Unable to allocate address tuple!\n");
		return 1;
	}

	tuple->host      = BOSSNODE;
	tuple->objtype   = TBDB_OBJECTTYPE_TESTBED; /* "TBCONTROL" */
	tuple->objname	 = reqp->nodeid;
	tuple->eventtype = TBDB_EVENTTYPE_RESET;

	if (myevent_send(tuple)) {
		error("doreset: Error sending event\n");
		return 1;
	} else {
	        info("Reset event sent for %s\n", reqp->nodeid);
	}

	address_tuple_free(tuple);
#else
	info("No event system - no reset performed.\n");
#endif
	return 0;
}

/*
 * Return trafgens info
 */
COMMAND_PROTOTYPE(dotrafgens)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
	int		nrows;

	res = mydb_query("select vi.vname,role,proto,"
			 "  vnode,port,ip,target_vnode,target_port,target_ip, "
			 "  generator "
			 " from virt_trafgens as vi "
			 "where vi.vnode='%s' and "
			 " vi.pid='%s' and vi.eid='%s'",
			 10, reqp->nickname, reqp->pid, reqp->eid);

	if (!res) {
		error("TRAFGENS: %s: DB Error getting virt_trafgens\n",
		      reqp->nodeid);
		return 1;
	}
	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}

	while (nrows) {
		char myname[TBDB_FLEN_VNAME+2];
		char peername[TBDB_FLEN_VNAME+2];

		row = mysql_fetch_row(res);

		if (row[5] && row[5][0]) {
			strcpy(myname, row[5]);
			strcpy(peername, row[8]);
		}
		else {
			/* This can go away once the table is purged */
			strcpy(myname, row[3]);
			strcat(myname, "-0");
			strcpy(peername, row[6]);
			strcat(peername, "-0");
		}

		OUTPUT(buf, sizeof(buf),
		        "TRAFGEN=%s MYNAME=%s MYPORT=%s "
			"PEERNAME=%s PEERPORT=%s "
			"PROTO=%s ROLE=%s GENERATOR=%s\n",
			row[0], myname, row[4], peername, row[7],
			row[2], row[1], row[9]);

		client_writeback(sock, buf, strlen(buf), tcp);

		nrows--;
		if (verbose)
			info("TRAFGENS: %s", buf);
	}
	mysql_free_result(res);
	return 0;
}

/*
 * Return nseconfigs info
 */
COMMAND_PROTOTYPE(donseconfigs)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	int		nrows;

	if (!tcp) {
		error("NSECONFIGS: %s: Cannot do UDP mode!\n", reqp->nodeid);
		return 1;
	}

	res = mydb_query("select nseconfig from nseconfigs as nse "
			 "where nse.pid='%s' and nse.eid='%s' "
			 "and nse.vname='%s'",
			 1, reqp->pid, reqp->eid, reqp->nickname);

	if (!res) {
		error("NSECONFIGS: %s: DB Error getting nseconfigs\n",
		      reqp->nodeid);
		return 1;
	}
	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}

	row = mysql_fetch_row(res);

	/*
	 * Just shove the whole thing out.
	 */
	if (row[0] && row[0][0]) {
		client_writeback(sock, row[0], strlen(row[0]), tcp);
	}
	mysql_free_result(res);
	return 0;
}

/*
 * Report that the node has entered a new state
 */
COMMAND_PROTOTYPE(dostate)
{
	char 		newstate[128];	/* More then we will ever need */
	MYSQL_RES	*res;
	int		nrows;
	int		i;
#ifdef EVENTSYS
	address_tuple_t tuple;
#endif

#ifdef LBS
	return 0;
#endif

	/*
	 * Dig out state that the node is reporting
	 */
	if (sscanf(rdata, "%128s", newstate) != 1 ||
	    strlen(newstate) == sizeof(newstate)) {
		error("DOSTATE: %s: Bad arguments\n", reqp->nodeid);
		return 1;
	}

        /*
         * Check to make sure that this is not a state that must be reported
         * by the securestate mechanism - we can tell because there are one
         * or more PCR values required in the tpm_quote_values table for
         * the state.
         */
	res = mydb_query("select q.pcr from nodes as n "
			"left join tpm_quote_values as q "
                        "on n.op_mode = q.op_mode "
			"where n.node_id='%s' and q.state ='%s'",
			1, reqp->nodeid,newstate);
	if (!res){
		error("state: %s: DB error check for pcr list\n",
			reqp->nodeid);
		return 1;
	}

	nrows = mysql_num_rows(res);

        mysql_free_result(res);

	if (nrows){
            error("state: %s: tried to go into secure state %s using "
                    "insecure state command\n",reqp->nodeid,newstate);
            return 1;
            // XXX Probably should send a SECVIOLATION state and/or send
            // mail, but this needs more thought before making it the
            // default action.
        }

	/*
	 * Sanity check. No special or weird chars.
	 */
	for (i = 0; i < strlen(newstate); i++) {
		if (! (isalnum(newstate[i]) ||
		       newstate[i] == '_' || newstate[i] == '-')) {
			error("DOSTATE: %s: Bad state name\n", reqp->nodeid);
			return 1;
		}
	}

#ifdef EVENTSYS
	/*
	 * Send the state out via an event
	 */
	/* XXX: Maybe we don't need to alloc a new tuple every time through */
	tuple = address_tuple_alloc();
	if (tuple == NULL) {
		error("dostate: Unable to allocate address tuple!\n");
		return 1;
	}

	tuple->host      = BOSSNODE;
	tuple->objtype   = "TBNODESTATE";
	tuple->objname	 = reqp->nodeid;
	tuple->eventtype = newstate;

	if (myevent_send(tuple)) {
		error("dostate: Error sending event\n");
		return 1;
	}

	address_tuple_free(tuple);
#endif /* EVENTSYS */

	/* Leave this logging on all the time for now. */
	info("%s: STATE: %s\n", reqp->nodeid, newstate);
	return 0;

}

/* There are probably classic functions available to do this but I couldn't
 * find one that will convert two bytes of ACII to one byte.  sscanf writes a
 * full int and atoi gives us an int too
 */
static unsigned char hextochar(char *in)
{
	unsigned char lh, rh;

	lh = in[0];
	if (lh >= '0' && lh <= '9')
		lh = lh - '0';
	else if (lh >= 'A' && lh <= 'F')
		lh = lh - 'A' + 10;
	else if (lh >= 'a' && lh <= 'f')
		lh = lh - 'a' + 10;

	rh = in[1];
	if (rh >= '0' && rh <= '9')
		rh = rh - '0';
	else if (rh >= 'A' && rh <= 'F')
		rh = rh - 'A' + 10;
	else if (rh >= 'a' && rh <= 'f')
		rh = rh - 'a' + 10;

	return (lh << 4) | rh;
}

static int ishex(char in)
{
	return ((in >= 'a' && in <= 'f') || (in >= 'A' && in <= 'F') ||
	    (in >= '0' && in <= '9'));
}

/*
 * Report that the node has entered a new state - secure version: the report
 * includes a TPM quote that will be checked against the database.
 * If this check fails, we report a SECVIOLATION event instead, and tell the
 * client so.
 * TODO: Should probably reduce code duplication from dostate()
 */
COMMAND_PROTOTYPE(dosecurestate)
{
	char 		newstate[128];	/* More then we will ever need */
        char            quote[1024];
        char            pcomp[256];
        unsigned char   quote_bin[256];
        unsigned char   pcomp_bin[128];
	ssize_t		pcomplen, quotelen;
        int             quote_passed;
        char            result[16];
	ETPM_NONCE	nonce;

	MYSQL_RES	*res;
	MYSQL_ROW	row;
	int		nrows;
        unsigned long   *nlen;

        int             i,j;
	unsigned int	temp;

        unsigned short  wantpcrs;
        TPM_PCR         *pcrs;

#ifdef EVENTSYS
	address_tuple_t tuple;
#endif

	/*
	 * Dig out state that the node is reporting and the quote
	 */
	if (rdata == NULL ||
	    sscanf(rdata, "%127s %1023s %255s", newstate, quote, pcomp) != 3 ||
	    strlen(newstate) + 1 == sizeof(newstate) ||
	    strlen(quote) + 1 == sizeof(quote) ||
	    strlen(pcomp) + 1 == sizeof(pcomp)) {
		error("SECURESTATE: %s: Bad arguments\n", reqp->nodeid);
		return 1;
	}

	/*
	 * Have to covert the hex representations of quote and pcomp into
	 * simple binary.
	 */
        if ((strlen(quote) % 2) != 0) {
            error("SECURESTATE: %s: Malformed quote: odd length\n");
            return 1;
        }
        quotelen = strlen(quote)/2;
        printf("quotelen is %d\n",quotelen);
        for (i = 0; i < quotelen; i++) {
		if (!ishex(quote[i * 2]) || !ishex(quote[i * 2 + 1])) {
			error("Error parsing quote\n");
			// XXX: Send error to client
			return 1;
		}
		quote_bin[i] = hextochar(&quote[i * 2]);
        }

        if ((strlen(pcomp) % 2) != 0) {
            error("SECURESTATE: %s: Malformed pcomp: odd length\n");
            return 1;
        }
        pcomplen = strlen(pcomp)/2;
        for (i = 0; i < pcomplen; i++) {
		if (!ishex(pcomp[i * 2]) || !ishex(pcomp[i * 2 + 1])) {
			error("Error parsing pcomp\n");
			// XXX: Send error to client
			return 1;
		}
		pcomp_bin[i] = hextochar(&pcomp[i * 2]);
        }

        /*
         * Pull the nonce out, verify the exipration date, and clear it so that
         * it can't be used again.
         */
	res = mydb_query("select nonce, (expires >= UNIX_TIMESTAMP()) "
			"from nonces "
			"where node_id='%s' and purpose='state-%s'",
			2, reqp->nodeid,newstate);
	if (!res){
		error("SECURESTATE: %s: DB error getting nonce\n",
			reqp->nodeid);
		return 1;
	}

	nrows = mysql_num_rows(res);

	if (!nrows){
		error("%s: no nonce in database for this node.\n",
			reqp->nodeid);
		mysql_free_result(res);
                // XXX: return error to client
		return 1;
	}

        // Delete from the database so that it can't be used again
	mydb_update("delete from nonces where node_id='%s' and "
                "purpose='state-%s' ", reqp->nodeid,newstate);


        row = mysql_fetch_row(res);
	nlen = mysql_fetch_lengths(res);
        // XXX: Check to make sure the expire check is working
        if (strcmp(row[1],"1") != 0) {
            error("SECURESTATE: %s: Nonce is expired\n");
            mysql_free_result(res);
            // XXX: return error to client
            return 1;
        }

        // Have to covert the hex representation in the database back into
        // simple binary
        if (nlen[0] != TPM_NONCE_BYTES * 2) {
            error("SECURESTATE: %s: Nonce length is incorrect (%d)",
                    reqp->nodeid, nlen[0]);
        }
        for (i = 0; i < TPM_NONCE_BYTES; i++) {
            if (sscanf(row[0] + (i*2),"%2x", &temp) != 1) {
                error("SECURESTATE: %s: Error parsing nonce\n", reqp->nodeid);
                mysql_free_result(res);
                // XXX: return error to client
                return 1;
            }

	    nonce[i] = (unsigned char)temp;
        }

        mysql_free_result(res);

        /*
         * Make a list of the PCR values we need to have verified
         */
	res = mydb_query("select q.pcr,q.value from nodes as n "
			 "left join tpm_quote_values as q "
			 "on (n.op_mode = q.op_mode or q.op_mode='*') "
			 "and n.node_id = q.node_id "
			 "where n.node_id='%s' and q.state ='%s' "
			 "order by q.pcr",
			 2, reqp->nodeid, newstate);
	if (!res) {
		error("SECURESTATE: %s: DB error getting pcr list\n",
			reqp->nodeid);
		return 1;
	}

	nrows = mysql_num_rows(res);

	if (!nrows){
		error("%s: no TPM quote values in database for state %s\n",
			reqp->nodeid, newstate);
		mysql_free_result(res);
		return 1;
	}

        wantpcrs = 0;
        pcrs = malloc(nrows*sizeof(TPM_PCR));
        for (i = 0; i < nrows; i++) {
            int pcr;

            row = mysql_fetch_row(res);
            // XXX: Check for nonsensical values for the pcr index
            // XXX: Check for nlen...
            // XXX: Check for proper PCR size
            pcr = atoi(row[0]);
            wantpcrs |= (1 << pcr);
            for (j = 0; j < TPM_PCR_BYTES; j++) {
                if (sscanf(row[1] + (j*2),"%2x", &temp) != 1) {
                    error("SECURESTATE: %s: Error parsing PCR\n", reqp->nodeid);
                    free(pcrs);
                    mysql_free_result(res);
                    // XXX: return error to client
                    return 1;
                }
		pcrs[i][j] = (unsigned char)temp;
            }

        }

        mysql_free_result(res);

        /*
         * Get the identity key for vertification purposes
         */
	res = mydb_query("select tpmidentity "
			"from node_hostkeys "
			"where node_id='%s' ",
			1, reqp->nodeid);

	if (!res){
		error("securestate: %s: DB error getting tpmidentity\n",
			reqp->nodeid);
                free(pcrs);
		return 1;
	}

	nrows = mysql_num_rows(res);

	if (!nrows){
		error("%s: no tpmidentity in database for this node.\n",
			reqp->nodeid);
                free(pcrs);
		mysql_free_result(res);
		return 1;
	}

	row = mysql_fetch_row(res);
	nlen = mysql_fetch_lengths(res);
	if (!nlen || !nlen[0]){
		error("%s: invalid identity length.\n",
			reqp->nodeid);
                free(pcrs);
		mysql_free_result(res);
		return 1;
	}

        // NOTE: Do *not* free the mysql result until *after* the call to
        // verify, as we're passing the identiy key directly from the SQL
        // result.

        /*
         * Parse and check the quote
         *
	 * quote and pcomp both come from the client's TPM - they are both
	 * returned from the quote operation.  We must dig up our nonce again.
         */
        quote_passed = tmcd_tpm_verify_quote(quote_bin, quotelen, pcomp_bin,
                pcomplen, nonce, wantpcrs, pcrs, (unsigned char *)row[0]);

	mysql_free_result(res);
        free(pcrs);

	/* The state reported below depends on whether the quote checked. */
	if (!quote_passed)
		strcpy(newstate, "SECVIOLATION");

#ifdef EVENTSYS
	/*
	 * Send the state out via an event
	 */
	/* XXX: Maybe we don't need to alloc a new tuple every time through */
	tuple = address_tuple_alloc();
	if (tuple == NULL) {
		error("dostate: Unable to allocate address tuple!\n");
		return 1;
	}

        // TODO: It might be nice to mark in the event that it was verified
        // securely, but the connection to the event server is secure, and
        // we'll refuse the insecure state command for secure states.
	tuple->host      = BOSSNODE;
	tuple->objtype   = "TBNODESTATE";
	tuple->objname	 = reqp->nodeid;
	tuple->eventtype = newstate;

	if (myevent_send(tuple)) {
		error("dostate: Error sending event\n");
		return 1;
	}

	address_tuple_free(tuple);
#endif /* EVENTSYS */

        /*
         * Let the client know whether the quote checks out or not. Note that
         * we do this *after* sending the event so that a malicious client
         * can't stall or prevent the event notification by trying to hold up
         * the TCP connection.
         * Probably a slightly simpler way to do this, but want to stick with
         * the common idioms in this file.
         */
        if (quote_passed) {
            OUTPUT(result, sizeof(result), "OK");
        } else { 
            OUTPUT(result, sizeof(result), "FAILED");
        }
	client_writeback(sock, result, strlen(result), tcp);


	/* Leave this logging on all the time for now. */
	info("%s: SECURESTATE: %s\n", reqp->nodeid, newstate);
	return 0;

}

/*
 * Prepare for a TPM quote: give the client the encrypted identity key,
 * a nonce to use in the quote, and the set of PCRs that need to be included in
 * the quote. This saves some state (the nonce) that will be checked again in
 * dosecurestate().
 */
COMMAND_PROTOTYPE(doquoteprep)
{
	char            newstate[128];	/* More then we will ever need */
        ETPM_NONCE       nonce;
        char            nonce_hex[2*TPM_NONCE_BYTES + 1];
        int             i;

        // XXX: is MYBUFSIZE big enough?
	char		buf[MYBUFSIZE];
	char		*bufp = buf;
	char		*bufe = &buf[MYBUFSIZE];

	MYSQL_RES	*res;
	MYSQL_ROW	row;
	int		nrows;
        unsigned long   *nlen;

	/*
	 * Dig out state that the node is reporting - we need this so that we
         * can tell it what PCRs to include
	 */
	if (rdata == NULL || sscanf(rdata, "%128s", newstate) != 1 ||
	    strlen(newstate) == sizeof(newstate)) {
		error("DOQUOTEPREP: %s: Bad arguments\n", reqp->nodeid);
		return 1;
	}

        /*
         * Get the set of PCRs that have to be quoted to move into this state.
         */
	res = mydb_query("select q.pcr from nodes as n "
			 "left join tpm_quote_values as q "
			 "on (n.op_mode = q.op_mode or q.op_mode='*') "
			 "and n.node_id = q.node_id "
			 "where n.node_id='%s' and q.state ='%s' "
			 "order by q.pcr",
			 1, reqp->nodeid, newstate);
	if (!res){
		error("quoteprep: %s: DB error getting pcr list\n",
			reqp->nodeid);
		return 1;
	}

	nrows = mysql_num_rows(res);

	if (!nrows){
		error("%s: no TPM quote values in database for state %s\n",
			reqp->nodeid,newstate);
		mysql_free_result(res);
		return 1;
	}

	bufp += OUTPUT(bufp, bufe - bufp, "PCR=");

        for (i = 0; i < nrows; i++) {
            row = mysql_fetch_row(res);
            // XXX: Is this already passed to us as a string?
            bufp += OUTPUT(bufp, bufe - bufp,"%s",row[0]);
            if (i < (nrows - 1)) {
                    bufp += OUTPUT(bufp, bufe - bufp, ",");
            }
        }

        bufp += OUTPUT(bufp, bufe - bufp, " ");
        mysql_free_result(res);

        /*
         * Grab the (encrypted) identity key for the node - noone else will be
         * able to decrypt it, so we don't have to be too paranoid about who
         * we give it to.
         */
	res = mydb_query("select tpmidentity "
			"from node_hostkeys "
			"where node_id='%s' ",
			1, reqp->nodeid);

	if (!res){
		error("quoteprep: %s: DB error getting tpmidentity\n",
			reqp->nodeid);
		return 1;
	}

	nrows = mysql_num_rows(res);

	if (!nrows){
		error("%s: no tpmidentity in database for this node.\n",
			reqp->nodeid);
		mysql_free_result(res);
		return 1;
	}

	row = mysql_fetch_row(res);
	nlen = mysql_fetch_lengths(res);
	if (!nlen || !nlen[0]){
		error("%s: invalid identity length.\n",
			reqp->nodeid);
		mysql_free_result(res);
		return 1;
	}

	bufp += OUTPUT(bufp, bufe - bufp, "IDENTITY=");
        for (i = 0;i < nlen[0];++i)
                bufp += OUTPUT(bufp, bufe - bufp,
                        "%.02x", (0xff & ((char)*(row[0]+i))));

        bufp += OUTPUT(bufp, bufe - bufp, " ");
	mysql_free_result(res);
        
        /*
         * Generate a cryptographic nonce - we have to keep track of this to
         * prevent replay attacks.
         */
        if (tmcd_tpm_generate_nonce(nonce)) {
            error("DOQUOTEPREP: %s: Failed to generate nonce\n", reqp->nodeid);
            return 1;
        }

        // Make a hex representation of the nonce
        for (i = 0; i < TPM_NONCE_BYTES; i++) {
            sprintf(nonce_hex + (i*2),"%.02x",nonce[i]);
        }
        nonce_hex[TPM_NONCE_BYTES*2] = '\0';
        // XXX
        info("NONCE: %s\n", nonce_hex);

        // Store the nonce in the database. It expires in one minute, and we
        // overwrite any existing nonces for this node/state combo
	mydb_update("replace into nonces "
		    " (node_id, purpose, nonce, expires) "
		    " values ('%s', 'state-%s','%s', UNIX_TIMESTAMP()+60)",
		    reqp->nodeid,newstate,nonce_hex);

	bufp += OUTPUT(bufp, bufe - bufp, "NONCE=%s",nonce_hex);

	bufp += OUTPUT(bufp, bufe - bufp, "\n");

        /*
         * Return to the client
         */
	client_writeback(sock, buf, bufp - buf, tcp);

        return 0;
}

/*
 * Get the decryption key for the image a node is suposed to be loading
 */
COMMAND_PROTOTYPE(doimagekey)
{
	char		buf[MYBUFSIZE];
	char		*bufp = buf;
	char		*bufe = &buf[MYBUFSIZE];

	MYSQL_RES	*res;
	MYSQL_ROW	row;
	int		nrows;
        unsigned long   *nlen;

        /*
	 * No arguments - we don't allow the client to ask for a specific image
         * key, just the one for the image they are supposed to be loading
         * according to the database
         */
        
        /*
         * Make sure that this node is in the right state - hardcoding it is
         * probably not a good idea, but the right way to get it isn't clear
         */
        res = mydb_query("select op_mode,eventstate from nodes where "
			 "node_id='%s'", 2, reqp->nodeid);
	if (!res) {
		error("IMAGEKEY: %s: DB Error getting event state\n",
		      reqp->nodeid);
		return 1;
	}
	if ((nrows = (int)mysql_num_rows(res)) != 1) {
		error("IMAGEKEY: %s: DB Error getting event state\n",
		      reqp->nodeid);
		mysql_free_result(res);
		return 1;
	}

	row = mysql_fetch_row(res);
	nlen = mysql_fetch_lengths(res);
	if (!nlen) {
		error("IMAGEKEY: %s: DB Error getting event state\n",
		      reqp->nodeid);
		mysql_free_result(res);
                return 1;
        }

        if (strncmp(row[0],SECURELOAD_OPMODE, nlen[0]) ||
            strncmp(row[1],SECURELOAD_STATE, nlen[1])) {
		error("IMAGEKEY: %s: Node is in the wrong state\n",
		      reqp->nodeid);
		mysql_free_result(res);
                return 1;
        }
        mysql_free_result(res);

        /*
         * Grab and return the key itself
         */
	res = mydb_query("select i.auth_uuid,i.auth_key,i.decryption_key,"
			 " i.imagename,p.pid,g.gid "
			 "from current_reloads as r "
			 "left join images as i on i.imageid=r.image_id "
			 "left join projects as p on i.pid_idx=p.pid_idx "
			 "left join groups as g on i.gid_idx=g.gid_idx "
			 "where node_id='%s' order by r.idx",
			 6, reqp->nodeid);
	if (!res) {
		error("IMAGEKEY: %s: DB Error getting key\n", reqp->nodeid);
		return 1;
	}
	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		info("IMAGEKEY: %s: No current reload for this node\n",
		     reqp->nodeid);
		mysql_free_result(res);
		return 0;
	}

	/*
	 * Prior to version 33 there was no extended image info send along
	 * in loadinfo and thus no way to match an image with the
	 * corresponding secure load info. Hence we allow only a single
	 * image in this case and return it in the old format of one field
	 * per line.
	 */
	if (vers < 33) {
		if (nrows > 1) {
			info("IMAGEKEY: %s: client cannot handle multiple images\n",
			     reqp->nodeid);
			mysql_free_result(res);
			return 0;
		}
		row = mysql_fetch_row(res);
		if (row[0])
			bufp += OUTPUT(bufp, bufe - bufp,
				       "UUID=%s\n", row[0]);
		if (row[1])
			bufp += OUTPUT(bufp, bufe - bufp,
				       "SIGKEY=%s\n", row[1]);
		if (row[2])
			bufp += OUTPUT(bufp, bufe - bufp,
				       "ENCKEY=%s\n", row[2]);
		nrows = 0;
	}
	/*
	 * Note: if there is more than one reload, we are only grabbing the
	 * 'most recent' due to the 'order by' clause.
	 */
	while (nrows) {
		row = mysql_fetch_row(res);
		nlen = mysql_fetch_lengths(res);
		if (!row || !nlen) {
			error("IMAGEKEY: %s: no auth/encryption key info\n",
			      reqp->nodeid);
			mysql_free_result(res);
			return 1;
		}
        
		if (!row[3] || !row[3][0] || !row[4] || !row[4][0] ||
		    !row[5] || !row[5][0]) {
			error("IMAGEKEY: %s: missing or incomplete imageinfo\n",
			      reqp->nodeid);
			mysql_free_result(res);
			return 1;
		}

		bufp += OUTPUT(bufp, bufe - bufp,
			       "IMAGEID=%s,%s,%s", row[4], row[5], row[3]);
		if (row[0])
			bufp += OUTPUT(bufp, bufe - bufp,
				       " UUID=%s", row[0]);
		if (row[1])
			bufp += OUTPUT(bufp, bufe - bufp,
				       " SIGKEY=%s", row[1]);
		if (row[2])
			bufp += OUTPUT(bufp, bufe - bufp,
				       " ENCKEY=%s", row[2]);
		bufp += OUTPUT(bufp, bufe - bufp, "\n");

		nrows--;
	}
	client_writeback(sock, buf, strlen(buf), tcp);

        mysql_free_result(res);
        return 0;
}

/*
 * Return creator of experiment. Total hack. Must kill this.
 */
COMMAND_PROTOTYPE(docreator)
{
	char		buf[MYBUFSIZE];

	/* There was a $ anchored CREATOR= pattern in common/config/rc.misc . */
	if (vers<=20)
		OUTPUT(buf, sizeof(buf), "CREATOR=%s\n", reqp->creator);
	else
		OUTPUT(buf, sizeof(buf), "CREATOR=%s SWAPPER=%s\n",
		       reqp->creator, reqp->swapper);

	client_writeback(sock, buf, strlen(buf), tcp);
	if (verbose)
		info("CREATOR: %s", buf);
	return 0;
}

/*
 * Return tunnels info.
 */
COMMAND_PROTOTYPE(dotunnels)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
	int		nrows;

	res = mydb_query("select lma.lanid,lma.memberid,"
			 "   lma.attrkey,lma.attrvalue from lans as l "
			 "left join lan_members as lm on lm.lanid=l.lanid "
			 "left join lan_member_attributes as lma on "
			 "     lma.lanid=lm.lanid and "
			 "     lma.memberid=lm.memberid "
			 "where l.exptidx='%d' and l.type='tunnel' and "
			 "      lm.node_id='%s' and "
			 "      lma.attrkey like 'tunnel_%%'",
			 4, reqp->exptidx, reqp->nodeid);

	if (!res) {
		error("TUNNELS: %s: DB Error getting tunnels\n", reqp->nodeid);
		return 1;
	}
	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}

	while (nrows) {
		row = mysql_fetch_row(res);

		OUTPUT(buf, sizeof(buf),
		       "TUNNEL=%s MEMBER=%s KEY='%s' VALUE='%s'\n",
		       row[0], row[1], row[2], row[3]);
		client_writeback(sock, buf, strlen(buf), tcp);

		nrows--;
		if (verbose)
			info("TUNNEL=%s MEMBER=%s KEY='%s' VALUE='%s'\n",
			     row[0], row[1], row[2], row[3]);
	}
	mysql_free_result(res);
	return 0;
}

/*
 * Return vnode list for a widearea node.
 */
COMMAND_PROTOTYPE(dovnodelist)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
	int		nrows;

	res = mydb_query("select r.node_id,n.jailflag from reserved as r "
			 "left join nodes as n on r.node_id=n.node_id "
                         "left join node_types as nt on nt.type=n.type "
                         "where nt.isvirtnode=1 and n.phys_nodeid='%s'",
                         2, reqp->nodeid);

	if (!res) {
		error("VNODELIST: %s: DB Error getting vnode list\n",
		      reqp->nodeid);
		return 1;
	}
	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}

	while (nrows) {
		row = mysql_fetch_row(res);

		if (vers <= 6) {
			OUTPUT(buf, sizeof(buf), "%s\n", row[0]);
		}
		else {
			/* XXX Plab? */
			OUTPUT(buf, sizeof(buf),
			       "VNODEID=%s JAILED=%s\n", row[0], row[1]);
		}
		client_writeback(sock, buf, strlen(buf), tcp);

		nrows--;
		if (verbose)
			info("VNODELIST: %s", buf);
	}
	mysql_free_result(res);
	return 0;
}

/*
 * Return subnode list, and their types.
 */
COMMAND_PROTOTYPE(dosubnodelist)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
	int		nrows;

	if (vers <= 23)
		return 0;

	res = mydb_query("select n.node_id,nt.class from nodes as n "
                         "left join node_types as nt on nt.type=n.type "
                         "where nt.issubnode=1 and n.phys_nodeid='%s'",
                         2, reqp->nodeid);

	if (!res) {
		error("SUBNODELIST: %s: DB Error getting vnode list\n",
		      reqp->nodeid);
		return 1;
	}
	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}

	while (nrows) {
		row = mysql_fetch_row(res);

		OUTPUT(buf, sizeof(buf), "NODEID=%s TYPE=%s\n", row[0], row[1]);
		client_writeback(sock, buf, strlen(buf), tcp);
		nrows--;
		if (verbose)
			info("SUBNODELIST: %s", buf);
	}
	mysql_free_result(res);
	return 0;
}

/*
 * DB stuff
 */
static MYSQL	db;
static int	db_connected;
static char     db_dbname[DBNAME_SIZE];
static void	mydb_disconnect();

static int
mydb_connect()
{
	/*
	 * Each time we talk to the DB, we check to see if the name
	 * matches the last connection. If so, nothing more needs to
	 * be done. If we switched DBs (checkdbredirect()), then drop
	 * the current connection and form a new one.
	 */
	if (db_connected) {
		if (strcmp(db_dbname, dbname) == 0)
			return 1;
		mydb_disconnect();
	}

	mysql_init(&db);
	if (mysql_real_connect(&db, 0, "tmcd", 0,
			       dbname, 0, 0, CLIENT_INTERACTIVE) == 0) {
		error("%s: connect failed: %s\n", dbname, mysql_error(&db));
		return 0;
	}
	strcpy(db_dbname, dbname);
	db_connected = 1;
	return 1;
}

static void
mydb_disconnect()
{
	mysql_close(&db);
	db_connected = 0;
}

/*
 * Just so we can include bootinfo from the pxe directory.
 */
int	dbinit(void) { return 0;}
void	dbclose(void) {}

MYSQL_RES *
mydb_query(char *query, int ncols, ...)
{
	MYSQL_RES	*res;
	char		querybuf[2*MYBUFSIZE];
	va_list		ap;
	int		n;

	va_start(ap, ncols);
	n = vsnprintf(querybuf, sizeof(querybuf), query, ap);
	va_end(ap);
	if (n >= sizeof(querybuf)) {
		error("query too long for buffer\n");
		return (MYSQL_RES *) 0;
	}

	if (! mydb_connect())
		return (MYSQL_RES *) 0;

	if (mysql_real_query(&db, querybuf, n) != 0) {
		error("%s: query failed: %s, retrying\n",
		      dbname, mysql_error(&db));
		mydb_disconnect();
		/*
		 * Try once to reconnect.  In theory, the caller (client)
		 * will retry the tmcc call and we will reconnect and
		 * everything will be fine.  The problem is that the
		 * client may get a different tmcd process each time,
		 * and every one of those will fail once before
		 * reconnecting.  Hence, the client could wind up failing
		 * even if it retried.
		 */
		if (!mydb_connect() ||
		    mysql_real_query(&db, querybuf, n) != 0) {
			error("%s: query failed: %s\n",
			      dbname, mysql_error(&db));
			return (MYSQL_RES *) 0;
		}
	}

	res = mysql_store_result(&db);
	if (res == 0) {
		error("%s: store_result failed: %s\n",
		      dbname, mysql_error(&db));
		mydb_disconnect();
		return (MYSQL_RES *) 0;
	}

	if (ncols && ncols != (int)mysql_num_fields(res)) {
		error("%s: Wrong number of fields returned "
		      "Wanted %d, Got %d\n",
		      dbname, ncols, (int)mysql_num_fields(res));
		mysql_free_result(res);
		return (MYSQL_RES *) 0;
	}
	return res;
}

int
mydb_update(char *query, ...)
{
	char		querybuf[8 * 1024];
	va_list		ap;
	int		n;

	va_start(ap, query);
	n = vsnprintf(querybuf, sizeof(querybuf), query, ap);
	va_end(ap);
	if (n >= sizeof(querybuf)) {
		error("query too long for buffer\n");
		return 1;
	}

	if (! mydb_connect())
		return 1;

	if (mysql_real_query(&db, querybuf, n) != 0) {
		error("%s: query failed: %s\n", dbname, mysql_error(&db));
		mydb_disconnect();
		return 1;
	}
	return 0;
}

/*
 * Map IP to node ID (plus other info).
 */
static int
iptonodeid(struct in_addr ipaddr, tmcdreq_t *reqp, char* nodekey)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;

	/*
	 * I love a good query!
	 *
	 * The join on node_types using control_iface is to prevent the
	 * (unlikely) possibility that we get an experimental interface
	 * trying to contact us! I doubt that can happen though.
	 *
	 * XXX Locally, the jail flag is not set on the phys node, only
	 * on the virtnodes. This is okay since all the routines that
	 * check jailflag also check to see if its a vnode or physnode.
	 */
	/*
	 * Widearea nodes have wanodekeys that should be used to get
	 * the nodeid.
	 */
	if ((nodekey != NULL) && (strlen(nodekey) > 1)) {
		res = mydb_query("SELECT t.class,t.type,n.node_id,"
				 " n.jailflag,r.pid,r.eid,r.vname, "
				 " e.gid,e.testdb,n.update_accounts, "
				 " n.role,e.expt_head_uid,e.expt_swap_uid, "
				 " e.sync_server,t.class,t.type, "
				 " t.isremotenode,t.issubnode,e.keyhash, "
				 " nk.sfshostid,e.eventkey,0, "
				 " 0, "
				 " e.elab_in_elab,e.elabinelab_singlenet, "
				 " e.idx,e.creator_idx,e.swapper_idx, "
				 " u.admin,dedicated_wa_types.attrvalue "
				 "   AS isdedicated_wa, "
				 " r.genisliver_idx,r.tmcd_redirect, "
				 " r.sharing_mode,e.geniflags "
				 "FROM nodes AS n "
				 "LEFT JOIN reserved AS r ON "
				 "  r.node_id=n.node_id "
				 "LEFT JOIN experiments AS e ON "
				 " e.pid=r.pid and e.eid=r.eid "
				 "LEFT JOIN node_types AS t ON "
				 " t.type=n.type "
				 "LEFT JOIN node_hostkeys AS nk ON "
				 " nk.node_id=n.node_id "
				 "LEFT JOIN users AS u ON "
				 " u.uid_idx=e.swapper_idx "
				 "LEFT OUTER JOIN "
				 "  (SELECT type,attrvalue "
				 "    FROM node_type_attributes "
				 "    WHERE attrkey='notmcdinfo' "
				 "      AND attrvalue='1' "
				 "     GROUP BY type) AS notmcdinfo_types "
				 "  ON n.type=notmcdinfo_types.type "
				 "LEFT OUTER JOIN "
				 "  (SELECT type,attrvalue "
				 "   FROM node_type_attributes "
				 "   WHERE attrkey='dedicated_widearea' "
				 "   GROUP BY type) AS dedicated_wa_types "
				 "  ON n.type=dedicated_wa_types.type "
				 "WHERE n.node_id IN "
				 "     (SELECT node_id FROM widearea_nodeinfo "
				 "      WHERE privkey='%s') "
				 "  AND notmcdinfo_types.attrvalue IS NULL",
				 34, nodekey);
	}
	else if (reqp->isvnode) {
		char	clause[BUFSIZ];

		if (reqp->external_key[0]) {
			sprintf(clause,
				"r.external_resource_key is not null and "
				"r.external_resource_key='%s'",
				reqp->external_key);
		}
		else {
			sprintf(clause,
				"(i.IP='%s' and i.role='ctrl') or "
				"nv.jailip='%s'",
				inet_ntoa(ipaddr), inet_ntoa(ipaddr));
		}
		res = mydb_query("select vt.class,vt.type,np.node_id,"
				 " nv.jailflag,r.pid,r.eid,r.vname, "
				 " e.gid,e.testdb,nv.update_accounts, "
				 " np.role,e.expt_head_uid,e.expt_swap_uid, "
				 " e.sync_server,pt.class,pt.type, "
				 " pt.isremotenode,vt.issubnode,e.keyhash, "
				 " nk.sfshostid,e.eventkey,vt.isplabdslice, "
				 " ps.admin, "
				 " e.elab_in_elab,e.elabinelab_singlenet, "
				 " e.idx,e.creator_idx,e.swapper_idx, "
				 " u.admin,null, "
				 " r.genisliver_idx,r.tmcd_redirect, "
				 " r.sharing_mode,e.geniflags "
				 "from nodes as nv "
				 "left join nodes as np on "
				 " np.node_id=nv.phys_nodeid "
				 "left join interfaces as i on "
				 " i.node_id=np.node_id "
				 "left join reserved as r on "
				 " r.node_id=nv.node_id "
				 "left join experiments as e on "
				 "  e.pid=r.pid and e.eid=r.eid "
				 "left join node_types as pt on "
				 " pt.type=np.type "
				 "left join node_types as vt on "
				 " vt.type=nv.type "
				 "left join plab_slices as ps on "
				 " ps.pid=e.pid and ps.eid=e.eid "
				 "left join node_hostkeys as nk on "
				 " nk.node_id=nv.node_id "
				 "left join users as u on "
				 " u.uid_idx=e.swapper_idx "
				 "where nv.node_id='%s' and (%s)",
				 34, reqp->vnodeid, clause);
	}
	else {
		char	clause[BUFSIZ];

		if (reqp->external_key[0]) {
			sprintf(clause,
				"r.external_resource_key is not null and "
				"r.external_resource_key='%s'",
				reqp->external_key);
		}
		else {
			sprintf(clause,
				"i.IP='%s' and i.role='ctrl'",
				inet_ntoa(ipaddr));
		}
		res = mydb_query("select t.class,t.type,n.node_id,n.jailflag,"
				 " r.pid,r.eid,r.vname,e.gid,e.testdb, "
				 " n.update_accounts,n.role, "
				 " e.expt_head_uid,e.expt_swap_uid, "
				 " e.sync_server,t.class,t.type, "
				 " t.isremotenode,t.issubnode,e.keyhash, "
				 " nk.sfshostid,e.eventkey,0, "
				 " 0,e.elab_in_elab,e.elabinelab_singlenet, "
				 " e.idx,e.creator_idx,e.swapper_idx, "
				 " u.admin,dedicated_wa_types.attrvalue "
				 "   as isdedicated_wa, "
				 " r.genisliver_idx,r.tmcd_redirect, "
				 " r.sharing_mode,e.geniflags "
				 "from interfaces as i "
				 "left join nodes as n on n.node_id=i.node_id "
				 "left join reserved as r on "
				 "  r.node_id=i.node_id "
				 "left join experiments as e on "
				 " e.pid=r.pid and e.eid=r.eid "
				 "left join node_types as t on "
				 " t.type=n.type "
				 "left join node_hostkeys as nk on "
				 " nk.node_id=n.node_id "
				 "left join users as u on "
				 " u.uid_idx=e.swapper_idx "
				 "left outer join "
				 "  (select type,attrvalue "
				 "    from node_type_attributes "
				 "    where attrkey='notmcdinfo' "
				 "      and attrvalue='1' "
				 "     group by type) as notmcdinfo_types "
				 "  on n.type=notmcdinfo_types.type "
				 "left outer join "
				 "  (select type,attrvalue "
				 "   from node_type_attributes "
				 "   where attrkey='dedicated_widearea' "
				 "   group by type) as dedicated_wa_types "
				 "  on n.type=dedicated_wa_types.type "
				 "where (%s) "
				 "  and notmcdinfo_types.attrvalue is NULL",
				 34, clause);
	}

	if (!res) {
		error("iptonodeid: %s: DB Error getting interfaces!\n",
		      inet_ntoa(ipaddr));
		return 1;
	}

	if (! (int)mysql_num_rows(res)) {
		mysql_free_result(res);
		return 1;
	}
	row = mysql_fetch_row(res);
	if (!row[0] || !row[1] || !row[2]) {
		error("iptonodeid: %s: Malformed DB response!\n",
		      inet_ntoa(ipaddr));
		mysql_free_result(res);
		return 1;
	}
	reqp->client = ipaddr;
	strncpy(reqp->class,  row[0],  sizeof(reqp->class));
	strncpy(reqp->type,   row[1],  sizeof(reqp->type));
	strncpy(reqp->pclass, row[14], sizeof(reqp->pclass));
	strncpy(reqp->ptype,  row[15], sizeof(reqp->ptype));
	strncpy(reqp->nodeid, row[2],  sizeof(reqp->nodeid));
	if(nodekey != NULL) {
		strncpy(reqp->privkey, nodekey, PRIVKEY_LEN);
	}
	else {
		strcpy(reqp->privkey, "");
	}
	reqp->islocal      = (! strcasecmp(row[16], "0") ? 1 : 0);
	reqp->jailflag     = (! strcasecmp(row[3],  "0") ? 0 : 1);
	reqp->issubnode    = (! strcasecmp(row[17], "0") ? 0 : 1);
	reqp->isplabdslice = (! strcasecmp(row[21], "0") ? 0 : 1);
	reqp->isplabsvc    = (row[22] && strcasecmp(row[22], "0")) ? 1 : 0;
	reqp->elab_in_elab = (row[23] && strcasecmp(row[23], "0")) ? 1 : 0;
	reqp->singlenet    = (row[24] && strcasecmp(row[24], "0")) ? 1 : 0;
	reqp->isdedicatedwa = (row[29] && !strncmp(row[29], "1", 1)) ? 1 : 0;
	reqp->geniflags    = 0;

	if (row[8])
		strncpy(reqp->testdb, row[8], sizeof(reqp->testdb));
	if (row[4] && row[5]) {
		strncpy(reqp->pid, row[4], sizeof(reqp->pid));
		strncpy(reqp->eid, row[5], sizeof(reqp->eid));
		if (row[25])
			reqp->exptidx = atoi(row[25]);
		else {
			error("iptonodeid: %s: in non-existent experiment %s/%s!\n",
			      inet_ntoa(ipaddr), reqp->pid, reqp->eid);
			mysql_free_result(res);
			return 1;
		}
		reqp->allocated = 1;

		if (row[6])
			strncpy(reqp->nickname, row[6],sizeof(reqp->nickname));
		else
			strcpy(reqp->nickname, reqp->nodeid);

		strcpy(reqp->creator, row[11]);
		reqp->creator_idx = atoi(row[26]);
		if (row[12]) {
			strcpy(reqp->swapper, row[12]);
			reqp->swapper_idx = atoi(row[27]);
		}
		else {
			strcpy(reqp->swapper, reqp->creator);
			reqp->swapper_idx = reqp->creator_idx;
		}
		if (row[28])
			reqp->swapper_isadmin = atoi(row[28]);
		else
			reqp->swapper_isadmin = 0;

		/*
		 * If there is no gid (yes, thats bad and a mistake), then
		 * copy the pid in. Also warn.
		 */
		if (row[7])
			strncpy(reqp->gid, row[7], sizeof(reqp->gid));
		else {
			strcpy(reqp->gid, reqp->pid);
			error("iptonodeid: %s: No GID for %s/%s (pid/eid)!\n",
			      reqp->nodeid, reqp->pid, reqp->eid);
		}
		/* Sync server for the experiment */
		if (row[13])
			strcpy(reqp->syncserver, row[13]);
		/* keyhash for the experiment */
		if (row[18])
			strcpy(reqp->keyhash, row[18]);
		/* event key for the experiment */
		if (row[20])
			strcpy(reqp->eventkey, row[20]);
		/* geni sliver idx */
		if (row[30])
			reqp->genisliver_idx = atoi(row[30]);
		else
			reqp->genisliver_idx = 0;
		if (row[32])
			strcpy(reqp->sharing_mode, row[32]);
		/* geni flags idx */
		if (row[33])
			reqp->geniflags = atoi(row[33]);
		else
			reqp->geniflags = 0;
	}

	if (row[9])
		reqp->update_accounts = atoi(row[9]);
	else
		reqp->update_accounts = 0;

	/* SFS hostid for the node */
	if (row[19])
		strcpy(reqp->sfshostid, row[19]);

	reqp->iscontrol = (! strcasecmp(row[10], "ctrlnode") ? 1 : 0);

	/* If a vnode, copy into the nodeid. Eventually split this properly */
	strcpy(reqp->pnodeid, reqp->nodeid);
	if (reqp->isvnode) {
		strcpy(reqp->nodeid,  reqp->vnodeid);
	}

	mysql_free_result(res);
	return 0;
}

/*
 * Map nodeid to PID/EID pair.
 */
static int
nodeidtoexp(char *nodeid, char *pid, char *eid, char *gid)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;

	res = mydb_query("select r.pid,r.eid,e.gid from reserved as r "
			 "left join experiments as e on "
			 "     e.pid=r.pid and e.eid=r.eid "
			 "where node_id='%s'",
			 3, nodeid);
	if (!res) {
		error("nodeidtoexp: %s: DB Error getting reserved!\n", nodeid);
		return 1;
	}

	if (! (int)mysql_num_rows(res)) {
		mysql_free_result(res);
		return 1;
	}
	row = mysql_fetch_row(res);
	mysql_free_result(res);
	strncpy(pid, row[0], TBDB_FLEN_PID);
	strncpy(eid, row[1], TBDB_FLEN_EID);

	/*
	 * If there is no gid (yes, thats bad and a mistake), then copy
	 * the pid in. Also warn.
	 */
	if (row[2]) {
		strncpy(gid, row[2], TBDB_FLEN_GID);
	}
	else {
		strcpy(gid, pid);
		error("nodeidtoexp: %s: No GID for %s/%s (pid/eid)!\n",
		      nodeid, pid, eid);
	}

	return 0;
}

/*
 * Check for DBname redirection.
 */
static int
checkdbredirect(tmcdreq_t *reqp)
{
	if (! reqp->allocated || !strlen(reqp->testdb))
		return 0;

	/* This changes the DB we talk to. */
	strcpy(dbname, reqp->testdb);

	/*
	 * Okay, lets test to make sure that DB exists. If not, fall back
	 * on the main DB.
	 */
	if (nodeidtoexp(reqp->nodeid, reqp->pid, reqp->eid, reqp->gid)) {
		error("CHECKDBREDIRECT: %s: %s DB does not exist\n",
		      reqp->nodeid, dbname);
		strcpy(dbname, DEFAULT_DBNAME);
	}
	return 0;
}

#ifdef EVENTSYS
/*
 * Connect to the event system. It's not an error to call this function if
 * already connected. Returns 1 on failure, 0 on sucess.
 */
int
event_connect()
{
	if (!event_handle) {
		event_handle =
		  event_register("elvin://localhost:" BOSSEVENTPORT, 0);
	}

	if (event_handle) {
		return 0;
	} else {
		error("event_connect: "
		      "Unable to register with event system!\n");
		return 1;
	}
}

/*
 * Send an event to the event system. Automatically connects (registers)
 * if not already done. Returns 0 on sucess, 1 on failure.
 */
int myevent_send(address_tuple_t tuple) {
	event_notification_t notification;

	if (event_connect()) {
		return 1;
	}

	notification = event_notification_alloc(event_handle,tuple);
	if (notification == (event_notification_t) NULL) {
		error("myevent_send: Unable to allocate notification!");
		return 1;
	}

	if (! event_notify(event_handle, notification)) {
		event_notification_free(event_handle, notification);

		error("myevent_send: Unable to send notification!");
		/*
		 * Let's try to disconnect from the event system, so that
		 * we'll reconnect next time around.
		 */
		if (!event_unregister(event_handle)) {
			error("myevent_send: "
			      "Unable to unregister with event system!");
		}
		event_handle = NULL;
		return 1;
	} else {
		event_notification_free(event_handle,notification);
		return 0;
	}
}

/*
 * This is for bootinfo inclusion.
 */
int	bievent_init(void) { return 0; }

int
bievent_send(struct in_addr ipaddr, void *opaque, char *event)
{
	tmcdreq_t		*reqp = (tmcdreq_t *) opaque;
	static address_tuple_t	tuple;

	if (!tuple) {
		tuple = address_tuple_alloc();
		if (tuple == NULL) {
			error("bievent_send: Unable to allocate tuple!\n");
			return -1;
		}
	}
	tuple->host      = BOSSNODE;
	tuple->objtype   = "TBNODESTATE";
	tuple->objname	 = reqp->nodeid;
	tuple->eventtype = event;

	if (myevent_send(tuple)) {
		error("bievent_send: Error sending event\n");
		return -1;
	}
	return 0;
}
#endif /* EVENTSYS */

/*
 * Lets hear it for global state...Yeah!
 */
static char udpbuf[8192];
static int udpfd = -1, udpix;

/*
 * Write back to client
 */
int
client_writeback(int sock, void *buf, int len, int tcp)
{
	int	cc;
	char	*bufp = (char *) buf;

	if (tcp) {
		while (len) {
			if ((cc = WRITE(sock, bufp, len)) <= 0) {
				if (cc < 0) {
					errorc("writing to TCP client");
					return -1;
				}
				error("write to TCP client aborted");
				return -1;
			}
			byteswritten += cc;
			len  -= cc;
			bufp += cc;
		}
	} else {
		if (udpfd != sock) {
			if (udpfd != -1)
				error("UDP reply in progress!?");
			udpfd = sock;
			udpix = 0;
		}
		if (udpix + len > sizeof(udpbuf)) {
			error("client data write truncated");
			len = sizeof(udpbuf) - udpix;
		}
		memcpy(&udpbuf[udpix], bufp, len);
		udpix += len;
	}
	return 0;
}

void
client_writeback_done(int sock, struct sockaddr_in *client)
{
	int err;

	/*
	 * XXX got an error before we wrote anything,
	 * still need to send a reply.
	 */
	if (udpfd == -1)
		udpfd = sock;

	if (sock != udpfd)
		error("UDP reply out of sync!");
	else if (udpix != 0) {
		err = sendto(udpfd, udpbuf, udpix, 0,
			     (struct sockaddr *)client, sizeof(*client));
		if (err < 0)
			errorc("writing to UDP client");
	}
	byteswritten = udpix;
	udpfd = -1;
	udpix = 0;
}

/*
 * IsAlive(). Mark nodes as being alive.
 */
COMMAND_PROTOTYPE(doisalive)
{
	int		doaccounts = 0;
	char		buf[MYBUFSIZE];

	/*
	 * See db/node_status script, which uses this info (timestamps)
	 * to determine when nodes are down.
	 */
	mydb_update("replace into node_status "
		    " (node_id, status, status_timestamp) "
		    " values ('%s', 'up', now())",
		    reqp->nodeid);

	/*
	 * Return info about what needs to be updated.
	 */
	if (reqp->update_accounts)
		doaccounts = 1;

	/*
	 * At some point, maybe what we will do is have the client
	 * make a request asking what needs to be updated. Right now,
	 * just return yes/no and let the client assume it knows what
	 * to do (update accounts).
	 */
	OUTPUT(buf, sizeof(buf), "UPDATE=%d\n", doaccounts);
	client_writeback(sock, buf, strlen(buf), tcp);

	return 0;
}

/*
 * Return ipod info for a node
 */
COMMAND_PROTOTYPE(doipodinfo)
{
	char		buf[MYBUFSIZE], *bp;
	unsigned char	randdata[16], hashbuf[16*2+1];
	int		fd, cc, i;

	if (!tcp) {
		error("IPODINFO: %s: Cannot do this in UDP mode!\n",
		      reqp->nodeid);
		return 1;
	}

	if ((fd = open("/dev/urandom", O_RDONLY)) < 0) {
		errorc("opening /dev/urandom");
		return 1;
	}
	if ((cc = read(fd, randdata, sizeof(randdata))) < 0) {
		errorc("reading /dev/urandom");
		close(fd);
		return 1;
	}
	if (cc != sizeof(randdata)) {
		error("Short read from /dev/urandom: %d", cc);
		close(fd);
		return 1;
	}
	close(fd);

	bp = (char *)hashbuf;
	for (i = 0; i < sizeof(randdata); i++) {
		bp += sprintf(bp, "%02x", randdata[i]);
	}
	*bp = '\0';

	mydb_update("update nodes set ipodhash='%s' "
		    "where node_id='%s'",
		    hashbuf, reqp->nodeid);

	/*
	 * XXX host/mask hardwired to us
	 */
	OUTPUT(buf, sizeof(buf), "HOST=%s MASK=255.255.255.255 HASH=%s\n",
		inet_ntoa(myipaddr), hashbuf);
	client_writeback(sock, buf, strlen(buf), tcp);

	return 0;
}

/*
 * Return ntp config for a node.
 */
COMMAND_PROTOTYPE(dontpinfo)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
	int		nrows;

	if (!tcp) {
		error("NTPINFO: %s: Cannot do this in UDP mode!\n",
		      reqp->nodeid);
		return 1;
	}

	/*
	 * First get the servers and peers.
	 */
	res = mydb_query("select type,IP from ntpinfo where node_id='%s'",
			 2, reqp->nodeid);

	if (!res) {
		error("NTPINFO: %s: DB Error getting ntpinfo!\n",
		      reqp->nodeid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res))) {
		while (nrows) {
			row = mysql_fetch_row(res);
			if (row[0] && row[0][0] &&
			    row[1] && row[1][0]) {
				if (!strcmp(row[0], "peer")) {
					OUTPUT(buf, sizeof(buf),
					       "PEER=%s\n", row[1]);
				}
				else {
					OUTPUT(buf, sizeof(buf),
					       "SERVER=%s\n", row[1]);
				}
				client_writeback(sock, buf, strlen(buf), tcp);
				if (verbose)
					info("NTPINFO: %s", buf);
			}
			nrows--;
		}
	}
	else if (reqp->islocal) {
		/*
		 * All local nodes default to a our local ntp server,
		 * which is typically a CNAME to ops.
		 */
		OUTPUT(buf, sizeof(buf), "SERVER=%s.%s\n",
		       NTPSERVER, OURDOMAIN);

		client_writeback(sock, buf, strlen(buf), tcp);
		if (verbose)
			info("NTPINFO: %s", buf);
	}
	mysql_free_result(res);

	/*
	 * Now get the drift.
	 */
	res = mydb_query("select ntpdrift from nodes "
			 "where node_id='%s' and ntpdrift is not null",
			 1, reqp->nodeid);

	if (!res) {
		error("NTPINFO: %s: DB Error getting ntpdrift!\n",
		      reqp->nodeid);
		return 1;
	}

	if ((int)mysql_num_rows(res)) {
		row = mysql_fetch_row(res);
		if (row[0] && row[0][0]) {
			OUTPUT(buf, sizeof(buf), "DRIFT=%s\n", row[0]);
			client_writeback(sock, buf, strlen(buf), tcp);
			if (verbose)
				info("NTPINFO: %s", buf);
		}
	}
	mysql_free_result(res);

	return 0;
}

/*
 * Upload the current ntp drift for a node.
 */
COMMAND_PROTOTYPE(dontpdrift)
{
	float		drift;

	if (!tcp) {
		error("NTPDRIFT: %s: Cannot do this in UDP mode!\n",
		      reqp->nodeid);
		return 1;
	}
	if (!reqp->islocal) {
		error("NTPDRIFT: %s: remote nodes not allowed!\n",
		      reqp->nodeid);
		return 1;
	}

	/*
	 * Node can be free?
	 */

	if (sscanf(rdata, "%f", &drift) != 1) {
		error("NTPDRIFT: %s: Bad argument\n", reqp->nodeid);
		return 1;
	}
	mydb_update("update nodes set ntpdrift='%f' where node_id='%s'",
		    drift, reqp->nodeid);

	if (verbose)
		info("NTPDRIFT: %f", drift);
	return 0;
}

/*
 * Return the config for a virtual (jailed) node.
 */
COMMAND_PROTOTYPE(dojailconfig)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
	char		*bufp = buf, *ebufp = &buf[sizeof(buf)];
	int		low, high;

	/*
	 * Only vnodes get a jailconfig of course, and only allocated ones.
	 */
	if (!reqp->isvnode) {
		/* Silent error is fine */
		return 0;
	}
	/*
	 * geni nodes get something completely different.
	 */
	if (reqp->genisliver_idx && reqp->jailflag == 0) {
		OUTPUT(bufp, sizeof(buf),
		       "EVENTSERVER=\"event-server.%s\"\n", OURDOMAIN);
		client_writeback(sock, buf, strlen(buf), tcp);
		return 0;
	}
	if (!reqp->jailflag)
		return 0;

	/*
	 * Get the portrange for the experiment. Cons up the other params I
	 * can think of right now.
	 */
	res = mydb_query("select low,high from ipport_ranges "
			 "where pid='%s' and eid='%s'",
			 2, reqp->pid, reqp->eid);

	if (!res) {
		error("JAILCONFIG: %s: DB Error getting config!\n",
		      reqp->nodeid);
		return 1;
	}

	if ((int)mysql_num_rows(res) == 0) {
		low  = 0;
		high = 0x0000FFFF;
	}
	else {
		row  = mysql_fetch_row(res);
		low  = atoi(row[0]);
		high = atoi(row[1]);
	}
	mysql_free_result(res);

	/*
	 * Now need the sshdport and jailip for this node.
	 */
	res = mydb_query("select sshdport,jailip from nodes "
			 "where node_id='%s'",
			 2, reqp->nodeid);

	if (!res) {
		error("JAILCONFIG: %s: DB Error getting config!\n",
		      reqp->nodeid);
		return 1;
	}

	if ((int)mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return 0;
	}
	row   = mysql_fetch_row(res);

	bzero(buf, sizeof(buf));
	if (row[1]) {
		bufp += OUTPUT(bufp, ebufp - bufp,
			       "JAILIP=\"%s,%s\"\n", row[1], JAILIPMASK);
	}
	bufp += OUTPUT(bufp, ebufp - bufp,
		       "PORTRANGE=\"%d,%d\"\n"
		       "SSHDPORT=%d\n"
		       "SYSVIPC=1\n"
		       "INETRAW=1\n"
		       "BPFRO=1\n"
		       "INADDRANY=1\n"
		       "IPFW=1\n"
		       "IPDIVERT=1\n"
		       "ROUTING=%d\n"
		       "DEVMEM=%d\n"
		       "EVENTSERVER=\"event-server.%s\"\n",
		       low, high, atoi(row[0]), reqp->islocal, reqp->islocal,
		       OURDOMAIN);

	client_writeback(sock, buf, strlen(buf), tcp);
	mysql_free_result(res);

	/*
	 * See if a per-node-type vnode disk size is specified
	 */
	res = mydb_query("select na.attrvalue from nodes as n "
			 "left join node_type_attributes as na on "
			 "  n.type=na.type "
			 "where n.node_id='%s' and "
			 "na.attrkey='virtnode_disksize'", 1, reqp->pnodeid);
	if (res) {
		if ((int)mysql_num_rows(res) != 0) {
			row = mysql_fetch_row(res);
			if (row[0]) {
				bufp = buf;
				bufp += OUTPUT(bufp, ebufp - bufp,
					       "VDSIZE=%d\n", atoi(row[0]));
			}
			client_writeback(sock, buf, strlen(buf), tcp);
		}
		mysql_free_result(res);
	}

	/*
	 * Now return the IP interface list that this jail has access to.
	 * These are tunnels or ip aliases on the real interfaces, but
	 * its easier just to consult the virt_lans table for all the
	 * ip addresses for this vnode.
	 */
	bufp  = buf;
	bufp += OUTPUT(bufp, ebufp - bufp, "IPADDRS=\"");

	res = mydb_query("select ip from virt_lans "
			 "where vnode='%s' and pid='%s' and eid='%s'",
			 1, reqp->nickname, reqp->pid, reqp->eid);

	if (!res) {
		error("JAILCONFIG: %s: DB Error getting virt_lans table\n",
		      reqp->nodeid);
		return 1;
	}
	if (mysql_num_rows(res)) {
		int nrows = mysql_num_rows(res);

		while (nrows) {
			nrows--;
			row = mysql_fetch_row(res);

			if (row[0] && row[0][0]) {
				bufp += OUTPUT(bufp, ebufp - bufp, "%s", row[0]);
				if (nrows)
					bufp += OUTPUT(bufp, ebufp - bufp, ",");

			}
		}
	}
	mysql_free_result(res);

	OUTPUT(bufp, ebufp - bufp, "\"\n");
	client_writeback(sock, buf, strlen(buf), tcp);
	return 0;
}

/*
 * Return the config for a virtual Plab node.
 */
COMMAND_PROTOTYPE(doplabconfig)
{
	MYSQL_RES	*res, *res2;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
        char            *bufp = buf, *ebufp = &buf[MYBUFSIZE];

	if (!reqp->isplabdslice) {
		/* Silent error is fine */
		return 0;
	}
	/* XXX Check for Plab-ness */

	/*
	 * Now need the sshdport for this node.
	 */
	res = mydb_query("select n.sshdport, ps.admin, i.IP "
                         " from reserved as r "
                         " left join nodes as n "
                         "  on n.node_id=r.node_id "
                         " left join interfaces as i "
                         "  on n.phys_nodeid=i.node_id "
                         " left join plab_slices as ps "
                         "  on r.pid=ps.pid and r.eid=ps.eid "
                         " where i.role='ctrl' and r.node_id='%s'",
			 3, reqp->nodeid);

        /*
         * .. And the elvind port.
         */
        res2 = mydb_query("select attrvalue from node_attributes "
                          " where node_id='%s' and attrkey='elvind_port'",
                          1, reqp->pnodeid);

	if (!res || !res2) {
		error("PLABCONFIG: %s: DB Error getting config!\n",
		      reqp->nodeid);
                if (res) {
                    mysql_free_result(res);
                }
                if (res2) {
                    mysql_free_result(res2);
                }
		return 1;
	}

        /* Add the sshd port (if any) to the output */
        if ((int)mysql_num_rows(res) > 0) {
            row = mysql_fetch_row(res);
            bufp += OUTPUT(bufp, ebufp-bufp,
                           "SSHDPORT=%d SVCSLICE=%d IPADDR=%s ",
                           atoi(row[0]), atoi(row[1]), row[2]);
        }
        mysql_free_result(res);

        /* Add the elvind port to the output */
        if ((int)mysql_num_rows(res2) > 0) {
            row = mysql_fetch_row(res2);
            bufp += OUTPUT(bufp, ebufp-bufp, "ELVIND_PORT=%d ",
                           atoi(row[0]));
        }
        else {
            /*
             * XXX: should not hardwire port number here, but what should
             *      I reference for it?
             */
             bufp += OUTPUT(bufp, ebufp-bufp, "ELVIND_PORT=%d ", 2917);
        }
        mysql_free_result(res2);

        OUTPUT(bufp, ebufp-bufp, "\n");
        client_writeback(sock, buf, strlen(buf), tcp);

	/* XXX Anything else? */

	return 0;
}

/*
 * Return the config for a subnode (this is returned to the physnode).
 */
COMMAND_PROTOTYPE(dosubconfig)
{
	if (vers <= 23)
		return 0;

	if (!reqp->issubnode) {
		error("SUBCONFIG: %s: Not a subnode\n", reqp->nodeid);
		return 1;
	}

	if (! strcmp(reqp->type, "ixp-bv"))
		return(doixpconfig(sock, reqp, rdata, tcp, vers));

	if (! strcmp(reqp->type, "mica2"))
		return(dorelayconfig(sock, reqp, rdata, tcp, vers));

	error("SUBCONFIG: %s: Invalid subnode class %s\n",
	      reqp->nodeid, reqp->class);
	return 1;
}

COMMAND_PROTOTYPE(doixpconfig)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
	struct in_addr  mask_addr, bcast_addr;
	char		bcast_ip[16];

	/*
	 * Get the "control" net address for the IXP from the interfaces
	 * table. This is really a virtual pci/eth interface.
	 */
	res = mydb_query("select i1.IP,i1.iface,i2.iface,i2.mask,i2.IP "
			 " from nodes as n "
			 "left join interfaces as i1 on i1.node_id=n.node_id "
			 "     and i1.role='ctrl' "
			 "left join interfaces as i2 on i2.node_id='%s' "
			 "     and i2.card=i1.card "
			 "where n.node_id='%s'",
			 5, reqp->pnodeid, reqp->nodeid);

	if (!res) {
		error("IXPCONFIG: %s: DB Error getting config!\n",
		      reqp->nodeid);
		return 1;
	}
	if ((int)mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return 0;
	}
	row   = mysql_fetch_row(res);
	if (!row[1]) {
		error("IXPCONFIG: %s: No IXP interface!\n", reqp->nodeid);
		return 1;
	}
	if (!row[2]) {
		error("IXPCONFIG: %s: No host interface!\n", reqp->nodeid);
		return 1;
	}
	if (!row[3]) {
		error("IXPCONFIG: %s: No mask!\n", reqp->nodeid);
		return 1;
	}
	inet_aton(CHECKMASK(row[3]), &mask_addr);
	inet_aton(row[0], &bcast_addr);

	bcast_addr.s_addr = (bcast_addr.s_addr & mask_addr.s_addr) |
		(~mask_addr.s_addr);
	strcpy(bcast_ip, inet_ntoa(bcast_addr));

	OUTPUT(buf, sizeof(buf),
	       "IXP_IP=\"%s\"\n"
	       "IXP_IFACE=\"%s\"\n"
	       "IXP_BCAST=\"%s\"\n"
	       "IXP_HOSTNAME=\"%s\"\n"
	       "HOST_IP=\"%s\"\n"
	       "HOST_IFACE=\"%s\"\n"
	       "NETMASK=\"%s\"\n",
	       row[0], row[1], bcast_ip, reqp->nickname,
	       row[4], row[2], row[3]);

	client_writeback(sock, buf, strlen(buf), tcp);
	mysql_free_result(res);
	return 0;
}

/*
 * return slothd params - just compiled in for now.
 */
COMMAND_PROTOTYPE(doslothdparams)
{
	char buf[MYBUFSIZE];

	OUTPUT(buf, sizeof(buf), "%s\n", SDPARAMS);
	client_writeback(sock, buf, strlen(buf), tcp);
	return 0;
}

/*
 * Return program agent info.
 */
COMMAND_PROTOTYPE(doprogagents)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
	int		nrows;

	res = mydb_query("select vname,command,dir,timeout,expected_exit_code "
			 "from virt_programs "
			 "where vnode='%s' and pid='%s' and eid='%s'",
			 5, reqp->nickname, reqp->pid, reqp->eid);

	if (!res) {
		error("PROGRAM: %s: DB Error getting virt_agents\n",
		      reqp->nodeid);
		return 1;
	}
	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}
	/*
	 * First spit out the UID, then the agents one to a line.
	 */
	OUTPUT(buf, sizeof(buf), "UID=%s\n", reqp->swapper);
	client_writeback(sock, buf, strlen(buf), tcp);
	if (verbose)
		info("PROGAGENTS: %s", buf);

	while (nrows) {
		char	*bufp = buf, *ebufp = &buf[sizeof(buf)];

		row = mysql_fetch_row(res);

		bufp += OUTPUT(bufp, ebufp - bufp, "AGENT=%s", row[0]);
		if (vers >= 23) {
			if (row[2] && strlen(row[2]) > 0)
				bufp += OUTPUT(bufp, ebufp - bufp,
					       " DIR=%s", row[2]);
			if (row[3] && strlen(row[3]) > 0)
				bufp += OUTPUT(bufp, ebufp - bufp,
					       " TIMEOUT=%s", row[3]);
			if (row[4] && strlen(row[4]) > 0)
				bufp += OUTPUT(bufp, ebufp - bufp,
					       " EXPECTED_EXIT_CODE=%s",
					       row[4]);
		}
		if (vers >= 13)
			bufp += OUTPUT(bufp, ebufp - bufp,
				       " COMMAND='%s'", row[1]);
		OUTPUT(bufp, ebufp - bufp, "\n");
		client_writeback(sock, buf, strlen(buf), tcp);

		nrows--;
		if (verbose)
			info("PROGAGENTS: %s", buf);
	}
	mysql_free_result(res);
	return 0;
}

/*
 * Return sync server info.
 */
COMMAND_PROTOTYPE(dosyncserver)
{
	char		buf[MYBUFSIZE];

	if (!strlen(reqp->syncserver))
		return 0;

	OUTPUT(buf, sizeof(buf),
	       "SYNCSERVER SERVER='%s.%s.%s.%s' ISSERVER=%d\n",
	       reqp->syncserver,
	       reqp->eid, reqp->pid, OURDOMAIN,
	       (strcmp(reqp->syncserver, reqp->nickname) ? 0 : 1));
	client_writeback(sock, buf, strlen(buf), tcp);

	if (verbose)
		info("%s", buf);

	return 0;
}

/*
 * Return keyhash info
 */
COMMAND_PROTOTYPE(dokeyhash)
{
	char		buf[MYBUFSIZE];

	if (!strlen(reqp->keyhash))
		return 0;

	OUTPUT(buf, sizeof(buf), "KEYHASH HASH='%s'\n", reqp->keyhash);
	client_writeback(sock, buf, strlen(buf), tcp);

	if (verbose)
		info("%s", buf);

	return 0;
}

/*
 * Return eventkey info
 */
COMMAND_PROTOTYPE(doeventkey)
{
	char		buf[MYBUFSIZE];

	if (!strlen(reqp->eventkey))
		return 0;

	OUTPUT(buf, sizeof(buf), "EVENTKEY KEY='%s'\n", reqp->eventkey);
	client_writeback(sock, buf, strlen(buf), tcp);

	if (verbose)
		info("%s", buf);

	return 0;
}

/*
 * Return routing stuff for all vnodes mapped to the requesting physnode
 */
COMMAND_PROTOTYPE(doroutelist)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
	int		n, nrows;

	/*
	 * Get the routing type from the nodes table.
	 */
	res = mydb_query("select routertype from nodes where node_id='%s'",
			 1, reqp->nodeid);

	if (!res) {
		error("ROUTES: %s: DB Error getting router type!\n",
		      reqp->nodeid);
		return 1;
	}

	if ((int)mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return 0;
	}

	/*
	 * Return type. At some point we might have to return a list of
	 * routes too, if we support static routes specified by the user
	 * in the NS file.
	 */
	row = mysql_fetch_row(res);
	if (! row[0] || !row[0][0]) {
		mysql_free_result(res);
		return 0;
	}
	sprintf(buf, "ROUTERTYPE=%s\n", row[0]);
	mysql_free_result(res);

	client_writeback(sock, buf, strlen(buf), tcp);
	if (verbose)
		info("ROUTES: %s", buf);

	/*
	 * Get the routing type from the nodes table.
	 */
	res = mydb_query("select vr.vname,src,dst,dst_type,dst_mask,nexthop,cost "
			 "from virt_routes as vr "
			 "left join v2pmap as v2p "
			 "using (pid,eid,vname) "
			 "where vr.pid='%s' and "
			 " vr.eid='%s' and v2p.node_id='%s'",
			 7, reqp->pid, reqp->eid, reqp->nodeid);

	if (!res) {
		error("ROUTELIST: %s: DB Error getting manual routes!\n",
		      reqp->nodeid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}
	n = nrows;

	while (n) {
		char dstip[32];

		row = mysql_fetch_row(res);

		/*
		 * OMG, the Linux route command is too stupid to accept a
		 * host-on-a-subnet as the subnet address, so we gotta mask
		 * off the bits manually for network routes.
		 *
		 * Eventually we'll perform this operation in the NS parser
		 * so it appears in the DB correctly.
		 */
		if (strcmp(row[3], "net") == 0) {
			struct in_addr tip, tmask;

			inet_aton(row[2], &tip);
			inet_aton(row[4], &tmask);
			tip.s_addr &= tmask.s_addr;
			strncpy(dstip, inet_ntoa(tip), sizeof(dstip));
		} else
			strncpy(dstip, row[2], sizeof(dstip));

		sprintf(buf, "ROUTE NODE=%s SRC=%s DEST=%s DESTTYPE=%s DESTMASK=%s "
			"NEXTHOP=%s COST=%s\n",
			row[0], row[1], dstip, row[3], row[4], row[5], row[6]);

		client_writeback(sock, buf, strlen(buf), tcp);

		n--;
	}
	mysql_free_result(res);
	if (verbose)
	    info("ROUTES: %d routes in list\n", nrows);

	return 0;
}

/*
 * Return routing stuff for all vnodes mapped to the requesting physnode
 */
COMMAND_PROTOTYPE(dorole)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];

	/*
	 * Get the erole from the reserved table
	 */
	res = mydb_query("select erole from reserved where node_id='%s'",
			 1, reqp->nodeid);

	if (!res) {
		error("ROLE: %s: DB Error getting the role of this node!\n",
		      reqp->nodeid);
		return 1;
	}

	if ((int)mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return 0;
	}

	row = mysql_fetch_row(res);
	if (! row[0] || !row[0][0]) {
		mysql_free_result(res);
		return 0;
	}
	sprintf(buf, "%s\n", row[0]);
	mysql_free_result(res);

	client_writeback(sock, buf, strlen(buf), tcp);
	if (verbose)
		info("ROLE: %s", buf);

	return 0;
}

/*
 * Return entire config.
 */
COMMAND_PROTOTYPE(dofullconfig)
{
	char		buf[MYBUFSIZE];
	int		i;
	int		mask;

	if (reqp->isvnode)
		mask = FULLCONFIG_VIRT;
	else
		mask = FULLCONFIG_PHYS;

	for (i = 0; i < numcommands; i++) {
		if (command_array[i].fullconfig & mask) {
			if (tcp && !isssl && !reqp->islocal &&
			    (command_array[i].flags & F_REMREQSSL) != 0) {
				/*
				 * Silently drop commands that are not
				 * allowed for remote non-ssl connections.
				 */
				continue;
			}
			/*
			 * Silently drop all TPM-required commands right now.
			 */
			if ((command_array[i].flags & F_REQTPM)) {
				continue;
			}
			OUTPUT(buf, sizeof(buf),
			       "*** %s\n", command_array[i].cmdname);
			client_writeback(sock, buf, strlen(buf), tcp);
			command_array[i].func(sock, reqp, rdata, tcp, vers);
			client_writeback(sock, buf, strlen(buf), tcp);
		}
	}
	return 0;
}

/*
 * Report node resource usage. This also serves as an isalive(),
 * so we send back update info. The format for upload is:
 *
 *  LA1=x.y LA5=x.y LA15=x.y DUSED=x ...
 */
COMMAND_PROTOTYPE(dorusage)
{
	char		buf[MYBUFSIZE];
	float		la1, la5, la15, dused;
        int             plfd;
        struct timeval  now;
        struct tm       *tmnow;
        char            pllogfname[MAXPATHLEN];
        char            timebuf[10];

	if (sscanf(rdata, "LA1=%f LA5=%f LA15=%f DUSED=%f",
		   &la1, &la5, &la15, &dused) != 4) {
		strncpy(buf, rdata, 64);
		error("RUSAGE: %s: Bad arguments: %s...\n", reqp->nodeid, buf);
		return 1;
	}

	/*
	 * See db/node_status script, which uses this info (timestamps)
	 * to determine when nodes are down.
	 *
	 * XXX: Plab physnode status is reported from the management slice.
         *
	 */
	mydb_update("replace into node_rusage "
		    " (node_id, status_timestamp, "
		    "  load_1min, load_5min, load_15min, disk_used) "
		    " values ('%s', now(), %f, %f, %f, %f)",
		    reqp->pnodeid, la1, la5, la15, dused);

	if (reqp->isplabdslice) {
		mydb_update("replace into node_status "
			    " (node_id, status, status_timestamp) "
			    " values ('%s', 'up', now())",
			    reqp->pnodeid);

		mydb_update("replace into node_status "
			    " (node_id, status, status_timestamp) "
			    " values ('%s', 'up', now())",
			    reqp->vnodeid);
        }

	/*
	 * At some point, maybe what we will do is have the client
	 * make a request asking what needs to be updated. Right now,
	 * just return yes/no and let the client assume it knows what
	 * to do (update accounts).
	 */
	OUTPUT(buf, sizeof(buf), "UPDATE=%d\n", reqp->update_accounts);
	client_writeback(sock, buf, strlen(buf), tcp);

        /* We're going to store plab up/down data in a file for a while. */
        if (reqp->isplabsvc) {
            gettimeofday(&now, NULL);
            tmnow = localtime((time_t *)&now.tv_sec);
            strftime(timebuf, sizeof(timebuf), "%Y%m%d", tmnow);
            snprintf(pllogfname, sizeof(pllogfname),
                     "%s/%s-isalive-%s",
                     PLISALIVELOGDIR,
                     reqp->pnodeid,
                     timebuf);

            snprintf(buf, sizeof(buf), "%ld %ld\n",
                     (long)now.tv_sec, (long)now.tv_usec);
            plfd = open(pllogfname, O_WRONLY|O_APPEND|O_CREAT,
                        S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
            if (plfd < 0) {
                errorc("Can't open log: %s", pllogfname);
            } else {
                write(plfd, buf, strlen(buf));
                close(plfd);
            }
        }

	return 0;
}

/*
 * Report time intervals to use in the watchdog process.  All times in
 * seconds (0 means never, -1 means no value is available in the DB):
 *
 *	INTERVAL=	how often to check for new intervals
 *	ISALIVE=	how often to report isalive info
 *			(note that also controls how often new accounts
 *			 are noticed)
 *	NTPDRIFT=	how often to report NTP drift values
 *	CVSUP=		how often to check for software updates
 *	RUSAGE=		how often to collect/report resource usage info
 */
COMMAND_PROTOTYPE(dodoginfo)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE], *bp;
	int		nrows, *iv;
	int		iv_interval, iv_isalive, iv_ntpdrift, iv_cvsup;
	int		iv_rusage, iv_hkeys, iv_dhcpdconf;

	/*
	 * XXX sitevar fetching should be a library function
	 */
	res = mydb_query("select name,value,defaultvalue from "
			 "sitevariables where name like 'watchdog/%%'", 3);
	if (!res || (nrows = (int)mysql_num_rows(res)) == 0) {
		error("WATCHDOGINFO: no watchdog sitevars\n");
		if (res)
			mysql_free_result(res);
		return 1;
	}

	iv_interval = iv_isalive = iv_ntpdrift = iv_cvsup =
		iv_rusage = iv_hkeys = -1;
	while (nrows) {
		iv = 0;
		row = mysql_fetch_row(res);
		if (strcmp(row[0], "watchdog/interval") == 0) {
			iv = &iv_interval;
		} else if (strcmp(row[0], "watchdog/ntpdrift") == 0) {
			iv = &iv_ntpdrift;
		} else if (strcmp(row[0], "watchdog/cvsup") == 0) {
			iv = &iv_cvsup;
		} else if (strcmp(row[0], "watchdog/rusage") == 0) {
			iv = &iv_rusage;
		} else if (strcmp(row[0], "watchdog/hostkeys") == 0) {
			iv = &iv_hkeys;
		} else if (strcmp(row[0], "watchdog/dhcpdconf") == 0) {
			iv = &iv_dhcpdconf;
		} else if (strcmp(row[0], "watchdog/isalive/local") == 0) {
			if (reqp->islocal && !reqp->isvnode)
				iv = &iv_isalive;
		} else if (strcmp(row[0], "watchdog/isalive/vnode") == 0) {
			if (reqp->islocal && reqp->isvnode)
				iv = &iv_isalive;
		} else if (strcmp(row[0], "watchdog/isalive/plab") == 0) {
			if (!reqp->islocal && reqp->isplabdslice)
				iv = &iv_isalive;
		} else if (strcmp(row[0], "watchdog/isalive/wa") == 0) {
			if (!reqp->islocal && !reqp->isplabdslice)
				iv = &iv_isalive;
		}

		if (iv) {
			/* use the current value if set */
			if (row[1] && row[1][0])
				*iv = atoi(row[1]) * 60;
			/* else check for default value */
			else if (row[2] && row[2][0])
				*iv = atoi(row[2]) * 60;
			else
				error("WATCHDOGINFO: sitevar %s not set\n",
				      row[0]);
		}
		nrows--;
	}
	mysql_free_result(res);

	/*
	 * XXX adjust for local policy
	 * - vnodes and plab nodes do not send NTP drift or cvsup
	 * - vnodes do not report host keys
	 * - widearea nodes do not record drift
	 * - local nodes do not cvsup
	 * - only a plab node service slice reports rusage
	 *   (which it uses in place of isalive)
	 */
	if ((reqp->islocal && reqp->isvnode) || reqp->isplabdslice) {
		iv_ntpdrift = iv_cvsup = 0;
		if (!reqp->isplabdslice)
			iv_hkeys = 0;
	}
	if (!reqp->islocal)
		iv_ntpdrift = 0;
	else
		iv_cvsup = 0;
	if (reqp->isplabsvc)
		iv_isalive = 0;
	else if (reqp->islocal && reqp->sharing_mode[0] && !reqp->isvnode)
		iv_rusage = 60;
	else
		iv_rusage = 0;

	bp = buf;
	bp += OUTPUT(bp, sizeof(buf),
		     "INTERVAL=%d ISALIVE=%d NTPDRIFT=%d CVSUP=%d "
		     "RUSAGE=%d HOSTKEYS=%d DHCPDCONF=%d",
		     iv_interval, iv_isalive, iv_ntpdrift, iv_cvsup,
		     iv_rusage, iv_hkeys, iv_dhcpdconf);
	if (vers >= 29) {
	        int rootpswdinterval = 0;
#ifdef DYNAMICROOTPASSWORDS
	        rootpswdinterval = 3600;
#endif
		OUTPUT(bp, sizeof(buf) - (bp - buf), " SETROOTPSWD=%d\n",
		       rootpswdinterval);
	}
	else
		OUTPUT(bp, sizeof(buf) - (bp - buf), "\n");

	client_writeback(sock, buf, strlen(buf), tcp);

	if (verbose)
		info("%s", buf);

	return 0;
}

/*
 * Stash info returned by the host into the DB
 * Right now we only recognize CDVERSION=<str> for CD booted systems.
 */
COMMAND_PROTOTYPE(dohostinfo)
{
	char		*bp, buf[MYBUFSIZE];

	bp = rdata;
	if (sscanf(bp, "CDVERSION=%31[a-zA-Z0-9-]", buf) == 1) {
		if (verbose)
			info("HOSTINFO CDVERSION=%s\n", buf);
		if (mydb_update("update nodes set cd_version='%s' "
				"where node_id='%s'",
				buf, reqp->nodeid)) {
			error("HOSTINFO: %s: DB update failed\n", reqp->nodeid);
			return 1;
		}
	}
	return 0;
}

/*
 * XXX Stash ssh host keys into the DB.
 */
COMMAND_PROTOTYPE(dohostkeys)
{
#define MAXKEY		4096
#define RSAV1_STR	"SSH_HOST_KEY='"
#define RSAV2_STR	"SSH_HOST_RSA_KEY='"
#define DSAV2_STR	"SSH_HOST_DSA_KEY='"

	char	*bp, rsav1[2*MAXKEY], rsav2[2*MAXKEY], dsav2[2*MAXKEY];
	char	buf[MAXKEY];

#if 0
	printf("%s\n", rdata);
#endif

	/*
	 * The maximum key length we accept is 1024 bytes, but after we
	 * run it through mysql_escape_string() it could potentially double
	 * in size (although that is very unlikely).
	 */
	rsav1[0] = rsav2[0] = dsav2[0] = 0;

	/*
	 * Sheesh, perl string matching would be so much easier!
	 */
	bp = rdata;
	while (*bp) {
		char	*ep, *kp, *thiskey = (char *) NULL;

		while (*bp == ' ')
			bp++;
		if (! *bp)
			break;

		if (! strncasecmp(bp, RSAV1_STR, strlen(RSAV1_STR))) {
			thiskey = rsav1;
			bp += strlen(RSAV1_STR);
		}
		else if (! strncasecmp(bp, RSAV2_STR, strlen(RSAV2_STR))) {
			thiskey = rsav2;
			bp += strlen(RSAV2_STR);
		}
		else if (! strncasecmp(bp, DSAV2_STR, strlen(DSAV2_STR))) {
			thiskey = dsav2;
			bp += strlen(DSAV2_STR);
		}
		else {
			error("HOSTKEYS: %s: "
			      "Unrecognized key type '%.8s ...'\n",
			      reqp->nodeid, bp);
			if (verbose)
				error("HOSTKEYS: %s\n", rdata);
			return 1;
		}
		kp = buf;
		ep = &buf[sizeof(buf) - 1];

		/* Copy the part between the single quotes to the holding buf */
		while (*bp && *bp != '\'' && kp < ep)
			*kp++ = *bp++;
		if (*bp != '\'') {
			error("HOSTKEYS: %s: %s key data too long!\n",
			      reqp->nodeid,
			      thiskey == rsav1 ? "RSA v1" :
			      (thiskey == rsav2 ? "RSA v2" : "DSA v2"));
			if (verbose)
				error("HOSTKEYS: %s\n", rdata);
			return 1;
		}
		bp++;
		*kp = '\0';

		/* Okay, turn into something for mysql statement. */
		thiskey[0] = '\'';
		mysql_escape_string(&thiskey[1], buf, strlen(buf));
		strcat(thiskey, "'");
	}
	if (mydb_update("update node_hostkeys set "
			"       sshrsa_v1=%s,sshrsa_v2=%s,sshdsa_v2=%s "
			"where node_id='%s'",
			(rsav1[0] ? rsav1 : "NULL"),
			(rsav2[0] ? rsav2 : "NULL"),
			(dsav2[0] ? dsav2 : "NULL"),
			reqp->nodeid)) {
		error("HOSTKEYS: %s: setting hostkeys!\n", reqp->nodeid);
		return 1;
	}
	if (verbose) {
		info("sshrsa_v1=%s,sshrsa_v2=%s,sshdsa_v2=%s\n",
		     (rsav1[0] ? rsav1 : "NULL"),
		     (rsav2[0] ? rsav2 : "NULL"),
		     (dsav2[0] ? dsav2 : "NULL"));
	}
	return 0;
}

COMMAND_PROTOTYPE(dofwinfo)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
	char		fwname[TBDB_FLEN_VNAME+2];
	char		fwtype[TBDB_FLEN_VNAME+2];
	char		fwstyle[TBDB_FLEN_VNAME+2];
	int		n, nrows;
	char		*vlan;

	/*
	 * See if this node's experiment has an associated firewall
	 *
	 * XXX will only work if there is one firewall per experiment.
	 */
	res = mydb_query("select r.node_id,v.type,v.style,v.log,f.fwname,"
			 "  i.IP,i.mac,f.vlan "
			 "from firewalls as f "
			 "left join reserved as r on"
			 "  f.pid=r.pid and f.eid=r.eid and f.fwname=r.vname "
			 "left join virt_firewalls as v on "
			 "  v.pid=f.pid and v.eid=f.eid and v.fwname=f.fwname "
			 "left join interfaces as i on r.node_id=i.node_id "
			 "where f.pid='%s' and f.eid='%s' "
			 "and i.role='ctrl'",	/* XXX */
			 8, reqp->pid, reqp->eid);

	if (!res) {
		error("FWINFO: %s: DB Error getting firewall info!\n",
		      reqp->nodeid);
		return 1;
	}

	/*
	 * Common case, no firewall
	 */
	if ((int)mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		strncpy(buf, "TYPE=none\n", sizeof(buf));
		client_writeback(sock, buf, strlen(buf), tcp);
		return 0;
	}

	/*
	 * DB data is bogus
	 */
	row = mysql_fetch_row(res);
	if (!row[0] || !row[0][0]) {
		mysql_free_result(res);
		error("FWINFO: %s: DB Error in firewall info, no firewall!\n",
		      reqp->nodeid);
		strncpy(buf, "TYPE=none\n", sizeof(buf));
		client_writeback(sock, buf, strlen(buf), tcp);
		return 0;
	}

	/*
	 * There is a firewall, but it isn't us.
	 * Put out base info with TYPE=remote.
	 */
	if (strcmp(row[0], reqp->nodeid) != 0) {
		char *fwip;

		/*
		 * XXX sorta hack: if we are a HW enforced firewall,
		 * then the client doesn't need to do anything.
		 * Set the FWIP to 0 to indicate this.
		 */
		if (strcmp(row[1], "ipfw2-vlan") == 0)
			fwip = "0.0.0.0";
		else
			fwip = row[5];
		OUTPUT(buf, sizeof(buf), "TYPE=remote FWIP=%s\n", fwip);
		mysql_free_result(res);
		client_writeback(sock, buf, strlen(buf), tcp);
		if (verbose)
			info("FWINFO: %s", buf);
		return 0;
	}

	/*
	 * Grab vlan info if available
	 */
	if (row[7] && row[7][0])
		vlan = row[7];
	else
		vlan = "0";

	/*
	 * We are the firewall.
	 * Put out base information and all the rules
	 *
	 * XXX for now we use the control interface for in/out
	 */
	OUTPUT(buf, sizeof(buf),
	       "TYPE=%s STYLE=%s IN_IF=%s OUT_IF=%s IN_VLAN=%s OUT_VLAN=%s\n",
	       row[1], row[2], row[6], row[6], vlan, vlan);
	client_writeback(sock, buf, strlen(buf), tcp);
	if (verbose)
		info("FWINFO: %s", buf);

	/*
	 * Put out info about firewall rule logging
	 */
	if (vers > 25 && row[3] && row[3][0]) {
		OUTPUT(buf, sizeof(buf), "LOG=%s\n", row[3]);
		client_writeback(sock, buf, strlen(buf), tcp);
		if (verbose)
			info("FWINFO: %s", buf);
	}

	strncpy(fwtype, row[1], sizeof(fwtype));
	strncpy(fwstyle, row[2], sizeof(fwstyle));
	strncpy(fwname, row[4], sizeof(fwname));
	mysql_free_result(res);

	/*
	 * Return firewall variables
	 */
	if (vers > 21) {
		/*
		 * Grab the node gateway MAC which is not currently part
		 * of the firewall variables table.
		 */
		res = mydb_query("select value from sitevariables "
				 "where name='node/gw_mac'", 1);
		if (res && mysql_num_rows(res) > 0) {
			row = mysql_fetch_row(res);
			if (row[0]) {
				OUTPUT(buf, sizeof(buf),
				       "VAR=EMULAB_GWIP VALUE=\"%s\"\n",
				       CONTROL_ROUTER_IP);
				client_writeback(sock, buf, strlen(buf), tcp);
				OUTPUT(buf, sizeof(buf),
				       "VAR=EMULAB_GWMAC VALUE=\"%s\"\n",
				       row[0]);
				client_writeback(sock, buf, strlen(buf), tcp);
			}
		}
		if (res)
			mysql_free_result(res);

		res = mydb_query("select name,value from default_firewall_vars",
				 2);
		if (!res) {
			error("FWINFO: %s: DB Error getting firewall vars!\n",
			      reqp->nodeid);
			nrows = 0;
		} else
			nrows = (int)mysql_num_rows(res);
		for (n = nrows; n > 0; n--) {
			row = mysql_fetch_row(res);
			if (!row[0] || !row[1])
				continue;
			OUTPUT(buf, sizeof(buf), "VAR=%s VALUE=\"%s\"\n",
			       row[0], row[1]);
			client_writeback(sock, buf, strlen(buf), tcp);
		}
		if (res)
			mysql_free_result(res);
		if (verbose)
			info("FWINFO: %d variables\n", nrows);
	}

	/*
	 * Get the user firewall rules from the DB and return them.
	 */
	res = mydb_query("select ruleno,rule from firewall_rules "
			 "where pid='%s' and eid='%s' and fwname='%s' "
			 "order by ruleno",
			 2, reqp->pid, reqp->eid, fwname);
	if (!res) {
		error("FWINFO: %s: DB Error getting firewall rules!\n",
		      reqp->nodeid);
		return 1;
	}
	nrows = (int)mysql_num_rows(res);

	for (n = nrows; n > 0; n--) {
		row = mysql_fetch_row(res);
		OUTPUT(buf, sizeof(buf), "RULENO=%s RULE=\"%s\"\n",
		       row[0], row[1]);
		client_writeback(sock, buf, strlen(buf), tcp);
	}

	mysql_free_result(res);
	if (verbose)
	    info("FWINFO: %d user rules\n", nrows);

	/*
	 * Get the default firewall rules from the DB and return them.
	 */
	res = mydb_query("select ruleno,rule from default_firewall_rules "
			 "where type='%s' and style='%s' and enabled!=0 "
			 "order by ruleno",
			 2, fwtype, fwstyle);
	if (!res) {
		error("FWINFO: %s: DB Error getting default firewall rules!\n",
		      reqp->nodeid);
		return 1;
	}
	nrows = (int)mysql_num_rows(res);

	for (n = nrows; n > 0; n--) {
		row = mysql_fetch_row(res);
		OUTPUT(buf, sizeof(buf), "RULENO=%s RULE=\"%s\"\n",
		       row[0], row[1]);
		client_writeback(sock, buf, strlen(buf), tcp);
	}

	mysql_free_result(res);
	if (verbose)
	    info("FWINFO: %d default rules\n", nrows);

	/*
	 * Ohhh...I gotta bad case of the butt-uglies!
	 *
	 * Return the list of the unqualified names of the firewalled hosts
	 * along with their IP addresses.  The client code uses this to
	 * construct a local hosts file so that symbolic host names can
	 * be used in firewall rules.
	 *
	 * We also return the control net MAC address for each node so
	 * that we can provide proxy ARP.
	 */
	if (vers > 24) {
		res = mydb_query("select r.vname,i.IP,i.mac "
			"from reserved as r "
			"left join interfaces as i on r.node_id=i.node_id "
			"where r.pid='%s' and r.eid='%s' and i.role='ctrl'",
			 3, reqp->pid, reqp->eid);
		if (!res) {
			error("FWINFO: %s: DB Error getting host info!\n",
			      reqp->nodeid);
			return 1;
		}
		nrows = (int)mysql_num_rows(res);

		for (n = nrows; n > 0; n--) {
			row = mysql_fetch_row(res);
			if (vers > 25) {
				OUTPUT(buf, sizeof(buf),
				       "HOST=%s CNETIP=%s CNETMAC=%s\n",
				       row[0], row[1], row[2]);
			} else {
				OUTPUT(buf, sizeof(buf),
				       "HOST=%s CNETIP=%s\n",
				       row[0], row[1]);
			}
			client_writeback(sock, buf, strlen(buf), tcp);
		}

		mysql_free_result(res);
		if (verbose)
			info("FWINFO: %d firewalled hosts\n", nrows);
	}

	/*
	 * We also start returning the MAC addresses for boss, ops, fs
	 * and subboss servers so that we can create ARP entries. This is
	 * necessary if the servers are in the same subnet as the nodes
	 * and thus the gateway is not used to contact them.
	 */
	if (vers > 33) {
		res = mydb_query("select node_id,IP,mac from interfaces "
				 "where role='ctrl' and node_id in "
				 "('boss','ops','fs',"
				 "(select distinct subboss_id from subbosses))",
				 3);
		if (!res) {
			error("FWINFO: %s: DB Error getting server info!\n",
			      reqp->nodeid);
			return 1;
		}
		nrows = (int)mysql_num_rows(res);

		if (nrows > 0) {
			struct in_addr cnet, cmask, naddr;

			inet_aton(CONTROL_NETWORK, &cnet);
			inet_aton(CONTROL_NETMASK, &cmask);
			cnet.s_addr &= cmask.s_addr;

			for (n = nrows; n > 0; n--) {
				row = mysql_fetch_row(res);

				/*
				 * Return the ones on the node control net.
				 */
				inet_aton(row[1], &naddr);
				naddr.s_addr &= cmask.s_addr;
				if (naddr.s_addr != cnet.s_addr)
					continue;

				OUTPUT(buf, sizeof(buf),
				       "SERVER=%s CNETIP=%s CNETMAC=%s\n",
				       row[0], row[1], row[2]);
				client_writeback(sock, buf, strlen(buf), tcp);
			}
		}

		mysql_free_result(res);
		if (verbose)
			info("FWINFO: %d server hosts\n", nrows);
	}

	return 0;
}

/*
 * Return the config for an inner emulab
 */
COMMAND_PROTOTYPE(doemulabconfig)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
	char		*bufp = buf, *ebufp = &buf[sizeof(buf)];
	char		myrole[32], rolebuf[256];
	int		nrows;

	/*
	 * Must be an elab_in_elab experiment.
	 */
	if (!reqp->elab_in_elab) {
		/* Silent error is fine */
		return 0;
	}

	/*
	 * Get the control network IPs. At the moment, it is assumed that
	 * that all the nodes are on a single lan. If we get fancier about
	 * that, we will have to figure out how to label the specific lans
	 * se we know which is which.
	 *
	 * Note the single vs dual control network differences!
	 */
	if (reqp->singlenet) {
		res = mydb_query("select r.node_id,r.inner_elab_role,"
				 "   i.IP,r.vname from reserved as r "
				 "left join interfaces as i on "
				 "     i.node_id=r.node_id and i.role='ctrl' "
				 "where r.pid='%s' and r.eid='%s' and "
				 "      r.inner_elab_role is not null",
				 4, reqp->pid, reqp->eid);
	}
	else {
		res = mydb_query("select r.node_id,r.inner_elab_role,"
				 "   vl.ip,r.vname from reserved as r "
				 "left join virt_lans as vl on "
				 "     vl.vnode=r.vname and "
				 "     vl.pid=r.pid and vl.eid=r.eid "
				 "where r.pid='%s' and r.eid='%s' and "
				 "      r.inner_elab_role is not null",
				 4, reqp->pid, reqp->eid);
	}

	if (!res) {
		error("EMULABCONFIG: %s: DB Error getting elab_in_elab\n",
		      reqp->nodeid);
		return 1;
	}
	if (! mysql_num_rows(res)) {
		mysql_free_result(res);
		return 0;
	}
	nrows = (int)mysql_num_rows(res);

	/*
	 * Lets find the role for the current node and spit that out first.
	 */
	bzero(buf, sizeof(buf));
	while (nrows--) {
		row = mysql_fetch_row(res);

		if (!strcmp(row[0], reqp->nodeid)) {
			bufp += OUTPUT(bufp, ebufp - bufp, "ROLE=%s\n", row[1]);
			strncpy(myrole, row[1], sizeof(myrole));
			break;
		}
	}

	/*
	 * Spit the names of the boss, ops and fs nodes for everyones benefit.
	 */
	mysql_data_seek(res, 0);
	nrows = (int)mysql_num_rows(res);

	while (nrows--) {
		row = mysql_fetch_row(res);

		if (!strcmp(row[1], "boss") || !strcmp(row[1], "boss+router") ||
		    !strcmp(row[1], "boss+fs+router")) {
			bufp += OUTPUT(bufp, ebufp - bufp, "BOSSNODE=%s\n",
				       row[3]);
			bufp += OUTPUT(bufp, ebufp - bufp, "BOSSIP=%s\n",
				       row[2]);
		}
		else if (!strcmp(row[1], "ops") || !strcmp(row[1], "ops+fs")) {
			bufp += OUTPUT(bufp, ebufp - bufp, "OPSNODE=%s\n",
				       row[3]);
			bufp += OUTPUT(bufp, ebufp - bufp, "OPSIP=%s\n",
				       row[2]);
		}
		else if (!strcmp(row[1], "fs")) {
			bufp += OUTPUT(bufp, ebufp - bufp, "FSNODE=%s\n",
				       row[3]);
			bufp += OUTPUT(bufp, ebufp - bufp, "FSIP=%s\n",
				       row[2]);
		}
	}
	mysql_free_result(res);

	/*
	 * Some package info and other stuff from sitevars.
	 */
	res = mydb_query("select name,value from sitevariables "
			 "where name like 'elabinelab/%%'", 2);
	if (!res) {
		error("EMULABCONFIG: %s: DB Error getting elab_in_elab\n",
		      reqp->nodeid);
		return 1;
	}
	nrows = (int)mysql_num_rows(res);
	while (nrows--) {
		row = mysql_fetch_row(res);
		if (strcmp(row[0], "elabinelab/boss_pkg_dir") == 0) {
			bufp += OUTPUT(bufp, ebufp - bufp, "BOSS_PKG_DIR=%s\n",
				       row[1]);
		} else if (strcmp(row[0], "elabinelab/boss_pkg") == 0) {
			bufp += OUTPUT(bufp, ebufp - bufp, "BOSS_PKG=%s\n",
				       row[1]);
		} else if (strcmp(row[0], "elabinelab/ops_pkg_dir") == 0) {
			bufp += OUTPUT(bufp, ebufp - bufp, "OPS_PKG_DIR=%s\n",
				       row[1]);
		} else if (strcmp(row[0], "elabinelab/ops_pkg") == 0) {
			bufp += OUTPUT(bufp, ebufp - bufp, "OPS_PKG=%s\n",
				       row[1]);
		} else if (strcmp(row[0], "elabinelab/fs_pkg_dir") == 0) {
			bufp += OUTPUT(bufp, ebufp - bufp, "FS_PKG_DIR=%s\n",
				       row[1]);
		} else if (strcmp(row[0], "elabinelab/fs_pkg") == 0) {
			bufp += OUTPUT(bufp, ebufp - bufp, "FS_PKG=%s\n",
				       row[1]);
		} else if (strcmp(row[0], "elabinelab/windows") == 0) {
			bufp += OUTPUT(bufp, ebufp - bufp, "WINSUPPORT=%s\n",
				       row[1]);
		}
	}
	mysql_free_result(res);

	/*
	 * Stuff from the experiments table.
	 */
	res = mydb_query("select elabinelab_cvstag,elabinelab_nosetup "
			 "   from experiments "
			 "where pid='%s' and eid='%s'",
			 2, reqp->pid, reqp->eid);
	if (!res) {
		error("EMULABCONFIG: %s: DB Error getting experiments info\n",
		      reqp->nodeid);
		return 1;
	}
	if ((int)mysql_num_rows(res)) {
	    row = mysql_fetch_row(res);

	    if (row[0] && row[0][0]) {
		bufp += OUTPUT(bufp, ebufp - bufp, "CVSSRCTAG=%s\n",
			       row[0]);
	    }
	    if (row[1] && strcmp(row[1], "0")) {
	        bufp += OUTPUT(bufp, ebufp - bufp, "NOSETUP=1\n");
	    }
	}
	mysql_free_result(res);

	/*
	 * Tell the inner elab if its a single control network setup.
	 */
	bufp += OUTPUT(bufp, ebufp - bufp, "SINGLE_CONTROLNET=%d\n",
		       reqp->singlenet);

	/*
	 * Put out new attributes.  Eventually, most of the above
	 * parameters should come from here.  Note that a node can have
	 * multiple roles and we need to construct a query for that.
	 */
	rolebuf[0] = '\0';
	if (strchr(myrole, '+')) {
		int gotone = 0;
		int len = sizeof(rolebuf) - 1;
		if (len > 0 && strstr(myrole, "boss")) {
			if (gotone)
				strcat(rolebuf, " or ");
			strncat(rolebuf, "role='boss'", len);
			len -= 11;
			gotone = 1;
		}
		if (len > 0 && strstr(myrole, "ops")) {
			if (gotone)
				strcat(rolebuf, " or ");
			strncat(rolebuf, "role='ops'", len);
			len -= 10;
			gotone = 1;
		}
		if (len > 0 && strstr(myrole, "fs")) {
			if (gotone)
				strcat(rolebuf, " or ");
			strncat(rolebuf, "role='fs'", len);
			len -= 9;
			gotone = 1;
		}
		if (len > 0 && strstr(myrole, "router")) {
			if (gotone)
				strcat(rolebuf, " or ");
			strncat(rolebuf, "role='router'", len);
			len -= 13;
			gotone = 1;
		}
	} else {
		snprintf(rolebuf, sizeof(rolebuf), "role='%s'", myrole);
	}

	/*
	 * Get all the attributes for the node's role.
	 * Note taht for backward compat, we don't worry if the attributes
	 * table does not exist.
	 */
	res = mydb_query("select attrkey,attrvalue "
			 "   from elabinelab_attributes "
			 "where exptidx=%d and (%s) order by ordering",
			 2, reqp->exptidx, rolebuf);
	if (res) {
		nrows = (int)mysql_num_rows(res);
		while (bufp < ebufp && nrows--) {
			row = mysql_fetch_row(res);
			if (row[0] && row[0][0] && row[1])
				bufp += OUTPUT(bufp, ebufp - bufp, "%s=\"%s\"\n",
					       row[0], row[1]);
		}
		mysql_free_result(res);
	}

	client_writeback(sock, buf, strlen(buf), tcp);
	return 0;
}

/*
 * Return the config for an emulated ("inner") planetlab
 */
COMMAND_PROTOTYPE(doeplabconfig)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
	char		*bufp = buf, *ebufp = &buf[sizeof(buf)];
	int		nrows;

	/*
	 * We only respond if we are a PLC node
	 */
	res = mydb_query("select node_id from reserved "
			 "where pid='%s' and eid='%s' and plab_role='plc'",
			 1, reqp->pid, reqp->eid);
	if (!res) {
		error("EPLABCONFIG: %s: DB Error getting plab_role\n",
		      reqp->nodeid);
		return 1;
	}
	if (!mysql_num_rows(res)) {
		mysql_free_result(res);
		return 0;
	}
	row = mysql_fetch_row(res);
	if (!row[0] || strcmp(row[0], reqp->nodeid) != 0) {
		mysql_free_result(res);
		return 0;
	}
	mysql_free_result(res);

	/*
	 * VNAME=<vname> PNAME=<FQpname> ROLE=<role> CNETIP=<IP> CNETMAC=<MAC>
	 */
	res = mydb_query("select r.node_id,r.vname,r.plab_role,i.IP,i.mac "
			 "  from reserved as r join interfaces as i "
			 "  where r.node_id=i.node_id and "
			 "    i.role='ctrl' and r.pid='%s' and r.eid='%s'",
			 5, reqp->pid, reqp->eid);

	if (!res || mysql_num_rows(res) == 0) {
		error("EMULABCONFIG: %s: DB Error getting plab_in_elab info\n",
		      reqp->nodeid);
		if (res)
			mysql_free_result(res);
		return 1;
	}
	nrows = (int)mysql_num_rows(res);

	/*
	 * Spit out the PLC node first just cuz
	 */
	bzero(buf, sizeof(buf));
	while (nrows--) {
		row = mysql_fetch_row(res);

		if (!strcmp(row[2], "plc")) {
			bufp += OUTPUT(bufp, ebufp - bufp,
				       "VNAME=%s PNAME=%s.%s ROLE=%s CNETIP=%s CNETMAC=%s\n",
				       row[1], row[0], OURDOMAIN, row[2],
				       row[3], row[4]);
			client_writeback(sock, buf, strlen(buf), tcp);
			break;
		}
	}

	/*
	 * Then all the nodes
	 */
	mysql_data_seek(res, 0);
	nrows = (int)mysql_num_rows(res);

	while (nrows--) {
		row = mysql_fetch_row(res);
		bufp = buf;

		if (!strcmp(row[1], "plc"))
			continue;

		bufp += OUTPUT(bufp, ebufp - bufp,
			       "VNAME=%s PNAME=%s.%s ROLE=%s CNETIP=%s CNETMAC=%s\n",
			       row[1], row[0], OURDOMAIN, row[2],
			       row[3], row[4]);
		client_writeback(sock, buf, strlen(buf), tcp);
	}
	mysql_free_result(res);

	/*
	 * Now report all the configured experimental interfaces:
	 *
	 * VNAME=<vname> IP=<IP> NETMASK=<mask> MAC=<MAC>
	 */
	res = mydb_query("select vl.vnode,i.IP,i.mask,i.mac from reserved as r"
			 "  left join virt_lans as vl"
			 "    on r.pid=vl.pid and r.eid=vl.eid"
			 "  left join interfaces as i"
			 "    on vl.ip=i.IP and r.node_id=i.node_id"
			 "  where r.pid='%s' and r.eid='%s' and"
			 "    r.plab_role!='none'"
			 "    and i.IP!='' and i.role='expt'",
			 4, reqp->pid, reqp->eid);

	if (!res) {
		error("EMULABCONFIG: %s: DB Error getting plab_in_elab info\n",
		      reqp->nodeid);
		return 1;
	}
	nrows = (int)mysql_num_rows(res);
	while (nrows--) {
		row = mysql_fetch_row(res);
		bufp = buf;

		bufp += OUTPUT(bufp, ebufp - bufp,
			       "VNAME=%s IP=%s NETMASK=%s MAC=%s\n",
			       row[0], row[1], row[2], row[3]);
		client_writeback(sock, buf, strlen(buf), tcp);
	}
	mysql_free_result(res);

	/*
	 * Grab lanlink on which the node should be/contact plc.
	 */
	/*
	 * For now, just assume that plab_plcnet is a valid lan name and
	 * join it with virtlans and ifaces.
	 */
	res = mydb_query("select vl.vnode,r.node_id,vn.plab_plcnet,"
			 "       vn.plab_role,i.IP,i.mask,i.mac"
			 "  from reserved as r left join virt_lans as vl"
			 "    on r.exptidx=vl.exptidx"
			 "  left join interfaces as i"
			 "    on vl.ip=i.IP and r.node_id=i.node_id"
			 "  left join virt_nodes as vn"
			 "    on vl.vname=vn.plab_plcnet and r.vname=vn.vname"
			 "      and vn.exptidx=r.exptidx"
			 "  where r.pid='%s' and r.eid='%s' and"
                         "    r.plab_role != 'none' and i.IP != ''"
			 "      and vn.plab_plcnet != 'none'"
			 "      and vn.plab_plcnet != 'control'",
			 7,reqp->pid,reqp->eid);
	if (!res) {
	    error("EPLABCONFIG: %s: DB Error getting plab_in_elab info\n",
		  reqp->nodeid);
	    return 1;
	}
	nrows = (int)mysql_num_rows(res);
	while (nrows--) {
	    row = mysql_fetch_row(res);
	    bufp = buf;

	    bufp += OUTPUT(bufp,ebufp-bufp,
			   "VNAME=%s PNAME=%s.%s PLCNETWORK=%s ROLE=%s IP=%s NETMASK=%s MAC=%s\n",
			   row[0],row[1],OURDOMAIN,row[2],row[3],row[4],row[5],
			   row[6]);
	    client_writeback(sock,buf,strlen(buf),tcp);
	}
	mysql_free_result(res);

	return 0;
}

/*
 * Hack to test timeouts and other anomolous situations for tmcc
 */
COMMAND_PROTOTYPE(dotmcctest)
{
	char		buf[MYBUFSIZE];
	int		logit;

	logit = verbose;

	if (logit)
		info("TMCCTEST: %s\n", rdata);

	/*
	 * Always allow the test that doesn't tie up a server thread
	 */
	if (strncmp(rdata, "noreply", strlen("noreply")) == 0)
		return 0;

	/*
	 * The rest can tie up a server thread for non-trivial amounts of
	 * time, only allow if debugging.
	 */
	if (!debug) {
		strcpy(buf, "tmcctest disabled\n");
		goto doit;
	}

	/*
	 * Delay reply by the indicated amount of time
	 */
	if (strncmp(rdata, "delayreply", strlen("delayreply")) == 0) {
		int delay = 0;
		if (sscanf(rdata, "delayreply %d", &delay) == 1 &&
		    delay < 20) {
			sleep(delay);
			sprintf(buf, "replied after %d seconds\n", delay);
		} else {
			strcpy(buf, "bogus delay value\n");
		}
		goto doit;
	}

	if (tcp) {
		/*
		 * Reply in pieces
		 */
		if (strncmp(rdata, "splitreply", strlen("splitreply")) == 0) {
			memset(buf, '1', MYBUFSIZE/4);
			buf[MYBUFSIZE/4] = 0;
			client_writeback(sock, buf, strlen(buf), tcp);
			sleep(1);
			memset(buf, '2', MYBUFSIZE/4);
			buf[MYBUFSIZE/4] = 0;
			client_writeback(sock, buf, strlen(buf), tcp);
			sleep(2);
			memset(buf, '4', MYBUFSIZE/4);
			buf[MYBUFSIZE/4] = 0;
			client_writeback(sock, buf, strlen(buf), tcp);
			sleep(4);
			memset(buf, '0', MYBUFSIZE/4);
			buf[MYBUFSIZE/4-1] = '\n';
			buf[MYBUFSIZE/4] = 0;
			logit = 0;
		} else {
			strcpy(buf, "no such TCP test\n");
		}
	} else {
		strcpy(buf, "no such UDP test\n");
	}

 doit:
	client_writeback(sock, buf, strlen(buf), tcp);
	if (logit)
		info("%s", buf);
	return 0;
}

/*
 * Return node localization. For example, boss root pub key, root password,
 * stuff like that.
 */
COMMAND_PROTOTYPE(dolocalize)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
	char		*bufp = buf, *ebufp = &buf[sizeof(buf)];
	int		nrows;

	*bufp = 0;

	/*
	 * XXX sitevar fetching should be a library function.
	 * WARNING: This sitevar (node/ssh_pubkey) is referenced in
	 *          install/boss-install during initial setup.
	 */
	res = mydb_query("select name,value "
			 "from sitevariables where name='node/ssh_pubkey'",
			 2);
	if (!res || (nrows = (int)mysql_num_rows(res)) == 0) {
		error("DOLOCALIZE: sitevar node/ssh_pubkey does not exist\n");
		if (res)
			mysql_free_result(res);
		return 1;
	}

	row = mysql_fetch_row(res);
	if (row[1]) {
	    bufp += OUTPUT(bufp, ebufp - bufp, "ROOTPUBKEY='%s'\n", row[1]);
	}
	mysql_free_result(res);
	client_writeback(sock, buf, strlen(buf), tcp);
	return 0;
}

/*
 * Return root password
 */
COMMAND_PROTOTYPE(dorootpswd)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE], hashbuf[MYBUFSIZE], *bp;

	res = mydb_query("select attrvalue from node_attributes "
			 " where node_id='%s' and "
			 "       attrkey='root_password'",
			 1, reqp->pnodeid);

	if (!res || (int)mysql_num_rows(res) == 0) {
		unsigned char	randdata[5];
		int		fd, cc, i;

		if ((fd = open("/dev/urandom", O_RDONLY)) < 0) {
			errorc("opening /dev/urandom");
			return 1;
		}
		if ((cc = read(fd, randdata, sizeof(randdata))) < 0) {
			errorc("reading /dev/urandom");
			close(fd);
			return 1;
		}
		if (cc != sizeof(randdata)) {
			error("Short read from /dev/urandom: %d", cc);
			close(fd);
			return 1;
		}
		close(fd);

		bp = hashbuf;
		for (i = 0; i < sizeof(randdata); i++) {
			bp += sprintf(bp, "%02x", randdata[i]);
		}
		*bp = '\0';

		mydb_update("replace into node_attributes set "
			    "  node_id='%s', "
			    "  attrkey='root_password',attrvalue='%s'",
			    reqp->nodeid, hashbuf);
	}
	else {
		row = mysql_fetch_row(res);
		strcpy(hashbuf, row[0]);
	}
	if (res)
		mysql_free_result(res);

	/*
	 * Need to crypt() this for the node since we obviously do not want
	 * to return the plain text.
	 */
	sprintf(buf, "$1$%s", hashbuf);
	bp = crypt(hashbuf, buf);

	OUTPUT(buf, sizeof(buf), "HASH=%s\n", bp);
	client_writeback(sock, buf, strlen(buf), tcp);
	return 0;
}

/*
 * Upload boot log to DB for the node.
 */
COMMAND_PROTOTYPE(dobootlog)
{
	char		*cp = (char *) NULL;
	int		len;

	/*
	 * Dig out the log message text.
	 */
	while (*rdata && isspace(*rdata))
		rdata++;

	/*
	 * Stash optional text. Must escape it of course.
	 */
	if ((len = strlen(rdata))) {
		char	buf[MAXTMCDPACKET];

		memcpy(buf, rdata, len);

		/*
		 * Ick, Ick, Ick. For non-ssl mode I should have required
		 * that the client side close its output side so that we
		 * could read till EOF. Or, included a record len. As it
		 * stands, fragmentation will cause a large message (like
		 * console output) to not appear all at once. Getting this in
		 * a backwards compatable manner is a pain in the ass. So, I
		 * just bumped the version number. Newer tmcc will close the
		 * output side.
		 *
		 * Note that tmcc version 22 now closes its write side.
		 */
		if (vers >= 22 && tcp && !isssl) {
			char *bp = &buf[len];

			while (len < sizeof(buf)) {
				int cc = READ(sock, bp, sizeof(buf) - len);

				if (cc <= 0)
					break;

				len += cc;
				bp  += cc;
			}
		}

		if ((cp = (char *) malloc((2*len)+1)) == NULL) {
			error("DOBOOTLOG: %s: Out of Memory\n", reqp->nodeid);
			return 1;
		}
		mysql_escape_string(cp, buf, len);

		if (mydb_update("replace into node_bootlogs "
				" (node_id, bootlog, bootlog_timestamp) values "
				" ('%s', '%s', now())",
				reqp->nodeid, cp)) {
			error("DOBOOTLOG: %s: DB Error setting bootlog\n",
			      reqp->nodeid);
			free(cp);
			return 1;

		}
		if (verbose)
		    printf("DOBOOTLOG: %d '%s'\n", len, cp);

		free(cp);
	}
	return 0;
}

/*
 * Tell us about boot problems with a specific error code that we can use
 * in os_setup to figure out what went wrong, and if we should retry.
 */
COMMAND_PROTOTYPE(dobooterrno)
{
	int		myerrno;

	/*
	 * Dig out errno that the node is reporting
	 */
	if (sscanf(rdata, "%d", &myerrno) != 1) {
		error("DOBOOTERRNO: %s: Bad arguments\n", reqp->nodeid);
		return 1;
	}

	/*
	 * and update DB.
	 */
	if (mydb_update("update nodes set boot_errno='%d' "
			"where node_id='%s'",
			myerrno, reqp->nodeid)) {
		error("DOBOOTERRNO: %s: setting boot errno!\n", reqp->nodeid);
		return 1;
	}
	if (verbose)
		info("DOBOOTERRNO: errno=%d\n", myerrno);

	return 0;
}

/*
 * Tell us about battery statistics.
 */
COMMAND_PROTOTYPE(dobattery)
{
	float		capacity = 0.0, voltage = 0.0;
	char		buf[MYBUFSIZE];

	/*
	 * Dig out the capacity and voltage, then
	 */
	if ((sscanf(rdata,
		    "CAPACITY=%f VOLTAGE=%f",
		    &capacity,
		    &voltage) != 2) ||
	    (capacity < 0.0f) || (capacity > 100.0f) ||
	    (voltage < 5.0f) || (voltage > 15.0f)) {
		error("DOBATTERY: %s: Bad arguments\n", reqp->nodeid);
		return 1;
	}

	/*
	 * ... update DB.
	 */
	if (mydb_update("UPDATE nodes SET battery_percentage=%f,"
			"battery_voltage=%f,"
			"battery_timestamp=UNIX_TIMESTAMP(now()) "
			"WHERE node_id='%s'",
			capacity, voltage, reqp->nodeid)) {
		error("DOBATTERY: %s: setting boot errno!\n", reqp->nodeid);
		return 1;
	}
	if (verbose) {
		info("DOBATTERY: capacity=%.2f voltage=%.2f\n",
		     capacity,
		     voltage);
	}

	OUTPUT(buf, sizeof(buf), "OK\n");
	client_writeback(sock, buf, strlen(buf), tcp);

	return 0;
}

/*
 * Spit back the topomap. This is a backup for when NFS fails.
 * We send back the gzipped version.
 */
COMMAND_PROTOTYPE(dotopomap)
{
	FILE		*fp;
	char		buf[MYBUFSIZE];
	int		cc;

	/*
	 * Open up the file on boss and spit it back.
	 */
	sprintf(buf, "%s/expwork/%s/%s/topomap.gz", TBROOT,
		reqp->pid, reqp->eid);

	if ((fp = fopen(buf, "r")) == NULL) {
		errorc("DOTOPOMAP: Could not open topomap for %s",
		       reqp->nodeid);
		return 1;
	}

	while (1) {
		cc = fread(buf, sizeof(char), sizeof(buf), fp);
		if (cc == 0) {
			if (ferror(fp)) {
				fclose(fp);
				return 1;
			}
			break;
		}
		client_writeback(sock, buf, cc, tcp);
	}
	fclose(fp);
	return 0;
}

/*
 * Spit back the ltmap. This is a backup for when NFS fails.
 * We send back the gzipped version.
 */
COMMAND_PROTOTYPE(doltmap)
{
	FILE		*fp;
	char		buf[MYBUFSIZE];
	int		cc;

	/*
	 * Open up the file on boss and spit it back.
	 */
	sprintf(buf, "%s/expwork/%s/%s/ltmap.gz", TBROOT,
		reqp->pid, reqp->eid);

	if ((fp = fopen(buf, "r")) == NULL) {
		errorc("DOLTMAP: Could not open ltmap for %s",
		       reqp->nodeid);
		return 1;
	}

	while (1) {
		cc = fread(buf, sizeof(char), sizeof(buf), fp);
		if (cc == 0) {
			if (ferror(fp)) {
				fclose(fp);
				return 1;
			}
			break;
		}
		client_writeback(sock, buf, cc, tcp);
	}
	fclose(fp);
	return 0;
}

/*
 * Spit back the ltpmap. This is a backup for when NFS fails.
 * We send back the gzipped version.  Note that it is ok if this
 * file does not exist.
 */
COMMAND_PROTOTYPE(doltpmap)
{
	FILE		*fp;
	char		buf[MYBUFSIZE];
	int		cc;

	/*
	 * Open up the file on boss and spit it back.
	 */
	sprintf(buf, "%s/expwork/%s/%s/ltpmap.gz", TBROOT,
		reqp->pid, reqp->eid);

	if ((fp = fopen(buf, "r")) == NULL)
		return 0;

	while (1) {
		cc = fread(buf, sizeof(char), sizeof(buf), fp);
		if (cc == 0) {
			if (ferror(fp)) {
				fclose(fp);
				return 1;
			}
			break;
		}
		client_writeback(sock, buf, cc, tcp);
	}
	fclose(fp);
	return 0;
}

/*
 * Return user environment.
 */
COMMAND_PROTOTYPE(douserenv)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
	int		nrows;

	res = mydb_query("select name,value from virt_user_environment "
			 "where pid='%s' and eid='%s' order by idx",
			 2, reqp->pid, reqp->eid);

	if (!res) {
		error("USERENV: %s: DB Error getting virt_user_environment\n",
		      reqp->nodeid);
		return 1;
	}
	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}

	while (nrows) {
		char	*bufp = buf, *ebufp = &buf[sizeof(buf)];

		row = mysql_fetch_row(res);

		bufp += OUTPUT(bufp, ebufp - bufp, "%s=%s\n", row[0], row[1]);
		client_writeback(sock, buf, strlen(buf), tcp);

		nrows--;
		if (verbose)
			info("USERENV: %s", buf);
	}
	mysql_free_result(res);
	return 0;
}

/*
 * Return tip tunnels for the node.
 */
COMMAND_PROTOTYPE(dotiptunnels)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
	int		nrows;

	res = mydb_query("select vtt.vnode,tl.server,tl.portnum,tl.keylen,"
			 "tl.keydata "
			 "from virt_tiptunnels as vtt "
			 "left join reserved as r on r.vname=vtt.vnode and "
			 "  r.pid=vtt.pid and r.eid=vtt.eid "
			 "left join tiplines as tl on tl.node_id=r.node_id "
			 "where vtt.pid='%s' and vtt.eid='%s' and "
			 "vtt.host='%s'",
			 5, reqp->pid, reqp->eid, reqp->nickname);

	if (!res) {
		error("TIPTUNNELS: %s: DB Error getting virt_tiptunnels\n",
		      reqp->nodeid);
		return 1;
	}
	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}

	while (nrows) {
		row = mysql_fetch_row(res);

		if (row[1]) {
			OUTPUT(buf, sizeof(buf),
			       "VNODE=%s SERVER=%s PORT=%s KEYLEN=%s KEY=%s\n",
			       row[0], row[1], row[2], row[3], row[4]);
			client_writeback(sock, buf, strlen(buf), tcp);
		}

		nrows--;
		if (verbose)
			info("TIPTUNNELS: %s", buf);
	}
	mysql_free_result(res);
	return 0;
}

COMMAND_PROTOTYPE(dorelayconfig)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
	int		nrows;

	res = mydb_query("select tl.server,tl.portnum from tiplines as tl "
			 "where tl.node_id='%s'",
			 2, reqp->nodeid);

	if (!res) {
		error("RELAYCONFIG: %s: DB Error getting relay config\n",
		      reqp->nodeid);
		return 1;
	}
	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}

	while (nrows) {
		row = mysql_fetch_row(res);

		OUTPUT(buf, sizeof(buf),
		       "TYPE=%s\n"
		       "CAPSERVER=%s\n"
		       "CAPPORT=%s\n",
		       reqp->type, row[0], row[1]);
		client_writeback(sock, buf, strlen(buf), tcp);

		nrows--;
		if (verbose)
			info("RELAYCONFIG: %s", buf);
	}
	mysql_free_result(res);
	return 0;
}

/*
 * Return trace config
 */
COMMAND_PROTOTYPE(dotraceconfig)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[2*MYBUFSIZE], *ebufp = &buf[sizeof(buf)];
	int		nrows;

	/*
	 * Get delay parameters for the machine. The point of this silly
	 * join is to get the type out so that we can pass it back. Of
	 * course, this assumes that the type is the BSD name, not linux.
	 */
	if (! reqp->isvnode) {
	  res = mydb_query("select linkvname,i0.MAC,i1.MAC,t.vnode,i2.MAC, "
			   "       t.trace_type,t.trace_expr,t.trace_snaplen, "
			   "       t.idx "
			   "from traces as t "
			   "left join interfaces as i0 on "
			   " i0.node_id=t.node_id and i0.iface=t.iface0 "
			   "left join interfaces as i1 on "
			   " i1.node_id=t.node_id and i1.iface=t.iface1 "
			   "left join reserved as r on r.vname=t.vnode and "
			   "     r.pid='%s' and r.eid='%s' "
			   "left join virt_lans as v on v.vname=t.linkvname  "
			   " and v.pid=r.pid and v.eid=r.eid and "
			   "     v.vnode=t.vnode "
			   "left join interfaces as i2 on "
			   "     i2.node_id=r.node_id and i2.IP=v.ip "
			   " where t.node_id='%s'",
			   9, reqp->pid, reqp->eid, reqp->nodeid);
	}
	else {
	  res = mydb_query("select linkvname,i0.mac,i1.mac,t.vnode,'', "
			   "       t.trace_type,t.trace_expr,t.trace_snaplen, "
			   "       t.idx "
			   "from traces as t "
			   "left join vinterfaces as i0 on "
			   " i0.vnode_id=t.node_id and "
			   " i0.unit=SUBSTRING(t.iface0, 5) "
			   "left join vinterfaces as i1 on "
			   " i1.vnode_id=t.node_id and "
			   " i1.unit=SUBSTRING(t.iface1, 5) "
			   "left join reserved as r on r.vname=t.vnode and "
			   "     r.pid='%s' and r.eid='%s' "
			   "left join virt_lans as v on v.vname=t.linkvname  "
			   " and v.pid=r.pid and v.eid=r.eid and "
			   "     v.vnode=t.vnode "
			   "left join interfaces as i2 on "
			   "     i2.node_id=r.node_id and i2.IP=v.ip "
			   " where t.node_id='%s'",
			   9, reqp->pid, reqp->eid, reqp->nodeid);
	}

	if (!res) {
		error("TRACEINFO: %s: DB Error getting trace table!\n",
		      reqp->nodeid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}
	while (nrows) {
		char	*bufp = buf;
		int     idx;

		row = mysql_fetch_row(res);

		/*
		 * XXX plab hack: add the vnode number to the idx to
		 * prevent vnodes on the same pnode from using the same
		 * port number!
		 */
		idx = atoi(row[8]);
		if (reqp->isplabdslice &&
		    strncmp(reqp->nodeid, "plabvm", 6) == 0) {
		    char *cp = index(reqp->nodeid, '-');
		    if (cp && *(cp+1))
			idx += (atoi(cp+1) * 10);
		}

		bufp += OUTPUT(bufp, ebufp - bufp,
			       "TRACE LINKNAME=%s IDX=%d MAC0=%s MAC1=%s "
			       "VNODE=%s VNODE_MAC=%s "
			       "TRACE_TYPE=%s TRACE_EXPR='%s' "
			       "TRACE_SNAPLEN=%s\n",
			       row[0], idx,
			       (row[1] ? row[1] : ""),
			       (row[2] ? row[2] : ""),
			       row[3], row[4],
			       row[5], row[6], row[7]);

		client_writeback(sock, buf, strlen(buf), tcp);
		nrows--;
		if (verbose)
			info("TRACEINFO: %s", buf);
	}
	mysql_free_result(res);

	return 0;
}

/*
 * Acquire plab node elvind port update.
 *
 * Since there is (currently) no way to hard reserve a port on a plab node
 * we might bind to a non-default port, and so must track this so that
 * client vservers on the plab node know where to connect.
 *
 * XXX: should make sure it's the service slice reporting this.
 */
COMMAND_PROTOTYPE(doelvindport)
{
	char		buf[MYBUFSIZE];
	unsigned int	elvport = 0;

	if (sscanf(rdata, "%u",
		   &elvport) != 1) {
		strncpy(buf, rdata, 64);
		error("ELVIND_PORT: %s: Bad arguments: %s...\n", reqp->nodeid,
                      buf);
		return 1;
	}

	/*
         * Now shove the elvin port # we got into the db.
	 */
	mydb_update("replace into node_attributes "
                    " values ('%s', '%s', %u)",
		    reqp->pnodeid, "elvind_port", elvport);

	return 0;
}

/*
 * Return all event keys on plab node to service slice.
 */
COMMAND_PROTOTYPE(doplabeventkeys)
{
	char		buf[MYBUFSIZE];
        int             nrows = 0;
	MYSQL_RES	*res;
	MYSQL_ROW	row;

        if (!reqp->isplabsvc) {
                error("PLABEVENTKEYS: Unauthorized request from node: %s\n",
                      reqp->vnodeid);
                return 1;
        }

        res = mydb_query("select e.pid, e.eid, e.eventkey from reserved as r "
                         " left join nodes as n on r.node_id = n.node_id "
                         " left join experiments as e on r.pid = e.pid "
                         "  and r.eid = e.eid "
                         " where n.phys_nodeid = '%s' ",
                         3, reqp->pnodeid);

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}

	while (nrows) {
		row = mysql_fetch_row(res);

		OUTPUT(buf, sizeof(buf),
                       "PID=%s EID=%s KEY=%s\n",
                       row[0], row[1], row[2]);

		client_writeback(sock, buf, strlen(buf), tcp);
		nrows--;
		if (verbose)
			info("PLABEVENTKEYS: %s\n", buf);
	}
	mysql_free_result(res);

	return 0;
}

/*
 * Return a map of pc node id's with their interface MAC addresses.
 */
COMMAND_PROTOTYPE(dointfcmap)
{
	char		buf[MYBUFSIZE] = {0}, pc[8] = {0};
        int             nrows = 0, npcs = 0;
	MYSQL_RES	*res;
	MYSQL_ROW	row;

        res = mydb_query("select node_id, mac from interfaces "
			 "where node_id like 'pc%%' order by node_id",
                         2);

	nrows = (int)mysql_num_rows(res);
	if (verbose)
		info("intfcmap: nrows %d\n", nrows);
	if (nrows == 0) {
		mysql_free_result(res);
		return 0;
	}

	while (nrows) {
		row = mysql_fetch_row(res);
		/* if (verbose) info("intfcmap: %s %s\n", row[0], row[1]); */

		/* Consolidate interfaces on the same pc into a single line. */
		if (pc[0] == '\0') {
			/* First pc. */
			strncpy(pc, row[0], 8);
			snprintf(buf, MYBUFSIZE, "%s %s", row[0], row[1]);
		} else if (strncmp(pc, row[0], 8) == 0 ) {
			/* Same pc, append. */
			strcat(buf, " ");
			strcat(buf, row[1]);
		} else {
			/* Different pc, dump this one and start the next. */
			strcat(buf, "\n");
			client_writeback(sock, buf, strlen(buf), tcp);
			npcs++;

			strncpy(pc, row[0], 8);
			snprintf(buf, MYBUFSIZE, "%s %s", row[0], row[1]);
		}
		nrows--;
	}
	strcat(buf, "\n");
	client_writeback(sock, buf, strlen(buf), tcp); /* Dump the last one. */
	npcs++;
	if (verbose)
		info("intfcmap: npcs %d\n", npcs);

	mysql_free_result(res);

	return 0;
}


/*
 * Return motelog info for this node.
 */
COMMAND_PROTOTYPE(domotelog)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    char buf[MYBUFSIZE];
    int nrows;

    res = mydb_query("select vnm.logfileid,ml.classfilepath,ml.specfilepath "
		     "from virt_node_motelog as vnm "
		     "left join motelogfiles as ml on vnm.pid=ml.pid "
		     "  and vnm.logfileid=ml.logfileid "
		     "left join reserved as r on r.vname=vnm.vname "
		     "  and vnm.eid=r.eid and vnm.pid=r.pid "
		     "where vnm.pid='%s' and vnm.eid='%s' "
		     "  and vnm.vname='%s'",
		     3,reqp->pid,reqp->eid,reqp->nickname);

    if (!res) {
	error("MOTELOG: %s: DB Error getting virt_node_motelog\n",
	      reqp->nodeid);
    }

    /* no motelog stuff for this node */
    if ((nrows = (int)mysql_num_rows(res)) == 0) {
	mysql_free_result(res);
	return 0;
    }

    while (nrows) {
	row = mysql_fetch_row(res);

	/* only specfilepath can possibly be null */
	OUTPUT(buf, sizeof(buf),
	       "MOTELOGID=%s CLASSFILE=%s SPECFILE=%s\n",
	       row[0],row[1],row[2]);
	client_writeback(sock, buf, strlen(buf), tcp);

	--nrows;
	if (verbose) {
	    info("MOTELOG: %s", buf);
	}
    }

    mysql_free_result(res);
    return 0;
}

/*
 * Return motelog info for this node.
 */
COMMAND_PROTOTYPE(doportregister)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE], service[128];
	int		rc, port;

	/*
	 * Dig out the service and the port number.
	 * Need to be careful about not overflowing the buffer.
	 */
	sprintf(buf, "%%%ds %%d", (int)sizeof(service));
	rc = sscanf(rdata, buf, service, &port);

	if (rc == 0) {
		error("doportregister: No service specified!\n");
		return 1;
	}
	if (rc != 1 && rc != 2) {
		error("doportregister: Wrong number of arguments!\n");
		return 1;
	}

	/* No special characters means it will fit */
	mysql_escape_string(buf, service, strlen(service));
	if (strlen(buf) >= sizeof(service)) {
		error("doportregister: Illegal chars in service!\n");
		return 1;
	}
	strcpy(service, buf);

	/*
	 * Single argument, lookup up service.
	 */
	if (rc == 1) {
		res = mydb_query("select port,node_id "
				 "   from port_registration "
				 "where pid='%s' and eid='%s' and "
				 "      service='%s'",
				 2, reqp->pid, reqp->eid, service);

		if (!res) {
			error("doportregister: %s: "
			      "DB Error getting registration for %s\n",
			      reqp->nodeid, service);
			return 1;
		}
		if ((int)mysql_num_rows(res) > 0) {
			row = mysql_fetch_row(res);
			OUTPUT(buf, sizeof(buf), "PORT=%s NODEID=%s.%s\n",
			       row[0], row[1], OURDOMAIN);
			client_writeback(sock, buf, strlen(buf), tcp);
			if (verbose)
				info("PORTREG: %s: %s", reqp->nodeid, buf);
		}
		mysql_free_result(res);
		return 0;
	}

	/*
	 * If port is zero, clear it from the DB
	 */
	if (port == 0) {
		mydb_update("delete from port_registration  "
			    "where pid='%s' and eid='%s' and "
			    "      service='%s'",
			    reqp->pid, reqp->eid, service);
	}
	else {
		/*
		 * Register port for the service.
		 */
		if (mydb_update("replace into port_registration set "
				"     pid='%s', eid='%s', exptidx=%d, "
				"     service='%s', node_id='%s', port='%d'",
				reqp->pid, reqp->eid, reqp->exptidx,
				service, reqp->nodeid, port)) {
			error("doportregister: %s: DB Error setting %s=%d!\n",
			      reqp->nodeid, service, port);
			return 1;
		}
	}
	if (verbose)
		info("PORTREG: %s: %s=%d\n", reqp->nodeid, service, port);

	return 0;
}

/*
 * Return dhcpd configuration as a set of key-value pairs, one node
 * per line.
 */
COMMAND_PROTOTYPE(dodhcpdconf)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	int nodes, nrows, remain, rc;
	char buf[MYBUFSIZE];
	char *b;

	res = mydb_query("select erole from reserved where node_id = '%s' and erole = 'subboss'", 1,
	                 reqp->nodeid);

	if (!res) {
		error("dodhcpconf: %s: "
		      "DB Error checking for %s in subbosses table\n",
		      reqp->nodeid);
		return 1;
	}

	/* node isn't a subboss, so nothing to return */
	if (mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return 0;
	}
	mysql_free_result(res);

	res = mydb_query("select n.node_id,n.pxe_boot_path,i.IP,i.mac,n.type,r.eid,r.pid,"
			 "r.inner_elab_role,r.inner_elab_boot,r.plab_role,r.plab_boot,"
			 "n.next_pxe_boot_path "
			 "from nodes as n "
			 "left join subbosses as s on n.node_id = s.node_id "
			 "left join interfaces as i on n.node_id = i.node_id "
			 "left join reserved as r on n.node_id = r.node_id "
			 "where s.subboss_id = '%s' and "
	                 "s.service='dhcp' and i.role='ctrl' "
			 "order by n.priority", 12, reqp->nodeid);

	if (!res) {
		error("dodhcpconf: %s: "
		      "DB Error getting dhcpd configuration for %s\n",
		      reqp->nodeid);
		return 1;
	}

	nodes = (int)mysql_num_rows(res);
	while (nodes > 0) {
		char tftp_server[16];
		char bootinfo_server[16];
		int elabinelab_singlenet = 0;
		int inner_elab_boot = 0;
		int plab_boot = 0;
		MYSQL_RES *res2;

		memset(tftp_server, 0, sizeof(tftp_server));
		memset(bootinfo_server, 0, sizeof(bootinfo_server));

		rc = 0;

		b = buf;
		remain = sizeof(buf);

		row = mysql_fetch_row(res);
		rc = snprintf(b, remain, "HOSTNAME=%s MAC=%s IP=%s", row[0], row[3], row[2]);

		if (rc < 0) {
			error("dodhcpdconf: error creating output\n");
			return 1;
		}

		b += rc;
		remain -= rc;

		/* Check to see if this is an inner elab node or a plab node.
		 * if so, use the boss/boss+router/plc for this experiment as
		 * the boot server.
		 */
		if (row[8]) {
			inner_elab_boot = atoi(row[8]);
		}
		if (row[10]) {
			plab_boot = atoi(row[10]);
		}

		if (inner_elab_boot) {
			res2 = mydb_query("select elabinelab_singlenet from experiments where "
					  "eid = '%s' and pid = '%s'", 1, row[5], row[6]);

			if (!res2) {
				error("dodhcpconf: %s: "
				      "DB Error getting experiment info for %s:%s\n",
				      row[6], row[5]);
				return 1;
			}

			if (mysql_num_rows(res2)) {
				MYSQL_ROW row2 = mysql_fetch_row(res2);
				elabinelab_singlenet = atoi(row2[0]);
			}
			mysql_free_result(res2);

		}
		if ((inner_elab_boot && row[7] && !strcmp(row[7], "node")) ||
		    (plab_boot && row[9] && !strcmp(row[9], "node"))) {
			res2 = mydb_query("select i.IP from reserved as r, interfaces as i "
					  "where r.node_id = i.node_id and "
					  "r.eid = '%s' and r.pid = '%s' and "
					  "(r.inner_elab_role = 'boss' or "
					  "r.inner_elab_role = 'boss+router' or "
					  "r.inner_elab_role = 'boss+fs+router' or "
					  "r.plab_role='plc') and i.role='ctrl'", 1,
					  row[5], row[6]);

			if (!res2) {
				error("dodhcpconf: %s: "
				      "DB Error getting experiment info for %s:%s\n",
				      row[6], row[5]);
				return 1;
			}

			if (mysql_num_rows(res2)) {
				MYSQL_ROW row2 = mysql_fetch_row(res2);
				strlcpy(tftp_server, row2[0], sizeof(tftp_server));
				/* XXX should this server do bootinfo as well? */
				strlcpy(bootinfo_server, row2[0], sizeof(bootinfo_server));
			}
			mysql_free_result(res2);
		}


		res2 = mydb_query("select s.subboss_id,s.service,i.IP from subbosses as s, "
		                  "interfaces as i where s.node_id = '%s' and s.service != 'dhcp' "
		                  "and s.subboss_id = i.node_id and i.role = 'ctrl'", 3,
		                  row[0]);
		if (!res) {
			error("dodhcpconf: %s: "
			      "DB Error getting subbosses for %s\n",
			      row[0]);
			return 1;
		}

		nrows = (int)mysql_num_rows(res2);
		while (nrows > 0) {
			MYSQL_ROW row2;

			row2 = mysql_fetch_row(res2);
			if (strcmp(row2[1], "tftp") == 0) {
				if (tftp_server[0] == '\0') {
					strlcpy(tftp_server, row2[2], sizeof(tftp_server));
				}
			} else if (strcmp(row2[1], "bootinfo") == 0) {
				if (bootinfo_server[0] == '\0') {
					strlcpy(bootinfo_server, row2[2], sizeof(bootinfo_server));
				}
			}

			if (rc < 0) {
				error("dodhcpdconf: error creating output\n");
				return 1;
			}

			nrows--;
		}

		mysql_free_result(res2);

		if (inner_elab_boot) {
			rc = snprintf(b, remain, " INNER_ELAB_BOOT=1");
			b += rc;
			remain -= rc;
			if (elabinelab_singlenet) {
				rc = snprintf(b, remain, " SINGLENET=1");
				b += rc;
				remain -= rc;
			}
		} else if (plab_boot) {
			rc = snprintf(b, remain, " PLAB_BOOT=1");
			b += rc;
			remain -= rc;
		}

		if (tftp_server[0]) {
			rc = snprintf(b, remain, " TFTP=%s", tftp_server);
			b += rc;
			remain -= rc;
		}

		if (bootinfo_server[0]) {
			rc = snprintf(b, remain, " BOOTINFO=%s", bootinfo_server);
			b += rc;
			remain -= rc;
		}

		if (row[11] && row[11][0]) {
			rc = snprintf(b, remain, " FILENAME=\"%s\"", row[11]);

			if (rc < 0) {
				error("dodhcpdconf: error creating output\n");
				return 1;
			}

			b += rc;
			remain -= rc;
		} else if (row[1] && row[1][0]) {
			rc = snprintf(b, remain, " FILENAME=\"%s\"", row[1]);

			if (rc < 0) {
				error("dodhcpdconf: error creating output\n");
				return 1;
			}

			b += rc;
			remain -= rc;
		} else {
			MYSQL_ROW row2;
			res2 = mydb_query("select attrvalue from node_type_attributes where "
			                  "attrkey = 'pxe_boot_path' and type = '%s'", 1, row[4]);

			if (!res2) {
				error("dodhcpconf: %s: "
				      "DB Error getting pxe_boot_path for %s\n",
				      row[0]);
				return 1;
			}


			if ((int)mysql_num_rows(res2)) {
				row2 = mysql_fetch_row(res2);
				rc = 0;

				if (row2[0]!= NULL) {
					rc = snprintf(b, remain, " FILENAME=\"%s\"", row2[0]);
				}

				if (rc < 0) {
					error("dodhcpdconf: error creating output\n");
					return 1;
				}

				b += rc;
				remain -= rc;
			}

			mysql_free_result(res2);
		}

		snprintf(b, remain, "\n");

		client_writeback(sock, buf, strlen(buf), tcp);
		if (verbose)
			info("DHCPDCONF: %s: %s", reqp->nodeid, buf);

		nodes--;
	}

	mysql_free_result(res);
	return 0;
}

/*
 * Return bootwhat info using the bootinfo routine from the pxe directory.
 * This is used by the dongle boot on widearea nodes. Better to use tmcd
 * then open up another daemon to the widearea.
 */
COMMAND_PROTOTYPE(dobootwhat)
{
	boot_info_t	boot_info;
	boot_what_t	*boot_whatp = (boot_what_t *) &boot_info.data;
	char		buf[MYBUFSIZE];
	char		*bufp = buf, *ebufp = &buf[sizeof(buf)];

	boot_info.opcode  = BIOPCODE_BOOTWHAT_REQUEST;
	boot_info.version = BIVERSION_CURRENT;

	if(strlen(reqp->privkey) > 1) { /* We have a private key, so prepare bootinfo for it. */
		boot_info.opcode = BIOPCODE_BOOTWHAT_KEYED_REQUEST;
		strncpy(boot_info.data, reqp->privkey, PRIVKEY_LEN);
	}

	if (bootinfo(reqp->client, (reqp->isvnode) ? reqp->nodeid : NULL,
		     &boot_info, (void *) reqp, (reqp->isvnode) ? 1 : 0)) {
		OUTPUT(buf, sizeof(buf), "STATUS=failed\n");
	}
	else {
		bufp += OUTPUT(bufp, ebufp - bufp,
			      "STATUS=success TYPE=%d",
			      boot_whatp->type);
		if (boot_whatp->type == BIBOOTWHAT_TYPE_PART) {
			bufp += OUTPUT(bufp, ebufp - bufp, " WHAT=%d",
				      boot_whatp->what.partition);
		}
		else if (boot_whatp->type == BIBOOTWHAT_TYPE_SYSID) {
			bufp += OUTPUT(bufp, ebufp - bufp, " WHAT=%d",
				      boot_whatp->what.sysid);
		}
		else if (boot_whatp->type == BIBOOTWHAT_TYPE_MFS) {
			bufp += OUTPUT(bufp, ebufp - bufp, " WHAT=%s",
				      boot_whatp->what.mfs);
		}
		if (strlen(boot_whatp->cmdline)) {
			bufp += OUTPUT(bufp, ebufp - bufp, " CMDLINE='%s'",
				      boot_whatp->cmdline);
		}
		bufp += OUTPUT(bufp, ebufp - bufp, "\n");
	}
	client_writeback(sock, buf, strlen(buf), tcp);

	info("BOOTWHAT: %s: %s\n", reqp->nodeid, buf);
	return 0;
}

COMMAND_PROTOTYPE(dotpmblob)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	int		nrows,i;
	unsigned long	*nlen;
	int		hex = 1;
	char		buf[MYBUFSIZE];
	char		*bufp = buf;
	char		*bufe = &buf[MYBUFSIZE];

	if (rdata)
		if (strncasecmp(rdata, "nohex", strlen("nohex")) == 0)
			hex = 0;

	/*
	 * Get the blob
	 */
	res = mydb_query("select tpmblob "
			"from node_hostkeys "
			"where node_id='%s' ",
			1, reqp->nodeid);

	if (!res) {
		error("gettpmblob: %s: DB error getting tpmblob\n",
		      reqp->nodeid);
		return 1;
	}

	nrows = mysql_num_rows(res);
	if (!nrows) {
		error("%s: no node_hostkeys info in the database!\n",
			reqp->nodeid);
		mysql_free_result(res);
		return 1;
	}

	row = mysql_fetch_row(res);
	nlen = mysql_fetch_lengths(res);
	if (!nlen || !nlen[0]) {
		mysql_free_result(res);
#if 0 /* not an error yet */
		error("%s: no TPM blob.\n", reqp->nodeid);
		return 1;
#endif
		return 0;
	}

	bufp += OUTPUT(bufp, bufe - bufp,
		       (hex ? "BLOBHEX=" : "BLOB="));
	if (hex) {
		for (i = 0;i < nlen[0];++i)
			bufp += OUTPUT(bufp, bufe - bufp,
				       "%.02x", (0xff & ((char)*(row[0]+i))));
	} else {
		for (i = 0;i < nlen[0];++i)
			bufp += OUTPUT(bufp, bufe - bufp,
				       "%c", (char)*(row[0]+i));
	}
	bufp += OUTPUT(bufp, bufe - bufp, "\n");

	client_writeback(sock, buf, bufp - buf, tcp);

	mysql_free_result(res);
	return 0;
}

COMMAND_PROTOTYPE(dotpmpubkey)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	int		nrows;
	char		buf[MYBUFSIZE];

	res = mydb_query("select tpmx509 "
			"from node_hostkeys "
			"where node_id='%s' ",
			1, reqp->nodeid);
	if (!res) {
		error("gettpmpub: %s: DB error getting tpmx509\n",
			reqp->nodeid);
		return 1;
	}
	nrows = mysql_num_rows(res);
	if (!nrows) {
		error("%s: no node_hostkeys info in the database!\n",
			reqp->nodeid);
		mysql_free_result(res);
		return 1;
	}

	row = mysql_fetch_row(res);
	if (!row || !row[0]) {
		mysql_free_result(res);
#if 0 /* not an error yet */
		error("%s: no x509 cert.\n", reqp->nodeid);
		return 1;
#endif
		return 0;
	}

	OUTPUT(buf, sizeof(buf), "TPMPUB=%s\n", row[0]);

	client_writeback(sock, buf, strlen(buf), tcp);

	mysql_free_result(res);
	return 0;
}

COMMAND_PROTOTYPE(dotpmdummy)
{
	char buf[MYBUFSIZE];

	OUTPUT(buf, sizeof(buf),
	    "You'd better be using the TPM, you twicky wabbit!\n");

	client_writeback(sock, buf, strlen(buf), tcp);

	return 0;
}
