/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003, 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * This odd program is used to generate last login information on a per
 * user and per node basis. That is, for each user we want the last time
 * they logged in anyplace, and for each node we want the last time anyone
 * logged into it. The latter is obviously more useful for scheduling
 * purposes. 
 *
 * We get this information from all of the syslog entries that are reported in
 * by all the nodes when people ssh login. We have set up each experimental
 * node to report auth.info to @users.emulab.net. Note that the start flags
 * to syslogd (on users) must be changed to allows these incoming UDP packets.
 * In /etc/rc.conf:
 *
 *	syslogd_flags="-a 155.101.132.0/22"
 *
 * This program parses that file and inserts a bunch of entries into the DB.
 * See the update comands below.
 *
 * The entry in users:/etc/syslog.conf to capture the syslog data coming
 * from the nodes:
 *
 *	auth.info			/var/log/logins
 *
 * while on each experimental node:
 * 
 *	auth.info			@users.emulab.net
 *
 * Of course, you need to make sure /var/log/logins is cleaned periodically,
 * so put an entry in /etc/newsyslog.conf:
 *
 *	/var/log/logins           640  7     500 *     Z
 *
 * To prevent information loss that can occurr between the primary file
 * and the first roll file (logins.0.gz), the program actually reads the
 * first roll file (it is gzipped of course). This causes us to do some
 * extra work, but thats okay since we really do not want to lose that data.
 */

#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <utmp.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <setjmp.h>
#include <sys/fcntl.h>
#include <sys/param.h>
#include <sys/syslog.h>
#include <stdarg.h>
#include <assert.h>
#include <netdb.h>
#include <sys/socket.h>
#include <mysql/mysql.h>
#include <zlib.h>
#include "tbdb.h"

/*
 * This is the NFS mountpoint where users:/var is mounted.
 */
#ifndef USERSVAR
#define USERSVAR	"/usr/testbed/usersvar"
#endif

#define LOGINS		"log/logins"
#define SSHD		"sshd"

#ifndef LOG_TESTBED
#define LOG_TESTBED	LOG_USER
#endif

static char		*progname;
static int		debug = 0;
static int		doit(gzFile *infp);
static char		opshostname[MAXHOSTNAMELEN];
static jmp_buf		deadline;
static int		deadfl;

static void
usage(void)
{
	fprintf(stderr, "Usage: %s [-a]\n", progname);
	exit(-1);
}

static void
dead()
{
	deadfl = 1;
	longjmp(deadline, 1);
}

int
main(int argc, char **argv)
{
	gzFile	       *infp;
	char		buf[BUFSIZ], *bp, **aliases;
	struct hostent  *he;
	int		ch, errors = 0;
	int		backcount = 0;

	progname = argv[0];

	while ((ch = getopt(argc, argv, "a:")) != -1)
		switch(ch) {
		case 'a':
			backcount = atoi(optarg);
			break;
			
		case 'h':
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc)
		usage();

	openlog("genlastlog", LOG_PID, LOG_TESTBED);
	syslog(LOG_NOTICE, "genlastlog starting");

	if (!dbinit()) {
		syslog(LOG_ERR, "Could not connect to DB!");
		exit(1);
	}

	/*
	 * We need the canonical hostname for the usersnode so that we can
	 * put those logins in another table.
	 */
	if ((he = gethostbyname(USERNODE)) == NULL) {
		syslog(LOG_ERR, "gethostname %s: %s",
		       USERNODE, hstrerror(h_errno));
		exit(-1);
	}
	strncpy(opshostname, he->h_name, strlen(opshostname));
	if (bp = strchr(opshostname, '.')) 
		*bp = 0;

	while (backcount) {
		sprintf(buf, "%s/%s.%d.gz", USERSVAR, LOGINS, backcount);

		/*
		 * Use setjmp and timer to prevent NFS lockup.
		 */
		if (setjmp(deadline) == 0) {
			alarm(15);

			if ((infp = gzopen(buf, "r")) == NULL) {
				syslog(LOG_ERR, "Opening %s: %m", buf);
				errors++;
			}
			else {
				doit(infp);
				gzclose(infp);
			}
		}
		backcount--;
		alarm(0);
	}
	
	sprintf(buf, "%s/%s", USERSVAR, LOGINS);

	if (setjmp(deadline) == 0) {
		alarm(15);

		if ((infp = gzopen(buf, "r")) == NULL) {
			syslog(LOG_ERR, "Opening %s: %m", buf);
			errors++;
		}
		else {
			doit(infp);
			gzclose(infp);
		}
	}
	alarm(0);

	syslog(LOG_NOTICE, "genlastlog ending");
	exit(errors);
	
}

