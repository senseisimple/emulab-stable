/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
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
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/fcntl.h>
#include <paths.h>
#include <setjmp.h>
#include <mysql/mysql.h>
#include "decls.h"
#include "config.h"
#include "ssl.h"
#include "log.h"
#include "tbdefs.h"

#ifdef EVENTSYS
#include "event.h"
#endif

/*
 * XXX This needs to be localized!
 */
#define FSPROJDIR	FSNODE ":" FSDIR_PROJ
#define FSGROUPDIR	FSNODE ":" FSDIR_GROUPS
#define FSUSERDIR	FSNODE ":" FSDIR_USERS
#define PROJDIR		"/proj"
#define GROUPDIR	"/groups"
#define USERDIR		"/users"
#define RELOADPID	"emulab-ops"
#define RELOADEID	"reloading"
#define FSHOSTID	"/usr/testbed/etc/fshostid"
//#define FSHOSTID	"./fshostid"

#define TESTMODE
#define NETMASK		"255.255.255.0"

/* Defined in configure and passed in via the makefile */
#define DBNAME_SIZE	64
#define HOSTID_SIZE	(32+64)
#define DEFAULT_DBNAME	TBDBNAME

int		debug = 0;
static int	insecure = 0;
static int	portnum = TBSERVER_PORT;
static char     dbname[DBNAME_SIZE];
static struct in_addr myipaddr;
static char	fshostid[HOSTID_SIZE];
static int	nodeidtoexp(char *nodeid, char *pid, char *eid, char *gid);
static int	iptonodeid(struct in_addr, char *,char *,char *,char *,int *);
static int	nodeidtonickname(char *nodeid, char *nickname);
static int	nodeidtocontrolnet(char *nodeid, int *net);
static int	checkdbredirect(char *nodeid);
static void	tcpserver(int sock);
static void	udpserver(int sock);
static int      handle_request(int, struct sockaddr_in *, char *, int);
int		client_writeback(int sock, void *buf, int len, int tcp);
void		client_writeback_done(int sock, struct sockaddr_in *client);
MYSQL_RES *	mydb_query(char *query, int ncols, ...);
int		mydb_update(char *query, ...);

/* thread support */
#define MAXCHILDREN	25
#define MINCHILDREN	5
static int	udpchild;
static int	numchildren;
static int	maxchildren = 15;
static volatile int killme;

#ifdef EVENTSYS
int			myevent_send(address_tuple_t address);
static event_handle_t	event_handle = NULL;
#endif

/*
 * Commands we support.
 */
#define COMMAND_PROTOTYPE(x) \
	static int \
	x(int sock, char *nodeid, char *rdata, int tcp, \
	  int islocal, char *nodetype, int vers)

COMMAND_PROTOTYPE(doreboot);
COMMAND_PROTOTYPE(dostatus);
COMMAND_PROTOTYPE(doifconfig);
COMMAND_PROTOTYPE(doaccounts);
COMMAND_PROTOTYPE(dodelay);
COMMAND_PROTOTYPE(dohosts);
COMMAND_PROTOTYPE(dohostsV2);
COMMAND_PROTOTYPE(dorpms);
COMMAND_PROTOTYPE(dodeltas);
COMMAND_PROTOTYPE(dotarballs);
COMMAND_PROTOTYPE(dostartcmd);
COMMAND_PROTOTYPE(dostartstat);
COMMAND_PROTOTYPE(doready);
COMMAND_PROTOTYPE(doreadycount);
COMMAND_PROTOTYPE(dolog);
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
COMMAND_PROTOTYPE(doisalive);
COMMAND_PROTOTYPE(doipodinfo);

struct command {
	char	*cmdname;
	int    (*func)(int, char *, char *, int, int, char *, int);
} command_array[] = {
	{ "reboot",	doreboot },
	{ "status",	dostatus },
	{ "ifconfig",	doifconfig },
	{ "accounts",	doaccounts },
	{ "delay",	dodelay },
	{ "hostnamesV2",dohostsV2 },	/* This will go away */
	{ "hostnames",	dohosts },
	{ "rpms",	dorpms },
	{ "deltas",	dodeltas },
	{ "tarballs",	dotarballs },
	{ "startupcmd",	dostartcmd },
	{ "startstatus",dostartstat }, /* Leave this before "startstat" */
	{ "startstat",	dostartstat },
	{ "readycount", doreadycount },
	{ "ready",	doready },
	{ "log",	dolog },
	{ "mounts",	domounts },
	{ "sfshostid",	dosfshostid },
	{ "loadinfo",	doloadinfo},
	{ "reset",	doreset},
	{ "routing",	dorouting},
	{ "trafgens",	dotrafgens},
	{ "nseconfigs",	donseconfigs},
	{ "creator",	docreator},
	{ "state",	dostate},
	{ "tunnels",	dotunnels},
	{ "vnodelist",	dovnodelist},
	{ "isalive",	doisalive},
	{ "ipodinfo",	doipodinfo},
};
static int numcommands = sizeof(command_array)/sizeof(struct command);

/* 
 * Simple struct used to make a linked list of ifaces
 */
struct node_interface {
	char *iface;
	struct node_interface *next;
};

int	directly_connected(struct node_interface *interfaces, char *iface);

char *usagestr = 
 "usage: tmcd [-d] [-p #]\n"
 " -d              Turn on debugging. Multiple -d options increase output\n"
 " -p portnum	   Specify a port number to listen on\n"
 " -c num	   Specify number of servers (must be %d <= x <= %d)\n"
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
}

