#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <stdarg.h>
#include <assert.h>
#include <mysql/mysql.h>
#include "decls.h"

/*
 * XXX This needs to be localized!
 */
#define FSPROJDIR	"fs.emulab.net:/q/proj"
#define FSUSERDIR	"fs.emulab.net:/users"
#define PROJDIR		"/proj"
#define USERDIR		"/users"
#define RELOADPID	"emulab-ops"
#define RELOADEID	"reloading"

#define TESTMODE
#define VERSION		2
#define NETMASK		"255.255.255.0"

/* Defined in configure and passed in via the makefile */
#define DBNAME_SIZE	64
#define DEFAULT_DBNAME	TBDBNAME

static int	debug = 0;
static int	portnum = TBSERVER_PORT;
static char     dbname[DBNAME_SIZE];
static int	nodeidtoexp(char *nodeid, char *pid, char *eid, char *gid);
static int	iptonodeid(struct in_addr ipaddr, char *bufp);
static int	nodeidtonickname(char *nodeid, char *nickname);
static int	nodeidtocontrolnet(char *nodeid, int *net);
static int	checkdbredirect(struct in_addr ipaddr);
int		client_writeback(int sock, void *buf, int len, int tcp);
void		client_writeback_done(int sock, struct sockaddr_in *client);
MYSQL_RES *	mydb_query(char *query, int ncols, ...);
int		mydb_update(char *query, ...);

/*
 * Commands we support.
 */
static int doreboot(int sock, struct in_addr ipaddr, char *rdata, int tcp);
static int dostatus(int sock, struct in_addr ipaddr, char *rdata, int tcp);
static int doifconfig(int sock, struct in_addr ipaddr, char *rdata, int tcp);
static int doaccounts(int sock, struct in_addr ipaddr, char *rdata, int tcp);
static int dodelay(int sock, struct in_addr ipaddr, char *rdata, int tcp);
static int dohosts(int sock, struct in_addr ipaddr, char *rdata, int tcp);
static int dorpms(int sock, struct in_addr ipaddr, char *rdata, int tcp);
static int dodeltas(int sock, struct in_addr ipaddr, char *rdata, int tcp);
static int dotarballs(int sock, struct in_addr ipaddr, char *rdata, int tcp);
static int dostartcmd(int sock, struct in_addr ipaddr, char *rdata, int tcp);
static int dostartstat(int sock, struct in_addr ipaddr, char *rdata,int tcp);
static int doready(int sock, struct in_addr ipaddr, char *rdata,int tcp);
static int doreadycount(int sock, struct in_addr ipaddr,char *rdata,int tcp);
static int dolog(int sock, struct in_addr ipaddr,char *rdata,int tcp);
static int domounts(int sock, struct in_addr ipaddr,char *rdata,int tcp);
static int doloadinfo(int sock, struct in_addr ipaddr,char *rdata,int tcp);
static int doreset(int sock, struct in_addr ipaddr,char *rdata,int tcp);

struct command {
	char	*cmdname;
	int    (*func)(int sock, struct in_addr ipaddr, char *rdata, int tcp);
} command_array[] = {
	{ "reboot",	doreboot },
	{ "status",	dostatus },
	{ "ifconfig",	doifconfig },
	{ "accounts",	doaccounts },
	{ "delay",	dodelay },
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
	{ "loadinfo",	doloadinfo},
	{ "reset",	doreset},
};
static int numcommands = sizeof(command_array)/sizeof(struct command);

/* 
 * Simple struct used to make a linked list of ifaces
 */
struct node_interface {
	char *iface;
	struct node_interface *next;
};

int		directly_connected(struct node_interface *interfaces, char *iface);

char *usagestr = 
 "usage: tmcd [-d] [-p #]\n"
 " -d              Turn on debugging. Multiple -d options increase output\n"
 " -p portnum	   Specify a port number to listen on\n"
 "\n";

void
usage()
{
	fprintf(stderr, usagestr);
	exit(1);
}

