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

/************************************************************************
 *                                                                      *
 * and.c -- AND library for platform-independent code.                  *
 *                                                                      *
 * Automatically renice jobs when they use too much CPU time.           *
 *                                                                      *
 * 1999-2004 Patrick Schemitz <schemitz@users.sourceforge.net>          *
 * http://and.sourceforge.net/                                          *
 *                                                                      *
 ***********************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include <grp.h>
#include <syslog.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <signal.h>
#include <regex.h>
#include <values.h>

#define DEBUG 0

/* OpenBSD getopt() is in unistd.h; Linux and Digital UNIX have getopt.h */
#if !defined(__OpenBSD__) && !defined(__FreeBSD__) && !defined(__svr4__) && !defined(__SunOS__)
#include <getopt.h>
#endif

/* GNU C library forgets to declare this one: */
#ifdef __GNUC__
int vsnprintf (char *str, size_t n, const char *format, va_list ap);
#define HAVE_VSNPRINTF
#endif

#include "and.h"



#ifndef DEFAULT_NICE
#define DEFAULT_NICE 0
#endif

#ifndef LOG_PERROR
#define LOG_PERROR 0
#endif

#ifndef DEFAULT_INTERVAL
#define DEFAULT_INTERVAL 60
#endif

#ifndef DEFAULT_CONFIG_FILE
#define DEFAULT_CONFIG_FILE "/etc/and.conf"
#endif

#ifndef DEFAULT_DATABASE_FILE
#define DEFAULT_DATABASE_FILE "/etc/and.priorities"
#endif

#ifndef AND_VERSION
#define AND_VERSION "1.0.7 or above (not compiled in)"
#endif

#ifndef AND_DATE
#define AND_DATE "27 Jan 2002 or later (not compiled in)"
#endif


#define bool  char
#define false (0)
#define true  (!false)


/* Maximum entries in priority database. You may change this,
   if you have a really large priority database. However, you
   shouldn't make too large databases since the get_priority()
   function takes too long otherwise. */

#define PRI_MAXENTRIES 100


/* Indices for the weight array when resolving a user/group/command/parent
   tuple to get the correct priority database entry. These are
   constants. Never ever change them! Change the affinity in the
   config file instead. */

enum {
  PRI_U, PRI_G, PRI_C, PRI_P, PRI_N
};


/* Priority database entry record. Consists of the user ID, the
   group ID, the command (string and compiled regexp), the parent
   (string and compiled regexp), and three nice levels. */

#define PARENT_ONLY 0
#define PARENT_AND_ANCHESTORS 1

#define PARENT_ONLY_KEYWORD "parent="
#define PARENT_AND_ANCHESTORS_KEYWORD "ancestor="

struct priority_db_entry {
  int uid;
  int gid;
  char command_str [128];
  regex_t *command;
  char parent_str [128];
  regex_t *parent;
  int parentmode;
  int nl [3];
};


/* Global variables for priority database (db), configuration (conf),
   and other arguments (args). */

struct{
  int n;
  struct priority_db_entry entry [PRI_MAXENTRIES];
} and_db;



/* AND configuration data. Some of this is compiled in; for some things
   there are command-line options, and some is read from and.conf */

struct {
  char hostname [512];
  int test;
  char *program;
  char *config_file;
  char *database_file;
  int verbose;
  int to_stdout;
  int nice_default;
  bool lock_interval;
  unsigned interval;
  unsigned time_mark [3];
  char affinity [5];
  int weight [PRI_N];
  int min_uid;
  int min_gid;
} and_config;



/* Initialise configuration parameters. Some are later over-
   ridden by command-line arguments and and.conf. */

void set_defaults (int argc, char **argv)
{
  and_config.test = 0;
  and_config.verbose = 0;
  and_config.to_stdout = 0;
  and_config.program = argv[0];
  and_config.lock_interval = false;
  and_config.interval = DEFAULT_INTERVAL;
  and_config.config_file = DEFAULT_CONFIG_FILE;
  and_config.database_file = DEFAULT_DATABASE_FILE;
  and_config.nice_default = DEFAULT_NICE;
  and_config.min_uid = 0;
  and_config.min_gid = 0;
  gethostname(and_config.hostname,511);
  and_config.hostname[511] = 0;
}



/* Log AND messages (to ./debug.and if in test mode, syslog() otherwise).
   If available, uses non-ANSI function vsnprintf() to avoid possible
   buffer overflow. */

