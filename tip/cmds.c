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
#if 0
static char sccsid[] = "@(#)cmds.c	8.1 (Berkeley) 6/6/93";
#endif
static const char rcsid[] =
	"$Id: cmds.c,v 1.2 2000-12-27 00:49:33 mike Exp $";
#endif /* not lint */

#include "tip.h"
#include "pathnames.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <err.h>
#ifndef LINUX
#include <libutil.h>
#endif
#include <stdio.h>

/*
 * tip
 *
 * miscellaneous commands
 */

static char *argv[10];		/* argument vector for take and put */

void	timeout();		/* timeout function called on alarm */

void suspend __P((char));
void genbrk __P((void));
void variable __P((void));
void finish __P((void));
void tipabort __P((char *));
void chdirectory __P((void));
void shell __P((void));

static int anyof __P((char *, char *));
static void tandem __P((char *));
static int args __P((char *, char **, int));
static void execute __P((char *));

void
flush_remote ()
{
#ifdef TIOCFLUSH
	int cmd = 0;
	ioctl (FD, TIOCFLUSH, &cmd);
#else
#if HAVE_TERMIOS
	tcflush (FD, TCIOFLUSH);
#else
	struct sgttyb buf;
	ioctl (FD, TIOCGETP, &buf);	/* this does a */
	ioctl (FD, TIOCSETP, &buf);	/*   wflushtty */
#endif
#endif
}

void
timeout()
{
	signal(SIGALRM, timeout);
	timedout = 1;
}

/*
 * Escape to local shell
 */
void
shell()
{
	int shpid, status;
	char *cp;

	printf("[sh]\r\n");
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	unraw();
	if ((shpid = fork())) {
		while (shpid != wait(&status));
		raw();
		printf("\r\n!\r\n");
		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		return;
	} else {
		signal(SIGQUIT, SIG_DFL);
		signal(SIGINT, SIG_DFL);
		if ((cp = rindex(value(SHELL), '/')) == NULL)
			cp = value(SHELL);
		else
			cp++;
		shell_uid();
		execl(value(SHELL), cp, 0);
		printf("\r\ncan't execl!\r\n");
		exit(1);
	}
}

/*
 * Turn on/off scripting
 */
void
setscript()
{
	char *line = value(RECORD);

	if (boolean(value(SCRIPT))) {
		if ((fscript = fopen(line, "a")) == NULL) {
			printf("can't create %s\r\n", value(RECORD));
			boolean(value(SCRIPT)) = FALSE;
		}
	} else {
		if (fscript != NULL) {
			fclose(fscript);
			fscript = NULL;
		}
	}
}

/*
 * Change current working directory of
 *   local portion of tip
 */
void
chdirectory()
{
	char dirname[PATH_MAX];
	register char *cp = dirname;

	if (prompt("[cd] ", dirname, sizeof(dirname))) {
		if (stoprompt)
			return;
		cp = value(HOME);
	}
	if (chdir(cp) < 0)
		printf("%s: bad directory\r\n", cp);
	printf("!\r\n");
}

void
tipabort(msg)
	char *msg;
{

	if (msg != NOSTR)
		printf("\r\n%s", msg);
	printf("\r\n[EOT]\r\n");
	daemon_uid();
#if HAVE_UUCPLOCK
	(void)uu_unlock(uucplock);
#endif
	unraw();
	exit(0);
}

void
finish()
{
	tipabort(NOSTR);
}

static void
execute(s)
	char *s;
{
	register char *cp;

	if ((cp = rindex(value(SHELL), '/')) == NULL)
		cp = value(SHELL);
	else
		cp++;
	shell_uid();
	execl(value(SHELL), cp, "-c", s, 0);
}

static int
args(buf, a, num)
	char *buf, *a[];
	int num;
{
	register char *p = buf, *start;
	register char **parg = a;
	register int n = 0;

	while (*p && n < num) {
		while (*p && (*p == ' ' || *p == '\t'))
			p++;
		start = p;
		if (*p)
			*parg = p;
		while (*p && (*p != ' ' && *p != '\t'))
			p++;
		if (p != start)
			parg++, n++;
		if (*p)
			*p++ = '\0';
	}
	return(n);
}

