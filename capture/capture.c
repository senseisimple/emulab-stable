/* 
 * File:	capture.c
 * Description: 
 * Author:	Leigh Stoller
 * 		Computer Science Dept.
 * 		University of Utah
 * Date:	27-Jul-92
 *
 * (c) Copyright 1992, 2000-2001, University of Utah, all rights reserved.
 */

/*
 * Testbed note:  This code has developed over the last several
 * years in RCS.  This is an import of the current version of
 * capture from the /usr/src/utah RCS repository into the testbed,
 * with some new hacks to port it to Linux.
 *
 * - dga, 10/10/2000
 */

/*
 * A LITTLE hack to record output from a tty device to a file, and still
 * have it available to tip using a pty/tty pair.
 */

#define SAFEMODE
	
#include <sys/param.h>

#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <syslog.h>
#ifdef HPBSD
#include <sgtty.h>
#else
#include <termios.h>
#endif
#include <errno.h>

#include <sys/param.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/termios.h>
#ifdef USESOCKETS
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <setjmp.h>
#include <netdb.h>
#endif
#include "capdecls.h"

#define geterr(e)	strerror(e)

void quit(int);
void reinit(int);
void newrun(int);
void terminate(int);
void cleanup(void);
void capture();
void usage();

#ifdef __linux__
#define _POSIX_VDISABLE '\0'
#define revoke(tty)	(0)
#endif

#ifdef HPBSD
#define TWOPROCESS	/* historic? */
#endif

/*
 *  Configurable things.
 */
#ifdef HPBSD
#define LOGPATH		"/usr/adm/tiplogs"
#else
#define LOGPATH		"/var/log/tiplogs"
#endif
#define PIDNAME		"%s/%s.pid"
#define LOGNAME		"%s/%s.log"
#define RUNNAME		"%s/%s.run"
#define TTYNAME		"%s/%s"
#define PTYNAME		"%s/%s-pty"
#define ACLNAME		"%s/%s.acl"
#define DEVNAME		"%s/%s"
#define BUFSIZE		4096
#define DROP_THRESH	(32*1024)

char 	*Progname;
char 	*Pidname;
char	*Logname;
char	*Runname;
char	*Ttyname;
char	*Ptyname;
char	*Devname;
char	*Machine;
char	*Bossnode = BOSSNODE;
int	logfd, runfd, devfd, ptyfd;
int	hwflow = 0, speed = B9600, debug = 0, runfile = 0;
#ifdef  USESOCKETS
int		   sockfd, tipactive, portnum;
struct sockaddr_in tipclient;
secretkey_t	   secretkey;
char		   ourhostname[MAXHOSTNAMELEN];
int		   needshake;
#endif

