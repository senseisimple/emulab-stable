/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static const char copyright[] =
"@(#) Copyright (c) 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)tip.c	8.1 (Berkeley) 6/6/93";
#endif
static const char rcsid[] =
	"$Id: tip.c,v 1.2 2000-12-27 00:49:35 mike Exp $";
#endif /* not lint */

/*
	Forward declarations
*/
void ttysetup (int speed);

/*
 * tip - UNIX link to other systems
 *  tip [-v] [-speed] system-name
 * or
 *  cu phone-number [-s speed] [-l line] [-a acu]
 */

#include "tip.h"
#include "pathnames.h"

#include <err.h>
#include <errno.h>
#include <sys/types.h>
#ifndef LINUX
#include <libutil.h>
#endif

/*
 * Baud rate mapping table
 */
#ifndef LINUX
#if !HAVE_TERMIOS
CONST int bauds[] = {
	0, 50, 75, 110, 134, 150, 200, 300, 600,
	1200, 1800, 2400, 4800, 9600, 19200, 38400, 57600, 115200, -1
};
#endif
#else
struct baudmap {
	CONST int BR;
	CONST int bval;
} baudmap[] = {
	{ 0,		B0 },
	{ 50,		B50 },
	{ 75,		B75 },
	{ 110,		B110 },
	{ 134,		B134 },
	{ 150,		B150 },
	{ 200,		B200 },
	{ 300,		B300 },
	{ 600,		B600 },
	{ 1200,		B1200 },
	{ 1800,		B1800 },
	{ 2400,		B2400 },
	{ 4800,		B4800 },
	{ 9600,		B9600 },
	{ 19200,	B19200 },
	{ 38400,	B38400 },
	{ 57600,	B57600 },
	{ 115200,	B115200 },
	{ -1,		0 }
};
#endif

#if !HAVE_TERMIOS
int	disc = OTTYDISC;		/* tip normally runs this way */
#endif

void	intprompt();
void	timeout();
void	tipdone();
char	*sname();
char	PNbuf[256];			/* This limits the size of a number */

int	rflag = 1;

static void usage __P((void));
void setparity __P((char *));
void xpwrite __P((int, char *, int));
void tipio __P((void));
void tipinfunc __P((char *, int));
void tipoutfunc __P((char *, int));
int prompt __P((char *, char *, size_t));
void unraw __P((void));
void shell_uid __P((void));
void daemon_uid __P((void));
void user_uid __P((void));
int speed __P((int));

