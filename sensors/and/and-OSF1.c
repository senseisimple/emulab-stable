/*

    AND auto nice daemon - renice programs according to their CPU usage.
    Copyright (C) 1999-2001 Patrick Schemitz <schemitz@users.sourceforge.net>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <sys/procfs.h>
#include <sys/times.h>
#include <sys/file.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <signal.h>
#include <math.h>
#include <sys/stat.h>

#include "and.h"


/*

  AND -- auto-nice daemon/Digital UNIX(OSF/1) version.

  OSF/1-specific AND version. Makes reasonable use of the OSF/1
  /proc filesystem and seems to be more portable than I thought.

  Also works for Solaris (SunOS 5) and IRIX/IRIX64. (Thanks to
  Dan Stromberg for telling me.)

  2000, 2001 Patrick Schemitz, <schemitz@users.sourceforge.net>
  http://and.sourceforge.net/
  
*/


static DIR *digitalunix_procdir = 0;


static struct and_procent digitalunix_proc;


int digitalunix_readproc (char *fn)
{
  prpsinfo_t ps;
  double cputime;
  int res, fd;
  if ((fd = open(fn, O_RDONLY)) < 0) return 0;
  res = ioctl(fd, PIOCPSINFO, &ps);
  close(fd);
  if (res < 0) return 0;
  digitalunix_proc.uid = ps.pr_uid;
  digitalunix_proc.gid = ps.pr_gid;
  digitalunix_proc.ppid = ps.pr_ppid;
  strncpy(digitalunix_proc.command,ps.pr_fname,1022);
  digitalunix_proc.command[1023] = 0;
  cputime = (double)ps.pr_time.tv_sec + (double)ps.pr_time.tv_nsec / 1.0E9;
  digitalunix_proc.utime = (int)cputime;
  digitalunix_proc.pid = ps.pr_pid;
  digitalunix_proc.nice = getpriority(PRIO_PROCESS, ps.pr_pid);
  /*
    printf("%5i  %-20s  %5i  %3i\n", digitalunix_proc.pid,
    digitalunix_proc.command, digitalunix_proc.utime,
    digitalunix_proc.nice);
  */
  and_printf(3, "OSF/1: process: %s pid: %d ppid: %d\n", 
             digitalunix_proc.command, 
             digitalunix_proc.pid, 
             digitalunix_proc.ppid);
  return 1;
}


struct and_procent *digitalunix_getnext ()
{
  char name [1024];
  struct dirent* entry;
  struct stat dirstat;
  if (!digitalunix_procdir) return NULL;
  while ((entry = readdir(digitalunix_procdir)) != NULL) {
    if (!isdigit(entry->d_name[0])) continue;
    sprintf(name,"/proc/%s",entry->d_name);
    if (access(name, R_OK) < 0) {
      and_printf(0,"read access denied: /proc/%s\n",entry->d_name);
      continue;
    }
    break;
  }
  if (!entry) return NULL;
  if (!digitalunix_readproc(name)) return NULL;
  return &digitalunix_proc;
}


struct and_procent *digitalunix_getfirst ()
{
  if (digitalunix_procdir) {
    rewinddir(digitalunix_procdir);
  } else {
    digitalunix_procdir = opendir("/proc");
    if (!digitalunix_procdir) {
      and_printf(0,"/proc: cannot open /proc. Aborting.\n");
      abort();
    }
  }
  return digitalunix_getnext();
}


int main (int argc, char** argv)
{
  and_setprocreader(&digitalunix_getfirst,&digitalunix_getnext);
  return and_main(argc,argv);
}