void and_printf (int required_verbosity, char *fmt, ...)
{
  va_list args;
  static int syslog_open = 0;
  static FILE *out = NULL;
  char buffer [2048];
  time_t t;
  va_start(args,fmt);
  if (and_config.verbose >= required_verbosity)
  {
    if (and_config.test) {
      /* in test mode, create log file; use stderr on failure */
      if (!and_config.to_stdout && !out) {
        out = fopen("./debug.and","wt");
        if (!out) {
          out = stderr;
        }
      }
      /* write time stamp to ./debug.and */
      t = time(NULL);
      strncpy(buffer,ctime(&t),2047);
      buffer[strlen(buffer)-1] = ' ';
      if (and_config.to_stdout)
        fputs(buffer,stdout);
      else
        fputs(buffer,out);
    }
    /* build actual log message */
#ifdef HAVE_VSNPRINTF
    vsnprintf(buffer,2048,fmt,args);
#else
    vsprintf(buffer,fmt,args); /* ... and hope for the best :( */
    buffer[2047] = 0;
#endif
    if (and_config.to_stdout) {
      fputs(buffer,stdout);
      fflush(stdout);
    } else {
      if (and_config.test) {
        /* log to ./debug.and in test mode */
        fputs(buffer,out);
        fflush(out);
      } else {
        /* write to syslog if in full operations */
        if (!syslog_open) {
          openlog(and_config.program,LOG_PERROR|LOG_PID,LOG_DAEMON);
          syslog_open = 1;
        }
        syslog(LOG_WARNING,"%s",buffer);
      }
    }
  }
  va_end(args);
}


/* Print priority database. */

void print_priorities ()
{
  int i;
  and_printf(0,"Priority database:\n");
  and_printf(0,"UID:   GID:   Command               Parent:                             NLs:\n");
  for (i=0; i<and_db.n; i++) {
    and_printf(0,"%5i  %5i  %-20s  %-20s %-13s  %2i,%2i,%2i\n",
	       and_db.entry[i].uid, and_db.entry[i].gid,
	       and_db.entry[i].command_str, 
               and_db.entry[i].parent_str,
               (and_db.entry[i].parentmode == PARENT_ONLY ? "(parent)" : "(ancestors)"),
               and_db.entry[i].nl[0],
	       and_db.entry[i].nl[1], and_db.entry[i].nl[2]);
  }
  and_printf(0,"%i entries.\n\n",and_db.n);
}


/* Print configuration parameters. */

void print_config ()
{
  and_printf(0,"Configuration parameters:\n"
	     "host name:            %s\n"
	     "operational mode:     %s\n"
	     "verbosity:             %2i\n"
	     "default nicelevel:     %2i\n"
	     "interval     [sec]:   %3u\n"
	     "level 0 from [sec]:   %3u\n"
	     "level 1 from [sec]:   %3u\n"
	     "level 2 from [sec]:   %3u\n"
             "minimum uid:          %i\n"
             "minimum gid:          %i\n"
	     "affinity:             %s\n"
	     "          U: %i\n"
	     "          G: %i\n"
	     "          C: %i\n"
	     "          P: %i\n\n",
	     and_config.hostname, 
	     (and_config.test?"just checkin'":"I'm serious."),
	     and_config.verbose,
	     and_config.nice_default, and_config.interval,
	     and_config.time_mark[0], and_config.time_mark[1],
	     and_config.time_mark[2], 
             and_config.min_uid, and_config.min_gid,
             and_config.affinity,
	     and_config.weight[PRI_U], and_config.weight[PRI_G],
	     and_config.weight[PRI_C], and_config.weight[PRI_P]);
}


/* Parse one line of the priority database */