int
main(argc, argv)
	char *argv[];
{
	char *system = NOSTR;
	register int i;
	register char *p;
	char sbuf[12];

	gid = getgid();
	egid = getegid();
	uid = getuid();
	euid = geteuid();

	if (argc > 4)
		usage();
	if (!isatty(0))
		errx(1, "must be interactive");

	STDIN = 0;

	for (; argc > 1; argv++, argc--) {
		if (argv[1][0] != '-')
			system = argv[1];
		else switch (argv[1][1]) {

		case 'v':
			vflag++;
			break;

		case 'c':
			rflag = 0;
			break;
		case 'r':
			rflag = 1;
			break;

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			BR = atoi(&argv[1][1]);
			break;

		default:
			warnx("%s, unknown option", argv[1]);
			break;
		}
	}

	(void)signal(SIGINT, tipdone);
	(void)signal(SIGQUIT, tipdone);
	(void)signal(SIGHUP, tipdone);
	(void)signal(SIGTERM, tipdone);
	(void)signal(SIGUSR1, tipdone);

	/* reset the tty on screw ups */
	(void)signal(SIGBUS, tipdone);
	(void)signal(SIGSEGV, tipdone);

	if ((i = hunt(system)) == 0) {
		printf("all ports busy\n");
		exit(3);
	}
	if (i == -1) {
		printf("%s: busy\n", system);
#if HAVE_UUCPLOCK
		(void)uu_unlock(uucplock);
#endif
		exit(3);
	}
	setbuf(stdout, NULL);
	loginit();

	/*
	 * Kludge, their's no easy way to get the initialization
	 *   in the right order, so force it here
	 */
	if ((PH = getenv("PHONES")) == NOSTR)
		PH = _PATH_PHONES;
	vinit();				/* init variables */
	setparity("even");			/* set the parity table */
	if ((i = speed(number(value(BAUDRATE)))) == 0) {
		printf("tip: bad baud rate %d\n", number(value(BAUDRATE)));
#if HAVE_UUCPLOCK
		(void)uu_unlock(uucplock);
#endif
		exit(3);
	}

	/*
	 * Now that we have the logfile and the ACU open
	 *  return to the real uid and gid.  These things will
	 *  be closed on exit.  Swap real and effective uid's
	 *  so we can get the original permissions back
	 *  for removing the uucp lock.
	 */
	user_uid();

	/*
	 * Hardwired connections require the
	 *  line speed set before they make any transmissions
	 *  (this is particularly true of things like a DF03-AC)
	 */
	if (HW)
		ttysetup(i);

	/*
	 * From here down the code is shared with
	 * the "cu" version of tip.
	 */

#if HAVE_TERMIOS
	tcgetattr (0, &otermios);
	ctermios = otermios;
	if (rflag) {
		ctermios.c_iflag = (IGNBRK|IGNPAR);
		ctermios.c_lflag = 0;
		ctermios.c_cflag = (CLOCAL|CREAD|CS8);
		ctermios.c_cc[VMIN] = 1;
		ctermios.c_cc[VTIME] = 5;
		ctermios.c_oflag = 0;
	} else {
#ifndef _POSIX_SOURCE
		ctermios.c_iflag = (IMAXBEL|IXANY|ISTRIP|IXON|BRKINT);
		ctermios.c_lflag = (PENDIN|IEXTEN|ISIG|ECHOCTL|ECHOE|ECHOKE);
#else
		ctermios.c_iflag = (ISTRIP|IXON|BRKINT);
		ctermios.c_lflag = (PENDIN|IEXTEN|ISIG|ECHOE);
#endif
		ctermios.c_cflag = (CLOCAL|HUPCL|CREAD|CS8);
		ctermios.c_cc[VINTR] = 	ctermios.c_cc[VQUIT] = _POSIX_VDISABLE;
		ctermios.c_cc[VSUSP] = ctermios.c_cc[VDISCARD] =
			ctermios.c_cc[VLNEXT] = _POSIX_VDISABLE;
#ifdef VDSUSP
		ctermios.c_cc[VDSUSP] = _POSIX_VDISABLE;
#endif
	}
#else /* HAVE_TERMIOS */
	ioctl(0, TIOCGETP, (char *)&defarg);
	ioctl(0, TIOCGETC, (char *)&defchars);
	ioctl(0, TIOCGLTC, (char *)&deflchars);
	ioctl(0, TIOCGETD, (char *)&odisc);
	arg = defarg;
	if (rflag)
		arg.sg_flags = RAW;
	else
		arg.sg_flags = CBREAK;
	arg.sg_flags |= PASS8 | ANYP;
	tchars = defchars;
	tchars.t_intrc = tchars.t_quitc = -1;
	ltchars = deflchars;
	ltchars.t_suspc = ltchars.t_dsuspc = ltchars.t_flushc
		= ltchars.t_lnextc = -1;
#endif /* HAVE_TERMIOS */
	raw();

	(void)signal(SIGALRM, timeout);

	/*
	 * Everything's set up now:
	 *	connection established (hardwired or dialup)
	 *	line conditioned (baud rate, mode, etc.)
	 *	internal data structures (variables)
	 * so, fire up!
	 */
	if (rflag)
		printf("\07connected (raw mode)\r\n");
	else
		printf("\07connected\r\n");

	tipio();
	/*NOTREACHED*/
}

static void
usage()
{
	fprintf(stderr, "usage: tip [-v] [-speed] [system-name]\n");
	exit(1);
}

void
tipdone(sig)
	int sig;
{
	switch (sig) {
	case SIGHUP:
		tipabort("Hangup.");
		break;
	case SIGTERM:
		tipabort("Killed.");
		break;
	default:
		printf("\r\nSignal %d", sig);
		tipabort(NOSTR);
	}
}

