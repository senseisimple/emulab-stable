#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>

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
		fprintf(stderr, "query too long for buffer\n");
		return 1;
	}

	mysql_init(&db);
	if (mysql_real_connect(&db, 0, 0, 0, dbname, 0, 0, 0) == 0) {
		fprintf(stderr, "%s: connect failed: %s\n",
			dbname, mysql_error(&db));
		return 1;
	}

	if (mysql_real_query(&db, querybuf, n) != 0) {
		fprintf(stderr, "%s: query failed: %s\n",
			dbname, mysql_error(&db));
		mysql_close(&db);
		return 1;
	}

	res = mysql_store_result(&db);
	if (res == 0) {
		fprintf(stderr, "%s: store_result failed: %s\n",
			dbname, mysql_error(&db));
		mysql_close(&db);
		return 1;

	}
	mysql_close(&db);

	nrows = (int)mysql_num_rows(res);
	switch (nrows) {
	case 0:
		fprintf(stderr, "%s: no entry for host %s\n",
			dbname, inet_ntoa(ipaddr));
		mysql_free_result(res);
		return 1;
	case 1:
		break;
	default:
		fprintf(stderr, "%s: %d entries for IP %s, using first\n",
			dbname, nrows, inet_ntoa(ipaddr));
		break;
	}

	ncols = (int)mysql_num_fields(res);
	switch (ncols) {
	case 1:
		break;
	default:
		fprintf(stderr, "%s: %d fields in query for IP %s!\n",
			dbname, ncols, inet_ntoa(ipaddr));
		mysql_free_result(res);
		return 1;
	}

	row = mysql_fetch_row(res);
	if (row[0] == 0 || row[0] == '\0') {
		fprintf(stderr, "%s: null query result for IP %s!\n",
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
main(int argc, char **argv)
{
	struct in_addr ipaddr;
	boot_info_t boot_info;
	boot_what_t *boot_whatp = (boot_what_t *)&boot_info.data;

	while (--argc > 0) {
		if (inet_aton(*++argv, &ipaddr))
			if (query_bootinfo_db(ipaddr, boot_whatp) == 0)
				printf("returned partition %d\n",
				       boot_whatp->what.partition);
			else
				printf("failed\n");
		else
			printf("bogus IP address `%s'\n", *argv);
	}
	exit(0);
}
#endif

#endif