void
variable()
{
	char	buf[256];

	if (prompt("[set] ", buf, sizeof(buf)))
		return;
	vlex(buf);
	if (vtable[BEAUTIFY].v_access&CHANGED)
		vtable[BEAUTIFY].v_access &= ~CHANGED;
	if (vtable[SCRIPT].v_access&CHANGED) {
		vtable[SCRIPT].v_access &= ~CHANGED;
		setscript();
		/*
		 * So that "set record=blah script" doesn't
		 *  cause two transactions to occur.
		 */
		if (vtable[RECORD].v_access&CHANGED)
			vtable[RECORD].v_access &= ~CHANGED;
	}
	if (vtable[RECORD].v_access&CHANGED) {
		vtable[RECORD].v_access &= ~CHANGED;
		if (boolean(value(SCRIPT)))
			setscript();
	}
	if (vtable[TAND].v_access&CHANGED) {
		vtable[TAND].v_access &= ~CHANGED;
		if (boolean(value(TAND)))
			tandem("on");
		else
			tandem("off");
	}
 	if (vtable[LECHO].v_access&CHANGED) {
 		vtable[LECHO].v_access &= ~CHANGED;
 		HD = boolean(value(LECHO));
 	}
	if (vtable[PARITY].v_access&CHANGED) {
		vtable[PARITY].v_access &= ~CHANGED;
		setparity(value(PARITY));
	}
}

/*
 * Turn tandem mode on or off for remote tty.
 */
static void
tandem(option)
	char *option;
{
#if HAVE_TERMIOS
	struct termios ttermios;
	tcgetattr (FD, &ttermios);
	if (strcmp(option,"on") == 0) {
		ttermios.c_iflag |= IXOFF;
		ctermios.c_iflag |= IXOFF;
	}
	else {
		ttermios.c_iflag &= ~IXOFF;
		ctermios.c_iflag &= ~IXOFF;
	}
	tcsetattr (FD, TCSANOW, &ttermios);
	tcsetattr (0, TCSANOW, &ctermios);
#else /* HAVE_TERMIOS */
	struct sgttyb rmtty;

	ioctl(FD, TIOCGETP, &rmtty);
	if (strcmp(option,"on") == 0) {
		rmtty.sg_flags |= TANDEM;
		arg.sg_flags |= TANDEM;
	} else {
		rmtty.sg_flags &= ~TANDEM;
		arg.sg_flags &= ~TANDEM;
	}
	ioctl(FD, TIOCSETP, &rmtty);
	ioctl(0,  TIOCSETP, &arg);
#endif /* HAVE_TERMIOS */
}

/*
 * Send a break.
 */
void
genbrk()
{

	ioctl(FD, TIOCSBRK, NULL);
	sleep(1);
	ioctl(FD, TIOCCBRK, NULL);
}

/*
 * Suspend tip
 */
void
suspend(c)
	char c;
{

	unraw();
	kill(c == CTRL('y') ? getpid() : 0, SIGTSTP);
	raw();
}

/*
 *	expand a file name if it includes shell meta characters
 */

char *
expand(name)
	char name[];
{
	static char xname[BUFSIZ];
	char cmdbuf[BUFSIZ];
	register int pid, l;
	register char *cp, *Shell;
	int s, pivec[2] /*, (*sigint)()*/;

	if (!anyof(name, "~{[*?$`'\"\\"))
		return(name);
	/* sigint = signal(SIGINT, SIG_IGN); */
	if (pipe(pivec) < 0) {
		warn("pipe");
		/* signal(SIGINT, sigint) */
		return(name);
	}
	snprintf(cmdbuf, sizeof(cmdbuf), "echo %s", name);
	if ((pid = vfork()) == 0) {
		Shell = value(SHELL);
		if (Shell == NOSTR)
			Shell = _PATH_BSHELL;
		close(pivec[0]);
		close(1);
		dup(pivec[1]);
		close(pivec[1]);
		close(2);
		shell_uid();
		execl(Shell, Shell, "-c", cmdbuf, 0);
		_exit(1);
	}
	if (pid == -1) {
		warn("fork");
		close(pivec[0]);
		close(pivec[1]);
		return(NOSTR);
	}
	close(pivec[1]);
	l = read(pivec[0], xname, BUFSIZ);
	close(pivec[0]);
	while (wait(&s) != pid);
		;
	s &= 0377;
	if (s != 0 && s != SIGPIPE) {
		fprintf(stderr, "\"Echo\" failed\n");
		return(NOSTR);
	}
	if (l < 0) {
		warn("read");
		return(NOSTR);
	}
	if (l == 0) {
		fprintf(stderr, "\"%s\": No match\n", name);
		return(NOSTR);
	}
	if (l == BUFSIZ) {
		fprintf(stderr, "Buffer overflow expanding \"%s\"\n", name);
		return(NOSTR);
	}
	xname[l] = 0;
	for (cp = &xname[l-1]; *cp == '\n' && cp > xname; cp--)
		;
	*++cp = '\0';
	return(xname);
}

/*
 * Are any of the characters in the two strings the same?
 */

static int
anyof(s1, s2)
	register char *s1, *s2;
{
	register int c;

	while ((c = *s1++))
		if (any(c, s2))
			return(1);
	return(0);
}
