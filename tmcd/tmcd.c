#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <syslog.h>
#include <signal.h>
#include <stdarg.h>
#include <assert.h>
#include <mysql/mysql.h>
#include "decls.h"

#define TESTMODE
#define DBNAME		"tbdb"
#define VERSION		1
#define NETMASK		"255.255.255.0"

static int	nodeidtoexp(char *nodeid, char *pid, char *eid);
static int	iptonodeid(struct in_addr ipaddr, char *bufp);
static int	nodeidtocontrolnet(char *nodeid, int *net);
int		client_writeback(int sock, void *buf, int len);
MYSQL_RES *	mydb_query(char *query, int ncols, ...);
int		mydb_update(char *query, ...);

/*
 * Commands we support.
 */
static int	doreboot(int sock, struct in_addr ipaddr, char *request);
static int	dostatus(int sock, struct in_addr ipaddr, char *request);
static int	doifconfig(int sock, struct in_addr ipaddr, char *request);
static int	doaccounts(int sock, struct in_addr ipaddr, char *request);
static int	dodelay(int sock, struct in_addr ipaddr, char *request);

struct command {
	char	*cmdname;
	int    (*func)(int sock, struct in_addr ipaddr, char *request);
} command_array[] = {
	{ "reboot",    doreboot },
	{ "status",    dostatus },
	{ "ifconfig",  doifconfig },
	{ "accounts",  doaccounts },
	{ "delay",     dodelay },
};
static int numcommands = sizeof(command_array)/sizeof(struct command);

main()
{
	int			sock, length, data, i, mlen, err;
	struct sockaddr_in	name, client;

	openlog("tmcd", LOG_PID, LOG_USER);
	syslog(LOG_NOTICE, "daemon starting (version %d)", VERSION);

#ifndef LBS
	(void)daemon(0, 0);
#endif
	/* Create socket from which to read. */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		syslog(LOG_ERR, "opening stream socket: %m");
		exit(1);
	}

	i - 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		       (char *)&i, sizeof(i)) < 0)
		syslog(LOG_ERR, "control setsockopt: %m");;
	
	/* Create name. */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons(TBSERVER_PORT);
	if (bind(sock, (struct sockaddr *) &name, sizeof(name))) {
		syslog(LOG_ERR, "binding stream socket: %m");
		exit(1);
	}
	/* Find assigned port value and print it out. */
	length = sizeof(name);
	if (getsockname(sock, (struct sockaddr *) &name, &length)) {
		syslog(LOG_ERR, "getting socket name: %m");
		exit(1);
	}
	if (listen(sock, 20) < 0) {
		syslog(LOG_ERR, "listening on socket: %m");
		exit(1);
	}
	syslog(LOG_NOTICE, "listening on port %d", ntohs(name.sin_port));

	while (1) {
		struct sockaddr_in client;
		int		   clientsock, length = sizeof(client);
		char		   buf[BUFSIZ], *bp, *cp;
		int		   cc;

		clientsock = accept(sock, (struct sockaddr *)&client, &length);

		/*
		 * Read in the command request.
		 */
		if ((cc = read(clientsock, buf, BUFSIZ - 1)) <= 0) {
			if (cc < 0)
				syslog(LOG_ERR, "Reading request: %m");
			syslog(LOG_ERR, "Connection aborted");
			close(clientsock);
			continue;
		}
		buf[cc] = '\0';
		bp = buf;
		while (*bp++) {
			if (*bp == '\r' || *bp == '\n')
				*bp = '\0';
		}
		bp = buf;
#ifdef TESTMODE
		/*
		 * Allow remote end to be specified for testing.
		 */
		if (strncmp("MYIP=", buf, strlen("MYIP=")) == 0) {
			char *tp;
			
			bp += strlen("MYIP=");
			tp = bp;
			while (! isspace(*tp))
				tp++;
			*tp++ = '\0';
			inet_aton(bp, &client.sin_addr);
			bp = tp;
		}
#endif
		cp = (char *) malloc(cc + 1);
		assert(cp);
		strcpy(cp, bp);
		
		syslog(LOG_INFO, "%s REQUEST: %s",
		       inet_ntoa(client.sin_addr), cp);

		/*
		 * Figure out what command was given.
		 */
		for (i = 0; i < numcommands; i++) {
			if (strncmp(cp, command_array[i].cmdname,
				    strlen(command_array[i].cmdname)) == 0) {
				err = command_array[i].func(clientsock,
							    client.sin_addr,
							    cp);
				break;
			}
		}
		if (i == numcommands)
			syslog(LOG_INFO, "%s Invalid Request: %s",
			       inet_ntoa(client.sin_addr), cp);
		else
			syslog(LOG_INFO, "%s REQUEST: %s: returned %d",
			       inet_ntoa(client.sin_addr), cp, err);
		
		free(cp);
		close(clientsock);
	}

	close(sock);
	syslog(LOG_NOTICE, "daemon terminating");
	exit(0);
}

/*
 * Accept notification of reboot. 
 */
