#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <syslog.h>

#include "bootwhat.h"
#include <mysql/mysql.h>

#ifdef USE_MYSQL_DB

static char dbname[] = TBDBNAME;
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
	char dbquery[] =
		"select n.next_boot_path, n.next_boot_cmd_line, "
		"n.def_boot_osid, "
		"p.partition, n.def_boot_cmd_line, n.def_boot_path from nodes "
		"as n left join partitions as p on n.node_id=p.node_id and "
		"n.def_boot_osid=p.osid left join interfaces as i on "
		"i.node_id=n.node_id where i.IP = '%s'";

#define NEXT_BOOT_PATH		0
#define NEXT_BOOT_CMD_LINE	1
#define DEF_BOOT_OSID		2
#define PARTITION		3
#define DEF_BOOT_CMD_LINE	4
#define DEF_BOOT_PATH		5

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
	case 6: /* Should have 6 fields */
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
	if (row[NEXT_BOOT_PATH] != 0 && row[NEXT_BOOT_PATH][0] != '\0') {
		info->type = BIBOOTWHAT_TYPE_MB;
		info->what.mb.tftp_ip.s_addr = 0;
		strncpy(info->what.mb.filename, row[NEXT_BOOT_PATH],
			MAX_BOOT_PATH-1);
		if (row[NEXT_BOOT_CMD_LINE] != 0 &&
		    row[NEXT_BOOT_CMD_LINE][0] != '\0')
			strncpy(info->cmdline, row[NEXT_BOOT_CMD_LINE],
				MAX_BOOT_CMDLINE-1);
		else
			info->cmdline[0] = 0;	/* Must zero first byte! */
		mysql_free_result(res);
		return 0;
	}

	/*
	 * There is either a partition number or default boot path.
	 * The default boot path overrides the partition.
	 */
	if (row[DEF_BOOT_PATH] != 0 && row[DEF_BOOT_PATH][0] != '\0') {
		info->type = BIBOOTWHAT_TYPE_MB;
		info->what.mb.tftp_ip.s_addr = 0;
		strncpy(info->what.mb.filename, row[DEF_BOOT_PATH],
			MAX_BOOT_PATH-1);
	}
	else if (row[PARTITION] != 0 && row[PARTITION][0] != '\0') {
		info->type = BIBOOTWHAT_TYPE_PART;
		info->what.partition = atoi(row[PARTITION]);
	}
	else {
		syslog(LOG_ERR, "%s: null query result for IP %s!",
			dbname, inet_ntoa(ipaddr));
		mysql_free_result(res);
		return 1;
	}
	if (row[DEF_BOOT_CMD_LINE] != 0 && row[DEF_BOOT_CMD_LINE][0] != '\0')
		strncpy(info->cmdline, row[DEF_BOOT_CMD_LINE],
			MAX_BOOT_CMDLINE-1);
	else
		info->cmdline[0] = 0;	/* Must zero first byte! */
	
	/*
	 * Just 45040 lines of code later, we are done. 
	 */
	mysql_free_result(res);
	return 0;

#undef NEXT_BOOT_PATH
#undef NEXT_BOOT_CMD_LINE
#undef DEF_BOOT_OSID
#undef PARTITION
#undef DEF_BOOT_CMD_LINE
#undef DEF_BOOT_PATH
}

int
ack_bootinfo_db(struct in_addr ipaddr, boot_what_t *info)
{
	char querybuf[1024];
	int n, nrows, ncols, part;
	MYSQL db;
	MYSQL_RES *res;
	MYSQL_ROW row;

	n = snprintf(querybuf, sizeof querybuf,
		     "select i.node_id,n.next_boot_path,n.next_boot_cmd_line "
		     "from nodes as n left join interfaces as i on "
		     "i.node_id=n.node_id where i.IP = '%s'",
		     inet_ntoa(ipaddr));
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

	row = mysql_fetch_row(res);

	if (row[1] == 0 || row[1][0] == '\0') {
		/* Nothing to do */
		mysql_free_result(res);
		mysql_close(&db);
		return 1;
	}
	
	/*
	 * Update the database to reflect that the boot has been done.
	 */
	n = snprintf(querybuf, sizeof querybuf,
		     "update nodes set next_boot_path='',"
		     "next_boot_cmd_line='' where node_id='%s'",
		     row[0]);
	mysql_free_result(res);
	
	if (n > sizeof querybuf) {
		syslog(LOG_ERR, "query too long for buffer");
		mysql_close(&db);
		return 1;
	}
	
	if (mysql_real_query(&db, querybuf, n) != 0) {
		syslog(LOG_ERR, "%s: query failed: %s",
			dbname, mysql_error(&db));
		mysql_close(&db);
		return 1;
	}
	syslog(LOG_INFO, "%s: next_boot_path cleared", inet_ntoa(ipaddr));

	mysql_close(&db);
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

static void
print_bootwhat(boot_what_t *bootinfo)
{
	switch (bootinfo->type) {
	case BIBOOTWHAT_TYPE_PART:
		printf("boot from partition %d\n",
		       bootinfo->what.partition);
		break;
	case BIBOOTWHAT_TYPE_SYSID:
		printf("boot from partition with sysid %d\n",
		       bootinfo->what.sysid);
		break;
	case BIBOOTWHAT_TYPE_MB:
		printf("boot multiboot image %s:%s\n",
		       inet_ntoa(bootinfo->what.mb.tftp_ip),
		       bootinfo->what.mb.filename);
		break;
	}
	if (bootinfo->cmdline[0])
		printf("Command line %s\n", bootinfo->cmdline);
		
}

main(int argc, char **argv)
{
	struct in_addr ipaddr;
	boot_info_t boot_info;
	boot_what_t *boot_whatp = (boot_what_t *)&boot_info.data;

	open_bootinfo_db();
	while (--argc > 0) {
		if (inet_aton(*++argv, &ipaddr))
			if (query_bootinfo_db(ipaddr, boot_whatp) == 0) {
				printf("%s: ", *argv);
				print_bootwhat(boot_whatp);
			} else
				printf("%s: failed\n", *argv);
		else
			printf("bogus IP address `%s'\n", *argv);
	}
	close_bootinfo_db();
	exit(0);
}
#endif

#endif
