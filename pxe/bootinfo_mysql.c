#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <syslog.h>

#include <oskit/boot/bootwhat.h>
#include <mysql/mysql.h>

#ifdef USE_MYSQL_DB

static char dbname[] = "tbdb";
static char dbquery[] =
	"select n.boot_default from nodes as n, interfaces as i "
	"where i.IP = \"%s\" and n.node_id = i.node_id";

static MYSQL db;

int
open_bootinfo_db(void)
{
	return 0;
}

int
query_bootinfo_db(struct in_addr ipaddr, boot_what_t *info)
{
	char querybuf[1024];
	int n, nrows, ncols, part;
	MYSQL db;
	MYSQL_RES *res;
	MYSQL_ROW row;

	n = snprintf(querybuf, sizeof querybuf, dbquery, inet_ntoa(ipaddr));
	if (n > sizeof querybuf) {
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

	res = mysql_store_result(&db);
	if (res == 0) {
		syslog(LOG_ERR, "%s: store_result failed: %s",
			dbname, mysql_error(&db));
		mysql_close(&db);
		return 1;

	}
	mysql_close(&db);

	nrows = (int)mysql_num_rows(res);
	switch (nrows) {
	case 0:
		syslog(LOG_ERR, "%s: no entry for host %s",
			dbname, inet_ntoa(ipaddr));
		mysql_free_result(res);
		return 1;
	case 1:
		break;
	default:
		syslog(LOG_ERR, "%s: %d entries for IP %s, using first",
			dbname, nrows, inet_ntoa(ipaddr));
		break;
	}

	ncols = (int)mysql_num_fields(res);
	switch (ncols) {
	case 1:
		break;
	default:
		syslog(LOG_ERR, "%s: %d fields in query for IP %s!",
			dbname, ncols, inet_ntoa(ipaddr));
		mysql_free_result(res);
		return 1;
	}

	row = mysql_fetch_row(res);
	if (row[0] == 0 || row[0] == '\0') {
		syslog(LOG_ERR, "%s: null query result for IP %s!",
			dbname, inet_ntoa(ipaddr));
		mysql_free_result(res);
		return 1;
	}

	/*
	 * Just 45000 lines of code later, we have the 32 bits of info
	 * we wanted!
	 */
	part = atoi(row[0]);
	mysql_free_result(res);

	info->type = BIBOOTWHAT_TYPE_PART;
	info->what.partition = part;
	return 0;
}

int
close_bootinfo_db(void)
{
	return 0;
}

#ifdef TEST
#include <stdarg.h>

void
syslog(int prio, const char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

main(int argc, char **argv)
{
	struct in_addr ipaddr;
	boot_info_t boot_info;
	boot_what_t *boot_whatp = (boot_what_t *)&boot_info.data;

	open_bootinfo_db();
	while (--argc > 0) {
		if (inet_aton(*++argv, &ipaddr))
			if (query_bootinfo_db(ipaddr, boot_whatp) == 0)
				printf("%s: returned partition %d\n",
				       *argv, boot_whatp->what.partition);
			else
				printf("%s: failed\n", *argv);
		else
			printf("bogus IP address `%s'\n", *argv);
	}
	close_bootinfo_db();
	exit(0);
}
#endif

#endif
