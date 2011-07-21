/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * The C version of injail.pl
 * A much smaller memory footprint.
 *
 * The point of this is to fire up the init code inside the jail,
 * and then wait for a signal from outside the jail. When that happens
 * kill off everything inside the jail and exit. So, like a mini version
 * of /sbin/init, since killing the jail cleanly from outside the jail
 * turns out to be rather difficult, and doing it from inside is very easy!
 */

#include <unistd.h>
#include <signal.h>
#include <stdio.h>

/*
 */

char *vnode;
char *prog;
int myuid;

void
handler(int signo)
{
	pid_t killpid = myuid ? 0 : -1;

	signal(SIGUSR1, SIG_IGN);
	kill(killpid, SIGTERM);
	sleep(1);
	kill(killpid, SIGKILL);
	exit(signo);
}

void
usage(void)
{
	fprintf(stderr, "%s: [ -v vnodename ] cmd cmdargs ...\n", prog);
}

char *defargv[] = { "/bin/sh", "/etc/rc", 0 };

int
main(int argc, char **argv)
{
	int ch;
	pid_t child;
	extern char **environ;

	prog = argv[0];
	myuid = getuid();

	while ((ch = getopt(argc, argv, "v:")) != -1)
		switch(ch) {
		case 'v':
			vnode = optarg;
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

#ifdef __FreeBSD__
	if (vnode)
		setproctitle("%s", vnode);
#endif

	signal(SIGTERM, SIG_IGN);
	signal(SIGUSR1, handler);

	child = fork();
	if (child == 0) {
		signal(SIGTERM, SIG_DFL);
		if (argc == 0)
			argv = defargv;

		execve(argv[0], argv, environ);
		fprintf(stderr, "exec of %s failed\n", argv[0]);
		_exit(1);
	}

#if 0
	/* XXX don't think we want this? */
	daemon(0, 0);
#endif
	(void) waitpid(child, 0, 0);

	/*
	 * If a command list was provided, we exit.
	 * Otherwise we wait forever.
	 */
	if (argc)
		handler(0);
	else {
		sigset_t mask;

		sigprocmask(0, 0, &mask);
		while (1)
			sigsuspend(&mask);
	}
	exit(0);
}