/*
 * Muck with user ID's.  We are setuid to the owner of the lock
 * directory when we start.  user_uid() reverses real and effective
 * ID's after startup, to run with the user's permissions.
 * daemon_uid() switches back to the privileged uid for unlocking.
 * Finally, to avoid running a shell with the wrong real uid,
 * shell_uid() sets real and effective uid's to the user's real ID.
 */
static int uidswapped;

void
user_uid()
{
	if (uidswapped == 0) {
		seteuid(uid);
		uidswapped = 1;
	}
}

void
daemon_uid()
{
	if (uidswapped) {
		seteuid(euid);
		uidswapped = 0;
	}
}

void
shell_uid()
{
	setegid(gid);
	seteuid(uid);
}

static int inrawmode;

/*
 * put the controlling keyboard into raw mode
 */
void
raw()
{
	if (inrawmode)
		return;

#if HAVE_TERMIOS
	tcsetattr (0, TCSANOW, &ctermios);
#else /* HAVE_TERMIOS */
	ioctl(0, TIOCSETP, &arg);
	ioctl(0, TIOCSETC, &tchars);
	ioctl(0, TIOCSLTC, &ltchars);
	ioctl(0, TIOCSETD, (char *)&disc);
#endif /* HAVE_TERMIOS */

	inrawmode = 1;
}


/*
 * return keyboard to normal mode
 */
void
unraw()
{
	if (!inrawmode)
		return;

#if HAVE_TERMIOS
	tcsetattr (0, TCSANOW, &otermios);
#else /* HAVE_TERMIOS */
	ioctl(0, TIOCSETD, (char *)&odisc);
	ioctl(0, TIOCSETP, (char *)&defarg);
	ioctl(0, TIOCSETC, (char *)&defchars);
	ioctl(0, TIOCSLTC, (char *)&deflchars);
#endif /* HAVE_TERMIOS */

	inrawmode = 0;
}

static	jmp_buf promptbuf;

/*
 * Print string ``s'', then read a string
 *  in from the terminal.  Handles signals & allows use of
 *  normal erase and kill characters.
 */
int
prompt(s, p, sz)
	char *s;
	register char *p;
	size_t sz;
{
	register char *b = p;
	sig_t oint, oquit;

	stoprompt = 0;
	oint = signal(SIGINT, intprompt);
	oquit = signal(SIGQUIT, SIG_IGN);
	unraw();
	printf("%s", s);
	if (setjmp(promptbuf) == 0)
		while ((*p = tipgetchar()) != '\n' && --sz > 0)
			p++;
	*p = '\0';

	raw();
	(void)signal(SIGINT, oint);
	(void)signal(SIGQUIT, oquit);
	return (stoprompt || p == b);
}

/*
 * Interrupt service routine during prompting
 */
void
intprompt()
{

	(void)signal(SIGINT, SIG_IGN);
	stoprompt = 1;
	printf("\r\n");
	longjmp(promptbuf, 1);
}

/*
 * ****TIPIO   TIPIO****
 *
 * Replace two process reader/writer with select-based single process
 * reader and writer.
 */
void
tipio()
{
	fd_set sfds, fds;
	int n, i, cc;
	char buf[4096];

	/*
	 * Check for scripting being turned on from the .tiprc file.
	 */
	if (boolean(value(SCRIPT)))
		setscript();

	n = STDIN;
	if (n < FD)
		n = FD;
	n++;
	FD_ZERO(&sfds);
	FD_SET(STDIN, &sfds);
	FD_SET(FD, &sfds);
	for (;;) {
		fds = sfds;
		i = select(n, &fds, NULL, NULL, NULL);
		if (i <= 0)
			tipabort("select failed");

		/*
		 * Check for user input first.
		 * It is lower volume and possibly more important (^C)
		 */
		if (FD_ISSET(STDIN, &fds)) {
			cc = read(STDIN, buf, sizeof(buf));
			if (cc < 0)
				tipabort("stdin read failed");
			if (cc == 0)
				finish();
			tipinfunc(buf, cc);
		}

		if (FD_ISSET(FD, &fds)) {
			cc = read(FD, buf, sizeof(buf));
			if (cc < 0)
				tipabort("device read failed");
			if (cc == 0)
				tipabort("device read EOF");

			tipoutfunc(buf, cc);
		}
	}
}

