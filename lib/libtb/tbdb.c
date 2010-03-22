/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * DB interface.
 */

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <assert.h>
#include "tbdb.h"
#include "log.h"
#include "config.h"
#include <mysql/errmsg.h>

/*
 * DB stuff
 */
static MYSQL	db;
static char    *dbname = TBDBNAME;
static char    *dbuser = (char *) NULL;
static char    *dbpass = (char *) NULL;
static char    *dbhost = (char *) NULL;

static int
mydb_connect()
{
	mysql_init(&db);
	if (mysql_real_connect(&db, dbhost, dbuser, dbpass,
			       dbname, 0, 0, CLIENT_INTERACTIVE) == 0) {
		error("%s: connect failed: %s\n", dbname, mysql_error(&db));
		return 0;
	}
	return 1;
}

static int
mydb_reconnect()
{
	while (1) {
		warning("Lost connection to DB; Attempting to reconnect ...");
		
		if (mydb_connect())
			return 1;

		sleep(5);
	}
}

static void
mydb_disconnect()
{
	mysql_close(&db);
}

/*
 * This is the exported function. Sheesh.
 */
int
dbinit(void)
{
	return mydb_connect();
}

int
dbinit_withparams(char *host, char *user, char *passwd, char *name)
{
	dbhost = host;
	dbuser = user;
	dbname = name;
	dbpass = passwd;
	return mydb_connect();
}

void
dbclose(void)
{
	mydb_disconnect();
}

unsigned long
mydb_escape_string(char *to, const char *from, unsigned long length)
{
	return mysql_real_escape_string(&db, to, from, length);
}

MYSQL_RES *
mydb_query(char *query, int ncols, ...)
{
	MYSQL_RES	*res;
	char		querybuf[2*BUFSIZ];
	va_list		ap;
	int		n;

	va_start(ap, ncols);
	n = vsnprintf(querybuf, sizeof(querybuf), query, ap);
	if (n > sizeof(querybuf)) {
		error("query too long for buffer");
		return (MYSQL_RES *) 0;
	}
 again:
	if (mysql_real_query(&db, querybuf, n) != 0) {
		error("%s: query failed: %s", dbname, mysql_error(&db));

		/*
		 * Auto reconnect when the server goes away; we treat it
		 * as a transient error that we want to recover from if
		 * at all possible. This has the potential to block the
		 * caller for a long time, but there are very few clients
		 * of this library, so should not matter.
		 */
		if (mysql_errno(&db) == CR_SERVER_LOST ||
		    mysql_errno(&db) == CR_SERVER_GONE_ERROR) {
			if (mydb_reconnect())
				goto again;
		}
		return (MYSQL_RES *) 0;
	}

	res = mysql_store_result(&db);
	if (res == 0) {
		error("%s: store_result failed: %s", dbname, mysql_error(&db));
		return (MYSQL_RES *) 0;
	}

	if (ncols && ncols != (int)mysql_num_fields(res)) {
		error("%s: Wrong number of fields returned "
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
	char		querybuf[2*BUFSIZ];
	va_list		ap;
	int		n;

	va_start(ap, query);
	n = vsnprintf(querybuf, sizeof(querybuf), query, ap);
	if (n > sizeof(querybuf)) {
		error("query too long for buffer");
		return 0;
	}
 again:
	if (mysql_real_query(&db, querybuf, n) != 0) {
		error("%s: query failed: %s", dbname, mysql_error(&db));

		/*
		 * Auto reconnect when the server goes away; we treat it
		 * as a transient error that we want to recover from if
		 * at all possible. This has the potential to block the
		 * caller for a long time, but there are very few clients
		 * of this library, so should not matter.
		 */
		if (mysql_errno(&db) == CR_SERVER_LOST ||
		    mysql_errno(&db) == CR_SERVER_GONE_ERROR) {
			if (mydb_reconnect())
				goto again;
		}
		return 0;
	}
	return 1;
}

/*
 * Map IP to node ID. 
 */
int
mydb_iptonodeid(char *ipaddr, char *bufp)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;

	res = mydb_query("select node_id from interfaces where IP='%s'",
			 1, ipaddr);
	if (!res) {
		error("iptonodeid: DB Error: %s", ipaddr);
		return 0;
	}

	if (! (int)mysql_num_rows(res)) {
		error("iptonodeid: No such nodeid: %s", ipaddr);
		mysql_free_result(res);
		return 0;
	}
	row = mysql_fetch_row(res);
	mysql_free_result(res);
	strcpy(bufp, row[0]);

	return 1;
}
 
/*
 * Map node ID to IP (control net interface for underlying physical node!).
 */
int
mydb_nodeidtoip(char *nodeid, char *bufp)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;

	res = mydb_query("select IP from nodes as n2 "
			 "left join nodes as n1 on n1.node_id=n2.phys_nodeid "
			 "left join interfaces as i on "
			 "     i.node_id=n1.node_id and i.role='ctrl' "
			 "where n2.node_id='%s'", 1, nodeid);

	if (!res) {
		error("nodeidtoip: DB Error: %s", nodeid);
		return 0;
	}

	if (! (int)mysql_num_rows(res)) {
		error("nodeidtoip: No such nodeid: %s", nodeid);
		mysql_free_result(res);
		return 0;
	}
	row = mysql_fetch_row(res);
	mysql_free_result(res);
	strcpy(bufp, row[0]);

	return 1;
}
 
