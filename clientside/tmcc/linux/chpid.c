/*
 * This is the chpid app written by the VINI folks.
 * I modified it to act as an init process when invoked with no arguments.
 */

/* gcc -Wall -O2 -g chpid.c -o chpid */
#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED
#define _SVID_SOURCE
#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/mount.h>
#include <sys/vfs.h>
#include <fcntl.h>
#include <unistd.h>
#include <sched.h>
#include <stdarg.h>
#include <dirent.h>

#ifndef MNT_FORCE
#define MNT_FORCE       0x00000001      /* Attempt to forcibily umount */
#endif /* MNT_FORCE */
#ifndef MNT_DETACH
#define MNT_DETACH      0x00000002      /* Just detach from the tree */
#endif /* MNT_DETACH */
#ifndef MNT_EXPIRE
#define MNT_EXPIRE      0x00000004      /* Mark for expiry */
#endif /* MNT_EXPIRE */

#ifndef MS_MOVE
#define MS_MOVE 8192
#endif
#ifndef MS_REC
#define MS_REC 16384
#endif

#ifdef OLD
# ifndef CLONE_NPSPACE
# define CLONE_NPSPACE  0x04000000      /* New process space */
# endif
# ifndef CLONE_NSYSVIPC
# define CLONE_NSYSVIPC 0x08000000      /* New sysvipc space */
# endif
# ifndef CLONE_NHOST
# define CLONE_NHOST    0x10000000      /* New networking host context (lo, hostname, etc) */
# endif
# ifndef CLONE_NDEV
# define CLONE_NDEV     0x20000000      /* New hash of devices I can touch */
# endif
# ifndef CLONE_NUSERS
# define CLONE_NUSERS   0x40000000      /* New users */
# endif
# ifndef CLONE_NTIME
# define CLONE_NTIME    0x80000000      /* New time context??? */
# endif
# ifndef CLONE_NS_NOSUID
# define CLONE_NS_NOSUID        0x00001000      /* Force MNT_NOSUID on all mounts in the namespace */
# endif
#endif

#ifndef CLONE_NEWUTS
#define CLONE_NEWUTS    0x04000000      /* New uts namespace (uname) */
#endif
#ifndef CLONE_NEWIPC
#define CLONE_NEWIPC    0x08000000      /* New sysvipc namespace */
#endif
#ifndef CLONE_NEWUID
#define CLONE_NEWUID    0x10000000      /* New users */
#endif
#ifndef CLONE_NEWNET
#define CLONE_NEWNET    0x40000000      /* New network namespace (lo, device, names sockets, etc) */
#endif
#ifndef CLONE_NEWPID
#define CLONE_NEWPID    0x80000000      /* New process space */
#endif
#ifndef CLONE_NEWTIME
#define CLONE_NEWTIME   0x40000000      /* New time context??? */
#endif
#ifndef CLONE_NEWDEV
#define CLONE_NEWDEV    0x00001000      /* New has of devices I can touch */
#endif


#ifndef PROC_SUPER_MAGIC
#define PROC_SUPER_MAGIC 0x9fa0
#endif /* PROC_SUPER_MAGIC */

struct user_desc;
static pid_t raw_clone(int flags, void *child_stack,
       int *parent_tidptr, struct user_desc *newtls, int *child_tidptr)
{
       return syscall(__NR_clone, flags, child_stack, parent_tidptr, newtls, child_tidptr);
}

static int raw_pivot_root(const char *new_root, const char *old_root)
{
       return syscall(__NR_pivot_root, new_root, old_root);
}

static void (*my_exit)(int status) = exit;

static void die(char *fmt, ...)
{
       va_list ap;
       va_start(ap, fmt);
       vfprintf(stderr, fmt, ap);
       va_end(ap);
       fflush(stderr);
       fflush(stdout);
       my_exit(1);
}

static void *xmalloc(size_t size)
{
       void *ptr;
       ptr = malloc(size);
       if (!ptr) die("malloc of %d bytes failed: %s\n", size, strerror(errno));
       return ptr;
}