void read_priorities ()
{
  FILE *priority;
  int bad_count, line_count;
  char buffer [1024];
  /* entry field */
  int uid;
  char uid_s [1024];
  int gid;
  char gid_s [1024];
  char command [1024];
  int parentmode;
  char parent [1024];
  char parent_s [1024];
  int nl0, nl1, nl2;
  /* auxillary structs */
  struct passwd *lookup_p;
  struct group *lookup_g;
  int error;
  char error_msg [1024];
  int i, entry;
  int linelen;
  regex_t *rex;
  bool section_matches = true;

  if ((priority = fopen(and_config.database_file,"rt")) == 0) {
    and_printf(0,"Priority database %s not found. Aborting.\n",
	       and_config.database_file);
    abort();
  }
  and_printf(0,"Priority database is: %s\n", and_config.database_file);
  /* Read file line by line */
  line_count = bad_count = 0;
  and_db.n = 0;
  while (!feof(priority)) {
    memset(buffer,0,1024);
    if (fgets(buffer,1022,priority) == 0) break;
    line_count++;
    /* Intercept empty lines, comments, and overflows */
    linelen = strlen(buffer);
    if (linelen == 0) {
      continue;
    }
    if (buffer[0] == 10 || buffer[0] == 13 || buffer[0] == '#') {
      continue;
    }
    if (linelen > 1022) {
      and_printf(0,"Priority database line %i too long: %s\n",
		 line_count, buffer);
      bad_count++;
      continue;
    }
    /* Handle host-specific parts */
    if ((buffer[0] == 'o') && (buffer[1] == 'n') && (buffer[2] == ' ')) {
      while (buffer[strlen(buffer)-1] < 32) {
	buffer[strlen(buffer)-1] = 0;
      }
      rex = (regex_t*)malloc(sizeof(regex_t));
      regcomp(rex,&buffer[3],REG_NOSUB|REG_EXTENDED);
      section_matches = (regexec(rex,and_config.hostname,0,0,0) == 0);
      and_printf(0,"Priority database line %i: section for host(s) %s will be %s.\n",
		 line_count, &buffer[3], (section_matches?"read":"skipped"));
      regfree(rex);
      continue;
    }
    if (!section_matches) {
      continue;
    }
    i = sscanf(buffer, "%s %s %s %s %i %i %i",
	       uid_s, gid_s, command, parent_s, &nl0, &nl1, &nl2);
    if (i != 7) {
      and_printf(0,"Priority database line %i is invalid: %s\n",
		 line_count, buffer);
      bad_count++;
      continue;
    }
    /* Identify UID */
    if (strcmp(uid_s,"*") == 0) {
      uid = -1;
    } else if ((lookup_p = getpwnam(uid_s)) != 0) {
      uid = lookup_p->pw_uid;
    } else if ((i = atoi(uid_s)) > 0) {
      uid = i;
    } else {
      and_printf(0,"Priority database line %i with invalid UID: %s\n",
		 line_count, uid_s);
      bad_count++;
      continue;
    }
    /* Identify GID */
    if (strcmp(gid_s,"*") == 0) {
      gid = -1;
    } else if ((lookup_g = getgrnam(gid_s)) != 0) {
      gid = lookup_g->gr_gid;
    } else if ((i = atoi(gid_s)) > 0) {
      gid = i;
    } else {
      and_printf(0,"Priority database line %i with invalid GID: %s\n",
		 line_count, gid_s);
      bad_count++;
      continue;
    }
    /* figure parent mode */
    if (strcmp(parent_s,"*") == 0) {
      strcpy(parent,"*");
      parentmode = PARENT_ONLY;
    } else if (strncmp(parent_s,PARENT_ONLY_KEYWORD,
                strlen(PARENT_ONLY_KEYWORD)) == 0) {
      strcpy(parent,&parent_s[strlen(PARENT_ONLY_KEYWORD)]);
      parentmode = PARENT_ONLY;
    } else if (strncmp(parent_s,PARENT_AND_ANCHESTORS_KEYWORD,
                       strlen(PARENT_AND_ANCHESTORS_KEYWORD)) == 0) {
      strcpy(parent,&parent_s[strlen(PARENT_AND_ANCHESTORS_KEYWORD)]);
      parentmode = PARENT_AND_ANCHESTORS;
    } else {
      and_printf(0,"Priority database line %i with bad parent keyword: %s\n",
		 line_count, parent_s);
      bad_count++;
      continue;
    }
    /* Find entry in database */
    entry = and_db.n;
    for (i=0; i<and_db.n; ++i) {
      if (and_db.entry[i].uid == uid && and_db.entry[i].gid == gid
	  && strcmp(and_db.entry[i].command_str,command) == 0
          && strcmp(and_db.entry[i].parent_str,parent) == 0) {
	entry = i;
	break;
      }
    }
    /* If not recycling an entry (i.e. overwriting the priorities),
       rebuild the regular expression */
    if (!and_db.entry[entry].command) {
      and_db.entry[entry].uid = uid;
      and_db.entry[entry].gid = gid;
      and_db.entry[entry].command = (regex_t*)malloc(sizeof(regex_t));
      error = regcomp(and_db.entry[entry].command,command,REG_NOSUB);
      if (error) {
	regerror(error,and_db.entry[entry].command,error_msg,1023);
	regfree(and_db.entry[entry].command);
	free(and_db.entry[entry].command);
	and_db.entry[entry].command = NULL;
	and_printf(0,"Priority database line %i with bad command regexp %s (%s)\n",
		   line_count, command, error_msg);
	bad_count++;
	continue;
      }
      strncpy(and_db.entry[entry].command_str,command,127);
      and_db.entry[entry].command_str[127] = 0;
      and_db.entry[entry].parent = (regex_t*)malloc(sizeof(regex_t));
      error = regcomp(and_db.entry[entry].parent,parent,REG_NOSUB);
      if (error) {
	regerror(error,and_db.entry[entry].parent,error_msg,1023);
	regfree(and_db.entry[entry].parent);
	free(and_db.entry[entry].parent);
	and_db.entry[entry].parent = NULL;
	and_printf(0,"Priority database line %i with bad parent regexp %s (%s)\n",
		   line_count, parent, error_msg);
	bad_count++;
	continue;
      }
      strncpy(and_db.entry[entry].parent_str,parent,127);
      and_db.entry[entry].parent_str[127] = 0;
      and_db.entry[entry].parentmode = parentmode;
    }
    and_db.entry[entry].nl[0] = nl0;
    and_db.entry[entry].nl[1] = nl1;
    and_db.entry[entry].nl[2] = nl2;
    if (entry == and_db.n) {
      and_db.n++;
    }
  }
  /* cleanup */
  fclose(priority);
  if (and_config.verbose > 1) {
    print_priorities();
  }
  if (bad_count) {
    and_printf(0,"Priority database contains %i bad lines. Aborting.\n",
	       bad_count);
    abort();
  }
}