int
main(int argc, char **argv)
{
	int			tcpsock, udpsock, ch;
	int			length, i, err = 0;
	struct sockaddr_in	name;

	while ((ch = getopt(argc, argv, "dp:")) != -1)
		switch(ch) {
		case 'p':
			portnum = atoi(optarg);
			break;
		case 'd':
			debug++;

		case 'h':
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc)
		usage();

	openlog("tmcd", LOG_PID, LOG_USER);
	syslog(LOG_NOTICE, "daemon starting (version %d)", VERSION);

#ifndef LBS
	(void)daemon(0, 0);
#endif

	/*
	 * Setup TCP socket
	 */

	/* Create socket from which to read. */
	tcpsock = socket(AF_INET, SOCK_STREAM, 0);
	if (tcpsock < 0) {
		syslog(LOG_ERR, "opening stream socket: %m");
		exit(1);
	}

	i = 1;
	if (setsockopt(tcpsock, SOL_SOCKET, SO_REUSEADDR,
		       (char *)&i, sizeof(i)) < 0)
		syslog(LOG_ERR, "control setsockopt: %m");;
	
	/* Create name. */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons((u_short) portnum);
	if (bind(tcpsock, (struct sockaddr *) &name, sizeof(name))) {
		syslog(LOG_ERR, "binding stream socket: %m");
		exit(1);
	}
	/* Find assigned port value and print it out. */
	length = sizeof(name);
	if (getsockname(tcpsock, (struct sockaddr *) &name, &length)) {
		syslog(LOG_ERR, "getting socket name: %m");
		exit(1);
	}
	if (listen(tcpsock, 20) < 0) {
		syslog(LOG_ERR, "listening on socket: %m");
		exit(1);
	}
	syslog(LOG_NOTICE, "listening on TCP port %d", ntohs(name.sin_port));

	/*
	 * Setup UDP socket
	 */

	/* Create socket from which to read. */
	udpsock = socket(AF_INET, SOCK_DGRAM, 0);
	if (udpsock < 0) {
		syslog(LOG_ERR, "opening dgram socket: %m");
		exit(1);
	}

	i = 1;
	if (setsockopt(udpsock, SOL_SOCKET, SO_REUSEADDR,
		       (char *)&i, sizeof(i)) < 0)
		syslog(LOG_ERR, "control setsockopt: %m");;
	
	/* Create name. */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons((u_short) portnum);
	if (bind(udpsock, (struct sockaddr *) &name, sizeof(name))) {
		syslog(LOG_ERR, "binding stream socket: %m");
		exit(1);
	}

	/* Find assigned port value and print it out. */
	length = sizeof(name);
	if (getsockname(udpsock, (struct sockaddr *) &name, &length)) {
		syslog(LOG_ERR, "getting socket name: %m");
		exit(1);
	}
	syslog(LOG_NOTICE, "listening on UDP port %d", ntohs(name.sin_port));

	while (1) {
		struct sockaddr_in client;
		struct sockaddr_in redirect_client;
		int		   redirect = 0;
		int		   clientsock, length = sizeof(client);
		char		   buf[MYBUFSIZE], *bp, *cp;
		int		   cc, nfds, istcp;
		fd_set		   fds;

		nfds = udpsock;
		if (tcpsock > nfds)
			nfds = tcpsock;
		FD_ZERO(&fds);
		FD_SET(tcpsock, &fds);
		FD_SET(udpsock, &fds);

		nfds = select(nfds+1, &fds, 0, 0, 0);
		if (nfds <= 0) {
			syslog(LOG_ERR, nfds == 0 ?
			       "select: returned zero!?" : "select: %m");
			exit(1);
		}

		if (FD_ISSET(tcpsock, &fds)) {
			istcp = 1;
			clientsock = accept(tcpsock,
					    (struct sockaddr *)&client,
					    &length);

			/*
			 * Read in the command request.
			 */
			if ((cc = read(clientsock, buf, MYBUFSIZE - 1)) <= 0) {
				if (cc < 0)
					syslog(LOG_ERR, "Reading request: %m");
				syslog(LOG_ERR, "Connection aborted");
				close(clientsock);
				continue;
			}
		} else {
			istcp = 0;
			clientsock = udpsock;

			cc = recvfrom(clientsock, buf, MYBUFSIZE - 1,
				      0, (struct sockaddr *)&client, &length);
			if (cc <= 0) {
				if (cc < 0)
					syslog(LOG_ERR, "Reading request: %m");
				syslog(LOG_ERR, "Connection aborted");
				continue;
			}
		}

		buf[cc] = '\0';
		bp = buf;

		/*
		 * Look for REDIRECT, which is a proxy request for a
		 * client other than the one making the request. Good
		 * for testing. Might become a general tmcd redirect at
		 * some point, so that we can test new tmcds.
		 */
		if (strncmp("REDIRECT=", buf, strlen("REDIRECT=")) == 0) {
			char *tp;
			
			bp += strlen("REDIRECT=");
			tp = bp;
			while (! isspace(*tp))
				tp++;
			*tp++ = '\0';
			redirect_client = client;
			redirect        = 1;
			inet_aton(bp, &client.sin_addr);
			bp = tp;
		}
		cp = (char *) malloc(cc + 1);
		assert(cp);
		strcpy(cp, bp);
		
#ifndef	TESTMODE
		/*
		 * IN TESTMODE, we allow redirect.
		 * Otherwise not since that would be a (minor) privacy
		 * risk, by allowing testbed nodes to get info for other
		 * nodes.
		 */
		if (redirect) {
			char	buf1[32], buf2[32];

			strcpy(buf1, inet_ntoa(redirect_client.sin_addr));
			strcpy(buf2, inet_ntoa(client.sin_addr));
			
			syslog(LOG_INFO,
			       "%s INVALID REDIRECT: %s", buf1, buf2);
			goto skipit;
		}
#endif
		/*
		 * Check for a redirect using the default DB. This allows
		 * for a simple redirect to a secondary DB for testing.
		 * Not very general. Might change to full blown tmcd
		 * redirection at some point, but this is a very quick and
		 * easy hack. Upon return, the dbname has been changed if
		 * redirection is in force. 
		 */
		strcpy(dbname, DEFAULT_DBNAME);
		if (checkdbredirect(client.sin_addr)) {
			/* Something went wrong */
			goto skipit;
		}
		
		/*
		 * Figure out what command was given.
		 */
		for (i = 0; i < numcommands; i++)
			if (strncmp(cp, command_array[i].cmdname,
				    strlen(command_array[i].cmdname)) == 0)
				break;

		/*
		 * And execute it.
		 */
		if (i == numcommands) {
			syslog(LOG_INFO, "%s INVALID REQUEST: %.8s...",
			       inet_ntoa(client.sin_addr), cp);
			goto skipit;
		}
		else {
			bp = cp + strlen(command_array[i].cmdname);

			/*
			 * XXX hack, don't log "log" contents,
			 * both for privacy and to keep our syslog smaller.
			 */
			if (command_array[i].func == dolog)
				syslog(LOG_INFO, "%s REQUEST: log %d chars",
				       inet_ntoa(client.sin_addr), strlen(bp));
			else
				syslog(LOG_INFO, "%s REQUEST: %s",
				       inet_ntoa(client.sin_addr),
				       command_array[i].cmdname);

			err = command_array[i].func(clientsock,
						    client.sin_addr,
						    bp, istcp);

			syslog(LOG_INFO, "%s REQUEST: %s: returned %d",
			       inet_ntoa(client.sin_addr),
			       command_array[i].cmdname, err);
		}

	skipit:
		free(cp);
		if (redirect) 
			client = redirect_client;

		if (istcp)
			close(clientsock);
		else
			client_writeback_done(clientsock, &client);
	}

	close(tcpsock);
	close(udpsock);
	syslog(LOG_NOTICE, "daemon terminating");
	exit(0);
}

/*
 * Accept notification of reboot. 
 */
