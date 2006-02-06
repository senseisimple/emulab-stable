/*

    AND auto nice daemon - renice programs according to their CPU usage.
    Copyright (C) 1999-2004, 2006 Patrick Schemitz <schemitz@users.sourceforge.net>
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <kvm.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/proc.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>

#ifdef __FreeBSD__
#include <sys/user.h>
#endif


#include "and.h"


/*

  AND -- auto-nice daemon/OpenBSD version.
  
  OpenBSD-specific AND version. Makes excessive use of the OpenBSD
  kernel memory interface, kvm, but also works for FreeBSD.
  
  2000, 2002 Patrick Schemitz, <schemitz@users.sourceforge.net>
  http://and.sourceforge.net/
  
*/


static kvm_t *openbsd_kvm = NULL;
static struct kinfo_proc *openbsd_pt = NULL;
static int openbsd_nproc = 0;
static int openbsd_next = 0;
static long openbsd_hz = -1;


static struct and_procent openbsd_proc;


static int openbsd_init ()
{
  struct nlist nlst [] = {
    { "_hz" },
    { 0 }
  };
  kvm_nlist(openbsd_kvm,nlst);
  if (nlst[0].n_type == 0) {
    and_printf(0,"KVM: nlist failed. Aborting.\n");
    abort();
  }
  if (kvm_read(openbsd_kvm,nlst[0].n_value,(char*)(&openbsd_hz),
	       sizeof(openbsd_hz)) != sizeof(openbsd_hz)) {
    and_printf(0,"KVM: hz symbol empty. Aborting.\n");
    abort();
  }
  return 1;
}


struct and_procent *openbsd_getnext ()
{
  if (!openbsd_pt) {
    and_printf(0,"KVM: no process table (late detection). Aborting.\n");
    abort();
  }
  if (openbsd_next >= openbsd_nproc) return NULL;

#if !defined(__FreeBSD_version) || __FreeBSD_version < 500000
  strncpy(openbsd_proc.command,openbsd_pt[openbsd_next].kp_proc.p_comm,1023);
  openbsd_proc.command[1023] = 0;
  openbsd_proc.pid = openbsd_pt[openbsd_next].kp_proc.p_pid;
  // 
  openbsd_proc.uid = openbsd_pt[openbsd_next].kp_eproc.e_pcred.p_ruid;
  openbsd_proc.gid = openbsd_pt[openbsd_next].kp_eproc.e_pcred.p_rgid;
#  if defined(__FreeBSD__)
  openbsd_proc.ppid = openbsd_pt[openbsd_next].kp_eproc.e_ppid;
  openbsd_proc.nice = openbsd_pt[openbsd_next].kp_proc.p_nice;
  openbsd_proc.stime =
    openbsd_pt[openbsd_next].kp_eproc.e_stats.p_start.tv_sec;
  openbsd_proc.utime =
    openbsd_pt[openbsd_next].kp_proc.p_runtime / (1000 * 1000);
  openbsd_proc.ctime =
    openbsd_pt[openbsd_next].kp_eproc.e_stats.p_cru.ru_utime.tv_sec +
    openbsd_pt[openbsd_next].kp_eproc.e_stats.p_cru.ru_stime.tv_sec;
#  else
  /* Adapted from top(1) port, as found in the misc@openbsd.org archive */
  openbsd_proc.ppid = openbsd_pt[openbsd_next].kp_proc.p_ppid; /* FIXME that correct? */
  openbsd_proc.nice = openbsd_pt[openbsd_next].kp_proc.p_nice-20;
  openbsd_proc.utime = (openbsd_pt[openbsd_next].kp_proc.p_uticks +
			openbsd_pt[openbsd_next].kp_proc.p_sticks +
			openbsd_pt[openbsd_next].kp_proc.p_iticks)
    / openbsd_hz;
#  endif
#else
  strncpy(openbsd_proc.command,openbsd_pt[openbsd_next].ki_comm,1023);
  openbsd_proc.command[1023] = 0;
  openbsd_proc.pid = openbsd_pt[openbsd_next].ki_pid;
  openbsd_proc.nice = openbsd_pt[openbsd_next].ki_nice;
  openbsd_proc.uid = openbsd_pt[openbsd_next].ki_ruid;
  openbsd_proc.gid = openbsd_pt[openbsd_next].ki_rgid;
  openbsd_proc.stime = openbsd_pt[openbsd_next].ki_start.tv_sec;
  openbsd_proc.utime =
    openbsd_pt[openbsd_next].ki_runtime / (1000 * 1000);
  openbsd_proc.ctime =
    openbsd_pt[openbsd_next].ki_childtime.tv_sec;
#endif
  and_printf(3, "OpenBSD: process %s pid: %d ppid: %d cpu_secs: %d nice: %d\n",
             openbsd_proc.command, openbsd_proc.pid, openbsd_proc.ppid,
	     openbsd_proc.utime, openbsd_proc.nice);
  openbsd_next++;
  return &openbsd_proc;
}


struct and_procent *openbsd_getfirst ()
{
  char errmsg [_POSIX2_LINE_MAX];
  if (!openbsd_kvm) {
    openbsd_kvm = kvm_openfiles(NULL,NULL,NULL,O_RDONLY,errmsg);
    if (!openbsd_kvm) {
      and_printf(0,"KVM: cannot open (\"%s\"). Aborting.\n",errmsg);
      abort();
    }
    openbsd_init();
  }
  openbsd_pt = kvm_getprocs(openbsd_kvm,KERN_PROC_ALL,0,&openbsd_nproc);
  if (!openbsd_pt) {
    and_printf(0,"KVM: cannot retrieve process table. Aborting.\n");
    abort();
  }
  openbsd_next = 0;
  return openbsd_getnext();
}


int main (int argc, char** argv)
{
  and_setprocreader(&openbsd_getfirst,&openbsd_getnext);
  return and_main(argc,argv);
}