int
main(int argc, char **argv)
{
	int			tcpsock, udpsock, ch;
	int			length, i, status, pid;
	struct sockaddr_in	name;
	FILE			*fp;
	char			buf[BUFSIZ];
	struct hostent		*he;
	extern char		build_info[];

	while ((ch = getopt(argc, argv, "dp:c:X")) != -1)
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
#ifdef LBS
		case 'X':
			insecure = 1;
			break;
#endif
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
		daemon(0, 0);
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
#ifdef	LBS
	strcpy(buf, BOSSNODE);
#else
	if (gethostname(buf, sizeof(buf)) < 0)
		pfatal("getting hostname");
#endif
	if ((he = gethostbyname(buf)) == NULL) {
		error("Could not get IP (%s) - %s\n", buf, hstrerror(h_errno));
		exit(1);
	}
	memcpy((char *)&myipaddr, he->h_addr, he->h_length);

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
	if (listen(tcpsock, 20) < 0) {
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

	signal(SIGTERM, cleanup);
	signal(SIGINT, cleanup);
	signal(SIGHUP, cleanup);

	/*
	 * Stash the pid away.
	 */
	sprintf(buf, "%s/tmcd.pid", _PATH_VARRUN);
	fp = fopen(buf, "w");
	if (fp != NULL) {
		fprintf(fp, "%d\n", getpid());
		(void) fclose(fp);
	}

	/*
	 * Now fork a set of children to handle requests. We keep the
	 * pool at a set level. No need to get too fancy at this point. 
	 */
	while (1) {
		while (!killme && numchildren < maxchildren) {
			int doudp = (udpchild ? 0 : 1);
			if ((pid = fork()) < 0) {
				errorc("forking server");
				goto done;
			}
			if (pid) {
				if (doudp)
					udpchild = pid;
				numchildren++;
				continue;
			}
			/* Child does useful work! Never Returns! */
			signal(SIGTERM, SIG_DFL);
			signal(SIGINT, SIG_DFL);
			signal(SIGHUP, SIG_DFL);
			
			if (doudp) 
				udpserver(udpsock);
			else
				tcpserver(tcpsock);
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
		error("server %d exited with status 0x%x!\n", pid, status);
		numchildren--;
		if (pid == udpchild)
			udpchild = 0;
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
 * Listen for UDP requests. This is not a secure channel, and so this should
 * eventually be killed off.
 */
static void
udpserver(int sock)
{
	char			buf[MYBUFSIZE];
	struct sockaddr_in	client;
	int			length, cc;
	
	info("udpserver starting\n");

	/*
	 * Wait for udp connections.
	 */
	while (1) {
		length = sizeof(client);		
		cc = recvfrom(sock, buf, MYBUFSIZE - 1,
			      0, (struct sockaddr *)&client, &length);
		if (cc <= 0) {
			if (cc < 0)
				errorc("Reading UDP request");
			error("UDP Connection aborted\n");
			continue;
		}
		buf[cc] = '\0';
		handle_request(sock, &client, buf, 0);
	}
	exit(1);
}

/*
 * Listen for TCP requests.
 */
static void
tcpserver(int sock)
{
	char			buf[MYBUFSIZE];
	struct sockaddr_in	client;
	int			length, cc, newsock;
	
	info("tcpserver starting\n");

	/*
	 * Wait for TCP connections.
	 */
	while (1) {
		length  = sizeof(client);
		newsock = ACCEPT(sock, (struct sockaddr *)&client, &length);
		if (newsock < 0) {
			errorc("accepting TCP connection");
			continue;
		}

		/*
		 * Read in the command request.
		 */
		if ((cc = READ(newsock, buf, MYBUFSIZE - 1)) <= 0) {
			if (cc < 0)
				errorc("Reading TCP request");
			error("TCP connection aborted\n");
			CLOSE(newsock);
			continue;
		}
		buf[cc] = '\0';
		handle_request(newsock, &client, buf, 1);
		CLOSE(newsock);
	}
	exit(1);
}

static int
handle_request(int sock, struct sockaddr_in *client, char *rdata, int istcp)
{
	struct sockaddr_in redirect_client;
	int		   redirect = 0, isvnode = 0;
	char		   buf[BUFSIZ], *bp, *cp;
	char		   nodeid[TBDB_FLEN_NODEID];
	char		   vnodeid[TBDB_FLEN_NODEID];
	char		   class[TBDB_FLEN_NODECLASS];
	char		   type[TBDB_FLEN_NODETYPE];
	int		   i, islocal, err = 0;
	int		   version = DEFAULT_VERSION;

	/*
	 * Look for special tags. 
	 */
	bp = rdata;
	while ((bp = strsep(&rdata, " ")) != NULL) {
		/*
		 * Look for VERSION. 
		 */
		if (sscanf(bp, "VERSION=%d", &i) == 1) {
			version = i;
			
			if (debug) {
				info("VERSION %d\n", version);
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
		 * Very temporary approach; should be done via a per-vnode
		 * cert or a key.
		 */
		if (sscanf(bp, "VNODEID=%30s", buf)) {
			isvnode = 1;
			strncpy(vnodeid, buf, sizeof(vnodeid));

			if (debug) {
				info("VNODEID %s\n", buf);
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
	if ((err = iptonodeid(client->sin_addr, (isvnode ? vnodeid : NULL),
			      nodeid, class, type, &islocal))) {
		if (err == 2) {
			error("No such node vnode mapping %s on %s\n",
			      vnodeid, nodeid);
		}
		else {
			error("No such node: %s\n",
			      inet_ntoa(client->sin_addr));
		}
		goto skipit;
	}

	/*
	 * Redirect is allowed from the local host only!
	 * I use this for testing. See below where I test redirect
	 * if the verification fails. 
	 */
	if (!insecure && redirect &&
	    redirect_client.sin_addr.s_addr != myipaddr.s_addr) {
		char	buf1[32], buf2[32];
		
		strcpy(buf1, inet_ntoa(redirect_client.sin_addr));
		strcpy(buf2, inet_ntoa(client->sin_addr));
			
		info("%s INVALID REDIRECT: %s\n", buf1, buf2);
		goto skipit;
	}

#ifdef  WITHSSL
	/*
	 * If the connection is not SSL, then it must be a local node.
	 */
	if (isssl) {
		if (tmcd_sslverify_client(nodeid, class, type, islocal)) {
			error("%s: SSL verification failure\n", nodeid);
			if (! redirect)
				goto skipit;
		}
	}
	else if (!islocal) {
		if (!istcp) {
			/*
			 * Simple "isalive" support for remote nodes.
			 */
			doisalive(sock, nodeid, rdata, istcp,
				  islocal, type, version);
			goto skipit;
		}
		error("%s: Remote node connected without SSL!\n", nodeid);
		if (!insecure)
			goto skipit;
	}
#else
	/*
	 * When not compiled for ssl, do not allow remote connections.
	 */
	if (!islocal) {
		if (!istcp) {
			/*
			 * Simple "isup" daemon support!
			 */
			doisalive(sock, nodeid, rdata, istcp,
				  islocal, type, version);
			goto skipit;
		}
		error("%s: Remote node connected without SSL!\n", nodeid);
		if (!insecure)
			goto skipit;
	}
#endif
	/*
	 * Check for a redirect using the default DB. This allows
	 * for a simple redirect to a secondary DB for testing.
	 * Upon return, the dbname has been changed if redirected.
	 */
	if (checkdbredirect(nodeid)) {
		/* Something went wrong */
		goto skipit;
	}

	/*
	 * Figure out what command was given.
	 */
	for (i = 0; i < numcommands; i++)
		if (strncmp(bp, command_array[i].cmdname,
			    strlen(command_array[i].cmdname)) == 0)
			break;

	if (i == numcommands) {
		info("%s: INVALID REQUEST: %.8s\n", nodeid, bp);
		goto skipit;
	}

	/*
	 * Execute it.
	 */
#ifdef	WITHSSL
	cp = isssl ? "ssl:yes" : "ssl:no";
#else
	cp = "";
#endif
	/*
	 * XXX hack, don't log "log" contents,
	 * both for privacy and to keep our syslog smaller.
	 */
	if (command_array[i].func == dolog)
		info("%s: vers:%d %s %s log %d chars\n", nodeid, version,
		     istcp ? "TCP" : "UDP", cp, strlen(rdata));
	else
		info("%s: vers:%d %s %s %s\n", nodeid, version,
		     istcp ? "TCP" : "UDP", cp, command_array[i].cmdname);

	err = command_array[i].func(sock, nodeid, rdata, istcp,
				    islocal, type, version);

	if (err)
		info("%s: %s: returned %d\n",
		     nodeid, command_array[i].cmdname, err);

 skipit:
	if (!istcp) 
		client_writeback_done(sock,
				      redirect ? &redirect_client : client);
	return 0;
}

/*
 * Accept notification of reboot. 
 */
COMMAND_PROTOTYPE(doreboot)
{
	MYSQL_RES	*res;	
	char		pid[64];
	char		eid[64];
	char		gid[64];

	/*
	 * Clear the current_reloads for this node, in case it just finished
	 * reloading. This needs to happen regardless of whether or not the
	 * node is free or in the reloading experiment.
	 * XXX: Is it better to blindly do the delete (which will be harmless
	 * if there is no entry for this node) or to check first, which
	 * might waste time?
	 */
	info("doreload: %s: Clearing current_reloads\n", nodeid);
	if (mydb_update("delete from current_reloads where node_id='%s'",
		        nodeid)) {
	    error("doreload: %s: DB Error clearing current_reloads!\n",
		  nodeid);
	    return 1;
	}

	/*
	 * Need to check the pid/eid to distinguish between a user
	 * initiated reload, and a admin scheduled reload. We don't want
	 * to deschedule an admin reload, which is supposed to happen when
	 * the node is released.
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid)) {
		info("REBOOT: %s: Node is free\n", nodeid);
		return 0;
	}

	info("REBOOT: %s: Node is in experiment %s/%s\n", nodeid, pid, eid);

	/*
	 * XXX This must match the reservation made in sched_reload
	 *     in the tbsetup directory.
	 */
	if (strcmp(pid, RELOADPID) ||
	    strcmp(eid, RELOADEID)) {
		return 0;
	}

	/*
	 * See if the node was in the reload state. If so we need to clear it
	 * and its reserved status.
	 */
	res = mydb_query("select node_id from scheduled_reloads "
			 "where node_id='%s'",
			 1, nodeid);
	if (!res) {
		error("REBOOT: %s: DB Error getting reload!\n", nodeid);
		return 1;
	}
	if ((int)mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return 0;
	}
	mysql_free_result(res);

	if (mydb_update("delete from scheduled_reloads where node_id='%s'",
			nodeid)) {
		error("REBOOT: %s: DB Error clearing reload!\n", nodeid);
		return 1;
	}
	info("REBOOT: %s cleared reload state\n", nodeid);

	if (mydb_update("delete from reserved where node_id='%s'", nodeid)) {
		error("REBOOT: %s: DB Error clearing reload!\n", nodeid);
		return 1;
	}
	info("REBOOT: %s cleared reserved state\n", nodeid);

	return 0;
}

/*
 * Return status of node. Is it allocated to an experiment, or free.
 */
COMMAND_PROTOTYPE(dostatus)
{
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		nickname[128];
	char		buf[MYBUFSIZE];

	/*
	 * Now check reserved table
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid)) {
		info("STATUS: %s: Node is free\n", nodeid);
		strcpy(buf, "FREE\n");
		client_writeback(sock, buf, strlen(buf), tcp);
		return 0;
	}

	/*
	 * Need the nickname too.
	 */
	if (nodeidtonickname(nodeid, nickname))
		strcpy(nickname, nodeid);

	sprintf(buf, "ALLOCATED=%s/%s NICKNAME=%s\n", pid, eid, nickname);
	client_writeback(sock, buf, strlen(buf), tcp);

	info("STATUS: %s: %s", nodeid, buf);
	return 0;
}

/*
 * Return ifconfig information to client.
 */
COMMAND_PROTOTYPE(doifconfig)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		buf[MYBUFSIZE];
	int		control_net, nrows;

	/*
	 * Now check reserved table
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid)) {
		info("IFCONFIG: %s: Node is free\n", nodeid);
		return 1;
	}

	/*
	 * Need to know the control network for the machine since
	 * we don't want to mess with that.
	 */
	if (nodeidtocontrolnet(nodeid, &control_net)) {
		error("IFCONFIG: %s: No Control Network\n", nodeid);
		return 1;
	}

	/*
	 * Find all the interfaces.
	 */
	res = mydb_query("select card,IP,IPalias,MAC,current_speed,duplex "
			 "from interfaces where node_id='%s'",
			 6, nodeid);
	if (!res) {
		error("IFCONFIG: %s: DB Error getting interfaces!\n", nodeid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		error("IFCONFIG: %s: No interfaces!\n", nodeid);
		mysql_free_result(res);
		return 1;
	}
	while (nrows) {
		row = mysql_fetch_row(res);
		if (row[1] && row[1][0]) {
			int card    = atoi(row[0]);
			char *speed  = "100";
			char *unit   = "Mbps";
			char *duplex = "full";

			/*
			 * INTERFACE can go away when all machines running
			 * updated (MAC based) setup. Right now MAC is at
			 * the end (see below) cause of the sharks, but they
			 * should be dead soon too.
			 */
			sprintf(buf, "INTERFACE=%d INET=%s MASK=%s",
				card, row[1], NETMASK);

			/*
			 * The point of this sillyness is to look for the
			 * special Shark case. The sharks have only one
			 * interface, so we use an IPalias on what we call
			 * the "control" interface. This test prevents us
			 * from returning an ifconfig line for the control
			 * interface, unless it has an IP alias. I could
			 * change the setup scripts to ignore this case on
			 * the PCs. Might end up doing that at some point. 
			 */
			if (row[2] && row[2][0]) {
				strcat(buf, " IPALIAS=");
				strcat(buf, row[2]);
			}
			else if (card == control_net)
				goto skipit;

			/*
			 * Tack on MAC, which should go up above after
			 * Sharks are sunk.
			 */
			strcat(buf, " MAC=");
			strcat(buf, row[3]);

			/*
			 * Tack on speed and duplex. 
			 */
			if (row[4] && row[4][0]) {
				speed = row[4];
			}
			if (row[5] && row[5][0]) {
				duplex = row[5];
			}
			
			sprintf(&buf[strlen(buf)],
				" SPEED=%s%s DUPLEX=%s", speed, unit, duplex);

			strcat(buf, "\n");
			client_writeback(sock, buf, strlen(buf), tcp);
			info("IFCONFIG: %s", buf);
		}
	skipit:
		nrows--;
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
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		buf[MYBUFSIZE];
	int		nrows, gidint;
	int		tbadmin;

	if (! tcp) {
		error("ACCOUNTS: %s: Cannot give account info out over UDP!\n",
		      nodeid);
		return 1;
	}

	/*
	 * Now check reserved table
	 */
	if (islocal && nodeidtoexp(nodeid, pid, eid, gid)) {
		error("ACCOUNTS: %s: Node is free\n", nodeid);
		return 1;
	}

        /*
	 * We need the unix GID and unix name for each group in the project.
	 */
	if (islocal) {
		res = mydb_query("select unix_name,unix_gid from groups "
				 "where pid='%s'",
				 2, pid);
	}
	else {
		/*
		 * XXX
		 * Temporary hack until we figure out the right model for
		 * remote nodes. For now, we use the pcremote-ok slot in
		 * in the project table to determine what remote nodes are
		 * okay'ed for the project. If connecting node type is in
		 * that list, then return all of the project groups, for
		 * each project that is allowed to get accounts on the type.
		 */
		res = mydb_query("select g.unix_name,g.unix_gid "
				 "  from projects as p "
				 "left join groups as g on p.pid=g.pid "
				 "where p.approved!=0 and "
				 "      FIND_IN_SET('%s',pcremote_ok)>0",
				 2, nodetype);
	}
	if (!res) {
		error("ACCOUNTS: %s: DB Error getting gids!\n", pid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		error("ACCOUNTS: %s: No Project!\n", pid);
		mysql_free_result(res);
		return 1;
	}

	while (nrows) {
		row = mysql_fetch_row(res);
		if (!row[1] || !row[1][1]) {
			error("ACCOUNTS: %s: No Project GID!\n", pid);
			mysql_free_result(res);
			return 1;
		}

		gidint = atoi(row[1]);
		sprintf(buf, "ADDGROUP NAME=%s GID=%d\n", row[0], gidint);
		client_writeback(sock, buf, strlen(buf), tcp);
		info("ACCOUNTS: %s", buf);

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
			nodeid)) {
		error("ACCOUNTS: %s: DB Error setting exit update_accounts!\n",
		      nodeid);
	}
			 
	/*
	 * Now onto the users in the project.
	 */
	if (islocal) {
		/*
		 * This crazy join is going to give us multiple lines for
		 * each user that is allowed on the node, where each line
		 * (for each user) differs by the project PID and it unix
		 * GID. The intent is to build up a list of GIDs for each
		 * user to return. Well, a primary group and a list of aux
		 * groups for that user. It might be cleaner to do this as
		 * multiple querys, but this makes it atomic.
		 */
		if (strcmp(pid, gid)) {
			res = mydb_query("select distinct "
			     "  u.uid,u.usr_pswd,u.unix_uid,u.usr_name, "
			     "  p.trust,g.pid,g.gid,g.unix_gid,u.admin, "
			     "  u.emulab_pubkey,u.home_pubkey, "
			     "  UNIX_TIMESTAMP(u.usr_modified) "
			     "from users as u "
			     "left join group_membership as p on p.uid=u.uid "
			     "left join groups as g on p.pid=g.pid "
			     "where ((p.pid='%s' and p.gid='%s')) "
			     "      and p.trust!='none' "
			     "      and u.status='active' order by u.uid",
			     12, pid, gid);
		}
		else {
			res = mydb_query("select distinct "
			     "  u.uid,u.usr_pswd,u.unix_uid,u.usr_name, "
			     "  p.trust,g.pid,g.gid,g.unix_gid,u.admin, "
			     "  u.emulab_pubkey,u.home_pubkey, "
			     "  UNIX_TIMESTAMP(u.usr_modified) "
			     "from users as u "
			     "left join group_membership as p on p.uid=u.uid "
			     "left join groups as g on "
			     "     p.pid=g.pid and p.gid=g.gid "
			     "where ((p.pid='%s')) and p.trust!='none' "
			     "      and u.status='active' order by u.uid",
			     12, pid);
		}
	}
	else {
		/*
		 * XXX
		 * Temporary hack until we figure out the right model for
		 * remote nodes. For now, we use the pcremote-ok slot in
		 * in the project table to determine what remote nodes are
		 * okay'ed for the project. If connecting node type is in
		 * that list, then return user info for all of the users
		 * in those projects (crossed with group in the project). 
		 */
		res = mydb_query("select distinct  "
				 "u.uid,u.usr_pswd,u.unix_uid,u.usr_name, "
				 "m.trust,g.pid,g.gid,g.unix_gid,u.admin, "
				 "u.emulab_pubkey,u.home_pubkey, "
				 "UNIX_TIMESTAMP(u.usr_modified) "
				 "from projects as p "
				 "left join group_membership as m "
				 "  on m.pid=p.pid "
				 "left join groups as g on "
				 "  g.pid=m.pid and g.gid=m.gid "
				 "left join users as u on u.uid=m.uid "
				 "where p.approved!=0 "
				 "      and FIND_IN_SET('%s',pcremote_ok)>0 "
				 "      and m.trust!='none' "
				 "      and u.status='active' "
				 "order by u.uid",
				 12, nodetype);
	}

	if (!res) {
		error("ACCOUNTS: %s: DB Error getting users!\n", pid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		error("ACCOUNTS: %s: No Users!\n", pid);
		mysql_free_result(res);
		return 0;
	}

	row = mysql_fetch_row(res);
	while (nrows) {
		MYSQL_ROW	nextrow;
		MYSQL_RES	*pubkeys_res;
		MYSQL_RES	*sfskeys_res;
		int		pubkeys_nrows, sfskeys_nrows, i, root = 0;
		int		auxgids[128], gcount = 0;
		char		glist[BUFSIZ];

		gidint     = -1;
		tbadmin    = root = atoi(row[8]);
		
		while (1) {
			
			/*
			 * The whole point of this mess. Figure out the
			 * main GID and the aux GIDs. Perhaps trying to make
			 * distinction between main and aux is unecessary, as
			 * long as the entire set is represented.
			 */
			if (strcmp(row[5], pid) == 0 &&
			    strcmp(row[6], gid) == 0) {
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
			sprintf(buf,
				"ADDUSER LOGIN=%s "
				"PSWD=%s UID=%s GID=%d ROOT=%d NAME=\"%s\" "
				"HOMEDIR=%s/%s GLIST=%s\n",
				row[0], row[1], row[2], gidint, root, row[3],
				USERDIR, row[0], glist);
		}
		else if (vers == 4) {
			snprintf(buf, sizeof(buf) - 1,
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
			sprintf(buf,
				"ADDUSER LOGIN=%s "
				"PSWD=%s UID=%s GID=%d ROOT=%d NAME=\"%s\" "
				"HOMEDIR=%s/%s GLIST=\"%s\" SERIAL=%s\n",
				row[0], row[1], row[2], gidint, root, row[3],
				USERDIR, row[0], glist, row[11]);
		}
			
		client_writeback(sock, buf, strlen(buf), tcp);

		info("ACCOUNTS: "
		     "ADDUSER LOGIN=%s "
		     "UID=%s GID=%d ROOT=%d GLIST=%s\n",
		     row[0], row[2], gidint, root, glist);

		if (vers < 5)
			goto skipkeys;

		/*
		 * Need a list of keys for this user.
		 */
		pubkeys_res = mydb_query("select comment,pubkey "
					 " from user_pubkeys "
					 "where uid='%s'",
					 2, row[0]);
	
		if (!pubkeys_res) {
			error("ACCOUNTS: %s: DB Error getting keys\n", row[0]);
			goto skipkeys;
		}
		if ((pubkeys_nrows = (int)mysql_num_rows(pubkeys_res))) {
			while (pubkeys_nrows) {
				MYSQL_ROW	pubkey_row;

				pubkey_row = mysql_fetch_row(pubkeys_res);

				sprintf(buf, "PUBKEY LOGIN=%s KEY=\"%s\"\n",
					row[0], pubkey_row[1]);
			
				client_writeback(sock, buf, strlen(buf), tcp);
				pubkeys_nrows--;

				info("ACCOUNTS: PUBKEY LOGIN=%s COMMENT=%s\n",
				     row[0], pubkey_row[0]);
			}
		}
		mysql_free_result(pubkeys_res);

		if (vers < 6)
			goto skipkeys;

		/*
		 * Need a list of SFS keys for this user.
		 */
		sfskeys_res = mydb_query("select comment,pubkey "
					 " from user_sfskeys "
					 "where uid='%s'",
					 2, row[0]);

		if (!sfskeys_res) {
			error("ACCOUNTS: %s: DB Error getting SFS keys\n", row[0]);
			goto skipkeys;
		}
		if ((sfskeys_nrows = (int)mysql_num_rows(sfskeys_res))) {
			while (sfskeys_nrows) {
				MYSQL_ROW	sfskey_row;

				sfskey_row = mysql_fetch_row(sfskeys_res);

				sprintf(buf, "SFSKEY KEY=\"%s\"\n",
					sfskey_row[1]);

				client_writeback(sock, buf, strlen(buf), tcp);
				sfskeys_nrows--;

				info("ACCOUNTS: SFSKEY LOGIN=%s COMMENT=%s\n",
				     row[0], sfskey_row[0]);
			}
		}
		mysql_free_result(sfskeys_res);
		
	skipkeys:
		row = nextrow;
	}
	mysql_free_result(res);
	return 0;
}

/*
 * Return account stuff.
 */
COMMAND_PROTOTYPE(dodelay)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		buf[2*MYBUFSIZE];
	int		nrows;

	/*
	 * Now check reserved table
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid))
		return 0;

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
		 "q1_droptail,q1_gentle "
                 " from delays as d "
		 "left join interfaces as i on "
		 " i.node_id=d.node_id and i.iface=iface0 "
		 "left join interfaces as j on "
		 " j.node_id=d.node_id and j.iface=iface1 "
		 " where d.node_id='%s'",	 
		 37, nodeid);
	if (!res) {
		error("DELAY: %s: DB Error getting delays!\n", nodeid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}
	while (nrows) {
		row = mysql_fetch_row(res);

		/*
		 * Yikes, this is ugly! Sanity check though, since I saw
		 * some bogus values in the DB.
		 */
		if (!row[0] || !row[1] || !row[2] || !row[3]) {
			error("DELAY: %s: DB values are bogus!\n", nodeid);
			mysql_free_result(res);
			return 1;
		}

		sprintf(buf, "DELAY INT0=%s INT1=%s "
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
			"DROPTAIL1=%s GENTLE1=%s \n",
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
			row[35], row[36]);
			
		client_writeback(sock, buf, strlen(buf), tcp);
		nrows--;
		info("DELAY: %s", buf);
	}
	mysql_free_result(res);

	return 0;
}

/*
 * Return host table information.
 */
COMMAND_PROTOTYPE(dohostsV2)
{
	/*
	 * This will go away. Ignore version and assume latest.
	 */
	return(dohosts(sock, nodeid, rdata, tcp, islocal,
		       nodetype, CURRENT_VERSION));
}

COMMAND_PROTOTYPE(dohosts)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
	char		pid[TBDB_FLEN_PID];
	char		eid[TBDB_FLEN_EID], gid[TBDB_FLEN_GID];
	int		nrows;
	int		rv = 0;

	/*
	 * We build up a canonical host table using this data structure.
	 * There is one item per node/iface. We need a shared structure
	 * though for each node, to allow us to compute the aliases.
	 */
	struct shareditem {
		char	*firstvlan;	/* The first vlan to another node */
	};
	struct hostentry {
		char	nodeid[TBDB_FLEN_NODEID];
		char	vname[TBDB_FLEN_VNAME];
		char	vlan[TBDB_FLEN_VNAME];
		int	virtiface;
		int	connected;
		struct in_addr	  ipaddr;
		struct shareditem *shared;
		struct hostentry  *next;
	} *hosts = 0, *host;

	/*
	 * Now check reserved table
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid))
		return 0;

	/*
	 * Now use the virt_nodes table to get a list of all of the
	 * nodes and all of the IPs on those nodes. This is the basis
	 * for the canonical host table. Join it with the reserved
	 * table to get the node_id at the same time (saves a step).
	 */
	res = mydb_query("select v.vname,v.ips,r.node_id from virt_nodes as v "
			 "left join reserved as r on "
			 " v.vname=r.vname and v.pid=r.pid and v.eid=r.eid "
			 " where v.pid='%s' and v.eid='%s' order by r.node_id",
			 3, pid, eid);

	if (!res) {
		error("HOSTNAMES: %s: DB Error getting virt_nodes!\n", nodeid);
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
		char		  *bp, *ip, *cp;
		struct shareditem *shareditem;
		
		row = mysql_fetch_row(res);
		if (!row[0] || !row[0][0] ||
		    !row[1] || !row[1][0])
			continue;

		if (! (shareditem = (struct shareditem *)
		       calloc(1, sizeof(*shareditem)))) {
			error("HOSTNAMES: Out of memory for shareditem!\n");
			exit(1);
		}

		bp = row[1];
		while (bp) {
			/*
			 * Note that the ips column is a space separated list
			 * of X:IP where X is a logical interface number.
			 */
			cp = strsep(&bp, ":");
			ip = strsep(&bp, " ");

			if (! (host = (struct hostentry *)
			              calloc(1, sizeof(*host)))) {
				error("HOSTNAMES: Out of memory!\n");
				exit(1);
			}

			strcpy(host->vname, row[0]);
			strcpy(host->nodeid, row[2]);
			host->virtiface = atoi(cp);
			host->shared = shareditem;
			inet_aton(ip, &host->ipaddr);
			host->next = hosts;
			hosts = host;
		}
	}
	mysql_free_result(res);

	/*
	 * Now we need to find the virtual lan name for each interface on
	 * each node. This is the user or system generated vlan name, and is
	 * in the virt_lans table. We use the virtiface number we got above
	 * to match on the member slot.
	 */
	res = mydb_query("select vname,member from virt_lans "
			 " where pid='%s' and eid='%s'",
			 2, pid, eid);

	if (!res) {
		error("HOSTNAMES: %s: DB Error getting virt_lans!\n", nodeid);
		rv = 1;
		goto cleanup;
	}
	if (! (nrows = mysql_num_rows(res))) {
		mysql_free_result(res);
		rv = 1;
		goto cleanup;
	}

	while (nrows--) {
		char	*bp, *cp;
		int	virtiface;
		
		row = mysql_fetch_row(res);
		if (!row[0] || !row[0][0] ||
		    !row[1] || !row[1][0])
			continue;

		/*
		 * Note that the members column looks like vname:X
		 * where X is a logical interface number we got above.
		 * Loop through and find the entry and stash the vlan
		 * name.
		 */
		bp = row[1];
		cp = strsep(&bp, ":");
		virtiface = atoi(bp);

		host = hosts;
		while (host) {
			if (host->virtiface == virtiface &&
			    strcmp(cp, host->vname) == 0) {
				strcpy(host->vlan, row[0]);
			}
			host = host->next;
		}
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
		if (strcmp(host->nodeid, nodeid) == 0 && host->vlan) {
			struct hostentry *tmphost = hosts;

			while (tmphost) {
				if (strlen(tmphost->vlan) &&
				    strcmp(host->vlan, tmphost->vlan) == 0 &&
				    strcmp(host->nodeid, tmphost->nodeid) &&
				    (!tmphost->shared->firstvlan ||
				     !strcmp(tmphost->vlan,
					     tmphost->shared->firstvlan))) {
					tmphost->connected = 1;
					
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
	host = hosts;
	while (host) {
		char	*alias = " ";

		if ((host->shared->firstvlan &&
		     !strcmp(host->shared->firstvlan, host->vlan)) ||
		    /* First interface on this node gets an alias */
		    (!strcmp(host->nodeid, nodeid) && !host->virtiface))
			alias = host->vname;
		
		/* Old format */
		if (vers == 2) {
			sprintf(buf,
				"NAME=%s LINK=%i IP=%s ALIAS=%s\n",
				host->vname, host->virtiface,
				inet_ntoa(host->ipaddr), alias);
		}
		else {
			sprintf(buf,
				"NAME=%s-%s IP=%s ALIASES='%s-%i %s'\n",
				host->vname, host->vlan,
				inet_ntoa(host->ipaddr),
				host->vname, host->virtiface, alias);
		}
		client_writeback(sock, buf, strlen(buf), tcp);
		info("HOSTNAMES: %s", buf);

		host = host->next;
	}
 cleanup:
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
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		buf[MYBUFSIZE], *bp, *sp;

	/*
	 * Now check reserved table
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid))
		return 0;

	/*
	 * Get RPM list for the node.
	 */
	res = mydb_query("select rpms from nodes where node_id='%s' ",
			 1, nodeid);

	if (!res) {
		error("RPMS: %s: DB Error getting RPMS!\n", nodeid);
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

		sprintf(buf, "RPM=%s\n", bp);
		client_writeback(sock, buf, strlen(buf), tcp);
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
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		buf[MYBUFSIZE], *bp, *sp, *tp;

	/*
	 * Now check reserved table
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid))
		return 0;

	/*
	 * Get Tarball list for the node.
	 */
	res = mydb_query("select tarballs from nodes where node_id='%s' ",
			 1, nodeid);

	if (!res) {
		error("TARBALLS: %s: DB Error getting tarballs!\n", nodeid);
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

		sprintf(buf, "DIR=%s TARBALL=%s\n", bp, tp);
		client_writeback(sock, buf, strlen(buf), tcp);
		info("TARBALLS: %s", buf);
		
	} while ((bp = sp));
	
	mysql_free_result(res);
	return 0;
}

/*
 * Return Deltas stuff.
 */
COMMAND_PROTOTYPE(dodeltas)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		buf[MYBUFSIZE], *bp, *sp;

	/*
	 * Now check reserved table
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid))
		return 0;

	/*
	 * Get Delta list for the node.
	 */
	res = mydb_query("select deltas from nodes where node_id='%s' ",
			 1, nodeid);

	if (!res) {
		error("DELTAS: %s: DB Error getting Deltas!\n", nodeid);
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

		sprintf(buf, "DELTA=%s\n", bp);
		client_writeback(sock, buf, strlen(buf), tcp);
		info("DELTAS: %s", buf);
		
	} while ((bp = sp));
	
	mysql_free_result(res);
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
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		buf[MYBUFSIZE];

	/*
	 * Now check reserved table
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid))
		return 0;

	/*
	 * Get run command for the node.
	 */
	res = mydb_query("select startupcmd from nodes where node_id='%s'",
			 1, nodeid);

	if (!res) {
		error("STARTUPCMD: %s: DB Error getting startup command!\n",
		       nodeid);
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
	sprintf(buf, "CMD='%s' UID=", row[0]);
	mysql_free_result(res);

	/*
	 * Now get the UID from the experiments table and tack that onto
	 * the string just created.
	 */
	res = mydb_query("select expt_head_uid from experiments "
			 "where eid='%s' and pid='%s'",
			 1, eid, pid);

	if (!res) {
		error("STARTUPCMD: %s: DB Error getting UID!\n", nodeid);
		return 1;
	}

	if ((int)mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return 0;
	}
	row = mysql_fetch_row(res);
	strcat(buf, row[0]);
	strcat(buf, "\n");
	mysql_free_result(res);
	
	client_writeback(sock, buf, strlen(buf), tcp);
	info("STARTUPCMD: %s", buf);
	
	return 0;
}

/*
 * Accept notification of start command exit status. 
 */
COMMAND_PROTOTYPE(dostartstat)
{
	char		pid[64];
	char		eid[64];
	char		gid[64];
	int		exitstatus;

	/*
	 * Dig out the exit status
	 */
	if (! sscanf(rdata, "%d", &exitstatus)) {
		error("STARTSTAT: %s: Invalid status: %s\n", nodeid, rdata);
		return 1;
	}
	
	info("STARTSTAT: "
	     "%s is reporting startup command exit status: %d\n",
	     nodeid, exitstatus);

	/*
	 * Make sure currently allocated to an experiment!
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid)) {
		info("STARTSTAT: %s: Node is free\n", nodeid);
		return 0;
	}

	/*
	 * Update the node table record with the exit status. Setting the
	 * field to a non-null string value is enough to tell whoever is
	 * watching it that the node is done.
	 */
	if (mydb_update("update nodes set startstatus='%d' "
			"where node_id='%s'", exitstatus, nodeid)) {
		error("STARTSTAT: %s: DB Error setting exit status!\n",
		      nodeid);
		return 1;
	}
	return 0;
}

/*
 * Accept notification of ready for action
 */
COMMAND_PROTOTYPE(doready)
{
	char		pid[64];
	char		eid[64];
	char		gid[64];

	/*
	 * Make sure currently allocated to an experiment!
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid)) {
		info("READY: %s: Node is free\n", nodeid);
		return 0;
	}

	/*
	 * Update the ready_bits table.
	 */
	if (mydb_update("update nodes set ready=1 "
			"where node_id='%s'", nodeid)) {
		error("READY: %s: DB Error setting ready bit!\n", nodeid);
		return 1;
	}

	info("READY: %s: Node is reporting ready\n", nodeid);

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
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		buf[MYBUFSIZE];
	int		total, ready, i;

	/*
	 * Make sure currently allocated to an experiment!
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid)) {
		info("READYCOUNT: %s: Node is free\n", nodeid);
		return 0;
	}

	/*
	 * See how many are ready. This is a non sync protocol. Clients
	 * keep asking until N and M are equal. Can only be used once
	 * of course, after experiment creation.
	 */
	res = mydb_query("select ready from reserved "
			 "left join nodes on nodes.node_id=reserved.node_id "
			 "where reserved.eid='%s' and reserved.pid='%s'",
			 1, eid, pid);

	if (!res) {
		error("READYCOUNT: %s: DB Error getting ready bits.\n",
		      nodeid);
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

	sprintf(buf, "READY=%d TOTAL=%d\n", ready, total);
	client_writeback(sock, buf, strlen(buf), tcp);
		
	info("READYCOUNT: %s: %s", nodeid, buf);

	return 0;
}

static char logfmt[] = "/proj/%s/logs/%s.log";

/*
 * Log some text to a file in the /proj/<pid>/exp/<eid> directory.
 */
COMMAND_PROTOTYPE(dolog)
{
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		logfile[sizeof(pid)+sizeof(eid)+sizeof(logfmt)];
	FILE		*fd;
	time_t		curtime;
	char		*tstr;

	/*
	 * Find the pid/eid of the requesting node
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid)) {
		info("LOG: %s: Node is free\n", nodeid);
		return 1;
	}

	snprintf(logfile, sizeof(logfile)-1, logfmt, pid, eid);
	fd = fopen(logfile, "a");
	if (fd == NULL) {
		error("LOG: %s: Could not open %s\n", nodeid, logfile);
		return 1;
	}

	curtime = time(0);
	tstr = ctime(&curtime);
	tstr[19] = 0;	/* no year */
	tstr += 4;	/* or day of week */

	while (isspace(*rdata))
		rdata++;

	fprintf(fd, "%s: %s\n\n%s\n=======\n", tstr, nodeid, rdata);
	fclose(fd);

	return 0;
}

/*
 * Return mount stuff.
 */
COMMAND_PROTOTYPE(domounts)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		buf[MYBUFSIZE];
	int		nrows;
	int		usesfs;
	char		*bp;

	/*
	 * Now check reserved table
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid)) {
		error("MOUNTS: %s: Node is free\n", nodeid);
		return 1;
	}

	/*
	 * Should SFS mounts be served?
	 */
	usesfs = 0;
	if (vers >= 6 && strlen(fshostid)) {
		while ((bp = strsep(&rdata, " ")) != NULL) {
			if (sscanf(bp, "USESFS=%d", &usesfs) == 1) {
				continue;
			}

			error("Unknown parameter to domounts: %s\n",
			      bp);
			break;
		}

		if (debug) {
			if (usesfs) {
				info("Using SFS\n");
			}
			else {
				info("Not using SFS\n");
			}
		}
	}
	
	/*
	 * If SFS is in use, the project mount is done via SFS.
	 */
	if (!usesfs) {
		/*
		 * Return project mount first. 
		 */
		sprintf(buf, "REMOTE=%s/%s LOCAL=%s/%s\n",
			FSPROJDIR, pid, PROJDIR, pid);
		client_writeback(sock, buf, strlen(buf), tcp);
		info("MOUNTS: %s", buf);
		
		/*
		 * If pid!=gid, then this is group experiment, and we return
		 * a mount for the group directory too.
		 */
		if (strcmp(pid, gid)) {
			sprintf(buf, "REMOTE=%s/%s/%s LOCAL=%s/%s/%s\n",
				FSGROUPDIR, pid, gid, GROUPDIR, pid, gid);
			client_writeback(sock, buf, strlen(buf), tcp);
			info("MOUNTS: %s", buf);
		}
	}
	else {
		/*
		 * Return SFS-based project mount.
		 */
		sprintf(buf, "SFS REMOTE=%s%s/%s LOCAL=%s/%s\n",
			fshostid, FSDIR_PROJ, pid, PROJDIR, pid);
		client_writeback(sock, buf, strlen(buf), tcp);
		info("MOUNTS: %s", buf);

		/*
		 * Return SFS-based group mount.
		 */
		if (strcmp(pid, gid)) {
			sprintf(buf, "SFS REMOTE=%s%s/%s/%s LOCAL=%s/%s/%s\n",
				fshostid, FSDIR_GROUPS, pid, gid,
				GROUPDIR, pid, gid);
			client_writeback(sock, buf, strlen(buf), tcp);
			info("MOUNTS: %s", buf);
		}
	}
	
	/*
	 * Now check for aux project access. Return a list of mounts for
	 * those projects.
	 */
	res = mydb_query("select pid from exppid_access "
			 "where exp_pid='%s' and exp_eid='%s'",
			 1, pid, eid);
	if (!res) {
		error("MOUNTS: %s: DB Error getting users!\n", pid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res))) {
		while (nrows) {
			row = mysql_fetch_row(res);

			sprintf(buf, "REMOTE=%s/%s LOCAL=%s/%s\n",
				FSPROJDIR, row[0], PROJDIR, row[0]);
			client_writeback(sock, buf, strlen(buf), tcp);

			nrows--;
		}
	}
	mysql_free_result(res);

	/*
	 * Now a list of user directories. These include the members of the
	 * experiments projects, plus all the members of all of the projects
	 * that have been granted access to share the nodes in that expt.
	 */
#ifdef  NOSHAREDEXPTS
	res = mydb_query("select u.uid from users as u "
			 "left join group_membership as p on p.uid=u.uid "
			 "where p.pid='%s' and p.gid='%s' and "
			 "      u.status='active' and p.trust!='none'",
			 1, pid, gid);
#else
	res = mydb_query("select distinct u.uid from users as u "
			 "left join exppid_access as a "
			 " on a.exp_pid='%s' and a.exp_eid='%s' "
			 "left join group_membership as p on p.uid=u.uid "
			 "where ((p.pid='%s' and p.gid='%s') or p.pid=a.pid) "
			 "       and u.status='active' and p.trust!='none'",
			 1, pid, eid, pid, gid);
#endif
	if (!res) {
		error("MOUNTS: %s: DB Error getting users!\n", pid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		error("MOUNTS: %s: No Users!\n", pid);
		mysql_free_result(res);
		return 0;
	}

	while (nrows) {
		row = mysql_fetch_row(res);
				
		sprintf(buf, "REMOTE=%s/%s LOCAL=%s/%s\n",
			FSUSERDIR, row[0], USERDIR, row[0]);
		client_writeback(sock, buf, strlen(buf), tcp);
		
		nrows--;
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

/*
 * Create dirsearch entry for node.
 */
COMMAND_PROTOTYPE(dosfshostid)
{
	char	pid[64];
	char	eid[64];
	char	gid[64];
	char	nickname[128];
	char	nodehostid[HOSTID_SIZE];
	char	sfspath[MYBUFSIZE], dspath[MYBUFSIZE];

	if (!strlen(fshostid)) {
		/* SFS not being used */
		info("dosfshostid: Called while SFS is not in use\n");
		return 0;
	}
	
	/*
	 * Now check reserved table
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid)) {
		error("dosfshostid: %s: Node is free\n", nodeid);
		return 1;
	}

	if (nodeidtonickname(nodeid, nickname))
		strcpy(nickname, nodeid);

	/*
	 * Create symlink names
	 */
	if (! sscanf(rdata, "%s", nodehostid)) {
		error("dosfshostid: No hostid reported!\n");
		return 1;
	}
	sprintf(sfspath, "/sfs/%s", nodehostid);
	// sprintf(dspath, "/proj/%s/.sfs/%s/%s", pid, eid, nickname);
	sprintf(dspath, "/proj/.sfs/%s.%s.%s", nickname, eid, pid);
	
	/*
	 * Really, there should be a cleaner way of doing this, but
	 * this works, at least for now.  Perhaps using the DB and a
	 * symlinking deamon alone would be better.
	 */
	if (setjmp(sfshostiddeadline) == 0) {
		sfshostiddeadfl = 0;
		signal(SIGALRM, dosfshostiddead);
		alarm(1);

		unlink(dspath);
		if (symlink(sfspath, dspath) < 0)
			sfshostiddeadfl = 1;
	}
	alarm(0);
	if (sfshostiddeadfl) {
		errorc("symlinking %s to %s", dspath, sfspath);
		return 1;
	}

	return 0;
}

/*
 * Return routing stuff.
 */
COMMAND_PROTOTYPE(dorouting)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		buf[MYBUFSIZE];
	int		nrows;

	/*
	 * Now check reserved table
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid)) {
		error("ROUTES: %s: Node is free\n", nodeid);
		return 1;
	}

	/*
	 * Get the routing type from the nodes table.
	 */
	res = mydb_query("select routertype from nodes where node_id='%s'",
			 1, nodeid);

	if (!res) {
		error("ROUTES: %s: DB Error getting router type!\n", nodeid);
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
	info("ROUTES: %s", buf);

	/*
	 * Get the routing type from the nodes table.
	 */
	res = mydb_query("select dst,dst_type,dst_mask,nexthop,cost "
			 "from virt_routes as vi "
			 "left join reserved as r on r.vname=vi.vname "
			 "where r.node_id='%s' and "
			 " vi.pid='%s' and vi.eid='%s'",
			 5, nodeid, pid, eid);
	
	if (!res) {
		error("ROUTES: %s: DB Error getting manual routes!\n", nodeid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}

	while (nrows) {
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
		if (strcmp(row[1], "net") == 0) {
			struct in_addr tip, tmask;

			inet_aton(row[0], &tip);
			inet_aton(row[2], &tmask);
			tip.s_addr &= tmask.s_addr;
			strncpy(dstip, inet_ntoa(tip), sizeof(dstip));
		} else
			strncpy(dstip, row[0], sizeof(dstip));

		sprintf(buf, "ROUTE DEST=%s DESTTYPE=%s DESTMASK=%s "
			"NEXTHOP=%s COST=%s\n",
			dstip, row[1], row[2], row[3], row[4]);
		client_writeback(sock, buf, strlen(buf), tcp);
		
		nrows--;
		info("ROUTES: %s", buf);
	}
	mysql_free_result(res);

	return 0;
}

/*
 * Return address from which to load an image, along with the partition that
 * it should be written to
 */
COMMAND_PROTOTYPE(doloadinfo)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];

	/*
	 * Get the address the node should contact to load its image
	 */
	res = mydb_query("select load_address,loadpart from images as i "
			"left join current_reloads as r on i.imageid = r.image_id "
			"where node_id='%s'",
			 2, nodeid);

	if (!res) {
		error("doloadinfo: %s: DB Error getting loading address!\n",
		       nodeid);
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
	sprintf(buf, "ADDR=%s PART=%s\n", row[0], row[1]);
	mysql_free_result(res);

	client_writeback(sock, buf, strlen(buf), tcp);
	info("doloadinfo: %s", buf);
	
	return 0;
}

/*
 * If next_pxe_boot_path is set, clear it. Otherwise, if next_boot_path
 * or next_boot_osid is set, clear both along with next_boot_cmd_line.
 * If neither is set, do nothing. Produces no output to the client.
 */
COMMAND_PROTOTYPE(doreset)
{
	MYSQL_RES	*res;	

	/*
	 * Check to see if next_pxe_boot_path is set
	 */
	res = mydb_query("select next_pxe_boot_path from nodes "
			"where node_id='%s' and next_pxe_boot_path is not null "
			"and next_pxe_boot_path != ''",
			 1, nodeid);

	if (!res) {
		error("doreset: %s: DB Error checking for "
		      "next_pxe_boot_path!\n", nodeid);
		return 1;
	}

	/*
	 * If we get a row back, then next_pxe_boot_path was set, because
	 * the query checks for emptiness
	 */
	if ((int)mysql_num_rows(res) > 0) {
		mysql_free_result(res);
		info("doreset: %s: Clearing next_pxe_boot_path\n", nodeid);
		if (mydb_update("update nodes set next_pxe_boot_path='' "
			"where node_id='%s'", nodeid)) {
		    error("doreset: %s: DB Error clearing "
			  "next_pxe_boot_path!\n", nodeid);
		    return 1;
		}
		return 0;
	}

	/*
	 * Check to see if next_boot_path or next_boot_osid are set
	 */
	res = mydb_query("select next_boot_path, next_boot_osid from nodes "
			"where node_id='%s' and "
			"((next_boot_path is not null and next_boot_path != '')"
			" or "
			"(next_boot_osid is not null and next_boot_osid != ''))",
			 2, nodeid);

	if (!res) {
		error("doreset: %s: DB Error checking for "
		      "next_boot_path or next_boot_osid!\n",
		       nodeid);
		return 1;
	}

	/*
	 * If we get a row back, then one of them was set, because
	 * the query checks for emptiness
	 */
	if ((int)mysql_num_rows(res) > 0) {
		mysql_free_result(res);
		info("doreset: %s: Clearing next_boot_*\n", nodeid);
		if (mydb_update("update nodes set next_boot_path='',"
			"next_boot_osid='',next_boot_cmd_line='' "
			"where node_id='%s'", nodeid)) {
		    error("doreset: %s: DB Error clearing next_boot_*!\n",
			  nodeid);
		    return 1;
		}
		return 0;
	}

	
	return 0;
}

/*
 * Return trafgens info
 */
COMMAND_PROTOTYPE(dotrafgens)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		buf[MYBUFSIZE];
	int		nrows;

	/*
	 * Now check reserved table
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid)) {
		error("TRAFGENS: %s: Node is free\n", nodeid);
		return 1;
	}

	res = mydb_query("select vi.vname,role,proto,"
			 "vnode,port,ip,target_vnode,target_port,target_ip, "
			 "generator "
			 "from virt_trafgens as vi "
			 "left join reserved as r on r.vname=vi.vnode "
			 "where r.node_id='%s' and "
			 " vi.pid='%s' and vi.eid='%s'",
			 10, nodeid, pid, eid);

	if (!res) {
		error("TRAFGENS: %s: DB Error getting virt_trafgens\n",
		      nodeid);
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

		sprintf(buf, "TRAFGEN=%s MYNAME=%s MYPORT=%s "
			"PEERNAME=%s PEERPORT=%s "
			"PROTO=%s ROLE=%s GENERATOR=%s\n",
			row[0], myname, row[4], peername, row[7],
			row[2], row[1], row[9]);
		       
		client_writeback(sock, buf, strlen(buf), tcp);
		
		nrows--;
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
	char		pid[64];
	char		eid[64];
	char		gid[64];
	int		nrows;

	if (!tcp) {
		error("NSECONFIGS: %s: Cannot do UDP mode!\n", nodeid);
		return 1;
	}

	/*
	 * Now check reserved table
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid)) {
		error("NSECONFIGS: %s: Node is free\n", nodeid);
		return 1;
	}

	res = mydb_query("select nseconfig from nseconfigs as nse "
			 "left join reserved as r on r.vname=nse.vname "
			 "where r.node_id='%s' and "
			 " nse.pid='%s' and nse.eid='%s'",
			 1, nodeid, pid, eid);

	if (!res) {
		error("NSECONFIGS: %s: DB Error getting nseconfigs\n", nodeid);
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
	char 		*newstate;
#ifdef EVENTSYS
	address_tuple_t tuple;
#endif

	info("%s\n", rdata);

#ifdef LBS
	return 0;
#endif

	/*
	 * Dig out state that the node is reporting
	 */
	while (isspace(*rdata)) {
		rdata++;
	}

	/*
	 * Pull blanks off the end of rdata
	 */
	newstate = rdata + (strlen(rdata) -1);
	while ((newstate >= rdata) && (*newstate == ' ')) {
		*newstate = '\0';
		newstate--;
	}

	newstate = rdata;

	/*
	 * Make sure there is some state being reported. Perhaps we should
	 * look in the  database to make sure that the one being reported is
	 * a valid one.
	 */
	if (!*newstate) {
	    error("dostate: No state reported!\n");
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
	tuple->objname	 = nodeid;
	tuple->eventtype = newstate;

	if (myevent_send(tuple)) {
		error("dostate: Error sending event\n");
	}

	address_tuple_free(tuple);
#endif /* EVENTSYS */
	
	info("STATE: %s\n", newstate);
	return 0;

}

/*
 * Return creator of experiment. Total hack. Must kill this.
 */
COMMAND_PROTOTYPE(docreator)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		buf[MYBUFSIZE];

	/*
	 * Now check reserved table
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid))
		return 0;

	/*
	 * Now get the UID from the experiments table.
	 */
	res = mydb_query("select expt_head_uid from experiments "
			 "where eid='%s' and pid='%s'",
			 1, eid, pid);

	if (!res) {
		error("CREATOR: %s: DB Error getting UID!\n", nodeid);
		return 1;
	}

	if ((int)mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return 0;
	}
	row = mysql_fetch_row(res);
	sprintf(buf, "CREATOR=%s\n", row[0]);
	mysql_free_result(res);

	client_writeback(sock, buf, strlen(buf), tcp);
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
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		buf[MYBUFSIZE];
	int		nrows;

	/*
	 * Now check reserved table
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid)) {
		error("TRAFGENS: %s: Node is free\n", nodeid);
		return 1;
	}

	res = mydb_query("select vname,isserver,peer_ip,port,password, "
			 " encrypt,compress,assigned_ip,proto "
			 "from tunnels where node_id='%s'",
			 9, nodeid);

	if (!res) {
		error("TUNNELS: %s: DB Error getting tunnels\n", nodeid);
		return 1;
	}
	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}

	while (nrows) {
		row = mysql_fetch_row(res);

		sprintf(buf, "TUNNEL=%s ISSERVER=%s PEERIP=%s PEERPORT=%s "
			"PASSWORD=%s ENCRYPT=%s COMPRESS=%s "
			"INET=%s MASK=%s PROTO=%s\n",
			row[0], row[1], row[2], row[3], row[4],
			row[5], row[6], row[7], NETMASK, row[8]);
		       
		client_writeback(sock, buf, strlen(buf), tcp);
		
		nrows--;
		info("TUNNELS: %s ISSERVER=%s PEERIP=%s PEERPORT=%s INET=%s\n",
		     row[0], row[1], row[2], row[3], row[7]);
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

	res = mydb_query("select r.node_id from reserved as r "
			 "left join nodes as n on r.node_id=n.node_id "
			 "left join node_types as nt on nt.type=n.type "
			 "where nt.isvirtnode=1 and n.phys_nodeid='%s'",
			 1, nodeid);

	if (!res) {
		error("VNODELIST: %s: DB Error getting vnode list\n", nodeid);
		return 1;
	}
	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}

	while (nrows) {
		row = mysql_fetch_row(res);

		sprintf(buf, "%s\n", row[0]);
		       
		client_writeback(sock, buf, strlen(buf), tcp);
		
		nrows--;
		info("VNODELIST: %s\n", row[0]);
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
	if (mysql_real_connect(&db, 0, 0, 0,
			       dbname, 0, 0, CLIENT_INTERACTIVE) == 0) {
		error("%s: connect failed: %s\n", dbname, mysql_error(&db));
		return 1;
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

MYSQL_RES *
mydb_query(char *query, int ncols, ...)
{
	MYSQL_RES	*res;
	char		querybuf[2*MYBUFSIZE];
	va_list		ap;
	int		n;

	va_start(ap, ncols);
	n = vsnprintf(querybuf, sizeof(querybuf), query, ap);
	if (n > sizeof(querybuf)) {
		error("query too long for buffer\n");
		return (MYSQL_RES *) 0;
	}

	if (! mydb_connect())
		return (MYSQL_RES *) 0;

	if (mysql_real_query(&db, querybuf, n) != 0) {
		error("%s: query failed: %s\n", dbname, mysql_error(&db));
		mydb_disconnect();
		return (MYSQL_RES *) 0;
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
	char		querybuf[MYBUFSIZE];
	va_list		ap;
	int		n;

	va_start(ap, query);
	n = vsnprintf(querybuf, sizeof(querybuf), query, ap);
	if (n > sizeof(querybuf)) {
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
iptonodeid(struct in_addr ipaddr, char *vnodeid,
	   char *nodeid, char *class, char *type, int *islocal)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;

	res = mydb_query("select t.class,t.type,n.node_id  "
			 " from interfaces as i "
			 "left join nodes as n on n.node_id=i.node_id "
			 "left join node_types as t on "
			 "  t.type=n.type and i.card=t.control_net "
			 "where i.IP='%s'",
			 3, inet_ntoa(ipaddr));

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
	mysql_free_result(res);

	if (!row[0] || !row[1] || !row[2]) {
		error("iptonodeid: %s: Malformed DB response!\n",
		      inet_ntoa(ipaddr)); 
		return 1;
	}
	strcpy(nodeid, row[2]);
	strcpy(class,  row[0]);
	strcpy(type,   row[1]);
	*islocal = (! strcasecmp(class, "pcremote") ? 0 : 1);

	if (! vnodeid)
		return 0;
	
	/*
	 * If a vnodeid has been provided, then we want to check to make
	 * sure its really mapped onto this node. Substitute the vnodeid
	 * for the nodeid. Temporary solution.
	 */
	res = mydb_query("select n.phys_nodeid from node_types as t "
			 "left join nodes as n on t.type=n.type "
			 "where n.node_id='%s' and t.isvirtnode=1 "
			 "  and n.phys_nodeid='%s'",
			 1, vnodeid, nodeid);

	if (!res) {
		error("iptonodeid: %s: DB Error getting vnode mapping: %s!\n",
		      nodeid, vnodeid);
		return 2;
	}

	if (! (int)mysql_num_rows(res)) {
		mysql_free_result(res);
		return 2;
	}
	mysql_free_result(res);
	strcpy(nodeid, vnodeid);
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
	strcpy(pid, row[0]);
	strcpy(eid, row[1]);

	/*
	 * If there is no gid (yes, thats bad and a mistake), then copy
	 * the pid in. Also warn.
	 */
	if (row[2]) {
		strcpy(gid, row[2]);
	}
	else {
		strcpy(gid, pid);
		error("nodeidtoexp: %s: No GID for %s/%s (pid/eid)!\n",
		      nodeid, pid, eid);
	}

	return 0;
}
 
/*
 * Map nodeid to its nickname.
 */
static int
nodeidtonickname(char *nodeid, char *nickname)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;

	res = mydb_query("select vname from reserved "
			 "where node_id='%s'",
			 1, nodeid);
	if (!res) {
		error("nodeidtonickname: %s: DB Error getting reserved!\n",
		      nodeid);
		return 1;
	}

	if (! (int)mysql_num_rows(res)) {
		mysql_free_result(res);
		return 1;
	}
	row = mysql_fetch_row(res);
	mysql_free_result(res);
	if (! row[0] || !row[0][0])
		return 1;
		
	strcpy(nickname, row[0]);
	return 0;
}
 
/*
 * Get control network for a nodeid
 */
static int
nodeidtocontrolnet(char *nodeid, int *net)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;

	res = mydb_query("select control_net from node_types as nt "
			 "left join nodes as n "
			 "on nt.type=n.type "
			 "where n.node_id='%s'",
			 1, nodeid);

	if (!res) {
		error("nodeidtocontrolnet: %s: DB Error!\n", nodeid);
		return 1;
	}

	if (! (int)mysql_num_rows(res)) {
		mysql_free_result(res);
		return 1;
	}
	row = mysql_fetch_row(res);
	mysql_free_result(res);
	*net = atoi(row[0]);

	return 0;
}

/*
 * Check for DBname redirection.
 */
static int
checkdbredirect(char *nodeid)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		pid[64];
	char		eid[64];
	char		gid[64];
	
	if (nodeidtoexp(nodeid, pid, eid, gid)) {
		/* info("CHECKDBREDIRECT: %s: Node is free\n", nodeid); */
		return 0;
	}

	/*
	 * Look for an alternate DB name.
	 */
	res = mydb_query("select testdb from experiments "
			 "where eid='%s' and pid='%s'",
			 1, eid, pid);
			 
	if (!res) {
		error("CHECKDBREDIRECT: "
		      "%s: DB Error getting testdb from table!\n", nodeid);
		return 1;
	}

	if (mysql_num_rows(res) == 0) {
		info("CHECKDBREDIRECT: "
		     "%s: Hmm, experiment not there anymore!\n", nodeid);
		mysql_free_result(res);
		return 0;
	}

	row = mysql_fetch_row(res);
	if (!row[0] || !row[0][0]) {
		mysql_free_result(res);
		return 0;
	}
	/* This changes the DB we talk to. */
	strcpy(dbname, row[0]);

	/*
	 * Okay, lets test to make sure that DB exists. If not, fall back
	 * on the main DB. 
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid)) {
		error("CHECKDBREDIRECT: %s: %s DB does not exist\n",
		      nodeid, dbname);
		strcpy(dbname, DEFAULT_DBNAME);
	}
	mysql_free_result(res);
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
		event_handle = event_register("elvin://" BOSSNODE,0);
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
	if (notification == NULL) {
		error("myevent_send: Unable to allocate notification!");
		return 1;
	}

	if (event_notify(event_handle, notification) == NULL) {
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
	udpfd = -1;
	udpix = 0;
}

/*
 * Return 1 if iface is in connected_interfaces, 0 otherwise
 */

int
directly_connected(struct node_interface *interfaces, char *iface) {

	struct node_interface *traverse;

	traverse = interfaces;
	if (iface == NULL) {
		/* Don't die if address is null */
		return 0;
	}

	while (traverse != NULL) {
		if (!strcmp(traverse->iface,iface)) {
			return 1;
		}
		traverse = traverse->next;
	}

	return 0;

}

/*
 * IsAlive(). Mark nodes as being alive. 
 */
COMMAND_PROTOTYPE(doisalive)
{
	MYSQL_RES	*res;
	int		doaccounts = 0;
	char		buf[MYBUFSIZE];

	mydb_update("update nodes "
		    "set status='up',status_timestamp=now() "
		    "where node_id='%s' or phys_nodeid='%s'",
		    nodeid, nodeid);

	/*
	 * Return info about what needs to be updated. 
	 */
	res = mydb_query("select update_accounts from nodes "
			 "where node_id='%s' and update_accounts!=0",
			 1, nodeid);
			 
	if (!res) {
		error("ISALIVE: %s: DB Error getting account info!\n", nodeid);
		return 1;
	}
	if (mysql_num_rows(res)) {
		doaccounts = 1;
	}
	mysql_free_result(res);

	/*
	 * At some point, maybe what we will do is have the client
	 * make a request asking what needs to be updated. Right now,
	 * just return yes/no and let the client assume it knows what
	 * to do (update accounts).
	 */
	sprintf(buf, "UPDATE=%d\n", doaccounts);
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
		error("IPODINFO: %s: Cannot do this in UDP mode!\n", nodeid);
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

	bp = hashbuf;
	for (i = 0; i < sizeof(randdata); i++) {
		bp += sprintf(bp, "%02x", randdata[i]);
	}
	*bp = '\0';

	mydb_update("update nodes set ipodhash='%s' "
		    "where node_id='%s'",
		    hashbuf, nodeid);
	
	/*
	 * XXX host/mask hardwired to us
	 */
	sprintf(buf, "HOST=%s MASK=255.255.255.255 HASH=%s\n",
		inet_ntoa(myipaddr), hashbuf);
	client_writeback(sock, buf, strlen(buf), tcp);

	return 0;
}
  