static int
doreboot(int sock, struct in_addr ipaddr, char *request)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		nodeid[32];
	int		n;

	if (iptonodeid(ipaddr, nodeid)) {
		syslog(LOG_ERR, "REBOOT: %s: No such node", inet_ntoa(ipaddr));
		return 1;
	}

	syslog(LOG_INFO, "REBOOT: %s is reporting a reboot", nodeid);

	/*
	 * See if the node was in the reload state. If so we need to clear it
	 * and its reserved status.
	 */
	res = mydb_query("select node_id from reloads where node_id='%s'",
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

	if (mydb_update("delete from reloads where node_id='%s'", nodeid)) {
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
dostatus(int sock, struct in_addr ipaddr, char *request)
{
	MYSQL_RES	*res;	
	char		nodeid[32];
	char		pid[64];
	char		eid[64];
	char		answer[128];
	int		n;

	if (iptonodeid(ipaddr, nodeid)) {
		syslog(LOG_ERR, "STATUS: %s: No such node", inet_ntoa(ipaddr));
		return 1;
	}

	/*
	 * Now check reserved table
	 */
	if (nodeidtoexp(nodeid, pid, eid)) {
		syslog(LOG_INFO, "STATUS: %s: Node is free", nodeid);
		strcpy(answer, "FREE\n");
	}
	else {
		syslog(LOG_INFO, "STATUS: %s: Node is in experiment %s/%s",
		       nodeid, pid, eid);
		snprintf(answer, sizeof(answer), "ALLOCATED=%s/%s\n",
			 pid, eid);
	}
	client_writeback(sock, answer, strlen(answer));

	return 0;
}

/*
 * Return ifconfig information to client.
 */
static int
doifconfig(int sock, struct in_addr ipaddr, char *request)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		nodeid[32];
	char		pid[64];
	char		eid[64];
	char		buf[BUFSIZ];
	int		control_net, nrows;

	if (iptonodeid(ipaddr, nodeid)) {
		syslog(LOG_ERR, "IFCONFIG: %s: No such node",
		       inet_ntoa(ipaddr));
		return 1;
	}

	/*
	 * Now check reserved table
	 */
	if (nodeidtoexp(nodeid, pid, eid)) {
		syslog(LOG_ERR, "IFCONFIG: %s: Node is free", nodeid);
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
	res = mydb_query("select card,IP from interfaces "
			 "where node_id='%s' and card!='%d'",
			 2, nodeid, control_net);
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
			sprintf(buf, "INTERFACE=%d INET=%s MASK=%s\n",
				atoi(row[0]), row[1], NETMASK);
			
			client_writeback(sock, buf, strlen(buf));
			syslog(LOG_INFO, "IFCONFIG: %s", buf);
		}
		nrows--;
	}
	mysql_free_result(res);

	return 0;
}

/*
 * Return account stuff.
 */