void read_config ()
{
  FILE *f;
  int i, val, bad, bad_f, line, u, g, c, p;
  unsigned uval;
  char buffer [1024];
  char param [1024];
  char value [1024];
  regex_t *rex;
  bool section_matches = true;
  if ((f = fopen(and_config.config_file,"rt")) == 0) {
    and_printf(0,"Configuration file %s not found. Aborting.\n",
	       and_config.config_file);
    abort();
  }
  and_printf(0,"Configuration file is: %s\n", and_config.config_file);
  /* Set defaults */
  strcpy(and_config.affinity,"ugcp");
  and_config.weight[PRI_U] = 8;
  and_config.weight[PRI_G] = 4;
  and_config.weight[PRI_C] = 2;
  and_config.weight[PRI_P] = 1;
  and_config.time_mark[0] = 120;   /*  2 min */
  and_config.time_mark[1] = 1200;  /* 20 min */
  and_config.time_mark[2] = 3600;  /* 60 min */
  /* Read file line by line */
  line = 0;
  bad = 0;
  fgets(buffer,1023,f);
  while (!feof(f)) {
    ++line;
    if (buffer[0] == 10 || buffer[0] == 13 || buffer[0] == '#') {
      memset(buffer,0,1024);
      if (fgets(buffer,1022,f) == 0) break;
      continue;
    }
    if ((buffer[0] == 'o') && (buffer[1] == 'n') && (buffer[2] == ' ')) {
      while (buffer[strlen(buffer)-1] < 32) {
	buffer[strlen(buffer)-1] = 0;
      }
      rex = (regex_t*)malloc(sizeof(regex_t));
      regcomp(rex,&buffer[3],REG_NOSUB|REG_EXTENDED);
      section_matches = (regexec(rex,and_config.hostname,0,0,0) == 0);
      and_printf(0,"Configuration file line %i: section for host(s) %s will be %s.\n",
		 line, &buffer[3], (section_matches?"read":"skipped"));
      regfree(rex);
      buffer[0] = '#'; /* trigger read next line */
      continue;
    }
    if (!section_matches) {
      buffer[0] = '#'; /* trigger read next line */
      continue;
    }
    if (sscanf(buffer,"%s %s",param,value) != 2) {
      ++bad;
      and_printf(0,"Configuration file line %i is invalid: %s\n",
		 line, buffer);
      memset(buffer,0,1024);
      if (fgets(buffer,1022,f) == 0) break;
      continue;
    }
    if (sscanf(value,"%i",&val) != 1) val = -1;
    if (sscanf(value,"%u",&uval) != 1) uval = UINT_MAX;
    if (strcmp(param,"defaultnice")==0) {
	if (val > -1) /* Prohibits a default is kill policy */
	and_config.nice_default = val;
      else {
	++bad;
	and_printf(0,"Configuration file line %i has invalid value for defaultnice: %s.\n",
		   line, value);
      }
    } else if (strcmp(param,"interval")==0) {
      if (and_config.lock_interval) {
	and_printf(0,"Configuration file line %i: interval locked by -i command-line option.\n",
		   line);
      } else {
	if (uval < UINT_MAX)
	  and_config.interval = uval;
	else {
	  ++bad;
	  and_printf(0,"Configuration file line %i has invalid value for interval: %s.\n",
		     line, value);
	}
      }
    } else if (strcmp(param,"minuid")==0) {
      if (uval < UINT_MAX)
	and_config.min_uid = uval;
      else {
	++bad;
	and_printf(0,"Configuration file line %i has invalid value for minuid: %s.\n",
		   line, value);
      }
    } else if (strcmp(param,"mingid")==0) {
      if (uval < UINT_MAX)
	and_config.min_gid = uval;
      else {
	++bad;
	and_printf(0,"Configuration file line %i has invalid value for mingid: %s.\n",
		   line, value);
      }
    } else if (strcmp(param,"lv1time")==0) {
      if (uval < UINT_MAX)
	and_config.time_mark[0] = uval;
      else {
	++bad;
	and_printf(0,"Configuration file line %i has invalid value for lv1time: %s.\n",
		   line, value);
      }
    } else if (strcmp(param,"lv2time")==0) {
      if (uval < UINT_MAX)
	and_config.time_mark[1] = uval;
      else {
	++bad;
	and_printf(0,"Configuration file line %i has invalid value for lv2time: %s.\n",
		   line, value);
      }
    } else if (strcmp(param,"lv3time")==0) {
      if (uval < UINT_MAX)
	and_config.time_mark[2] = uval;
      else {
	++bad;
	and_printf(0,"Configuration file line %i has invalid value for lv3time: %s.\n",
		   line, value);
      }
    } else if (strcmp(param,"affinity")==0) {
      bad_f = -1;
      u = g = c = p = 0;
      if (strlen(value) != 4) {
	  and_printf(0,"Configuration file line %i has invalid affinity: %s.\n",
		     line, value);
        ++bad;
      } else {
        for (i=0; i<4; i++) {
          switch(value[3-i]) {
	  case 'u':
	    if (!u) u = and_config.weight[PRI_U] = 1 << i;
	    else bad_f = i;
	    break;
	  case 'g':
	    if (!g) g = and_config.weight[PRI_G] = 1 << i;
	    else bad_f = i;
	    break;
	  case 'c':
	    if (!c) c = and_config.weight[PRI_C] = 1 << i;
	    else bad_f = i;
	    break;
	  case 'p':
	    if (!p) p = and_config.weight[PRI_P] = 1 << i;
	    else bad_f = i;
	    break;
	  default:
	    bad_f = 3-i;
	  }
        }
        if (bad_f > -1) {
          and_printf(0,"Configuration file line %i has invalid affinity: %s[%i]=%c.\n",
                     line, value, bad_f, value[bad_f]);
          ++bad;
        } else {
          strncpy(and_config.affinity,value,4);
          and_config.affinity[4] = 0;
        }
      }
    } else {
      ++bad;
      and_printf(0,"Configuration file line %i has invalid parameter: %s.\n",
		 line, param);
    }
    memset(buffer,0,1024);
    if (fgets(buffer,1022,f) == 0) break;
  }
  /* Cleanup, exit on errors. */
  fclose(f);
  if (and_config.verbose > 1) print_config();
  if (bad) {
    and_printf(0,"Configuration file contains %i bad lines. Aborting.\n",
	       bad);
    abort();
  }
}