void
tipinfunc(buf, nchar)
	char *buf;
	int nchar;
{
	int i;
	char gch;
	char escc = character(value(ESCAPE));
	char forcec = character(value(FORCE));
	static int bol = 1;
	static int inescape = 0;
	static int inforce = 0;

	for (i = 0; i < nchar; i++) {
		gch = buf[i] & 0177;
		if (inescape || (gch == escc && bol)) {
			esctable_t *p;

			/*
			 * Read the next char if not already in an escape.
			 * If there is no next char note that we are in an
			 * escape and exit.
			 */
			if (!inescape && ++i == nchar) {
				inescape = 1;
				break;
			}
			gch = buf[i] & 0177;
			inescape = 0;

			for (p = etable; p->e_char; p++)
				if (p->e_char == gch)
					break;

			/*
			 * If this is a legit escape command, process it.
			 * Otherwise, for an unrecognized sequence (which
			 * includes ESCAPE ESCAPE), we just send the escaped
			 * char with no further interpretation.
			 */
			if (p->e_char) {
				if ((p->e_flags&PRIV) && uid)
					continue;
				printf("%s", ctrl(escc));
				(*p->e_func)(gch);
				continue;
			}
		} else if (inforce || gch == forcec) {
			/*
			 * Same story, different character...
			 * Read the next char if not already in a force.
			 * If there is no next char note that we are forcing
			 * and exit.
			 */
			if (!inforce && ++i == nchar) {
				inforce = 1;
				break;
			}
			gch = buf[i] & 0177;
			inforce = 0;
		} else if (gch == character(value(RAISECHAR))) {
			boolean(value(RAISE)) = !boolean(value(RAISE));
			continue;
		} else if (gch == '\r') {
			bol = 1;
			xpwrite(FD, &gch, 1);
			if (boolean(value(HALFDUPLEX)))
				printf("\r\n");
			continue;
		}
		bol = any(gch, value(EOL));
		if (boolean(value(RAISE)) && islower(gch))
			gch = toupper(gch);
		xpwrite(FD, &gch, 1);
		if (boolean(value(HALFDUPLEX)))
			printf("%c", gch);
	}
}

void
tipoutfunc(buf, nchar)
	char *buf;
	int nchar;
{
	char *cp;

	for (cp = buf; cp < buf + nchar; cp++)
		*cp &= 0177;
	write(1, buf, nchar);

	if (boolean(value(SCRIPT)) && fscript != NULL) {
		if (boolean(value(BEAUTIFY))) {
			char *excepts = value(EXCEPTIONS);

			for (cp = buf; cp < buf + nchar; cp++)
				if ((*cp >= ' ' && *cp <= '~') ||
				    any(*cp, excepts))
					putc(*cp, fscript);
		} else
			fwrite(buf, 1, nchar, fscript);
	}
}

int
speed(n)
	int n;
{
#ifndef LINUX
#if HAVE_TERMIOS
	return (n);
#else
	register CONST int *p;

	for (p = bauds; *p != -1;  p++)
		if (*p == n)
			return (p - bauds);
	return (0);
#endif
#else
	struct baudmap *p;

	for (p = baudmap; p->BR != -1;  p++)
		if (p->BR == n)
			return (p->bval);
	return (0);
#endif
}

int
any(c, p)
	register char c, *p;
{
	while (p && *p)
		if (*p++ == c)
			return (1);
	return (0);
}

int
size(s)
	register char	*s;
{
	register int i = 0;

	while (s && *s++)
		i++;
	return (i);
}

char *
interp(s)
	register char *s;
{
	static char buf[256];
	register char *p = buf, c, *q;

	while ((c = *s++)) {
		for (q = "\nn\rr\tt\ff\033E\bb"; *q; q++)
			if (*q++ == c) {
				*p++ = '\\'; *p++ = *q;
				goto next;
			}
		if (c < 040) {
			*p++ = '^'; *p++ = c + 'A'-1;
		} else if (c == 0177) {
			*p++ = '^'; *p++ = '?';
		} else
			*p++ = c;
	next:
		;
	}
	*p = '\0';
	return (buf);
}

