/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2008 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <string.h>
#include "config.h"
#include "log.h"
#include "tbdb.h"
#include "bootwhat.h"
#include "bootinfo.h"
#include <mysql/mysql.h>

/* XXX Should be configured in */
#define NEWNODEPID      "emulab-ops"
#define NEWNODEOSID	"NEWNODE-MFS"

#ifdef USE_MYSQL_DB

static int parse_multiboot_path(char *path, boot_what_t *info);
static void parse_mfs_path(char *path, boot_what_t *info);
static int boot_newnode_mfs(struct in_addr, int, boot_what_t *);

int
open_bootinfo_db(void)
{
	if (!dbinit())
		return 1;

	return 0;
}

/*
  WARNING!!!
  
  DO NOT change this function without making corresponding changes to
  the perl version of this code in db/libdb.pm . They MUST ALWAYS
  return exactly the same result given the same inputs.
*/

int
query_bootinfo_db(struct in_addr ipaddr, int version, boot_what_t *info, char* key)
{
	int		nrows, rval = 0;
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		ipstr[32];
	int		haskey=0;

	char		savedkey[HOSTKEY_LENGTH];
	if(key != NULL)
		{
		strncpy(savedkey, key, HOSTKEY_LENGTH);
		haskey=1;
		}
	info->cmdline[0] = 0;	/* Must zero first byte! */
	info->flags      = 0;
	strcpy(ipstr, inet_ntoa(ipaddr));

#define DEF_BOOT_OSID		0
#define DEF_BOOT_CMDLINE	1
#define DEF_BOOT_PATH		2
#define DEF_BOOT_MFS		3
#define DEF_BOOT_PARTITION	4
#define TEMP_BOOT_OSID		5
#define TEMP_BOOT_PATH		6
#define TEMP_BOOT_MFS		7
#define TEMP_BOOT_PARTITION	8
#define NEXT_BOOT_OSID		9
#define NEXT_BOOT_CMDLINE	10
#define NEXT_BOOT_PATH		11
#define NEXT_BOOT_MFS		12
#define NEXT_BOOT_PARTITION	13
#define PID			14
#define PXE_BOOT_PATH		15
#define DEFINED(x)		(row[(x)] != NULL && row[(x)][0] != '\0')
#define TOINT(x)		(atoi(row[(x)]))

	if(! haskey) {
		res = mydb_query("select n.def_boot_osid, n.def_boot_cmd_line, "
				 "        odef.path, odef.mfs, pdef.partition, "
				 "       n.temp_boot_osid, "
				 "        otemp.path, otemp.mfs, ptemp.partition, "
				 "       n.next_boot_osid, n.next_boot_cmd_line, "
				 "        onext.path, onext.mfs, pnext.partition, "
				 "       r.pid,n.pxe_boot_path "
				 " from interfaces as i "
				 "left join nodes as n on i.node_id=n.node_id "
				 "left join reserved as r on i.node_id=r.node_id "
				 "left join partitions as pdef on "
				 "     n.node_id=pdef.node_id and "
				 "     n.def_boot_osid=pdef.osid "
				 "left join os_info as odef on "
				 "     odef.osid=n.def_boot_osid "
				 "left join partitions as ptemp on "
				 "     n.node_id=ptemp.node_id and "
				 "     n.temp_boot_osid=ptemp.osid "
				 "left join os_info as otemp on "
				 "     otemp.osid=n.temp_boot_osid "
				 "left join partitions as pnext on "
				 "     n.node_id=pnext.node_id and "
				 "     n.next_boot_osid=pnext.osid "
				 "left join os_info as onext on "
				 "     onext.osid=n.next_boot_osid "
				 "left outer join "
				 "  (select type,attrvalue from node_type_attributes "
				 "     where attrkey='nobootinfo' and attrvalue='1' "
				 "     group by type) as nobootinfo_types "
				 "  on n.type=nobootinfo_types.type "
				 "where i.IP='%s' "
				 "  and nobootinfo_types.attrvalue is NULL",
				 16, inet_ntoa(ipaddr));
	}
	else { /* User provided a widearea hostkey, so they don't have a necessarily-unique IP address. */
		/* This is meant to be similar to the above, but queries on the wideareanodekey instead. */
		res = mydb_query("SELECT n.def_boot_osid, n.def_boot_cmd_line, "
				 "odef.path, odef.mfs, pdef.partition, "
				 "n.temp_boot_osid, "
				 "otemp.path, otemp.mfs, ptemp.partition, "
				 "n.next_boot_osid, n.next_boot_cmd_line, "
				 "onext.path, onext.mfs, pnext.partition, "
				 "r.pid,n.pxe_boot_path "
				 "FROM nodes AS n "
				 "LEFT JOIN reserved AS r ON n.node_id=r.node_id "
				 "LEFT JOIN partitions AS pdef ON n.node_id=pdef.node_id AND n.def_boot_osid=pdef.osid "
				 "LEFT JOIN os_info AS odef ON odef.osid=n.def_boot_osid "
				 "LEFT JOIN partitions AS ptemp ON n.node_id=ptemp.node_id AND n.temp_boot_osid=ptemp.osid "
				 "LEFT JOIN os_info AS otemp ON otemp.osid=n.temp_boot_osid "
				 "LEFT JOIN partitions AS pnext ON n.node_id=pnext.node_id AND n.next_boot_osid=pnext.osid "
				 "LEFT JOIN os_info AS onext ON onext.osid=n.next_boot_osid "
				 "LEFT OUTER JOIN "
					"(SELECT type,attrvalue FROM node_type_attributes WHERE attrkey='nobootinfo' AND attrvalue='1' GROUP BY type) "
				 	"AS nobootinfo_types ON n.type=nobootinfo_types.type "
				 "WHERE n.node_id IN "
					"(SELECT node_id FROM widearea_nodeinfo WHERE privkey='%s') "
					"AND nobootinfo_types.attrvalue IS NULL;", 16, savedkey);
	}
	
	if (!res) {
		error("Query failed for host %s\n", ipstr);
		/* XXX Wrong. Should fail so client can request again later */
		return 0;
	}
	nrows = mysql_num_rows(res);

	switch (nrows) {
	case 0:
		mysql_free_result(res);
		return boot_newnode_mfs(ipaddr, version, info);
	case 1:
		break;
	default:
		error("%d entries for host %s\n", nrows, ipstr);
		break;
	}
	row = mysql_fetch_row(res);

	/*
	 * Version >=1 supports wait if not allocated. Client will recontact
	 * us later.
	 */
	if (version >= 1 && row[PID] == (char *) NULL) {
		info->type = BIBOOTWHAT_TYPE_WAIT;
		goto done;
	}

	/*
	 * If we received a query from a node whose PXE boot program is
	 * not an "Emulab pxeboot", then the node may be coming out of PXEWAIT
	 * and we need to tell it to reboot again to pick up the new PXE boot
	 * program.  An "Emulab pxeboot" is one that speaks bootinfo.
	 *
	 * XXX note that an "Emulab pxeboot" is currently identified by
	 * its not being the default pxeboot and its path containing the
	 * string "pxeboot" anywhere.
	 */
	if (DEFINED(PXE_BOOT_PATH) &&
	    strstr(row[PXE_BOOT_PATH], "pxeboot") == NULL) {
		info->type = BIBOOTWHAT_TYPE_REBOOT;
		goto done;
	}

	/*
	 * Check next_boot_osid. It overrides the others. It should be
	 * the case that partition and path/mfs are mutually exclusive.
	 * mfs might be set when path is set.  
	 */
	if (DEFINED(NEXT_BOOT_OSID)) {
		if (DEFINED(NEXT_BOOT_PATH)) {
			if (DEFINED(NEXT_BOOT_MFS) && TOINT(NEXT_BOOT_MFS) == 1){
				info->type = BIBOOTWHAT_TYPE_MFS;
				parse_mfs_path(row[NEXT_BOOT_PATH], info);
			}
			else {
				info->type = BIBOOTWHAT_TYPE_MB;
				parse_multiboot_path(row[NEXT_BOOT_PATH], info);
			}
		}
		else if (DEFINED(NEXT_BOOT_PARTITION)) {
			info->type = BIBOOTWHAT_TYPE_PART;
			info->what.partition = TOINT(NEXT_BOOT_PARTITION);
		}
		else {
			error("Invalid NEXT_BOOT entry for host %s\n", ipstr);
			rval = 1;
		}
		if (DEFINED(NEXT_BOOT_CMDLINE)) {
			/*
			 * XXX note that this will override any cmdline
			 * specified in the osid path.  Should append instead?
			 */
			strncpy(info->cmdline,
				row[NEXT_BOOT_CMDLINE], MAX_BOOT_CMDLINE-1);
		}
		goto done;
	}

	/*
	 * Check temp_boot_osid. It overrides def_boot but not next_boot.
	 */
	if (DEFINED(TEMP_BOOT_OSID)) {
		if (DEFINED(TEMP_BOOT_PATH)) {
			if (DEFINED(TEMP_BOOT_MFS) && TOINT(TEMP_BOOT_MFS) == 1){
				info->type = BIBOOTWHAT_TYPE_MFS;
				parse_mfs_path(row[TEMP_BOOT_PATH], info);
			}
			else {
				info->type = BIBOOTWHAT_TYPE_MB;
				parse_multiboot_path(row[TEMP_BOOT_PATH], info);
			}
		}
		else if (DEFINED(TEMP_BOOT_PARTITION)) {
			info->type = BIBOOTWHAT_TYPE_PART;
			info->what.partition = TOINT(TEMP_BOOT_PARTITION);
		}
		else {
			error("Invalid TEMP_BOOT entry for host %s\n", ipstr);
			rval = 1;
		}
		goto done;
	}

	/*
	 * Lastly, def_boot.
	 */
	if (DEFINED(DEF_BOOT_OSID)) {
		if (DEFINED(DEF_BOOT_PATH)) {
			if (DEFINED(DEF_BOOT_MFS) && TOINT(DEF_BOOT_MFS) == 1) {
				info->type = BIBOOTWHAT_TYPE_MFS;
				parse_mfs_path(row[DEF_BOOT_PATH], info);
			}
			else {
				info->type = BIBOOTWHAT_TYPE_MB;
				parse_multiboot_path(row[DEF_BOOT_PATH], info);
			}
		}
		else if (DEFINED(DEF_BOOT_PARTITION)) {
			info->type = BIBOOTWHAT_TYPE_PART;
			info->what.partition = TOINT(DEF_BOOT_PARTITION);
		}
		else {
			error("Invalid DEF_BOOT entry for host %s\n", ipstr);
			rval = 1;
		}
		if (DEFINED(DEF_BOOT_CMDLINE)) {
			/*
			 * XXX note that this will override any cmdline
			 * specified in the osid path.  Should append instead?
			 */
			strncpy(info->cmdline,
				row[DEF_BOOT_CMDLINE], MAX_BOOT_CMDLINE-1);
		}
		goto done;
	}
	/*
	 * If we get here, there is no bootinfo to give the client.
	 * New PXE boot clients get PXEWAIT, but older ones get an error.
	 */
	error("No OSIDs set for host %s\n", ipstr);
	if (version >= 1) {
		info->type = BIBOOTWHAT_TYPE_WAIT;
		goto done;
	}
	rval = 1;
 done:
	mysql_free_result(res);
	return rval;
}

