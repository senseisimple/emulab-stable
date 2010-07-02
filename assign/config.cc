/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2002-2009 University of Utah and the Flux Group.
 * All rights reserved.
 */

static const char rcsid[] = "$Id: config.cc,v 1.6 2009-06-16 21:11:05 ricci Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
  
/*
 * Dumb as a stump config for options that the test program supports
 */

void
parse_options(char **argv, struct config_param opts[], int nopt)
{
  extern char **environ;

  /* get from environment first - just skip unknown environment variables */
  config_parse(environ, opts, nopt, false);

  /* then command line - die if given bad option */
  config_parse(argv, opts, nopt, true);
}

int
config_parse(char **args, struct config_param cparams[], int nparams, 
        bool exit_on_error)
{
  int i, j, len;
  char *arg, *var, *val, buf[256];

  for (i = 0, arg = args[i]; arg != NULL; i++, arg = args[i]) {
    len = sizeof(buf) - 1;
    strncpy(buf, arg, len);
    buf[len] = 0;
    var = val = buf;
    if (strsep(&val, "=") == 0 || val == 0 || *val == 0) {
        if (exit_on_error) {
            cout << "*** Expected a configuration parameter, got "
                 << arg << endl;
            exit(EXIT_FATAL);
        } else {
            continue;
        }
    }

    for (j = 0; j < nparams; j++)
      if (strcmp(var, cparams[j].name) == 0)
	break;

    if (j == nparams) {
      if (exit_on_error) {
          cout << "*** Bad configuration parameter name: " << var << endl;
          exit(EXIT_FATAL);
      } else {
          continue;
      }
    }

    // Note: Unused
    if (cparams[j].func) {
      cparams[j].func(&cparams[j], val);
      continue;
    }

    switch (cparams[j].type) {
    case CONFIG_INT:
      *(int *)cparams[j].ptr = atoi(val);
      break;
    case CONFIG_FLOAT:
      *(float *)cparams[j].ptr = (float) atof(val);
      break;
    }
  }

  return 0;
}

void
dump_options(const char *str, struct config_param cparams[], int numparams)
{
  int i;

  fprintf(stderr,"%s:\n", str);
  for (i = 0; i < numparams; i++) {
    fprintf(stderr,"  %s ", cparams[i].name);
    switch (cparams[i].type) {
    case CONFIG_INT:
      fprintf(stderr,"%d\n",
	      *(int *)cparams[i].ptr);
      break;
    case CONFIG_FLOAT:
      fprintf(stderr,"%f\n",
	      *(float *)cparams[i].ptr);
      break;
    }
  }
}
