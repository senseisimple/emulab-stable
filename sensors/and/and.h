/*

    AND auto nice daemon - renice programs according to their CPU usage.
    Copyright (C) 1999-2004 Patrick Schemitz <schemitz@users.sourceforge.net>
    http://and.sourceforge.net/

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#ifndef AND_H
#define AND_H


/************************************************************************
 *                                                                      *
 * and.h -- AND library for platform-independent code.                  *
 *                                                                      *
 * 1999, 2004 Patrick Schemitz <schemitz@users.sourceforge.net>         *
 * http://and.sourceforge.net/                                          *
 *                                                                      *
 ***********************************************************************/


/*
 * and_procent -- process entry.
 *
 * AND-relevant information on a process.
 */
struct and_procent {
  /* to be filled by and-$OS.c: */
  int pid;
  int ppid;
  int uid;
  int gid;
  int nice;
  time_t stime;			// process start time in seconds.
  unsigned utime;		// CPU time in seconds.
  unsigned ctime;		// reaped child CPU time in seconds.
  char command [1024];
  /* to be filled by and.c: */
  struct and_procent *parent;
  struct and_procent *next;
};


/*
 * and_printf() - log message.
 *
 * Logs a message (in printf() format), either to stderr, or to syslog(),
 * depending on AND operational mode. In test mode (and -t), stderr is
 * used; syslog() otherwise. Use this to report any O/S-specific problems.
 * Log is suppressed if the current verbosity level is below the required
 * one.
 */
void and_printf (int required_verbosity, char *fmt, ...);


/*
 * and_setprocreader() -- set O/S specific handler for reading processes.
 *
 * getfirst and getnext are two functions returning a pointer to an
 * and_procent, or NULL if no more processes are available. The implementation
 * of these two functions are O/S specific. For Linux, reading through the
 * /proc filesystem is most suitable. See and-linux.c for a sample
 * implementation.
 *
 * Note: it is getfirst's task to also clean up any remainders of former
 * calls to getfirst and getnext, such as open DIR*.
 */
void and_setprocreader (struct and_procent *(*getfirst)(),
			struct and_procent *(*getnext)());


/*
 * and_main() -- start the AND.
 *
 * Takes over control. Call this after setting the and_procreader().
 */
int and_main (int argc, char** argv);


#endif