static int
doreboot(int sock, struct in_addr ipaddr, char *rdata, int tcp)
{
	MYSQL_RES	*res;	
	char		nodeid[32];
	char		pid[64];
	char		eid[64];
	char		gid[64];

	if (iptonodeid(ipaddr, nodeid)) {
		syslog(LOG_ERR, "REBOOT: %s: No such node", inet_ntoa(ipaddr));
		return 1;
	}

	syslog(LOG_INFO, "REBOOT: %s is reporting a reboot", nodeid);

	/*
	 * Need to check the pid/eid to distinguish between a user
	 * initiated reload, and a admin scheduled reload. We don't want
	 * to deschedule an admin reload, which is supposed to happen when
	 * the node is released.
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid)) {
		syslog(LOG_INFO, "REBOOT: %s: Node is free", nodeid);
		return 0;
	}

	syslog(LOG_INFO, "REBOOT: %s: Node is in experiment %s/%s",
	       nodeid, pid, eid);

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
	res = mydb_query("select node_id from scheduled_reloads where node_id='%s'",
			 1, nodeid);
	if (!res) {
		syslog(LOG_ERR, "REBOOT: %s: DB Error getting reload!",
		       nodeid);
		return 1;
	}
	if ((int)mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return 0;
	}
	mysql_free_result(res);

	if (mydb_update("delete from scheduled_reloads where node_id='%s'", nodeid)) {
		syslog(LOG_ERR, "REBOOT: %s: DB Error clearing reload!",
		       nodeid);
		return 1;
	}
	syslog(LOG_INFO, "REBOOT: %s cleared reload state", nodeid);

	if (mydb_update("delete from reserved where node_id='%s'", nodeid)) {
		syslog(LOG_ERR, "REBOOT: %s: DB Error clearing reload!",
		       nodeid);
		return 1;
	}
	syslog(LOG_INFO, "REBOOT: %s cleared reserved state", nodeid);

	return 0;
}

/*
 * Return status of node. Is it allocated to an experiment, or free.
 */
static int
dostatus(int sock, struct in_addr ipaddr, char *rdata, int tcp)
{
	char		nodeid[32];
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		nickname[128];
	char		buf[MYBUFSIZE];

	if (iptonodeid(ipaddr, nodeid)) {
		syslog(LOG_ERR, "STATUS: %s: No such node", inet_ntoa(ipaddr));
		return 1;
	}

	/*
	 * Now check reserved table
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid)) {
		syslog(LOG_INFO, "STATUS: %s: Node is free", nodeid);
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

	syslog(LOG_INFO, "STATUS: %s: %s", nodeid, buf);
	return 0;
}

/*
 * Return ifconfig information to client.
 */
static int
doifconfig(int sock, struct in_addr ipaddr, char *rdata, int tcp)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		nodeid[32];
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		buf[MYBUFSIZE];
	int		control_net, nrows;

	if (iptonodeid(ipaddr, nodeid)) {
		syslog(LOG_ERR, "IFCONFIG: %s: No such node",
		       inet_ntoa(ipaddr));
		return 1;
	}

	/*
	 * Now check reserved table
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid)) {
		syslog(LOG_INFO, "IFCONFIG: %s: Node is free", nodeid);
		return 1;
	}

	/*
	 * Need to know the control network for the machine since
	 * we don't want to mess with that.
	 */
	if (nodeidtocontrolnet(nodeid, &control_net)) {
		syslog(LOG_ERR, "IFCONFIG: %s: No Control Network", nodeid);
		return 1;
	}

	/*
	 * Find all the interfaces.
	 */
	res = mydb_query("select card,IP,IPalias,MAC from interfaces "
			 "where node_id='%s'",
			 4, nodeid);
	if (!res) {
		syslog(LOG_ERR, "IFCONFIG: %s: DB Error getting interfaces!",
		       nodeid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		syslog(LOG_ERR, "IFCONFIG: %s: No interfaces!", nodeid);
		mysql_free_result(res);
		return 1;
	}
	while (nrows) {
		row = mysql_fetch_row(res);
		if (row[1] && row[1][0]) {
			int card = atoi(row[0]);

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

			strcat(buf, "\n");
			client_writeback(sock, buf, strlen(buf), tcp);
			syslog(LOG_INFO, "IFCONFIG: %s", buf);
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
static int
doaccounts(int sock, struct in_addr ipaddr, char *rdata, int tcp)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		nodeid[32];
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		buf[MYBUFSIZE];
	int		nrows, gidint;
	int		shared = 0, tbadmin;

	if (iptonodeid(ipaddr, nodeid)) {
		syslog(LOG_ERR, "ACCOUNTS: %s: No such node",
		       inet_ntoa(ipaddr));
		return 1;
	}

	/*
	 * Now check reserved table
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid)) {
		syslog(LOG_ERR, "ACCOUNTS: %s: Node is free", nodeid);
		return 1;
	}

#ifdef  NOSHAREDEXPTS
	/*
	 * We have the pid name, but we need the GID number from the
	 * projects table to send over. 
	 */
	res = mydb_query("select unix_name,unix_gid from groups "
			 "where pid='%s'",
			 2, pid);
#else
	/*
	 * Get a list of gid/unix_gid for each group that is allowed
	 * access to the experiments nodes. This is the owner of the
	 * node, plus the additional pids granted access. 
	 */
	res = mydb_query("select g.unix_name,g.unix_gid from groups as g "
			 "left join exppid_access as a on g.pid=a.pid "
			 "where g.pid='%s' or "
			 "      (a.exp_pid='%s' and a.exp_eid='%s')",
			 2, pid, pid, eid);
#endif
	if (!res) {
		syslog(LOG_ERR, "ACCOUNTS: %s: DB Error getting gids!", pid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		syslog(LOG_ERR, "ACCOUNTS: %s: No Project!", pid);
		mysql_free_result(res);
		return 1;
	}

	while (nrows) {
		row = mysql_fetch_row(res);
		if (!row[1] || !row[1][1]) {
			syslog(LOG_ERR, "ACCOUNTS: %s: No Project GID!", pid);
			mysql_free_result(res);
			return 1;
		}

		gidint = atoi(row[1]);
		sprintf(buf, "ADDGROUP NAME=%s GID=%d\n", row[0], gidint);
		client_writeback(sock, buf, strlen(buf), tcp);
		syslog(LOG_INFO, "ACCOUNTS: %s", buf);

		nrows--;
	}
	mysql_free_result(res);

	/*
	 * Now onto the users in the project.
	 */
#ifdef  NOSHAREDEXPTS
	res = mydb_query("select distinct "
			 "  u.uid,u.usr_pswd,u.unix_uid,u.usr_name, "
			 "  p.trust,p.pid,p.gid,g.unix_gid,u.admin "
			 "from users as u "
			 "left join group_membership as p on p.uid=u.uid "
			 "left join groups as g on p.pid=g.pid "
			 "where p.pid='%s' and p.gid='%s' "
			 "      and u.status='active' order by u.uid",
			 9, pid, eid, pid, gid);
#else
	/*
	 * See if a shared experiment. Used below.
	 */
	res = mydb_query("select shared from experiments "
			 "where pid='%s' and eid='%s'",
			 1, pid, eid);
	
	if (!res) {
		syslog(LOG_ERR, "ACCOUNTS: %s: DB Error getting shared!", pid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		syslog(LOG_ERR, "ACCOUNTS: %s: No Experiment %s!", pid, eid);
		mysql_free_result(res);
		return 0;
	}
	row = mysql_fetch_row(res);
	shared = atoi(row[0]);
	mysql_free_result(res);
	
	/*
	 * This crazy join is going to give us multiple lines for each
	 * user that is allowed on the node, where each line (for each user)
	 * differs by the project PID and it unix GID. The intent is to
	 * build up a list of GIDs for each user to return. Well, a primary
	 * group and a list of aux groups for that user. It might be cleaner
	 * to do this as multiple querys, but this makes it atomic.
	 */
	if (strcmp(pid, gid)) {
		res = mydb_query("select distinct "
			 "  u.uid,u.usr_pswd,u.unix_uid,u.usr_name, "
			 "  p.trust,g.pid,g.gid,g.unix_gid,u.admin "
			 "from users as u "
			 "left join group_membership as p on p.uid=u.uid "
			 "left join groups as g on p.pid=g.pid "
			 "where ((p.pid='%s' and p.gid='%s')) "
			 "      and u.status='active' order by u.uid",
			 9, pid, gid);
	}
	else {
		res = mydb_query("select distinct "
			 "  u.uid,u.usr_pswd,u.unix_uid,u.usr_name, "
			 "  p.trust,g.pid,g.gid,g.unix_gid,u.admin "
			 "from users as u "
			 "left join group_membership as p on p.uid=u.uid "
			 "left join groups as g on "
			 "     p.pid=g.pid and p.gid=g.gid "
			 "where ((p.pid='%s')) "
			 "      and u.status='active' order by u.uid",
			 9, pid);
	}
#endif
	if (!res) {
		syslog(LOG_ERR, "ACCOUNTS: %s: DB Error getting users!", pid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		syslog(LOG_ERR, "ACCOUNTS: %s: No Users!", pid);
		mysql_free_result(res);
		return 0;
	}

	row = mysql_fetch_row(res);
	while (nrows) {
		MYSQL_ROW	nextrow;
		int		i, root = 0;
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

		/*
		 * Override root when a shared experiment, except for
		 * TB admin people.
		 */
		if (shared && !tbadmin)
			root = 0;

		sprintf(buf,
			"ADDUSER LOGIN=%s "
			"PSWD=%s UID=%s GID=%d ROOT=%d NAME=\"%s\" "
			"HOMEDIR=%s/%s GLIST=%s\n",
			row[0], row[1], row[2], gidint, root, row[3],
			USERDIR, row[0], glist);
			
		client_writeback(sock, buf, strlen(buf), tcp);
		syslog(LOG_INFO, "ACCOUNTS: %s", buf);

		row = nextrow;
	}
	mysql_free_result(res);

	return 0;
}

/*
 * Return account stuff.
 */
static int
dodelay(int sock, struct in_addr ipaddr, char *rdata, int tcp)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		nodeid[32];
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		buf[MYBUFSIZE];
	int		nrows;

	if (iptonodeid(ipaddr, nodeid)) {
		syslog(LOG_ERR, "DELAY: %s: No such node",
		       inet_ntoa(ipaddr));
		return 1;
	}

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
	res = mydb_query("select i.MAC,j.MAC,delay,bandwidth,lossrate "
                         " from delays "
			 "left join interfaces as i on "
			 " i.node_id=delays.node_id and i.iface=iface0 "
			 "left join interfaces as j on "
			 " j.node_id=delays.node_id and j.iface=iface1 "
			 " where delays.node_id='%s'",	 
			 5, nodeid);
	if (!res) {
		syslog(LOG_ERR, "DELAY: %s: DB Error getting delays!",
		       nodeid);
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
			syslog(LOG_ERR, "DELAY: %s: DB values are bogus!",
			       nodeid);
			mysql_free_result(res);
			return 1;
		}

		sprintf(buf,
			"DELAY INT0=%s INT1=%s DELAY=%s BW=%s PLR=%s\n",
			row[0], row[1], row[2], row[3], row[4]);
			
		client_writeback(sock, buf, strlen(buf), tcp);
		nrows--;
		syslog(LOG_INFO, "DELAY: %s", buf);
	}
	mysql_free_result(res);

	return 0;
}

/*
 * Return host table information.
 */
static int
dohosts(int sock, struct in_addr ipaddr, char *rdata, int tcp)
{

	char *tmp, *buf, *vname_list;
	char pid[64], eid[64];
	char		gid[64];
	char nickname[128]; /* XXX: Shouldn't be statically sized, potential buffer
						 * overflow
						 */ 

	char nodeid[32]; /* Testbed ID of the node */
	int control_net; /* Control network interface on this host */
	int ninterfaces; /* Number of interfaces on the host */
	int nnodes; /* Number of other nodes in this experiment */


	char *last_id; /* Used to determine link# */
	int link;
	int seen_direct;

	MYSQL_RES	*interface_result;
	MYSQL_RES	*nodes_result;
	MYSQL_RES	*vlan_result;
	MYSQL_RES	*reserved_result;
	MYSQL_ROW	row;

	struct node_interface *connected_interfaces, *temp_interface;
	int nvlans;

	if (iptonodeid(ipaddr, nodeid)) {
		syslog(LOG_ERR, "HOSTNAMES: %s: No such node",
		       inet_ntoa(ipaddr));
		return 1;
	}

	/*
	 * Now check reserved table
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid))
		return 0;
	/*
	 * Figure out which of our interfaces is on the control network
	 */

	interface_result = mydb_query("SELECT control_net "
			"FROM nodes LEFT JOIN node_types ON nodes.type = node_types.type "
			"WHERE nodes.node_id = '%s'",
			1, nodeid);

	if (!interface_result) {
		syslog(LOG_ERR, "dohosts: %s: DB Error getting interface!", nodeid);
		return 1;
	}

	/*
	 * There should be exactly 1 control network interface!
	 */
	if ((int)mysql_num_rows(interface_result) != 1) {
		mysql_free_result(interface_result);
		syslog(LOG_ERR, "HOSTNAMES: Unable to get control interface for %s",nodeid);
		return 0;
	}

	row = mysql_fetch_row(interface_result);
	control_net = atoi(row[0]);
	mysql_free_result(interface_result);

	/*
	 * For the virt_lans table, we need the virtual name, not the node_id
	 */
	if (nodeidtonickname(nodeid,nickname)) {
		strcpy(nickname,nodeid);
	}

	/*
	 * Now, we need to look through the virt_lans table to find a list of all the
	 * VLANs we're in
	 */
	vlan_result = mydb_query("SELECT vname "
			"FROM virt_lans "
			"WHERE member LIKE '%s:%%' AND pid='%s' AND eid='%s'",
			1, nickname,pid,eid);

	if (!vlan_result) {
		syslog(LOG_ERR, "dohosts: %s: DB Error getting virt_lans!", nodeid);
		return 1;
	}

	nvlans = (int)mysql_num_rows(vlan_result);
	connected_interfaces = NULL;

	/*
	 * Initializing vname_list to "0" makes the ORs easier to get right,
	 * and I want to make sure this string gets malloc()ed, to prevent
	 * problems with the free() inside the loop
	 */
	vname_list = (char*)malloc(2);
	strcpy(vname_list,"0");

	while (nvlans--) {
		row = mysql_fetch_row(vlan_result);

		/*
		 * Add this vlan to an OR of vlan names that could contain direct neighbors
		 */
		tmp = (char*)malloc(strlen(vname_list) + strlen(row[0]) +14);
		strcpy(tmp,vname_list);
		strcat(tmp," OR vname ='");
		strcat(tmp,row[0]);
		strcat(tmp,"'");
		free(vname_list);
		vname_list = tmp;

	}
	mysql_free_result(vlan_result);

	/*
	 * Now, get the members of all these VLANs
	 */
	vlan_result = mydb_query("SELECT member "
			"FROM virt_lans "
			"WHERE pid='%s' AND eid='%s' AND ( %s )",
			1, pid,eid,vname_list);
	free(vname_list);

	if (!vlan_result) {
		syslog(LOG_ERR, "dohosts: %s: DB Error getting virt_lan members!", nodeid);
		return 1;
	}

	nvlans = (int)mysql_num_rows(vlan_result);
	connected_interfaces = NULL;

	while (nvlans--) {
		struct node_interface *interface;

		row = mysql_fetch_row(vlan_result);

		/*
		 * Add this member to the linked list of directly connected
		 * interfaces
		 */
		interface = (struct node_interface *)malloc(sizeof(struct node_interface *));
		interface->iface = (char*)malloc(strlen(row[0]) +1);
		strcpy(interface->iface,row[0]);
		interface->next = connected_interfaces;
		connected_interfaces = interface;

	}

	mysql_free_result(vlan_result);

	/*
	 * Grab a list of all other hosts in this experiment - we'll sort out the
	 * directly connected ones while looping through them.
	 */
	nodes_result = 
		mydb_query("SELECT DISTINCT i.node_id, i.IP, i.IPalias, r.vname, CONCAT(r.vname,':',p.vport), i.card = t.control_net "
				"FROM interfaces AS i LEFT JOIN reserved AS r ON i.node_id = r.node_id "
				"LEFT JOIN nodes AS n ON i.node_id = n.node_id "
				"LEFT JOIN node_types AS t ON n.type = t.type "
				"LEFT JOIN portmap AS p ON i.iface = p.pport AND p.vnode=r.vname AND p.pid=r.pid AND p.eid=r.eid "
				"WHERE IP IS NOT NULL AND IP != '' AND r.pid='%s' AND r.eid='%s'"
				"ORDER BY node_id DESC, IP",
				6,pid,eid);

	if (!nodes_result) {
		syslog(LOG_ERR, "dohosts: %s: DB Error getting other nodes "
				"in the experiment!", nodeid);
		return 1;
	}
	
	if ((nnodes = (int)mysql_num_rows(nodes_result)) == 0) {
		syslog(LOG_ERR, "dohosts: %s: No nodes in this experiment!", nodeid);
		mysql_free_result(nodes_result);
		return 0;
	}

	last_id = ""; 
	link = 0; /* Acts as 'lindex' from the virt_names table */
	seen_direct = 0; /* Set to 1 if we've already seen at least
						one direct connection to this node */
	while (nnodes--) {
		MYSQL_ROW node_row = mysql_fetch_row(nodes_result);
		char	  *vname, vbuf[256];

		/*
		 * Watch for null vnames. Just construct something to make
		 * everyone happy.
		 */
		if (node_row[3])
			vname = node_row[3];
		else {
			strcpy(vbuf, "V");
			strcat(vbuf, node_row[0]);
			vname = vbuf;
		}

		/*
		 * Either the IP or IPalias column (or both!) could have
		 * matched. Code duplication galore!
		 */

		/*
		 * Let's take care of the IP first!
		 */

		/* Skip the control network interface */
		if (!(node_row[5] && atoi(node_row[5]))) {

			/*
			 * Keep track of node_ids, so we can get the LINK number rigth
			 */
			if (!strcmp(node_row[0],last_id)) {
				link++;
			} else {
				link = seen_direct = 0;
				last_id = node_row[0];
			}


			if (directly_connected(connected_interfaces,node_row[4])) {
				sprintf(buf, "NAME=%s LINK=%i IP=%s ALIAS=%s\n",
						vname, link, node_row[1],
						(!seen_direct) ? vname : " ");
				seen_direct = 1;
			} else {
				sprintf(buf, "NAME=%s LINK=%i IP=%s ALIAS= \n",
					vname, link, node_row[1]);
			}

			client_writeback(sock, buf, strlen(buf), tcp);
			syslog(LOG_INFO, "HOSTNAMES: %s", buf);
		}

		/*
		 * Make sure it really has an IPalias!
		 */

		if (node_row[2] && strcmp(node_row[2],"")) {
			if (!strcmp(node_row[0],last_id)) {
				link++;
			} else {
				link = seen_direct = 0;
				last_id = node_row[0];
			}

			if (directly_connected(connected_interfaces,node_row[4])) {
				sprintf(buf, "NAME=%s LINK=%i IP=%s ALIAS=%s\n",
						vname, link, node_row[2],
						(!seen_direct) ? vname : " ");
				seen_direct = 1;
			} else {
				sprintf(buf, "NAME=%s LINK=%i IP=%s ALIAS= \n",
					vname, link, node_row[2]);
			}

			client_writeback(sock, buf, strlen(buf), tcp);
			syslog(LOG_INFO, "HOSTNAMES: %s", buf);

		}
	}
	mysql_free_result(nodes_result);

	/*
	 * Clean up our linked list of interfaces
	 */
	while (connected_interfaces != NULL) {
		temp_interface = connected_interfaces;
		connected_interfaces = connected_interfaces->next;
		free(temp_interface->iface);
		free(temp_interface);
	}
	
	return 0;

}

/*
 * Return RPM stuff.
 */
static int
dorpms(int sock, struct in_addr ipaddr, char *rdata, int tcp)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		nodeid[32];
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		buf[MYBUFSIZE], *bp, *sp;

	if (iptonodeid(ipaddr, nodeid)) {
		syslog(LOG_ERR, "TARBALLS: %s: No such node",
		       inet_ntoa(ipaddr));
		return 1;
	}

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
		syslog(LOG_ERR, "RPMS: %s: DB Error getting RPMS!",
		       nodeid);
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
		syslog(LOG_INFO, "RPM: %s", buf);
		
	} while (bp = sp);
	
	mysql_free_result(res);
	return 0;
}

/*
 * Return Tarball stuff.
 */
static int
dotarballs(int sock, struct in_addr ipaddr, char *rdata, int tcp)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		nodeid[32];
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		buf[MYBUFSIZE], *bp, *sp, *tp;

	if (iptonodeid(ipaddr, nodeid)) {
		syslog(LOG_ERR, "TARBALLS: %s: No such node",
		       inet_ntoa(ipaddr));
		return 1;
	}

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
		syslog(LOG_ERR, "TARBALLS: %s: DB Error getting tarballs!",
		       nodeid);
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
		syslog(LOG_INFO, "TARBALLS: %s", buf);
		
	} while (bp = sp);
	
	mysql_free_result(res);
	return 0;
}

/*
 * Return Deltas stuff.
 */
static int
dodeltas(int sock, struct in_addr ipaddr, char *rdata, int tcp)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		nodeid[32];
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		buf[MYBUFSIZE], *bp, *sp;

	if (iptonodeid(ipaddr, nodeid)) {
		syslog(LOG_ERR, "DELTAS: %s: No such node",
		       inet_ntoa(ipaddr));
		return 1;
	}

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
		syslog(LOG_ERR, "DELTAS: %s: DB Error getting Deltas!",
		       nodeid);
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
		syslog(LOG_INFO, "DELTAS: %s", buf);
		
	} while (bp = sp);
	
	mysql_free_result(res);
	return 0;
}

