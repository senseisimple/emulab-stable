/*
 * A little ditty to pull the last log info out and report a list of
 * the last time each person has logged in.
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

#ifndef USERSVAR
#define USERSVAR "/usr/testbed/usersvar"
#endif

static char	*progname;
static int	doentry(FILE *fp, uid_t uid, int umode);
static jmp_buf	deadline;
static int	deadfl;

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
	char		buf[BUFSIZ], *uidarg;
	uid_t		uid;
	int		ch, rval, umode = 0;

	/*
	 * Web server sillyness. Must be some kind of FD mistake cause
	 * stderr is lost if we don't do this. 
	 */
	dup2(1, 2);

	progname = argv[0];

	while ((ch = getopt(argc, argv, "u:")) != -1)
		switch(ch) {
		case 'u':
			umode  = 1;
			uidarg = optarg;
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

	sprintf(buf, "%s/log/lastlog", USERSVAR);

	/*
	 * Protect against NFS timeout.
	 */
	alarm(3);	
	fp = fopen(buf, "r");
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

		if (umode)
		    printf("%s\n", buf);
		else
		    printf("%u %s\n", uid, buf);
	}
	return 1;
}