int
main(argc, argv)
	int argc;
	char **argv;
{
	char strbuf[MAXPATHLEN], *newstr();
	int flags, op, i;
	extern int optind;
	extern char *optarg;
#ifdef  USESOCKETS
	struct sockaddr_in name;
#endif

	Progname = (Progname = rindex(argv[0], '/')) ? ++Progname : *argv;

	while ((op = getopt(argc, argv, "rds:Hb:it")) != EOF)
		switch (op) {

		case 'b':
			Bossnode = optarg;
			break;

		case 'H':
			++hwflow;
			break;

		case 'd':
			debug++;
			break;

		case 'r':
			runfile++;
			break;

		case 's':
			if ((i = atoi(optarg)) == 0 ||
			    (speed = val2speed(i)) == 0)
				usage();
			break;
		}

	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();

#ifndef HPBSD
	if (!debug)
		(void)daemon(0, 0);
#endif

	Machine = argv[0];

	(void) sprintf(strbuf, PIDNAME, LOGPATH, argv[0]);
	Pidname = newstr(strbuf);
	(void) sprintf(strbuf, LOGNAME, LOGPATH, argv[0]);
	Logname = newstr(strbuf);
	(void) sprintf(strbuf, RUNNAME, LOGPATH, argv[0]);
	Runname = newstr(strbuf);
	(void) sprintf(strbuf, TTYNAME, TIPPATH, argv[0]);
	Ttyname = newstr(strbuf);
	(void) sprintf(strbuf, PTYNAME, TIPPATH, argv[0]);
	Ptyname = newstr(strbuf);
	(void) sprintf(strbuf, DEVNAME, DEVPATH, argv[1]);
	Devname = newstr(strbuf);

	openlog(Progname, LOG_PID, LOG_USER);
	dolog(LOG_NOTICE, "starting");

	signal(SIGINT, quit);
	signal(SIGTERM, quit);
	signal(SIGHUP, reinit);
	if (runfile)
		signal(SIGUSR1, newrun);
	signal(SIGUSR2, terminate);

	/*
	 * Open up run/log file, console tty, and controlling pty.
	 */
	if ((logfd = open(Logname, O_WRONLY|O_CREAT|O_APPEND, 0640)) < 0)
		die("%s: open: %s", Logname, geterr(errno));
	if (chmod(Logname, 0640) < 0)
		die("%s: chmod: %s", Logname, geterr(errno));

	if (runfile) {
		if ((runfd = open(Runname,O_WRONLY|O_CREAT|O_APPEND,0640)) < 0)
			die("%s: open: %s", Runname, geterr(errno));
		if (chmod(Runname, 0640) < 0)
			die("%s: chmod: %s", Runname, geterr(errno));
	}
#ifdef  USESOCKETS
	/*
	 * Create and bind our socket.
	 */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		die("socket(): opening stream socket: %s", geterr(errno));

	i = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
		       (char *)&i, sizeof(i)) < 0)
		die("setsockopt(): SO_REUSEADDR: %s", geterr(errno));
	
	/* Create wildcard name. */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = 0;
	if (bind(sockfd, (struct sockaddr *) &name, sizeof(name)))
		die("bind(): binding stream socket: %s", geterr(errno));

	/* Find assigned port value and print it out. */
	i = sizeof(name);
	if (getsockname(sockfd, (struct sockaddr *)&name, &i))
		die("getsockname(): %s", geterr(errno));
	portnum = ntohs(name.sin_port);

	if (listen(sockfd, 1) < 0)
		die("listen(): %s", geterr(errno));

	if (gethostname(ourhostname, sizeof(ourhostname)) < 0)
		die("gethostname(): %s", geterr(errno));

	createkey();
	dolog(LOG_NOTICE, "Ready! Listening on TCP port %d", portnum);
#else
	if ((ptyfd = open(Ptyname, O_RDWR)) < 0)
		die("%s: open: %s", Ptyname, geterr(errno));
#endif
	if ((devfd = open(Devname, O_RDWR|O_NONBLOCK)) < 0)
		die("%s: open: %s", Devname, geterr(errno));

	if (ioctl(devfd, TIOCEXCL, 0) < 0)
		warn("TIOCEXCL %s: %s", Devname, geterr(errno));

	writepid();
	rawmode(speed);

	capture();

	cleanup();
	exit(0);
}

#ifdef TWOPROCESS
int	pid;

void
capture()
{
	flags = FNDELAY;
	(void) fcntl(ptyfd, F_SETFL, &flags);

	if (pid = fork())
		in();
	else
		out();
}

/*
 * Loop reading from the console device, writing data to log file and
 * to the pty for tip to pick up.
 */
in()
{
	char buf[BUFSIZE];
	int cc, omask;
	
	while (1) {
		if ((cc = read(devfd, buf, BUFSIZE)) < 0) {
			if ((errno == EWOULDBLOCK) || (errno == EINTR))
				continue;
			else
				die("%s: read: %s", Devname, geterr(errno));
		}
		omask = sigblock(sigmask(SIGHUP)|sigmask(SIGTERM)|
				 sigmask(SIGUSR1)|sigmask(SIGUSR2));

		if (write(logfd, buf, cc) < 0)
			die("%s: write: %s", Logname, geterr(errno));

		if (runfile) {
			if (write(runfd, buf, cc) < 0)
				die("%s: write: %s", Runname, geterr(errno));
		}

		if (write(ptyfd, buf, cc) < 0) {
			if ((errno != EIO) && (errno != EWOULDBLOCK))
				die("%s: write: %s", Ptyname, geterr(errno));
		}
		(void) sigsetmask(omask);
	}
}

/*
 * Loop reading input from pty (tip), and send off to the console device.
 */
