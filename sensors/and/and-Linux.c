/*

    AND auto nice daemon - renice programs according to their CPU usage.
    Copyright (C) 1999-2003 Patrick Schemitz <schemitz@users.sourceforge.net>
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

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <syslog.h>
#include <sys/stat.h>
#include <signal.h>
#include <asm/param.h> /* kernel interrupt frequency: HZ */


#include "and.h"


/*

  AND -- auto-nice daemon/Linux version.
  
  Linux-specific AND version. Makes excessive use of the Linux
  /proc filesystem and is not portable.
  
  1999-2003 Patrick Schemitz, <schemitz@users.sourceforge.net>
  http://and.sourceforge.net/
  
*/


static DIR *linux_procdir = 0;


static struct and_procent linux_proc;


int linux_readproc (char *fn)
{
  /* Scan /proc/<pid>/stat file. Format described in proc(5).
     l1: pid comm state ppid 
     l2: pgrp session tty tpgid
     l3: flags minflt cminflt majflt cmajflt
     l4: utime stime cutime cstime
     l5: counter priority
  */
  FILE* f;
  int i;
  long li;
  long unsigned u, ujf, sjf;
  char state;
  char buffer [1024];
  if (!(f = fopen(fn,"rt"))) return 0;
  fscanf(f,"%d %1023s %c %d",&(linux_proc.pid),buffer,&state,&(linux_proc.ppid));
  fscanf(f,"%d %d %d %d",&i,&i,&i,&i);
  fscanf(f,"%lu %lu %lu %lu %lu",&u,&u,&u,&u,&u);
  fscanf(f,"%lu %lu %ld %ld",&ujf,&sjf,&li,&li);
  fscanf(f,"%ld %d",&li,&(linux_proc.nice));
  i = feof(f);
  fclose(f);
  if (i) return 0;
  if (state == 'Z') return 0; /* ignore zombies */
  ujf += sjf;  /* take into account both usr and sys time */
  ujf /= HZ;   /* convert jiffies to seconds */
  linux_proc.utime = ujf > INT_MAX ? INT_MAX: (int) ujf;
  buffer[strlen(buffer)-1] = 0;   /* remove () around command name */
  strncpy(linux_proc.command,&buffer[1],1023);
  linux_proc.command[1023] = 0;
  and_printf(3, "Linux: process %s pid: %d ppid: %d\n", 
             linux_proc.command, linux_proc.pid, linux_proc.ppid);
  return 1;
}


struct and_procent *linux_getnext ()
{
  char name [1024];
  struct dirent *entry;
  struct stat dirstat;
  if (!linux_procdir) return NULL;
  while ((entry = readdir(linux_procdir)) != NULL) { /* omit . .. apm bus... */
    if (isdigit(entry->d_name[0])) break;
  }
  if (!entry) return NULL;
  snprintf(name, 1024, "/proc/%s/stat",entry->d_name);
  /* stat() file to get uid/gid */
  if (stat(name,&dirstat)) return linux_getnext();
  /* read the job's stat "file" to get command, nice level, etc */
  if (!linux_readproc(name)) return linux_getnext();
  linux_proc.uid = dirstat.st_uid;
  linux_proc.gid = dirstat.st_gid;
  return &linux_proc;
}


struct and_procent *linux_getfirst ()
{
  if (linux_procdir) {
    rewinddir(linux_procdir);
  } else {
    linux_procdir = opendir("/proc");
    if (!linux_procdir) {
      and_printf(0,"cannot open /proc, aborting.\n");
      abort();
    }
  }
  return linux_getnext();
}


int main (int argc, char** argv)
{
  and_setprocreader(&linux_getfirst,&linux_getnext);
  return and_main(argc,argv);
}
