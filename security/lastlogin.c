/*
 * This program is invoked from the web interface to determine the last
 * time a user logged in on the users node. We parse the lastlog
 * file from the users node via NFS (see USERSVAR below), and spit out
 * the last time he/she logged in. This gives us usage information to aid
 * in scheduling and to determine who is really active. 
 *
 * Without a -u specification, print out a list of all users one per line 
 * with the date of their last login.
 * 
 * The output format is simple:
 *
 *	[uid] date time hostname
 *
 * where [uid] is added when doing all users, and date, time, hostname all
 * come from the lastlog data structure (see utmp.h).
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

/*
 * This is the NFS mountpoint where users:/var is mounted.
 */
#ifndef USERSVAR
#define USERSVAR	"/usr/testbed/usersvar"
#endif

#define LASTLOG		"log/lastlog"

static char	*progname;
static int	doentry(FILE *fp, uid_t uid, int umode);
static jmp_buf	deadline;
static int	deadfl;
static int	debug;

void
usage(void)
{
	fprintf(stderr, "Usage: %s [-u user]\n", progname);
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
	FILE	       *fp;
	char		buf[BUFSIZ], *uidarg, *pcarg = 0;
	uid_t		uid;
	int		ch, rval, umode = 0;

	/*
	 * Web server sillyness. Must be some kind of FD mistake cause
	 * stderr is lost if we don't do this. 
	 */
	dup2(1, 2);

	progname = argv[0];

	while ((ch = getopt(argc, argv, "u:d")) != -1)
		switch(ch) {
		case 'u':
			umode  = 1;
			uidarg = optarg;
			break;

		case 'd':
			debug = 1;
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

	/*
	 * Allow either numeric or alpha arg.
	 */
	if (umode) {
		if (isdigit(uidarg[0]))
			uid = atoi(uidarg);
		else {
			struct passwd	*pw = getpwnam(uidarg);

			if (pw == NULL) {
				fprintf(stderr, "Bad user %s!\n", uidarg);
				exit(-1);
			}
			uid = pw->pw_uid;
		}
	}

	sprintf(buf, "%s/%s", USERSVAR, LASTLOG);

	/*
	 * Protect against NFS timeout.
	 */
	if (setjmp(deadline) == 0) {
		alarm(3);
		fp = fopen(buf, "r");
	}
	alarm(0);

	if (deadfl) {
		fprintf(stderr, "Timed out opening %s\n", buf);
		exit(-1);
	}
	if (fp == NULL) {
		warn("Opening %s", buf);
		exit(-1);
	}

	if (umode) {
		long	seekoff = (long) (uid * sizeof(struct lastlog));
					  
		fseek(fp, 0L, SEEK_END);
		if (seekoff > ftell(fp)) {
			fprintf(stderr, "Error Locating uid %d.\n", uid);
			exit(-1);
		}
		
		rval = fseek(fp, (long) (uid * sizeof(struct lastlog)),
			     SEEK_SET);

		if (rval) {
			warn("Seeking to %ld", seekoff);
			exit(-1);
		}
		rval = 0;
		if (doentry(fp, uid, 1) <= 0)
		    rval = -1;
	}
	else {
		rval = 0;
		for (uid = 0; ; uid++) {
			if ((rval = doentry(fp, uid, 0)) <= 0)
				break;
		}
	}
	fclose(fp);
	exit(rval);
}

static int
doentry(FILE *fp, uid_t uid, int umode)
{
	struct lastlog	ll;
	char		buf[BUFSIZ];

	if (fread(&ll, sizeof(ll), 1, fp) != 1) {
		if (ferror(fp)) {
			fprintf(stderr, "Error reading entry %u\n", uid);
			return -1;
		}
		return 0;
	}

	if (ll.ll_time) {
		strftime(buf, sizeof(buf),
			 "20%y-%m-%d %H:%M:%S", localtime(&ll.ll_time));

		if (! ll.ll_host[0])
		    strcpy(ll.ll_host, "unknown");

		if (umode)
		    printf("%s %.*s", buf, sizeof(ll.ll_host), ll.ll_host);
		
		else 
			printf("%u %s %.*s", uid, buf,
			       sizeof(ll.ll_host), ll.ll_host);
		
		if (debug) {
			struct passwd	*pw = getpwuid(uid);

			if (pw) {
				printf(" (%s)", pw->pw_name);
			}
		}
		printf("\n");
	}
	return 1;
}