out()
{
	char buf[BUFSIZE];
	int cc, omask;
	struct timeval timeout;

	timeout.tv_sec  = 0;
	timeout.tv_usec = 100000;
	
	while (1) {
		omask = sigblock(SIGUSR2);
		if ((cc = read(ptyfd, buf, BUFSIZE)) < 0) {
			(void) sigsetmask(omask);
			if ((errno == EIO) || (errno == EWOULDBLOCK) ||
			    (errno == EINTR)) {
				select(0, 0, 0, 0, &timeout);
				continue;
			}
			else
				die("%s: read: %s", Ptyname, geterr(errno));
		}
		(void) sigsetmask(omask);

		omask = sigblock(sigmask(SIGHUP)|sigmask(SIGTERM)|
				 sigmask(SIGUSR1)|sigmask(SIGUSR2));

		if (write(devfd, buf, cc) < 0)
			die("%s: write: %s", Devname, geterr(errno));
		
		(void) sigsetmask(omask);
	}
}
#else
static fd_set	sfds;
static int	fdcount;
void
capture()
{
	fd_set fds;
	int i, cc, lcc, omask;
	char buf[BUFSIZE];
	struct timeval timeout;
#ifdef LOG_DROPS
	int drop_topty_chars = 0;
	int drop_todev_chars = 0;
#endif

	/*
	 * XXX for now we make both directions non-blocking.  This is a
	 * quick hack to achieve the goal that capture never block
	 * uninterruptably for long periods of time (use threads).
	 * This has the unfortunate side-effect that we may drop chars
	 * from the perspective of the user (use threads).  A more exotic
	 * solution would be to poll the readiness of output (use threads)
	 * as well as input and not read from one end unless we can write
	 * the other (use threads).
	 *
	 * I keep thinking (use threads) that there is a better way to do
	 * this (use threads).  Hmm...
	 */
	if (fcntl(devfd, F_SETFL, O_NONBLOCK) < 0)
		die("%s: fcntl(O_NONBLOCK): %s", Devname, geterr(errno));
#ifndef USESOCKETS
	/*
	 * It gets better!
	 * In FreeBSD 4.0 and beyond, the fcntl fails because the slave
	 * side is not open even though the only effect of this call is
	 * to set the file description's FNONBLOCK flag (i.e. the pty and
	 * tty code do nothing additional).  Turns out that we could use
	 * ioctl instead of fcntl to set the flag.  The call will still
	 * fail, but the flag will be left set.  Rather than rely on that
	 * dubious behavior, I temporarily open the slave, do the fcntl
	 * and close the slave again.
	 */
#ifdef __FreeBSD__
	if ((n = open(Ttyname, O_RDONLY)) < 0)
		die("%s: open: %s", Ttyname, geterr(errno));
#endif
	if (fcntl(ptyfd, F_SETFL, O_NONBLOCK) < 0)
		die("%s: fcntl(O_NONBLOCK): %s", Ptyname, geterr(errno));
#ifdef __FreeBSD__
	close(n);
#endif
#endif /* USESOCKETS */

	FD_ZERO(&sfds);
	FD_SET(devfd, &sfds);
	fdcount = devfd;
#ifdef  USESOCKETS
	if (devfd < sockfd)
		fdcount = sockfd;
	FD_SET(sockfd, &sfds);
#else
	if (devfd < ptyfd)
		fdcount = ptyfd;
	FD_SET(ptyfd, &sfds);
#endif	/* USESOCKETS */

	fdcount++;

	for (;;) {
#ifdef LOG_DROPS
		if (drop_topty_chars >= DROP_THRESH) {
			warn("%d dev -> pty chars dropped", drop_topty_chars);
			drop_topty_chars = 0;
		}
		if (drop_todev_chars >= DROP_THRESH) {
			warn("%d pty -> dev chars dropped", drop_todev_chars);
			drop_todev_chars = 0;
		}
#endif
		fds = sfds;
#ifdef	USESOCKETS
		if (needshake)
			timeout.tv_sec  = 15;
		else
#endif
			timeout.tv_sec  = 30;
		i = select(fdcount, &fds, NULL, NULL, &timeout);
		if (i < 0) {
			if (errno == EINTR) {
				warn("input select interrupted, continuing");
				continue;
			}
			die("%s: select: %s", Devname, geterr(errno));
		}
		if (i == 0) {
#ifdef	USESOCKETS
			if (needshake) {
				handshake();
				continue;
			}
#else
			warn("No fds ready!");
			sleep(1);
#endif
			continue;
		}
#ifdef	USESOCKETS
		if (FD_ISSET(sockfd, &fds)) {
			clientconnect();
		}
#endif	/* USESOCKETS */
		if (FD_ISSET(devfd, &fds)) {
			errno = 0;
			cc = read(devfd, buf, sizeof(buf));
			if (cc < 0)
				die("%s: read: %s", Devname, geterr(errno));
			if (cc == 0)
				die("%s: read: EOF", Devname);
			errno = 0;

			omask = sigblock(sigmask(SIGHUP)|sigmask(SIGTERM)|
					 sigmask(SIGUSR1)|sigmask(SIGUSR2));
#ifdef	USESOCKETS
			if (!tipactive)
				goto dropped;
#endif
			for (lcc = 0; lcc < cc; lcc += i) {
				i = write(ptyfd, &buf[lcc], cc-lcc);
				if (i < 0) {
					/*
					 * Either tip is blocked (^S) or
					 * not running (the latter should
					 * return EIO but doesn't due to a
					 * pty bug).  Note that we have
					 * dropped some chars.
					 */
					if (errno == EIO || errno == EAGAIN) {
#ifdef LOG_DROPS
						drop_topty_chars += (cc-lcc);
#endif
						goto dropped;
					}
					die("%s: write: %s",
					    Ptyname, geterr(errno));
				}
				if (i == 0) {
#ifdef	USESOCKETS
					goto disconnected;
#else
					die("%s: write: zero-length", Ptyname);
#endif
				}
			}
dropped:
			i = write(logfd, buf, cc);
			if (i < 0)
				die("%s: write: %s", Logname, geterr(errno));
			if (i != cc)
				die("%s: write: incomplete", Logname);
			if (runfile) {
				i = write(runfd, buf, cc);
				if (i < 0)
					die("%s: write: %s",
					    Runname, geterr(errno));
				if (i != cc)
					die("%s: write: incomplete", Runname);
			}
			(void) sigsetmask(omask);

		}
		if (FD_ISSET(ptyfd, &fds)) {
			omask = sigblock(sigmask(SIGUSR2));
			errno = 0;
			cc = read(ptyfd, buf, sizeof(buf), 0);
			(void) sigsetmask(omask);
			if (cc < 0) {
				/* XXX commonly observed */
				if (errno == EIO || errno == EAGAIN)
					continue;
				die("%s: read: %s", Ptyname, geterr(errno));
			}
			if (cc == 0) {
#ifdef	USESOCKETS
			disconnected:
				/*
				 * Other end disconnected.
				 */
				dolog(LOG_INFO, "%s disconnecting",
				      inet_ntoa(tipclient.sin_addr));
				FD_CLR(ptyfd, &sfds);
				close(ptyfd);
				tipactive = 0;
				continue;
#else
				/* ~115 chars at 115.2 kbaud */
				timeout.tv_sec  = 0;
				timeout.tv_usec = 10000;
				select(0, 0, 0, 0, &timeout);
				continue;
#endif
			}
			errno = 0;

			omask = sigblock(sigmask(SIGHUP)|sigmask(SIGTERM)|
					 sigmask(SIGUSR1));
			for (lcc = 0; lcc < cc; lcc += i) {
				i = write(devfd, &buf[lcc], cc-lcc);
				if (i < 0) {
					/*
					 * Device backed up (or FUBARed)
					 * Note that we dropped some chars.
					 */
					if (errno == EAGAIN) {
#ifdef LOG_DROPS
						drop_todev_chars += (cc-lcc);
#endif
						goto dropped2;
					}
					die("%s: write: %s",
					    Devname, geterr(errno));
				}
				if (i == 0)
					die("%s: write: zero-length", Devname);
			}
dropped2:
			(void) sigsetmask(omask);

		}
	}
}
#endif