/* Compute new nice level for given command/uid/gid/utime */

int and_getnice (int uid, int gid, char *command, struct and_procent *parent, unsigned cpu_seconds)
{
  int i, level, entry, exact = -1, last;
  struct and_procent *par;
  int exactness [PRI_MAXENTRIES];
  if (!command) {
    and_printf(0,"Process without command string encountered. Aborting.\n");
    abort();
  }
  if (uid == 0) {
    and_printf(3,"root is untouchable: %s\n", command);
    return 0;
  }
  if (uid < and_config.min_uid) {
    and_printf(3,"uid %i is untouchable: %s\n", uid, command);
    return 0;
  }
  if (gid < and_config.min_gid) {
    and_printf(3,"gid %i is untouchable: %s\n", gid, command);
    return 0;
  }
  
  /* Strategy: each priority database accumulates accuracy points
     for every aspect: user, group, command, parent. An exact hit is 
     worth the configured weight of the aspect (1, 2, 4, 8); a joker
     is worth 0; and a miss is with -MAXINT, effectively eliminating
     the entry (veto). At the end, the highest rated entry is
      used to determine the new nice level. */
  for (i=0; i<and_db.n; i++) {
    /* user id */
    if (uid == and_db.entry[i].uid) {
      exactness[i] = and_config.weight[PRI_U];
    } else if (and_db.entry[i].uid == -1) {
      exactness[i] = 0;
    } else {
      exactness[i] = -MAXINT;
    }
    /* group id */
    if (gid == and_db.entry[i].gid) {
      exactness[i] += and_config.weight[PRI_G];
    } else if (and_db.entry[i].gid == -1) {
      exactness[i] += 0;
    } else {
      exactness[i] = -MAXINT;
    }
    /* command */
    if (command!=NULL && regexec(and_db.entry[i].command,command,0,0,0) == 0) {
      exactness[i] += and_config.weight[PRI_C];
    } else if (strcmp(and_db.entry[i].command_str,"*") == 0) {
      exactness[i] += 0;
    } else {
      exactness[i] = -MAXINT;
    }
    /* parent */
    par = parent;
    while (par != NULL)
    {
      last = (and_db.entry[i].parentmode == PARENT_ONLY || 
              par->parent == NULL);
      if (regexec(and_db.entry[i].parent,par->command,0,0,0) == 0) {
        exactness[i] += and_config.weight[PRI_P];
        break;
      } else if (last && strcmp(and_db.entry[i].parent_str,"*") == 0) {
        exactness[i] += 0;
        break;
      } else if (last) {
        exactness[i] = -MAXINT;
        break;
      }
      par = par->parent;
    }
  }
  entry = 0;
  exact = -1;
  for (i=0; i<and_db.n; i++) {
    if (exactness[i] >= exact) { 
      /* >exact -> first entry wins, >=exact -> last entry wins */
      entry = i;
      exact = exactness[i];
    }
  }
  if (exact < 0) {
    and_printf(2,"no match for uid=%i gid=%i cmd=%s\n par=%s\n",
               uid, gid, command, (parent!=NULL?parent->command:"(orphan)"));
    return and_config.nice_default;
  }
  level = 2;
  while (level >= 0 && and_config.time_mark[level] > cpu_seconds) {
    --level;
  }
  and_printf(2,"command=%s (%i,%i,%s) hit on entry=%i, exactness=%i, level=%i.\n",
             command, uid, gid, (parent!=NULL?parent->command:"(orphan)"), 
             entry, exact, level);
  return (level >= 0 ? and_db.entry[entry].nl[level] : 0);
}



