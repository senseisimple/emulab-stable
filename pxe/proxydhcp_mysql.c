#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <syslog.h>

#ifdef USE_MYSQL_DB
#include <mysql/mysql.h>

static char dbname[] = TBDBNAME;
static MYSQL db;
static int parse_pathspec(char *path, struct in_addr *sip, char **filename);
static int clear_next_pxeboot(char *node);

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

int
open_db(void)
{
	return 0;
}

int
query_db(struct in_addr ipaddr, struct in_addr *sip,
	 char *bootprog, int bootlen)
{
	char querybuf[1024];
	int n, nrows, ncols, part;
	MYSQL db;
	MYSQL_RES *res;
	MYSQL_ROW row;
	char dbquery[] =
		"select pxe_boot_path, next_pxe_boot_path from nodes as n "
		"left join interfaces as i on "
		"i.node_id=n.node_id "
	        "where i.IP = '%s'";

#define PXEBOOT_PATH		0
#define NEXT_PXEBOOT_PATH	1

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

	/* Debug message into log:
	syslog(LOG_ERR, "USING QUERY: %s", querybuf);
	*/
	
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
	case 2: /* Should have 2 fields */
		break;
	default:
		syslog(LOG_ERR, "%s: %d fields in query for IP %s!",
			dbname, ncols, inet_ntoa(ipaddr));
		mysql_free_result(res);
		return 1;
	}

	row = mysql_fetch_row(res);

	/*
	 * Check next_boot_path.  If set, assume it is a multiboot kernel.
	 */
	if (row[NEXT_PXEBOOT_PATH] != 0 && row[NEXT_PXEBOOT_PATH][0] != '\0') {
		char	*filename;

		if (parse_pathspec(row[NEXT_PXEBOOT_PATH], sip, &filename))
			goto bad;

		if (strlen(filename) >= bootlen)
			goto bad;
		strcpy(bootprog, filename);

		mysql_free_result(res);
		return 0;
	}
	if (row[PXEBOOT_PATH] != 0 && row[PXEBOOT_PATH][0] != '\0') {
		char	*filename;

		if (parse_pathspec(row[PXEBOOT_PATH], sip, &filename))
			goto bad;

		if (strlen(filename) >= bootlen)
			goto bad;
		strcpy(bootprog, filename);

		mysql_free_result(res);
		return 0;
	}

 bad:
	mysql_free_result(res);
	return 1;
}
#undef PXEBOOT_PATH
#undef NEXT_PXEBOOT_PATH

int
close_db(void)
{
	return 0;
}

/*
 * Split a pathspec into the IP and Path.
 */
static int
parse_pathspec(char *path, struct in_addr *sip, char **filename)
{
	char		*p  = path;
	struct hostent	*he;

	strsep(&p, ":");
	if (!p || (he = gethostbyname(path)) == 0)
		return 1;

	memcpy((char *)sip, he->h_addr, sizeof(sip));
	*filename = p;
	return 0;
}

#ifdef TEST
static int
parse_host(char *name)
{
	struct hostent *he;

	if (!isdigit(name[0])) {
		he = gethostbyname(name);
		if (he == 0) {
			syslog(LOG_ERR, "%s: unknown host", name);
			return 0;
		}
		return *(int *)he->h_addr;
	}
	return inet_addr(name);
}

main(int argc, char **argv)
{
	struct in_addr ipaddr, server;
	char	       bootprog[256];

	if (open_db() != 0) {
		printf("bad config file\n");
		exit(1);
	}

	while (--argc > 0) {
		if (ipaddr.s_addr = parse_host(*++argv)) {
			if (query_db(ipaddr,
				     &server, bootprog, sizeof(bootprog))
			    == 0) {
				printf("%s: %s %s\n", *argv,
				       inet_ntoa(server), bootprog);
			} else
				printf("failed\n");
		} else
			printf("bogus IP address `%s'\n", *argv);
	}
	close_db();
	exit(0);
}
#endif
#endif