/*
 * Return node run command. We return the command to run, plus the UID
 * of the experiment creator to run it as!
 */
static int
dostartcmd(int sock, struct in_addr ipaddr, char *rdata, int tcp)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		nodeid[32];
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		buf[MYBUFSIZE], *bp, *sp;

	if (iptonodeid(ipaddr, nodeid)) {
		syslog(LOG_ERR, "STARTUPCMD: %s: No such node",
		       inet_ntoa(ipaddr));
		return 1;
	}

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
		syslog(LOG_ERR, "STARTUPCMD: %s: DB Error getting "
		       "startup command!",
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
		syslog(LOG_ERR, "STARTUPCMD: %s: DB Error getting UID!",
		       nodeid);
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
	syslog(LOG_INFO, "STARTUPCMD: %s", buf);
	
	return 0;
}

/*
 * Accept notification of start command exit status. 
 */
static int
dostartstat(int sock, struct in_addr ipaddr, char *rdata, int tcp)
{
	MYSQL_RES	*res;	
	char		nodeid[32];
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		*exitstatus;

	if (iptonodeid(ipaddr, nodeid)) {
		syslog(LOG_ERR, "STARTSTAT: %s: No such node",
		       inet_ntoa(ipaddr));
		return 1;
	}

	/*
	 * Dig out the exit status
	 */
	while (isspace(*rdata))
		rdata++;
	exitstatus = rdata;

	syslog(LOG_INFO, "STARTSTAT: "
	       "%s is reporting startup command exit status: %s",
	       nodeid, exitstatus);

	/*
	 * Make sure currently allocated to an experiment!
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid)) {
		syslog(LOG_INFO, "STARTSTAT: %s: Node is free", nodeid);
		return 0;
	}

	syslog(LOG_INFO, "STARTSTAT: %s: Node is in experiment %s/%s",
	       nodeid, pid, eid);

	/*
	 * Update the node table record with the exit status. Setting the
	 * field to a non-null string value is enough to tell whoever is
	 * watching it that the node is done.
	 */
	if (mydb_update("update nodes set startstatus='%s' "
			"where node_id='%s'", exitstatus, nodeid)) {
		syslog(LOG_ERR, "STARTSTAT: %s: DB Error setting exit status!",
		       nodeid);
		return 1;
	}
	return 0;
}