/*
 * SIGHUP means we want to close the old log file (because it has probably
 * been moved) and start a new version of it.
 */
void
reinit(int sig)
{
	/*
	 * We know that the any pending write to the log file completed
	 * because we blocked SIGHUP during the write.
	 */
	close(logfd);
	
	if ((logfd = open(Logname, O_WRONLY|O_CREAT|O_APPEND, 0640)) < 0)
		die("%s: open: %s", Logname, geterr(errno));
	if (chmod(Logname, 0640) < 0)
		die("%s: chmod: %s", Logname, geterr(errno));
	
	dolog(LOG_NOTICE, "new log started");

	if (runfile)
		newrun(sig);
}

/*
 * SIGUSR1 means we want to close the old run file and start a new version
 * of it. The run file is not rolled or saved, so we unlink it to make sure
 * that no one can hang onto an open fd.
 */
void
newrun(int sig)
{
	/*
	 * We know that the any pending write to the log file completed
	 * because we blocked SIGUSR1 during the write.
	 */
	close(runfd);
	unlink(Runname);

	if ((runfd = open(Runname, O_WRONLY|O_CREAT|O_APPEND, 0640)) < 0)
		die("%s: open: %s", Runname, geterr(errno));
	if (chmod(Runname, 0640) < 0)
		die("%s: chmod: %s", Runname, geterr(errno));
	
	dolog(LOG_NOTICE, "new run started");
}