static void umount_buffer(char *line)
{
       char *next, *mnt, *end, *type;
       int result;
       if (!line || !*line)
               return;
       next = strchr(line, '\n');
       *next++ = '0';
       type = line;
       mnt = strchr(line, ' ');
       *mnt++ = '\0';
       end = strchr(mnt, ' ');
       *end = '\0';
       umount_buffer(next);

#if 0
       /* For debugging list what I am unounting */
       write(STDOUT_FILENO, mnt, strlen(mnt));
       write(STDOUT_FILENO, "\n", 1);
#endif

       if (strcmp(type, "rootfs"))
               return;
       result = umount2(mnt, MNT_DETACH);
       if (result < 0)
               die("umount of '%s' failed: %d: %s\n",
                       mnt, errno, strerror(errno));
}

static void umount_all(void)
{
       static const char mounts[] = "/proc/self/mounts";
       char *buf;
       ssize_t bufsz, bytes;
       int fd, result;
       buf = NULL;
       bufsz = 4096;

       /* Temporarily mount /proc at / so I know I have it */
       result = mount("proc", "/proc", "proc", 0, NULL);
       if (result < 0)
               die("mount of proc failed: %d: %s\n",
                       errno, strerror(errno));
#if 0
       DIR *dir;
       struct dirent *entry;
       dir = opendir("/");
       while((entry = readdir(dir)) != NULL) {
               write(STDOUT_FILENO, entry->d_name, strlen(entry->d_name));
               write(STDOUT_FILENO, "\n", 1);
       }
       closedir(dir);
#endif
       fd = open(mounts, O_RDONLY);
       if (fd < 0)
               die("open of %s failed: %d: %s\n",
                       mounts, errno, strerror(errno));

       /* Read in all of the mount points */
       do {
               buf = xmalloc(bufsz);
               result = lseek(fd, 0, SEEK_SET);
               if (result != 0)
                       die("lseek failed: %d:%s\n",
                               errno, strerror(errno));
               bytes = read(fd, buf, bufsz);
               if (bytes < 0)
                       die("Read of %s failed: %d:%s\n",
                               mounts, errno, strerror(errno));
               if (bytes != bufsz) {
                       /* Terminate the buffer */
                       buf[bytes] = '\0';
               } else {
                       /* Free the buffer and try again */
                       free(buf);
                       buf = NULL;
                       bufsz <<= 1;
               }
       } while(!buf);
       /* Be good and close the file descriptor */
       result = close(fd);
       if (result < 0)
               fprintf(stderr, "close of %s failed: %d:%s\n",
                       mounts, errno, strerror(errno));

#if 0
       /* For debugging print the list of mounts */
       write(STDOUT_FILENO, buf, bufsz);
       write(STDOUT_FILENO, "\n\n", 2);
#endif

       umount_buffer(buf);
}