/*
 * Accept notification of ready for action
 */
static int
doready(int sock, struct in_addr ipaddr, char *rdata, int tcp)
{
	char		nodeid[32];
	char		pid[64];
	char		eid[64];
	char		gid[64];

	if (iptonodeid(ipaddr, nodeid)) {
		syslog(LOG_ERR, "READY: %s: No such node",
		       inet_ntoa(ipaddr));
		return 1;
	}

	/*
	 * Make sure currently allocated to an experiment!
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid)) {
		syslog(LOG_INFO, "READY: %s: Node is free", nodeid);
		return 0;
	}

	/*
	 * Update the ready_bits table.
	 */
	if (mydb_update("update nodes set ready=1 "
			"where node_id='%s'", nodeid)) {
		syslog(LOG_ERR, "READY: %s: DB Error setting ready bit!",
		       nodeid);
		return 1;
	}

	syslog(LOG_INFO, "READY: %s: Node is reporting ready", nodeid);

	/*
	 * Nothing is written back
	 */
	return 0;
}

/*
 * Return ready bits count (NofM)
 */
static int
doreadycount(int sock, struct in_addr ipaddr, char *rdata, int tcp)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		nodeid[32];
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		buf[MYBUFSIZE];
	int		total, ready, i;

	if (iptonodeid(ipaddr, nodeid)) {
		syslog(LOG_ERR, "READYCOUNT: %s: No such node",
		       inet_ntoa(ipaddr));
		return 1;
	}

	/*
	 * Make sure currently allocated to an experiment!
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid)) {
		syslog(LOG_INFO, "READYCOUNT: %s: Node is free", nodeid);
		return 0;
	}

	/*
	 * See how many are ready. This is a non sync protocol. Clients
	 * keep asking until N and M are equal. Can only be used once
	 * of course, after experiment creation.
	 */
	res = mydb_query("SELECT ready FROM nodes "
			 "LEFT JOIN reserved "
			 "ON nodes.node_id=reserved.node_id "
			 "WHERE reserved.eid='%s' and reserved.pid='%s'",
			 1, eid, pid);

	if (!res) {
		syslog(LOG_ERR, "READYCOUNT: %s: DB Error getting ready bits",
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

	sprintf(buf, "READY=%d TOTAL=%d\n", ready, total);
	client_writeback(sock, buf, strlen(buf), tcp);
		
	syslog(LOG_INFO, "READYCOUNT: %s: %s", nodeid, buf);

	return 0;
}

static char logfmt[] = "/proj/%s/logs/%s.log";

/*
 * Log some text to a file in the /proj/<pid>/exp/<eid> directory.
 */
static int
dolog(int sock, struct in_addr ipaddr, char *rdata, int tcp)
{
	char		nodeid[32];
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
	if (iptonodeid(ipaddr, nodeid)) {
		syslog(LOG_ERR, "LOG: %s: No such node",
		       inet_ntoa(ipaddr));
		return 1;
	}
	if (nodeidtoexp(nodeid, pid, eid, gid)) {
		syslog(LOG_INFO, "LOG: %s: Node is free", nodeid);
		return 1;
	}

	snprintf(logfile, sizeof(logfile)-1, logfmt, pid, eid);
	fd = fopen(logfile, "a");
	if (fd == NULL) {
		syslog(LOG_ERR, "LOG: %s: Could not open %s\n",
		       inet_ntoa(ipaddr), logfile);
		return 1;
	}

	curtime = time(0);
	tstr = ctime(&curtime);
	tstr[19] = 0;	/* no year */
	tstr += 4;	/* or day of week */

	while (isspace(*rdata))
		rdata++;

	fprintf(fd, "%s: %s\n\n%s\n=======\n", tstr, inet_ntoa(ipaddr), rdata);
	fclose(fd);

	return 0;
}

/*
 * Return mount stuff.
 */
static int
domounts(int sock, struct in_addr ipaddr, char *rdata, int tcp)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		nodeid[32];
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		buf[MYBUFSIZE];
	int		nrows;

	if (iptonodeid(ipaddr, nodeid)) {
		syslog(LOG_ERR, "MOUNTS: %s: No such node",
		       inet_ntoa(ipaddr));
		return 1;
	}

	/*
	 * Now check reserved table
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid)) {
		syslog(LOG_ERR, "MOUNTS: %s: Node is free", nodeid);
		return 1;
	}

	/*
	 * Return project mount first. 
	 */
	sprintf(buf, "REMOTE=%s/%s LOCAL=%s/%s\n",
		FSPROJDIR, pid, PROJDIR, pid);
	client_writeback(sock, buf, strlen(buf), tcp);

	/*
	 * Now check for aux project access. Return a list of mounts for
	 * those projects.
	 */
	res = mydb_query("select pid from exppid_access "
			 "where exp_pid='%s' and exp_eid='%s'",
			 1, pid, eid);
	if (!res) {
		syslog(LOG_ERR, "MOUNTS: %s: DB Error getting users!", pid);
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
			 "where p.pid='%s' and p.pid=p.gid and "
			 "      u.status='active'",
			 1, pid);
#else
	res = mydb_query("select distinct u.uid from users as u "
			 "left join exppid_access as a "
			 " on a.exp_pid='%s' and a.exp_eid='%s' "
			 "left join group_membership as p on p.uid=u.uid "
			 "where ((p.pid='%s' and p.pid=p.gid) or p.pid=a.pid) "
			 "       and u.status='active'",
			 1, pid, eid, pid);
#endif
	if (!res) {
		syslog(LOG_ERR, "MOUNTS: %s: DB Error getting users!", pid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		syslog(LOG_ERR, "MOUNTS: %s: No Users!", pid);
		mysql_free_result(res);
		return 0;
	}

	while (nrows) {
		row = mysql_fetch_row(res);
				
		sprintf(buf, "REMOTE=%s/%s LOCAL=%s/%s\n",
			FSUSERDIR, row[0], USERDIR, row[0]);
		client_writeback(sock, buf, strlen(buf), tcp);
		
		nrows--;
		syslog(LOG_INFO, "MOUNTS: %s", buf);
	}
	mysql_free_result(res);

	return 0;
}

/*
 * Return address from which to load an image, along with the partition that
 * it should be written to
 */
static int
doloadinfo(int sock, struct in_addr ipaddr, char *rdata, int tcp)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		nodeid[32];
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		buf[MYBUFSIZE], *bp, *sp;

	if (iptonodeid(ipaddr, nodeid)) {
		syslog(LOG_ERR, "doloadinfo: %s: No such node",
		       inet_ntoa(ipaddr));
		return 1;
	}

	/*
	 * Get the address the node should contact to load its image
	 */
	res = mydb_query("select load_address,loadpart from images as i "
			"left join current_reloads as r on i.imageid = r.image_id "
			"where node_id='%s'",
			 2, nodeid);

	if (!res) {
		syslog(LOG_ERR, "doloadinfo: %s: DB Error getting "
		       "loading address!",
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
	syslog(LOG_INFO, "doloadinfo: %s", buf);
	
	return 0;
}

/*
 * If next_pxe_boot_path is set, clear it. Otherwise, if next_boot_path
 * or next_boot_osid is set, clear both along with next_boot_cmd_line.
 * If neither is set, do nothing. Produces no output to the client.
 */
static int
doreset(int sock, struct in_addr ipaddr, char *rdata, int tcp)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		nodeid[32];
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		buf[MYBUFSIZE], *bp, *sp;

	if (iptonodeid(ipaddr, nodeid)) {
		syslog(LOG_ERR, "doreset: %s: No such node",
		       inet_ntoa(ipaddr));
		return 1;
	}

	/*
	 * Now check reserved table
	 */
	if (nodeidtoexp(nodeid, pid, eid, gid))
		return 0;

	/*
	 * Check to see if next_pxe_boot_path is set
	 */
	res = mydb_query("select next_pxe_boot_path from nodes "
			"where node_id='%s' and next_pxe_boot_path is not null "
			"and next_pxe_boot_path != ''",
			 1, nodeid);

	if (!res) {
		syslog(LOG_ERR, "doreset: %s: DB Error checking for "
		       "next_pxe_boot_path!",
		       nodeid);
		return 1;
	}

	/*
	 * If we get a row back, then next_pxe_boot_path was set, because
	 * the query checks for emptiness
	 */
	if ((int)mysql_num_rows(res) > 0) {
		mysql_free_result(res);
		syslog(LOG_INFO, "doreset: %s: Clearing next_pxe_boot_path",
			nodeid);
		if (mydb_update("update nodes set next_pxe_boot_path='' "
			"where node_id='%s'", nodeid)) {
		    syslog(LOG_ERR, "doreset: %s: DB Error clearing "
			    "next_pxe_boot_path!", nodeid);
		    return 1;
		}
		syslog(LOG_INFO, "doreset: %s: Clearing current_reloads",
			nodeid);
		if (mydb_update("delete from current_reloads "
			"where node_id='%s'", nodeid)) {
		    syslog(LOG_ERR, "doreset: %s: DB Error clearing "
			    "current_reloads!", nodeid);
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
		syslog(LOG_ERR, "doreset: %s: DB Error checking for "
		       "next_boot_path or next_boot_osid!",
		       nodeid);
		return 1;
	}

	/*
	 * If we get a row back, then one of them was set, because
	 * the query checks for emptiness
	 */
	if ((int)mysql_num_rows(res) > 0) {
		mysql_free_result(res);
		syslog(LOG_INFO, "doreset: %s: Clearing next_boot_*",
			nodeid);
		if (mydb_update("update nodes set next_boot_path='',"
			"next_boot_osid='',next_boot_cmd_line='' "
			"where node_id='%s'", nodeid)) {
		    syslog(LOG_ERR, "doreset: %s: DB Error clearing "
			    "next_boot_*!", nodeid);
		    return 1;
		}
		return 0;
	}

	
	return 0;
}

/*
 * DB stuff
 */
MYSQL_RES *
mydb_query(char *query, int ncols, ...)
{
	MYSQL		db;
	MYSQL_RES	*res;
	char		querybuf[2*MYBUFSIZE];
	va_list		ap;
	int		n;

	va_start(ap, ncols);
	n = vsnprintf(querybuf, sizeof(querybuf), query, ap);
	if (n > sizeof(querybuf)) {
		syslog(LOG_ERR, "query too long for buffer");
		return (MYSQL_RES *) 0;
	}

	mysql_init(&db);
	if (mysql_real_connect(&db, 0, 0, 0, dbname, 0, 0, 0) == 0) {
		syslog(LOG_ERR, "%s: connect failed: %s",
			dbname, mysql_error(&db));
		return (MYSQL_RES *) 0;
	}

	if (mysql_real_query(&db, querybuf, n) != 0) {
		syslog(LOG_ERR, "%s: query failed: %s",
			dbname, mysql_error(&db));
		mysql_close(&db);
		return (MYSQL_RES *) 0;
	}

	res = mysql_store_result(&db);
	if (res == 0) {
		syslog(LOG_ERR, "%s: store_result failed: %s",
			dbname, mysql_error(&db));
		mysql_close(&db);
		return (MYSQL_RES *) 0;
	}
	mysql_close(&db);

	if (ncols && ncols != (int)mysql_num_fields(res)) {
		syslog(LOG_ERR, "%s: Wrong number of fields returned "
		       "Wanted %d, Got %d",
			dbname, ncols, (int)mysql_num_fields(res));
		mysql_free_result(res);
		return (MYSQL_RES *) 0;
	}
	return res;
}

int
mydb_update(char *query, ...)
{
	MYSQL		db;
	char		querybuf[MYBUFSIZE];
	va_list		ap;
	int		n;

	va_start(ap, query);
	n = vsnprintf(querybuf, sizeof(querybuf), query, ap);
	if (n > sizeof(querybuf)) {
		syslog(LOG_ERR, "query too long for buffer");
		return 1;
	}

	mysql_init(&db);
	if (mysql_real_connect(&db, 0, 0, 0, dbname, 0, 0, 0) == 0) {
		syslog(LOG_ERR, "%s: connect failed: %s",
			dbname, mysql_error(&db));
		return 1;
	}

	if (mysql_real_query(&db, querybuf, n) != 0) {
		syslog(LOG_ERR, "%s: query failed: %s",
			dbname, mysql_error(&db));
		mysql_close(&db);
		return 1;
	}
	mysql_close(&db);
	return 0;
}

/*
 * Map IP to node ID. 
 */
static int
iptonodeid(struct in_addr ipaddr, char *bufp)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;

	res = mydb_query("select node_id from interfaces where IP='%s'", 1,
			 inet_ntoa(ipaddr));
	if (!res) {
		syslog(LOG_ERR, "iptonodeid: %s: DB Error getting interfaces!",
		       inet_ntoa(ipaddr));
		return 1;
	}

	if (! (int)mysql_num_rows(res)) {
		syslog(LOG_ERR, "Cannot map IP %s to nodeid",
		       inet_ntoa(ipaddr));
		mysql_free_result(res);
		return 1;
	}
	row = mysql_fetch_row(res);
	mysql_free_result(res);
	strcpy(bufp, row[0]);

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
		syslog(LOG_ERR, "nodeidtoexp: %s: DB Error getting reserved!",
		       nodeid);
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
	strcpy(gid, row[2]);

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
		syslog(LOG_ERR, "nodeidtonickname: %s: "
		       "DB Error getting reserved!",
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
		syslog(LOG_ERR, "nodeidtocontrolnet: %s: DB Error!", nodeid);
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
checkdbredirect(struct in_addr ipaddr)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		nodeid[32];
	char		pid[64];
	char		eid[64];
	char		gid[64];
	char		newdb[128];
	
	/*
	 * Find the nodeid.
	 */
	if (iptonodeid(ipaddr, nodeid)) {
		syslog(LOG_ERR, "CHECKDBREDIRECT: %s: No such node",
		       inet_ntoa(ipaddr));
		return 1;
	}
	if (nodeidtoexp(nodeid, pid, eid, gid)) {
		syslog(LOG_INFO, "CHECKDBREDIRECT: %s: Node is free", nodeid);
		return 0;
	}

	/*
	 * Look for an alternate DB name.
	 */
	res = mydb_query("select testdb from experiments "
			 "where eid='%s' and pid='%s'",
			 1, eid, pid);
			 
	if (!res) {
		syslog(LOG_ERR, "CHECKDBREDIRECT: "
		       "%s: DB Error getting testdb from table!", nodeid);
		return 1;
	}

	if (mysql_num_rows(res) == 0) {
		syslog(LOG_INFO, "CHECKDBREDIRECT: "
		       "%s: Hmm, experiment not there anymore!", nodeid);
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
	if (iptonodeid(ipaddr, nodeid)) {
		syslog(LOG_ERR, "CHECKDBREDIRECT: %s: %s DB does not exist",
		       inet_ntoa(ipaddr), dbname);
		strcpy(dbname, DEFAULT_DBNAME);
	}
	mysql_free_result(res);
	return 0;
}
 
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
			if ((cc = write(sock, bufp, len)) <= 0) {
				if (cc < 0) {
					syslog(LOG_ERR,
					       "writing to client: %m");
					return -1;
				}
				syslog(LOG_ERR, "write to client aborted");
				return -1;
			}
			len  -= cc;
			bufp += cc;
		}
	} else {
		if (udpfd != sock) {
			if (udpfd != -1)
				syslog(LOG_ERR, "UDP reply in progress!?");
			udpfd = sock;
			udpix = 0;
		}
		if (udpix + len > sizeof(udpbuf)) {
			syslog(LOG_ERR, "client data write truncated");
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
		syslog(LOG_ERR, "UDP reply out of sync!");
	else {
		err = sendto(udpfd, udpbuf, udpix, 0,
			     (struct sockaddr *)client, sizeof(*client));
		if (err < 0)
			syslog(LOG_ERR, "sendto client: %m");
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