/*
 * Set the node event status.
 */
int
mydb_setnodeeventstate(char *nodeid, char *eventtype)
{
	if (! mydb_update("update nodes set eventstate='%s' "
			  "where node_id='%s'",
			  eventtype, nodeid)) {
		error("setnodestatus: DB Error: %s/%s!", nodeid, eventtype);
		return 0;
	}
	return 1;
}

/*
 * See if all nodes in an experiment are at the specified event state.
 * Return number of nodes not in the proper state.
 */
int
mydb_checkexptnodeeventstate(char *pid, char *eid,
			     char *eventtype, int *count)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	int		nrows;

	res = mydb_query("select eventstate from reserved "
			 "left join nodes on nodes.node_id=reserved.node_id "
			 "where reserved.pid='%s' and reserved.eid='%s' ",
			 1, pid, eid);
	
	if (!res) {
		error("checkexptnodeeventstate: DB Error: %s/%s/%s",
		      pid, eid, eventtype);
		return 0;
	}

	if (! (nrows = mysql_num_rows(res))) {
		error("checkexptnodeeventstate: Warning: no nodes: %s/%s",
		      pid, eid);
		mysql_free_result(res);
		/* Warn, then return sucessfully */
		return 1;
	}

	*count = 0;
	while (nrows) {
		row = mysql_fetch_row(res);

		if (!row[0] || !row[0][0] || strcmp(row[0], eventtype))
			*count += 1;
		nrows--;
	}
	mysql_free_result(res);
	return 1;
}

/*
 * Set (or clear) event scheduler process ID. A zero is treated as
 * a request to clear it.
 *
 * NOTE: We want to fail if there is already a non-zero value in the DB
 * (although its okay to set it to zero no matter what). 
 */
int
mydb_seteventschedulerpid(char *pid, char *eid, int processid)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	int		retval = 1;

	if (! processid) {
		if (! mydb_update("update experiments set event_sched_pid=0 "
				  "where pid='%s' and eid='%s'",
				  pid, eid)) {
			error("seteventschedulerpid: DB Error(0): %s/%s!",
			      pid, eid);
			return 0;
		}
		return 1;
	}

	if (! mydb_update("lock tables experiments write")) {
		error("seteventschedulerpid: DB Error: locking table!");
		return 0;
	}

	res = mydb_query("select event_sched_pid from experiments "
			 "where pid='%s' and eid='%s'",
			 1, pid, eid);
	
	if (!res) {
		error("seteventschedulerpid: DB Error(1): %s/%s", pid, eid);
		return 0;
	}
	if (! mysql_num_rows(res)) {
		error("seteventschedulerpid: No such experiment: %s/%s",
		      pid, eid);
		retval = 0;
		goto done;
	}
	row = mysql_fetch_row(res);
	
	if (!row[0] || !row[0][0]) {
		error("seteventschedulerpid: Bad value for procid: %s/%s",
		      pid, eid);
		retval = 0;
		goto done;
	}
	if (atoi(row[0]) != 0) {
		info("seteventschedulerpid: A scheduler is running: %s/%s",
		     pid, eid);
		retval = 0;
		goto done;
	}
	if (! mydb_update("update experiments set event_sched_pid=%d "
			  "where pid='%s' and eid='%s'",
			  processid, pid, eid)) {
		error("seteventschedulerpid: DB Error(2): %s/%s!", pid, eid);
		retval = 0;
		goto done;
	}
	retval = 1;
  done:	
	if (! mydb_update("unlock tables")) {
		error("seteventschedulerpid: DB Error: unlocking table!");
		return 0;
	}
	mysql_free_result(res);
	return retval;
}
