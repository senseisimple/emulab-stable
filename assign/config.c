/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>

#include "config.h"

/*
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "config.h"
#include "tgenlib.h"
*/
  
/*
 * Dumb as a stump config for options that the test program supports
 */

void
parse_options(char **argv, struct config_param options[], int nopt)
{
  extern char **environ;

  /* get from environment first */
  config_parse(environ, options, nopt);

  /* then command line */
  config_parse(argv, options, nopt);
}

int
config_parse(char **args, struct config_param cparams[], int nparams)
{
  int i, j, len;
  char *arg, *bp, *var, *val, buf[256];

  for (i = 0, arg = args[i]; arg != 0; i++, arg = args[i]) {
    len = sizeof(buf) - 1;
    strncpy(buf, arg, len);
    buf[len] = 0;
    var = val = buf;
    if (strsep(&val, "=") == 0 || val == 0 || *val == 0)
      continue;

    for (j = 0; j < nparams; j++)
      if (strcmp(var, cparams[j].name) == 0)
	break;
    if (j == nparams)
      continue;

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
      /*
	case CONFIG_STRING:
	bp = malloc(strlen(val) + 1);
	assert(bp);
	strcpy(bp, val);
	*(char **)cparams[j].ptr = bp;
	break;
      */
    }
  }

  return 0;
}

void
dump_options(const char *str, struct config_param cparams[], int numparams)
{
  int i;

  fprintf(stderr,"%s configuration:\n", str);
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
      /*
	case CONFIG_STRING:
	fprintf(stderr,"%s\n", *(char **)cparams[i].ptr ?
	*(char **)cparams[i].ptr : "<unset>");
	break;
      */
    }
  }
}