static int
doaccounts(int sock, struct in_addr ipaddr, char *request)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		nodeid[32];
	char		pid[64];
	char		eid[64];
	char		buf[BUFSIZ];
	int		nrows, gid;

	if (iptonodeid(ipaddr, nodeid)) {
		syslog(LOG_ERR, "ACCOUNTS: %s: No such node",
		       inet_ntoa(ipaddr));
		return 1;
	}

	/*
	 * Now check reserved table
	 */
	if (nodeidtoexp(nodeid, pid, eid)) {
		syslog(LOG_ERR, "ACCOUNTS: %s: Node is free", nodeid);
		return 1;
	}

	/*
	 * We have the pid name, but we need the GID number from the
	 * projects table to send over. 
	 */
	res = mydb_query("select unix_gid from projects where pid='%s'",
			 1, pid);
	if (!res) {
		syslog(LOG_ERR, "ACCOUNTS: %s: DB Error getting gid!", pid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		syslog(LOG_ERR, "ACCOUNTS: %s: No Project!", pid);
		mysql_free_result(res);
		return 1;
	}
	row = mysql_fetch_row(res);
	if (!row[0] || !row[0][0]) {
		syslog(LOG_ERR, "ACCOUNTS: %s: No Project GID!", pid);
		mysql_free_result(res);
		return 1;
	}

	/*
	 * Format as a command so that we can send back multiple group
	 * names if that ever becomes necessary. The client side should
	 * be prepared to get multiple group commands.
	 */
	gid = atoi(row[0]);
	sprintf(buf, "ADDGROUP NAME=%s GID=%d\n", pid, gid);
	client_writeback(sock, buf, strlen(buf));
	mysql_free_result(res);
	syslog(LOG_INFO, "ACCOUNTS: %s", buf);

	/*
	 * Now onto the users in the project.
	 */
	res = mydb_query("select u.uid,u.usr_pswd,u.unix_uid,u.usr_name,"
			 "p.trust "
			 "from users as u "
			 "left join proj_memb as p on p.uid=u.uid "
			 "where p.pid='%s'",
			 5, pid);
	if (!res) {
		syslog(LOG_ERR, "ACCOUNTS: %s: DB Error getting users!", pid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		syslog(LOG_ERR, "ACCOUNTS: %s: No Users!", pid);
		mysql_free_result(res);
		return 0;
	}

	while (nrows) {
		int root = 0;

		row = mysql_fetch_row(res);

		if ((strcmp(row[4], "local_root") == 0) ||
		    (strcmp(row[4], "group_root") == 0))
			root = 1;

		sprintf(buf,
			"ADDUSER LOGIN=%s "
			"PSWD=%s UID=%s GID=%d ROOT=%d NAME=\"%s\"\n",
			row[0], row[1], row[2], gid, root, row[3]);
			
		client_writeback(sock, buf, strlen(buf));
		nrows--;
		syslog(LOG_INFO, "ACCOUNTS: %s", buf);
	}
	mysql_free_result(res);

	return 0;
}

/*
 * Return account stuff.
 */
static int
dodelay(int sock, struct in_addr ipaddr, char *request)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		nodeid[32];
	char		pid[64];
	char		eid[64];
	char		buf[BUFSIZ];
	int		nrows, gid;

	if (iptonodeid(ipaddr, nodeid)) {
		syslog(LOG_ERR, "DELAY: %s: No such node",
		       inet_ntoa(ipaddr));
		return 1;
	}

	/*
	 * Now check reserved table
	 */
	if (nodeidtoexp(nodeid, pid, eid))
		return 0;

	/*
	 * Get delay parameters for the machine. Might be more than one
	 * since someday we may allow a single machine to operate as a
	 * delay node across multiple links.
	 */
	res = mydb_query("select card0,card1,delay,bandwidth,lossrate "
			 "from delays where node_id='%s'",
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

		sprintf(buf,
			"DELAY INT0=%s INT1=%s DELAY=%s BW=%s PLR=%s\n",
			row[0], row[1], row[2], row[3], row[4]);
			
		client_writeback(sock, buf, strlen(buf));
		nrows--;
		syslog(LOG_INFO, "DELAY: %s", buf);
	}
	mysql_free_result(res);
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
	char		querybuf[BUFSIZ];
	va_list		ap;
	int		n;

	va_start(ap, ncols);
	n = vsnprintf(querybuf, sizeof(querybuf), query, ap);
	if (n > sizeof(querybuf)) {
		syslog(LOG_ERR, "query too long for buffer");
		return (MYSQL_RES *) 0;
	}

	mysql_init(&db);
	if (mysql_real_connect(&db, 0, 0, 0, DBNAME, 0, 0, 0) == 0) {
		syslog(LOG_ERR, "%s: connect failed: %s",
			DBNAME, mysql_error(&db));
		return (MYSQL_RES *) 0;
	}

	if (mysql_real_query(&db, querybuf, n) != 0) {
		syslog(LOG_ERR, "%s: query failed: %s",
			DBNAME, mysql_error(&db));
		mysql_close(&db);
		return (MYSQL_RES *) 0;
	}

	res = mysql_store_result(&db);
	if (res == 0) {
		syslog(LOG_ERR, "%s: store_result failed: %s",
			DBNAME, mysql_error(&db));
		mysql_close(&db);
		return (MYSQL_RES *) 0;
	}
	mysql_close(&db);

	if (ncols && ncols != (int)mysql_num_fields(res)) {
		syslog(LOG_ERR, "%s: Wrong number of fields returned "
		       "Wanted %d, Got %d",
			DBNAME, ncols, (int)mysql_num_fields(res));
		mysql_free_result(res);
		return (MYSQL_RES *) 0;
	}
	return res;
}

int
mydb_update(char *query, ...)
{
	MYSQL		db;
	MYSQL_RES	*res;
	char		querybuf[BUFSIZ];
	va_list		ap;
	int		n;

	va_start(ap, query);
	n = vsnprintf(querybuf, sizeof(querybuf), query, ap);
	if (n > sizeof(querybuf)) {
		syslog(LOG_ERR, "query too long for buffer");
		return 1;
	}

	mysql_init(&db);
	if (mysql_real_connect(&db, 0, 0, 0, DBNAME, 0, 0, 0) == 0) {
		syslog(LOG_ERR, "%s: connect failed: %s",
			DBNAME, mysql_error(&db));
		return 1;
	}

	if (mysql_real_query(&db, querybuf, n) != 0) {
		syslog(LOG_ERR, "%s: query failed: %s",
			DBNAME, mysql_error(&db));
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
		       ipaddr);
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
nodeidtoexp(char *nodeid, char *pid, char *eid)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;

	res = mydb_query("select pid,eid from reserved where node_id='%s'",
			 2, nodeid);
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
 * Write back to client
 */
int
client_writeback(int sock, void *buf, int len)
{
	int	cc;
	char	*bufp = (char *) buf;
	
	while (len) {
		if ((cc = write(sock, bufp, len)) <= 0) {
			if (cc < 0) {
				syslog(LOG_ERR, "writing to client: %m");
				return -1;
			}
			syslog(LOG_ERR, "write to client aborted");
			return -1;
		}
		len  -= cc;
		bufp += cc;
	}
	return 0;
}