/*
 * SIGUSR2 means we want to revoke the other side of the pty to close the
 * tip down gracefully.  We flush all input/output pending on the pty,
 * do a revoke on the tty and then close and reopen the pty just to make
 * sure everyone is gone.
 */
void
terminate(int sig)
{
#ifdef	USESOCKETS
	if (tipactive) {
		shutdown(ptyfd, SHUT_RDWR);
		close(ptyfd);
		FD_CLR(ptyfd, &sfds);
		ptyfd = 0;
		tipactive = 0;
		dolog(LOG_INFO, "%s revoked", inet_ntoa(tipclient.sin_addr));
	}
	else
		dolog(LOG_INFO, "revoked");

	/* Must be done *after* all the above stuff is done! */
	createkey();
#else
	int ofd = ptyfd;
	
	/*
	 * We know that the any pending access to the pty completed
	 * because we blocked SIGUSR2 during the operation.
	 */
#ifdef HPBSD
	int zero = 0;
	ioctl(ptyfd, TIOCFLUSH, &zero);
#else
	tcflush(ptyfd, TCIOFLUSH);
#endif
	if (revoke(Ttyname) < 0)
		dolog(LOG_WARNING, "could not revoke access to tty");
	close(ptyfd);
	
	if ((ptyfd = open(Ptyname, O_RDWR)) < 0)
		die("%s: open: %s", Ptyname, geterr(errno));

	/* XXX so we don't have to recompute the select mask */
	if (ptyfd != ofd) {
		dup2(ptyfd, ofd);
		close(ptyfd);
		ptyfd = ofd;
	}

#ifdef __FreeBSD__
	/* see explanation in capture() above */
	if ((ofd = open(Ttyname, O_RDONLY)) < 0)
		die("%s: open: %s", Ttyname, geterr(errno));
#endif
	if (fcntl(ptyfd, F_SETFL, O_NONBLOCK) < 0)
		die("%s: fcntl(O_NONBLOCK): %s", Ptyname, geterr(errno));
#ifdef __FreeBSD__
	close(ofd);
#endif
	
	dolog(LOG_NOTICE, "pty reset");
#endif	/* USESOCKETS */
}

/*
 *  Display proper usage / error message and exit.
 */
void
usage()
{
	fprintf(stderr, "usage: %s machine tty\n", Progname);
	exit(1);
}

warn(format, arg0, arg1, arg2, arg3)
	char *format, *arg0, *arg1, *arg2, *arg3;
{
	char msgbuf[BUFSIZE];

	snprintf(msgbuf, BUFSIZE, format, arg0, arg1, arg2, arg3);
	dolog(LOG_WARNING, msgbuf);
}

die(format, arg0, arg1, arg2, arg3)
	char *format, *arg0, *arg1, *arg2, *arg3;
{
	char msgbuf[BUFSIZE];

	snprintf(msgbuf, BUFSIZE, format, arg0, arg1, arg2, arg3);
	dolog(LOG_ERR, msgbuf);
	quit(0);
}