int main(int argc, char **argv, char **envp)
{
       pid_t pid;
       int status;
       struct rlimit rlim;
       int clone_flags;
       char **cmd_argv, *shell_argv[2];
       char *root = "/", *old = "/mnt";
       char *label = "none";
       int i;
       int tty, tty_force;
       int isinit = 0;
       char *init_argv[3];

       tty = 0;
       tty_force = 0;
       clone_flags = SIGCHLD;

       if (argc == 1 && strcmp(argv[0], "/sbin/init") == 0) {
	       isinit = 1;
       }

       for (i = 1; (i < argc) && (argv[i][0] == '-'); i++) {
               if (strcmp(argv[i], "--") == 0) {
                       break;
               }
               else if (((argc - i) >= 2) && (strcmp(argv[i], "-r") == 0)) {
                       clone_flags |= CLONE_NEWNS;
                       root = argv[i + 1];
                       i++;
               }
               else if (((argc - i) >= 2) && (strcmp(argv[i], "-o") == 0)) {
                       old = argv[i + 1];
                       i++;
               }
               else if (((argc - i) >= 2) && (strcmp(argv[i], "-l") == 0)) {
                       clone_flags |= CLONE_NEWNS;
                       label = argv[i + 1];
                       i++;
               }
               else if (strcmp(argv[i], "-m") == 0) {
                       clone_flags |= CLONE_NEWNS;
               }
               else if (strcmp(argv[i], "-h") == 0) {
                       clone_flags |= CLONE_NEWUTS;
               }
               else if (strcmp(argv[i], "-i") == 0) {
                       clone_flags |= CLONE_NEWIPC;
               }
               else if (strcmp(argv[i], "-n") == 0) {
                       clone_flags |= CLONE_NEWNET;
               }
               else if (strcmp(argv[i], "-p") == 0) {
                       clone_flags |= CLONE_NEWPID;
               }
               else if (strcmp(argv[i], "-u") == 0) {
                       clone_flags |= CLONE_NEWUID;
               }
               else if (strcmp(argv[i], "-d") == 0) {
                       clone_flags |= CLONE_NEWDEV;
               }
               else if (strcmp(argv[i], "-t") == 0) {
                       clone_flags |= CLONE_NEWTIME;
               }
               else if (strcmp(argv[i], "--tty") == 0) {
                       tty = 1;
               }
               else if (strcmp(argv[i], "--tty-force") == 0) {
                       tty = 1; tty_force = 1;
               }
               else {
                       die("Bad argument %s\n", argv[i]);
               }
       }
       cmd_argv = argv + i;
       if (cmd_argv[0] == NULL) {
	       if (isinit) {
		       /* chpid -i -h -n /bin/sh /etc/rc */
		       cmd_argv = init_argv;
		       init_argv[0] = "/bin/sh";
		       init_argv[1] = "/etc/rc";
		       init_argv[2] = NULL;
		       clone_flags |= (CLONE_NEWUTS|CLONE_NETIPC|CLONE_NEWNET);
	       } else {
		       cmd_argv = shell_argv;
		       shell_argv[0] = getenv("SHELL");
		       shell_argv[1] = NULL;
	       }
       }
       if (cmd_argv[0] == NULL) {
               die("No command specified\n");
       }
#if 1
       fprintf(stderr, "cmd_argv: %s\n", cmd_argv[0]);
#endif
       if (root[0] != '/') {
               die("root path: '%s' not absolute\n", root);
       }
       if (old[0] != '/') {
               die("old path: '%s' not absolute\n", old);
       }

#if 0
       status = getrlimit(RLIMIT_NPROC, &rlim);
       if (status < 0) {
               fprintf(stderr, "getrlimit RLIMIT_NPROC failed: %s\n",
                       strerror(errno));
               exit(1);
       }
       fprintf(stderr, "RLIMIT_NPROC: %llu %llu\n",
               (unsigned long long)rlim.rlim_cur, (unsigned long long)rlim.rlim_max);
#endif
#if 0
       rlim.rlim_cur = 256;
       status = setrlimit(RLIMIT_NPROC, &rlim);
       if (status < 0) {
               fprintf(stderr, "setrlimit RLIMIT_NPROC %lu %lu failed: %s\n",
                       (unsigned long)rlim.rlim_cur, (unsigned long)rlim.rlim_max,
                       strerror(errno));
               exit(2);
       }
#endif
       printf("Clone flags: %lx\n", clone_flags);
       pid = raw_clone(clone_flags, NULL, NULL, NULL, NULL);
       if (pid < 0) {
               fprintf(stderr, "clone_failed: pid: %d %d:%s\n",
                       pid, errno, strerror(errno));
               exit(2);
       }
       if (pid == 0) {
               /* In the child */
               int result;
               my_exit = _exit;

               /* FIXME allocate a process inside for controlling the new process space */
               if (clone_flags & CLONE_NEWPID) {
                       fprintf(stderr, "pid: %d, ppid: %d pgrp: %d sid: %d\n",
                               getpid(), getppid(), getpgid(0), getsid(0));
                       /* If CLONE_NEWPID isn't implemented exit */
                       if (getpid() != 1)
                               die("CLONE_NEWPID not implemented\n");
               }

               if (clone_flags & CLONE_NEWNS) {
                       struct statfs stfs;
#if 0
                       if (strcmp(root, "/") != 0) {
                               /* FIXME find a way to remove unreachable filesystems
                                * from the namespace.
                                */
                               result = chdir(root);
                               if (result < 0)
                                       die("chdir to '%s' failed: %d:%s\n",
                                               root, errno, strerror(errno));
                               /* Convert the current working directory into a mount point */
                               result = mount(".", ".", NULL, MS_BIND | MS_REC, NULL);
                               if (result < 0)
                                       die("bind of '%s' failed: %d: %s\n",
                                               root, errno, strerror(errno));
                               /* Update the current working directory */
                               result = chdir(root);
                               if (result < 0)
                                       die("chdir to '%s' failed: %d:%s\n",
                                               ".", errno, strerror(errno));
                               result = mount(".", "/", NULL, MS_MOVE, NULL);
                               if (result < 0)
                                       die("mount of '%s' failed: %d:%s\n",
                                               root, errno, strerror(errno));
                               result = chroot(".");
                               if (result < 0)
                                       die("chroot to '%s' failed: %d:%s\n",
                                               root, errno, strerror(errno));
                       }
#endif
#if 1
                       if (strcmp(root, "/") != 0) {
                               char put_old[PATH_MAX];
                               result = snprintf(put_old, sizeof(put_old), "%s%s", root, old);
                               if (result >= sizeof(put_old))
                                       die("path name to long\n");
                               if (result < 0)
                                       die("snprintf failed: %d:%s\n",
                                               errno, strerror(errno));

                               /* Ensure I have a mount point at the directory I want to export */
                               result = mount(root, root, NULL, MS_BIND | MS_REC, NULL);
                               if (result < 0)
                                       die("bind of '%s' failed: %d:%s\n",
                                               root, errno, strerror(errno));

                               /* Switch the mount points */
                               result = raw_pivot_root(root, put_old);
                               if (result < 0)
                                       die("pivot_root('%s', '%s') failed: %d:%s\n",
                                               root, put_old, errno, strerror(errno));

                               /* Unmount all of the old mounts */
                               result = umount2(old, MNT_DETACH);
                               if (result < 0)
                                       die("umount2 of '%s' failed: %d:%s\n",
                                               put_old, errno, strerror(errno));
                       }
#endif

                       result = statfs("/proc", &stfs);
                       if ((result == 0) && (stfs.f_type == PROC_SUPER_MAGIC))  {
		       		printf ("Umounting proc\n");
                               /* Unmount and remount proc so it reflects the new pid space */
                               result = umount2("/proc", MNT_DETACH);
                               if (result < 0)
                                       die("umount failed: %d:%s\n", errno, strerror(errno));

                               result = mount(label, "/proc", "proc", 0, NULL);
                               if (result < 0)
                                       die("mount failed: %d:%s\n",
                                               errno, strerror(errno));
                       }
               }
               if (tty) {
                       pid_t sid, pgrp;
                       sid = setsid();
                       if (sid < 0)
                               die("setsid failed: %d:%s\n",
                                       errno, strerror(errno));
                       fprintf(stderr, "pid: %d, ppid: %d pgrp: %d sid: %d\n",
                               getpid(), getppid(), getpgid(0), getsid(0));

                       result = ioctl(STDIN_FILENO, TIOCSCTTY, tty_force);
                       if (result < 0)
                               die("tiocsctty failed: %d:%s\n",
                                       errno, strerror(errno));

                       pgrp = tcgetpgrp(STDIN_FILENO);

                       fprintf(stderr, "pgrp: %d\n", pgrp);

                       fprintf(stderr, "pid: %d, ppid: %d pgrp: %d sid: %d\n",
                               getpid(), getppid(), getpgid(0), getsid(0));

               }
               result = execve(cmd_argv[0], cmd_argv, envp);
               die("execve of %s failed: %d:%s\n",
                       cmd_argv[0], errno, strerror(errno));
       }
       /* In the parent */
       fprintf(stderr, "child pid: %d\n", pid);
       pid = waitpid(pid, &status, 0);
       fprintf(stderr, "pid: %d exited status: %d\n",
               pid, status);
       if (pid < 0) {
               fprintf(stderr, "waitpid failed: %d %s\n",
                       errno, strerror(errno));
               exit(9);
       }
       if (pid == 0) {
               fprintf(stderr, "waitpid returned no pid!\n");
               exit(10);
       }
       if (WIFEXITED(status)) {
               fprintf(stderr, "pid: %d exited: %d\n",
                       pid, WEXITSTATUS(status));
       }
       if (WIFSIGNALED(status)) {
               fprintf(stderr, "pid: %d exited with a uncaught signal: %d %s\n",
                       pid, WTERMSIG(status), strsignal(WTERMSIG(status)));
       }
       if (WIFSTOPPED(status)) {
               fprintf(stderr, "pid: %d stopped with signal: %d\n",
                       pid, WSTOPSIG(status));
       }
       return 0;
}
