/*
 * DB interface.
 */

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
#include <assert.h>
#include "event-sched.h"
#include "libdb.h"
#include "config.h"

/*
 * DB stuff
 */
static MYSQL	db;
static char    *dbname = TBDBNAME;

int
dbinit(void)
{
	mysql_init(&db);
	if (mysql_real_connect(&db, 0, 0, 0,
			       dbname, 0, 0, CLIENT_INTERACTIVE) == 0) {
		ERROR("%s: connect failed: %s", dbname, mysql_error(&db));
		return 0;
	}
	return 1;
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
		ERROR("query too long for buffer");
		return (MYSQL_RES *) 0;
	}

	if (mysql_real_query(&db, querybuf, n) != 0) {
		ERROR("%s: query failed: %s", dbname, mysql_error(&db));
		return (MYSQL_RES *) 0;
	}

	res = mysql_store_result(&db);
	if (res == 0) {
		ERROR("%s: store_result failed: %s", dbname, mysql_error(&db));
		return (MYSQL_RES *) 0;
	}

	if (ncols && ncols != (int)mysql_num_fields(res)) {
		ERROR("%s: Wrong number of fields returned "
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
		ERROR("query too long for buffer");
		return 1;
	}
	if (mysql_real_query(&db, querybuf, n) != 0) {
		ERROR("%s: query failed: %s", dbname, mysql_error(&db));
		return 1;
	}
	return 0;
}