dolog(level, format, arg0, arg1, arg2, arg3)
	int level;
	char *format, *arg0, *arg1, *arg2, *arg3;
{
	char msgbuf[BUFSIZE];

	snprintf(msgbuf, BUFSIZE, format, arg0, arg1, arg2, arg3);
	if (debug) {
		fprintf(stderr, "%s - %s\n", Machine, msgbuf);
		fflush(stderr);
	}
	syslog(level, "%s - %s\n", Machine, msgbuf);
}

void
quit(int sig)
{
	cleanup();
	exit(1);
}

void
cleanup()
{
	dolog(LOG_NOTICE, "exiting");
#ifdef TWOPROCESS
	if (pid)
		(void) kill(pid, SIGTERM);
#endif
	(void) unlink(Pidname);
}

char *
newstr(str)
	char *str;
{
	char *malloc();
	register char *np;

	if ((np = malloc((unsigned) strlen(str) + 1)) == NULL)
		die("malloc: out of memory");

	return(strcpy(np, str));
}

#if 0
char *
geterr(errindx)
int errindx;
{
	extern int sys_nerr;
	extern char *sys_errlist[];
	char syserr[25];

	if (errindx < sys_nerr)
		return(sys_errlist[errindx]);

	(void) sprintf(syserr, "unknown error (%d)", errindx);
	return(syserr);
}
#endif

/*
 * Open up PID file and write our pid into it.
 */