char *
ctrl(c)
	char c;
{
	static char s[3];

	if (c < 040 || c == 0177) {
		s[0] = '^';
		s[1] = c == 0177 ? '?' : c+'A'-1;
		s[2] = '\0';
	} else {
		s[0] = c;
		s[1] = '\0';
	}
	return (s);
}

/*
 * Help command
 */
void
help(c)
	char c;
{
	register esctable_t *p;

	printf("%c\r\n", c);
	for (p = etable; p->e_char; p++) {
		if ((p->e_flags&PRIV) && uid)
			continue;
		printf("%2s", ctrl(character(value(ESCAPE))));
		printf("%-2s %c   %s\r\n", ctrl(p->e_char),
			p->e_flags&EXP ? '*': ' ', p->e_help);
	}
}

/*
 * Set up the "remote" tty's state
 */
void
ttysetup (int speed)
{
#if HAVE_TERMIOS
	struct termios termios;
	tcgetattr (FD, &termios);
	termios.c_iflag = (IGNBRK|IGNPAR);
	if (boolean(value(TAND)))
		termios.c_iflag |= IXOFF;
	termios.c_lflag = 0;
	termios.c_cflag = (CLOCAL|HUPCL|CREAD|CS8);
	termios.c_cc[VMIN] = 1;
	termios.c_cc[VTIME] = 5;
	termios.c_oflag = 0;
	cfsetispeed(&termios, speed);
	cfsetospeed(&termios, speed);
	tcsetattr (FD, TCSANOW, &termios);
#else /* HAVE_TERMIOS */
	unsigned bits = LDECCTQ;

	arg.sg_ispeed = arg.sg_ospeed = speed;
	arg.sg_flags = RAW;
	if (boolean(value(TAND)))
		arg.sg_flags |= TANDEM;
	ioctl(FD, TIOCSETP, (char *)&arg);
	ioctl(FD, TIOCLBIS, (char *)&bits);
#endif /* HAVE_TERMIOS */
}

/*
 * Return "simple" name from a file name,
 * strip leading directories.
 */
char *
sname(s)
	register char *s;
{
	register char *p = s;

	while (*s)
		if (*s++ == '/')
			p = s;
	return (p);
}

static char partab[0200];
static int bits8;

/*
 * Do a write to the remote machine with the correct parity.
 * We are doing 8 bit wide output, so we just generate a character
 * with the right parity and output it.
 */
void
xpwrite(fd, buf, n)
	int fd;
	char *buf;
	register int n;
{
	register int i;
	register char *bp;
	extern int errno;

	bp = buf;
	if (bits8 == 0)
		for (i = 0; i < n; i++) {
			*bp = partab[(*bp) & 0177];
			bp++;
		}
	if (write(fd, buf, n) < 0) {
		if (errno == EIO)
			tipabort("Lost carrier.");
		if (errno == ENODEV)
			tipabort("tty not available.");
		tipabort("Something wrong...");
	}
}

/*
 * Build a parity table with appropriate high-order bit.
 */
void
setparity(defparity)
	char *defparity;
{
	register int i, flip, clr, set;
	char *parity;
	extern char evenpartab[];

	if (value(PARITY) == NOSTR)
		value(PARITY) = defparity;
	parity = value(PARITY);
	if (equal(parity, "none")) {
		bits8 = 1;
		return;
	}
	bits8 = 0;
	flip = 0;
	clr = 0377;
	set = 0;
	if (equal(parity, "odd"))
		flip = 0200;			/* reverse bit 7 */
	else if (equal(parity, "zero"))
		clr = 0177;			/* turn off bit 7 */
	else if (equal(parity, "one"))
		set = 0200;			/* turn on bit 7 */
	else if (!equal(parity, "even")) {
		(void) fprintf(stderr, "%s: unknown parity value\r\n", parity);
		(void) fflush(stderr);
	}
	for (i = 0; i < 0200; i++)
		partab[i] = (evenpartab[i] ^ flip) | (set & clr);
}


/*
 * getchar with EOF check
 */
tipgetchar()
{
	char gch;

	if (read(STDIN, &gch, 1) != 1)
		finish();

	return gch;
}