static int
doit(gzFile *infp)
{
	int		i, skip = 0;
	time_t		curtime, ll_time;
	char		*user, node[64], prog[128];
	char		buf[BUFSIZ], *bp;
	struct tm	tm;

	while (1) {
		if (gzgets(infp, buf, BUFSIZ) == NULL)
			break;

		/*
		 * If the line does not contain a newline, then we skip it
		 * and try to sync up again. We consider ourselves synced
		 * when the buffer contains a newline in it.
		 */
		if (buf[strlen(buf) - 1] != '\n') {
			skip = 1;
			continue;
		}
		if (skip) {
			skip = 0;
			continue;
 		}

		/*
		 * Thank dog for strptime! Convert the syslog timestamp
		 * into a tm, and then into regular unix time.
		 */
		time(&curtime);
		localtime_r(&curtime, &tm);
		if ((bp = strptime(buf, "%b %e %T", &tm)) == NULL) {
			continue;
		}
		ll_time = mktime(&tm);

		/*
		 * If the constructed time is in the future, then we have
		 * year off by one (cause we are possibly looking at files
		 * created in the previous year). Set the year back by one,
		 * and redo.
		 */
		if (ll_time > curtime) {
			tm.tm_year--;
			ll_time = mktime(&tm);
		}

		/*
		 * Scanf the next part, which looks like:
		 *
		 *	node progname[pid]:
		 *
		 * Ensure we match the proper number of items.
		 */
		bzero(node, sizeof(node));
		if ((sscanf(bp, "%s %s:", node, prog) != 2))
			continue;

		/*
		 * Only sshd matters to us.
		 */
		if (strncmp(prog, SSHD, strlen(SSHD)))
			continue;

		/*
		 * Okay, these kinds of strings matter.
		 *
		 *	FreeBSD:	"Accepted rsa for USER" 
		 *	Linux 6.2:	"log: RSA authentication for USER"
		 *	Linux 7.1:	"session opened for user USER"
		 *      (several ssh2): "Accepted publickey for USER"
		 *      (several ssh2): "Accepted password for USER"
		 *      (several ssh2): "Accepted keyboard-interactive for USER"
		 */
#define L1	"Accepted rsa for "
#define L2	"session opened for user "
#define L3	"log: RSA authentication for "
#define L4	"Accepted publickey for "
#define L5	"Accepted password for "
#define L6	"Accepted keyboard-interactive for "
		
		/* Skip to end of program[pid]: and trailing space */
		bp = strchr(bp, ':');
		bp += 2;

		if (strncmp(bp, L1, strlen(L1)) == 0) {
		  /*fprintf(stdout,"Hit L1: ");*/
			bp += strlen(L1);
		}
		else if (strncmp(bp, L2, strlen(L2)) == 0) {
		  /*fprintf(stdout,"Hit L2: ");*/
			bp += strlen(L2);
		}
		else if (strncmp(bp, L3, strlen(L3)) == 0) {
		  /*fprintf(stdout,"Hit L3: ");*/
			bp += strlen(L3);
		}
		else if (strncmp(bp, L4, strlen(L4)) == 0) {
		  /*fprintf(stdout,"Hit L4: ");*/
			bp += strlen(L4);
		}
		else if (strncmp(bp, L5, strlen(L5)) == 0) {
		  /*fprintf(stdout,"Hit L5: ");*/
			bp += strlen(L5);
		}
		else if (strncmp(bp, L6, strlen(L6)) == 0) {
		  /*fprintf(stdout,"Hit L6: ");*/
			bp += strlen(L6);
		}
		else {
			continue;
		}

		/*
		 * The login name is the next token.
		 */
		if (! (user = strsep(&bp, " ")))
			continue;
		/*fprintf(stdout,"%s on %s\n",user,node);*/

		/* We do not care about ROOT logins. */
		if (strcasecmp(user, "ROOT") == 0)
			continue;

		if (mydb_update("replace into uidnodelastlogin "
				"(uid, node_id, date, time) "
				"values ('%s', '%s', "
				"        FROM_UNIXTIME(%ld, '%%Y-%%m-%%d'), "
				"        FROM_UNIXTIME(%ld, '%%T')) ",
				user, node, ll_time, ll_time) == 0)
			break;

		if (strncmp(node, opshostname, strlen(node)) == 0 ||
		    strncmp(node, "ops", strlen(node)) == 0) {
			if (mydb_update("replace into userslastlogin "
					"(uid, date, time) "
					"values ('%s', "
					"  FROM_UNIXTIME(%ld, '%%Y-%%m-%%d'), "
					"  FROM_UNIXTIME(%ld, '%%T')) ",
					user, ll_time, ll_time) == 0)
				break;
		}
		else {
			if (mydb_update("replace into nodeuidlastlogin "
					"(node_id, uid, date, time) "
					"values ('%s', '%s', "
					"  FROM_UNIXTIME(%ld, '%%Y-%%m-%%d'), "
					"  FROM_UNIXTIME(%ld, '%%T')) ",
					node, user, ll_time, ll_time) == 0)
				break;
		}
	}
	return 0;
}
