#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <syslog.h>

#include <oskit/boot/bootwhat.h>
#include <mysql/mysql.h>

#ifdef USE_MYSQL_DB

static char dbname[] = "tbdb";
static char dbquery[] =
   "select n.next_boot_path, n.next_boot_cmd_line, n.def_boot_image_id, "
   "d.img_desc, p.partition, n.def_boot_cmd_line, d.img_path from nodes "
   "as n left join partitions as p on n.node_id=p.node_id and "
   "n.def_boot_image_id=p.image_id left join disk_images as d on "
   "p.image_id=d.image_id left join interfaces as i on "
   "i.node_id=n.node_id where i.IP = '%s'";

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
	case 7: /* Should have 7 fields */
		break;
	default:
		syslog(LOG_ERR, "%s: %d fields in query for IP %s!",
			dbname, ncols, inet_ntoa(ipaddr));
		mysql_free_result(res);
		return 1;
	}

	row = mysql_fetch_row(res);

#if 1
	/*
	 * First element is next_boot_path.  If set, assume it is a
	 * multiboot kernel.
	 */
	if (row[0] != 0 && row[0][0] != '\0') {
		info->type = BIBOOTWHAT_TYPE_MB;
		info->what.mb.tftp_ip.s_addr = 0;
		strncpy(info->what.mb.filename, row[0], MAX_BOOT_PATH-1);
		if (row[1] != 0 && row[1][0] != '\0')
			strncpy(info->cmdline, row[1], MAX_BOOT_CMDLINE-1);
		else
			info->cmdline[0] = 0;	/* Must zero first byte! */
		mysql_free_result(res);
		return 0;
	}
#endif

	/* Fifth element (row[4]) should have the partition number */
	if (row[4] == 0 || row[4][0] == '\0') {
		syslog(LOG_ERR, "%s: null query result for IP %s!",
			dbname, inet_ntoa(ipaddr));
		mysql_free_result(res);
		return 1;
	}

	/*
	 * Just 45000 lines of code later, we have the 32 bits of info
	 * we wanted!
	 */
	part = atoi(row[4]);
	mysql_free_result(res);

	info->type = BIBOOTWHAT_TYPE_PART;
	info->what.partition = part;
	if (row[5] != 0 && row[5][0] != '\0')
		strncpy(info->cmdline, row[5], MAX_BOOT_CMDLINE-1);
	else
		info->cmdline[0] = 0;	/* Must zero first byte! */
	
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