int
close_bootinfo_db(void)
{
	dbclose();
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

/*
 * Arrange to boot the special newnode kernel.
 */
static int
boot_newnode_mfs(struct in_addr ipaddr, int version, boot_what_t *info)
{
	int		nrows;
	MYSQL_RES	*res;
	MYSQL_ROW	row;

	error("%s: nonexistent IP, booting '%s,%s'\n",
	      inet_ntoa(ipaddr), NEWNODEPID, NEWNODEOSID);

#define MFS_PATH	0

	res = mydb_query("select path from os_info where pid='%s' and "
			 "osname='%s' and mfs=1", 1, NEWNODEPID, NEWNODEOSID);

	if (!res) {
		error("Query failed\n");
		/* XXX Wrong. Should fail so client can request again later */
		return 0;
	}
	nrows = mysql_num_rows(res);

	switch (nrows) {
	case 0:
		error("No DB entry for OSID %s\n", NEWNODEOSID);
		mysql_free_result(res);
		return 1;
	case 1:
		break;
	default:
		error("Too many DB entries for OSID %s\n", NEWNODEOSID);
		mysql_free_result(res);
		return 1;
	}
	row = mysql_fetch_row(res);

	if (row[MFS_PATH] != 0 && row[MFS_PATH][0] != '\0') {
		info->type = BIBOOTWHAT_TYPE_MFS;
		parse_mfs_path(row[MFS_PATH], info);
		mysql_free_result(res);
		return 0;
	}
	mysql_free_result(res);
	error("No path info for OSID %s\n", NEWNODEOSID);
	return 1;
#undef  MFS_PATH
}

void
parse_mfs_path(char *str, boot_what_t *info)
{
	struct hostent *he;
	struct in_addr hip;
	char *path, *args;

	/* treat anything after a space as the command line */
	args = strchr(str, ' ');
	if (args != NULL) {
		*args++ = '\0';
		strncpy(info->cmdline, args, MAX_BOOT_CMDLINE-1);
	}

	/* no hostname, just copy string as is */
	path = strchr(str, ':');
	if (path == NULL) {
		strncpy(info->what.mfs, str, sizeof(info->what.mfs));
		return;
	}
	*path = '\0';

	/* hostname is a valid IP addr, copy as is */
	if (inet_addr(str) != INADDR_NONE) {
		*path = ':';
		strncpy(info->what.mfs, str, sizeof(info->what.mfs));
		return;
	}

	/* not a valid hostname, whine and copy it as is */
	he = gethostbyname(str);
	if (he == NULL) {
		*path = ':';
		error("Invalid hostname in MFS path '%s', passing anyway\n",
		      str);
		strncpy(info->what.mfs, str, sizeof(info->what.mfs));
		return;
	}
	*path = ':';

	/* valid hostname, translate to IP and replace in string */
	memcpy((char *)&hip, he->h_addr, he->h_length);
	strcpy(info->what.mfs, inet_ntoa(hip));
	strncat(info->what.mfs, path,
		sizeof(info->what.mfs)-strlen(info->what.mfs));
}

/*
 * ElabinElab hack. Its really terrible!
 */
#ifdef ELABINELAB
int
elabinelab_hackcheck(struct sockaddr_in *target)
{
	int		nrows;
	MYSQL_RES	*res;
	MYSQL_ROW	row;

	res = mydb_query("select i2.IP from interfaces as i1 "
			 "left join interfaces as i2 on i2.node_id=i1.node_id "
			 "     and i2.role='outer_ctrl' "
			 "where i1.IP='%s'", 1, inet_ntoa(target->sin_addr));
	if (!res) {
		error("elabinelab_hackcheck failed for host %s\n",
		      inet_ntoa(target->sin_addr));
		/* XXX Wrong. Should fail so client can request again later */
		return 0;
	}

	nrows = mysql_num_rows(res);

	switch (nrows) {
	case 0:
		/* No hack interface */
		mysql_free_result(res);
		return 0;
	case 1:
		break;
	default:
		error("elabinelab_hackcheck: Too many DB entries %s\n",
		      inet_ntoa(target->sin_addr));
		mysql_free_result(res);
		return 1;
	}
	row = mysql_fetch_row(res);

	if (row[0] != 0 && row[0][0] != '\0') {
		inet_aton(row[0], &(target->sin_addr));
		return 0;
	}
	mysql_free_result(res);
	error("elabinelab_hackcheck: No IP address %s\n",
	      inet_ntoa(target->sin_addr));
	return 1;
}
#endif

#ifdef TEST
#include <stdarg.h>

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
	case BIBOOTWHAT_TYPE_WAIT:
		printf("No boot; waiting till allocated\n");
		break;
	case BIBOOTWHAT_TYPE_MFS:
		printf("boot from MFS %s\n", bootinfo->what.mfs);
		break;
	}
	if (bootinfo->cmdline[0])
		printf("Command line %s\n", bootinfo->cmdline);
		
}

int
main(int argc, char **argv)
{
	struct in_addr ipaddr;
	boot_info_t boot_info;
	boot_what_t *boot_whatp = (boot_what_t *)&boot_info.data;

	open_bootinfo_db();
	while (--argc > 0) {
		if (inet_aton(*++argv, &ipaddr))
			if (query_bootinfo_db(ipaddr, 1, boot_whatp) == 0) {
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
