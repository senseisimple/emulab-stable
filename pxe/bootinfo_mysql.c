/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <syslog.h>

#include "bootwhat.h"
#include <mysql/mysql.h>

#ifdef USE_MYSQL_DB

static char dbname[] = TBDBNAME;
static MYSQL db;
static int parse_multiboot_path(char *path, boot_what_t *info);

int
open_bootinfo_db(void)
{
	return 0;
}

/*
  WARNING!!!
  
  DO NOT change this function without making corresponding changes to
  the perl version of this code in db/libdb.pm . They MUST ALWAYS
  return exactly the same result given the same inputs.
*/

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
		"n.def_boot_osid, p.partition, n.def_boot_cmd_line, "
	        "n.def_boot_path, o1.path, "
	        "n.next_boot_osid, o2.path from nodes "
		"as n left join partitions as p on n.node_id=p.node_id and "
		"n.def_boot_osid=p.osid left join interfaces as i on "
		"i.node_id=n.node_id "
    	        "left join os_info as o1 on o1.osid=n.def_boot_osid "
    	        "left join os_info as o2 on o2.osid=n.next_boot_osid "
	        "where i.IP = '%s'";

#define NEXT_BOOT_PATH		0
#define NEXT_BOOT_CMD_LINE	1
#define DEF_BOOT_OSID		2
#define PARTITION		3
#define DEF_BOOT_CMD_LINE	4
#define DEF_BOOT_PATH		5
#define DEF_BOOT_OSID_PATH	6
#define NEXT_BOOT_OSID		7
#define NEXT_BOOT_OSID_PATH	8

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
	case 9: /* Should have 9 fields */
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
	if ((row[NEXT_BOOT_PATH] != 0 &&
	     row[NEXT_BOOT_PATH][0] != '\0') ||
	    (row[NEXT_BOOT_OSID_PATH] != 0 &&
	     row[NEXT_BOOT_OSID_PATH][0] != '\0')) {
		info->type = BIBOOTWHAT_TYPE_MB;
		
		if (row[NEXT_BOOT_PATH] != 0 &&
		    row[NEXT_BOOT_PATH][0] != '\0')
		    parse_multiboot_path(row[NEXT_BOOT_PATH], info);
		else
		    parse_multiboot_path(row[NEXT_BOOT_OSID_PATH], info);

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
		parse_multiboot_path(row[DEF_BOOT_PATH], info);
	}
	else if (row[DEF_BOOT_OSID_PATH] != 0 &&
		 row[DEF_BOOT_OSID_PATH][0] != '\0') {
		info->type = BIBOOTWHAT_TYPE_MB;
		parse_multiboot_path(row[DEF_BOOT_OSID_PATH], info);
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
#undef DEF_BOOT_OSID_PATH
#undef NEXT_BOOT_OSID
#undef NEXT_BOOT_OSID_PATH
}

int
close_bootinfo_db(void)
{
	return 0;
}

/*
 * Split a multiboot path into the IP and Path.
 */
static int
parse_multiboot_path(char *path, boot_what_t *info)
{
	char		*p  = path;
	struct hostent	*he;

	info->type = BIBOOTWHAT_TYPE_MB;
	info->what.mb.tftp_ip.s_addr = 0;

	strsep(&p, ":");
	if (p) {
		he = gethostbyname(path);
		path = p;
	}
	else {
		he = gethostbyname("users.emulab.net");
	}
	if (he) {
		memcpy((char *)&info->what.mb.tftp_ip,
		       he->h_addr, sizeof(info->what.mb.tftp_ip));
	}

	strncpy(info->what.mb.filename, path, MAX_BOOT_PATH-1);

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