/**********************************************************************

 **********************************************************************/



static struct and_procent *(*and_getfirst)() = NULL;
static struct and_procent *(*and_getnext)() = NULL;


void and_setprocreader (struct and_procent *(*getfirst)(), 
			struct and_procent *(*getnext)())
{
  and_getfirst = getfirst;
  and_getnext = getnext;
}


struct and_procent* and_find_proc (struct and_procent *head, int ppid)
{
  struct and_procent *current = head;
  while (current != NULL)
  {
    if (current->pid == ppid)
      return current;
    current = current->next;
  }
  and_printf(1,"no parent for ppid: %d\n", ppid);
  return NULL;
}


void and_loop ()
{
  struct and_procent *head, *current, *new, *proc;
  int newnice;
  int njobs = 0;
  assert(and_getfirst != NULL);
  assert(and_getnext != NULL);
  head = NULL;
  current = NULL;
  proc = and_getfirst();
  while (proc != NULL) {
    new = (struct and_procent*)malloc(sizeof(struct and_procent));
    memcpy(new,proc,sizeof(struct and_procent));
    new->next = NULL;
    if (current != NULL) {
      current->next = new;
    } else {
      head = new;
    }
    current = new;
    proc = and_getnext();
  }
  current = head;
  while (current != NULL) {
    if (current->pid != current->ppid)
      current->parent = and_find_proc(head,current->ppid);
    else
      current->parent = NULL;
    and_printf(2, "process %s parent : %s\n", current->command,
             (current->parent != NULL ? current->parent->command : "(none)"));
    current = current->next;
  }
  current = head;
  while (current != NULL) {
    njobs++;
    newnice = and_getnice(current->uid,current->gid,current->command,
                          current->parent,current->utime);
    if (current->uid != 0) {
      if (newnice) {
	if (newnice > 0) {
	  if (newnice > current->nice) {
	    if (and_config.test)
	      and_printf(0,"would renice to %i: %i (%s)\n",newnice,current->pid,
			 current->command);
	    else {
	      and_printf(1,"renice to %i: %i (%s)\n",newnice,current->pid,
			 current->command);
	      setpriority(PRIO_PROCESS,current->pid,newnice);
	    }
	  }
	} else {
	  if (and_config.test)
	    and_printf(0,"would kill %i %i (%s)\n",newnice,current->pid,
		       current->command);
	  else {
	    and_printf(1,"kill %i %i (%s)\n",newnice,current->pid,
                       current->command);
	    kill(current->pid,-newnice);
	  }
	}
      }
    }
    current = current->next;
  }
  current = head;
  while (current != NULL) {
    proc = current;
    current = current->next;
    free(proc);
  }
}