writepid()
{
	int fd;
	char buf[8];
	
	if ((fd = open(Pidname, O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0)
		die("%s: open: %s", Pidname, geterr(errno));

	if (chmod(Pidname, 0644) < 0)
		die("%s: chmod: %s", Pidname, geterr(errno));
	
	(void) sprintf(buf, "%d\n", getpid());
	
	if (write(fd, buf, strlen(buf)) < 0)
		die("%s: write: %s", Pidname, geterr(errno));
	
	(void) close(fd);
}

/*
 * Put the console line into raw mode.
 */
rawmode(speed)
int speed;
{
#ifdef HPBSD
	struct sgttyb sgbuf;
	int bits;
	
	if (ioctl(devfd, TIOCGETP, (char *)&sgbuf) < 0)
		die("%s: ioctl(TIOCGETP): %s", Device, geterr(errno));
	sgbuf.sg_ispeed = sgbuf.sg_ospeed = speed;
	sgbuf.sg_flags = RAW|TANDEM;
	bits = LDECCTQ;
	if (ioctl(devfd, TIOCSETP, (char *)&sgbuf) < 0)
		die("%s: ioctl(TIOCSETP): %s", Device, geterr(errno));
	if (ioctl(devfd, TIOCLBIS, (char *)&bits) < 0)
		die("%s: ioctl(TIOCLBIS): %s", Device, geterr(errno));
#else
	struct termios t;

	if (tcgetattr(devfd, &t) < 0)
		die("%s: tcgetattr: %s", Devname, geterr(errno));
	(void) cfsetispeed(&t, speed);
	(void) cfsetospeed(&t, speed);
	cfmakeraw(&t);
	t.c_cflag |= CLOCAL;
	if (hwflow)
#ifdef __linux__
	        t.c_cflag |= CRTSCTS;
#else
		t.c_cflag |= CCTS_OFLOW | CRTS_IFLOW;
#endif
	t.c_cc[VSTART] = t.c_cc[VSTOP] = _POSIX_VDISABLE;
	if (tcsetattr(devfd, TCSAFLUSH, &t) < 0)
		die("%s: tcsetattr: %s", Devname, geterr(errno));
#endif
}

/*
 * From kgdbtunnel
 */
static struct speeds {
	int speed;		/* symbolic speed */
	int val;		/* numeric value */
} speeds[] = {
#ifdef B50
	{ B50,	50 },
#endif
#ifdef B75
	{ B75,	75 },
#endif
#ifdef B110
	{ B110,	110 },
#endif
#ifdef B134
	{ B134,	134 },
#endif
#ifdef B150
	{ B150,	150 },
#endif
#ifdef B200
	{ B200,	200 },
#endif
#ifdef B300
	{ B300,	300 },
#endif
#ifdef B600
	{ B600,	600 },
#endif
#ifdef B1200
	{ B1200, 1200 },
#endif
#ifdef B1800
	{ B1800, 1800 },
#endif
#ifdef B2400
	{ B2400, 2400 },
#endif
#ifdef B4800
	{ B4800, 4800 },
#endif
#ifdef B7200
	{ B7200, 7200 },
#endif
#ifdef B9600
	{ B9600, 9600 },
#endif
#ifdef B14400
	{ B14400, 14400 },
#endif
#ifdef B19200
	{ B19200, 19200 },
#endif
#ifdef B38400
	{ B38400, 38400 },
#endif
#ifdef B28800
	{ B28800, 28800 },
#endif
#ifdef B57600
	{ B57600, 57600 },
#endif
#ifdef B76800
	{ B76800, 76800 },
#endif
#ifdef B115200
	{ B115200, 115200 },
#endif
#ifdef B230400
	{ B230400, 230400 },
#endif
};
#define NSPEEDS (sizeof(speeds) / sizeof(speeds[0]))

int
val2speed(val)
	register int val;
{
	register int n;
	register struct speeds *sp;

	for (sp = speeds, n = NSPEEDS; n > 0; ++sp, --n)
		if (val == sp->val)
			return (sp->speed);

	return (0);
}

#ifdef USESOCKETS
int
clientconnect()
{
	int		cc, length = sizeof(tipclient);
	int		newfd;
	secretkey_t     key;

	newfd = accept(sockfd, (struct sockaddr *)&tipclient, &length);
	if (newfd < 0)
		die("accept(): accepting new client: %s", geterr(errno));

	/*
	 * Is there a better way to do this? I suppose we
	 * could shut the main socket down, and recreate
	 * it later when the client disconnects, but that
	 * sounds horribly brutish!
	 */
	if (tipactive) {
		close(newfd);
		dolog(LOG_NOTICE, "%s connecting, but tip is active",
		      inet_ntoa(tipclient.sin_addr));
		return 1;
	}
	ptyfd = newfd;

	/*
	 * Read the first part to verify the key. We must get the
	 * proper bits or this is not a valid tip connection.
	 */
	if ((cc = read(ptyfd, &key, sizeof(key))) <= 0) {
		close(ptyfd);
		dolog(LOG_NOTICE, "%s connecting, error reading key",
		      inet_ntoa(tipclient.sin_addr));
		return 1;
	}
	/* Verify size of the key is sane */
	if (cc != sizeof(key) ||
	    key.keylen != strlen(key.key) ||
	    strncmp(secretkey.key, key.key, key.keylen)) {
		close(ptyfd);
		dolog(LOG_NOTICE,
		      "%s connecting, secret key does not match",
		      inet_ntoa(tipclient.sin_addr));
		return 1;
	}
#ifndef SAFEMODE
	/* Do not spit this out into a public log file */
	dolog(LOG_INFO, "Key: %d: %s",
	      secretkey.keylen, secretkey.key);
#endif
	
	/*
	 * See Mike comments (use threads) above.
	 */
	if (fcntl(ptyfd, F_SETFL, O_NONBLOCK) < 0)
		die("fcntl(O_NONBLOCK): %s", geterr(errno));

	FD_SET(ptyfd, &sfds);
	if (ptyfd >= fdcount) {
		fdcount = ptyfd;
		fdcount++;
	}
	tipactive = 1;

	dolog(LOG_INFO, "%s connecting", inet_ntoa(tipclient.sin_addr));
	return 0;
}

/*
 * Generate our secret key and write out the file that local tip uses
 * to do a secure connect.
 */
int
createkey()
{
	int			cc, i, fd;
	unsigned char		buf[BUFSIZ], aclname[BUFSIZ];
	FILE		       *fp;

	/*
	 * Generate the key. Should probably generate a random
	 * number of random bits ...
	 */
	if ((fd = open("/dev/urandom", O_RDONLY)) < 0) {
		syslog(LOG_ERR, "opening /dev/urandom: %m");
		exit(1);
	}
	
	if ((cc = read(fd, buf, DEFAULTKEYLEN)) != DEFAULTKEYLEN) {
		if (cc < 0)
			syslog(LOG_ERR, "Reading random bits: %m");
		else
			syslog(LOG_ERR, "Reading random bits");
		exit(1);
	}
	close(fd);
	
	/*
	 * Convert into ascii text string since that is easier to
	 * deal with all around.
	 */
	secretkey.key[0] = 0;
	for (i = 0; i < DEFAULTKEYLEN; i++) {
		int len = strlen(secretkey.key);
		
		snprintf(&secretkey.key[len],
			 sizeof(secretkey.key) - len,
			 "%x", (unsigned int) buf[i]);
	}
	secretkey.keylen = strlen(secretkey.key);

#ifndef SAFEMODE
	/* Do not spit this out into a public log file */
	dolog(LOG_INFO, "NewKey: %d: %s", secretkey.keylen, secretkey.key);
#endif
	
	/*
	 * First write out the info locally so local tip can connect.
	 * This is still secure in that we rely on unix permission, which
	 * is how most of our security is based anyway.
	 */
	(void) sprintf(aclname, ACLNAME, TIPPATH, Machine);

	/*
	 * We want to control the mode bits when this file is created.
	 * Sure, could change the umask, but I hate that function.
	 */
	(void) unlink(aclname);
	if ((fd = open(aclname, O_WRONLY|O_CREAT|O_TRUNC, 0640)) < 0)
		die("%s: open: %s", aclname, geterr(errno));
	if ((fp = fdopen(fd, "w")) == NULL)
		die("fdopen(%s)", aclname, geterr(errno));

	fprintf(fp, "host:   %s\n", ourhostname);
	fprintf(fp, "port:   %d\n", portnum);
	fprintf(fp, "keylen: %d\n", secretkey.keylen);
	fprintf(fp, "key:    %s\n", secretkey.key);
	fclose(fp);

	/*
	 * Send the info over.
	 */
	handshake();
	return 1;
}

/*
 * Contact the boss node and tell it our local port number and the secret
 * key we are using.
 */
static	jmp_buf deadline;
static	int	deadbossflag;

void
deadboss()
{
	deadbossflag = 1;
	longjmp(deadline, 1);
}

int
handshake()
{
	int			sock, cc, err = 0;
	struct sockaddr_in	name;
	struct hostent	       *he;
	whoami_t		whoami;

	/*
	 * Global. If we fail, we keep trying from the main loop. This
	 * allows local tip to operate while still trying to contact the
	 * server periodically so remote tip can connect.
	 */
	needshake = 1;

	/*
	 * Don't do this if a local tip session is active. Typically, it
	 * means something is wrong, and that it would be a bad idea to
	 * interrupt a tip session without a potentially blocking operation,
	 * as that would really annoy the user. When the tip goes inactive,
	 * we will try again normally. 
	 */
	if (tipactive)
	    return 0;

	/* Our whoami info. */
	strcpy(whoami.nodeid, Machine);
	whoami.portnum = portnum;
	memcpy(&whoami.key, &secretkey, sizeof(secretkey));

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		die("socket(): %s", geterr(errno));
	}

	/* For alarm. */
	deadbossflag = 0;
	signal(SIGALRM, deadboss);
	if (setjmp(deadline)) {
		alarm(0);
		signal(SIGALRM, SIG_DFL);
		warn("Timed out connecting to %s\n", Bossnode);
		close(sock);
		return -1;
	}
	alarm(5);

	/* Need to map the hostname. */
	he = gethostbyname(Bossnode);
	if (! he) {
		die("gethostbyname(bossnode): %s", hstrerror(h_errno));
	}
	memcpy ((char *)&name.sin_addr, he->h_addr, he->h_length);
	name.sin_family = AF_INET;
	name.sin_port   = htons(SERVERPORT);

	if (connect(sock, (struct sockaddr *) &name, sizeof(name)) < 0) {
		warn("connect(bossnode): %s", geterr(errno));
		err = -1;
		goto done;
	}

	if ((cc = write(sock, &whoami, sizeof(whoami))) != sizeof(whoami)) {
		if (cc < 0)
			warn("write(bossnode): %s", geterr(errno));
		else
			warn("write(bossnode): Failed");
		err = -1;
		close(sock);
		goto done;
	}
	close(sock);
	needshake = 0;
 done:
	alarm(0);
	signal(SIGALRM, SIG_DFL);
	return err;
}
#endif