void and_getopt (int argc, char** argv)
{
#define OPTIONS "c:d:i:vstxh"
  int opt, value;
  opt = getopt(argc,argv,OPTIONS);
  while (opt != -1) {
    switch(opt) {
    case 'c':
      and_config.config_file = (char*)malloc(strlen(optarg)+1);
      assert(and_config.config_file);
      strcpy(and_config.config_file,optarg);
      break;
    case 'd':
      and_config.database_file = (char*)malloc(strlen(optarg)+1);
      assert(and_config.database_file);
      strcpy(and_config.database_file,optarg);
      break;
    case 'i':
      value = atoi(optarg);
      if (value > 0) {
	and_config.lock_interval = true;
	and_config.interval = value;
      } else {
	fprintf(stderr,"%s: illegal interval: %s\n",argv[0],optarg);
	exit(1);
      }
      break;
    case 's':
      and_config.to_stdout = 1;
      break;
    case 'v':
      ++and_config.verbose;
      break;
    case 't':
      and_config.test = 1;
      break;
    case 'x':
      and_config.test = 0;
      break;
    case 'h':
      printf("auto nice daemon version %s (%s)\n"
	     "%s [-v] [-s]  [-t] [-x] [-c configfile] [-d databasefile] [-i interval]\n"
	     "-v: verbosity -v, -vv, -vvv etc\n"
	     "-s: log to stdout (default is syslog, or debug.and)\n"
	     "-x: really execute renices and kills (default)\n"
	     "-t: test configuration (don't really renice)\n"
	     "-i interval: loop interval in seconds (default %i)\n"
	     "-c configfile: specify config file (default %s)\n"
	     "-d databasefile: specify priority database file (default %s)\n"
	     ,AND_VERSION,AND_DATE,argv[0],
	     DEFAULT_INTERVAL, DEFAULT_CONFIG_FILE, DEFAULT_DATABASE_FILE);
      exit(1);
    default:
      fprintf(stderr,"Try %s -h for help.\n", argv[0]);
      exit(1);
    }
    opt = getopt(argc,argv,OPTIONS);
  }
#undef OPTIONS
}


static int g_reload_conf;


void and_trigger_readconf (int sig)
{
  g_reload_conf = (sig == SIGHUP);
}


void and_readconf ()
{
  and_printf(0,"Re-reading configuration and priority database...\n");
  read_config();
  read_priorities();
  g_reload_conf = 0;
}


void and_worker ()
{
  read_config();
  read_priorities();
  signal(SIGHUP,and_trigger_readconf);
  and_printf(0,"AND ready.\n");
  g_reload_conf = 0;
  while (1) {
    if (g_reload_conf) {
      and_readconf();
    }
    and_loop();
    sleep(and_config.interval);
  }
}


int and_main (int argc, char** argv)
{
  set_defaults(argc,argv);
  and_getopt(argc,argv);
  if (and_config.test) {
    and_worker();
  } else {
    if (fork() == 0) and_worker();
  }
  return 0;
}

