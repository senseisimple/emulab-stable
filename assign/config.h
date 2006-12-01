/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 1999-2003 The University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#ifdef __cplusplus 
extern "C" {
#endif
  
/*
 * Dumb as a stump config for options that the test program supports
 */
struct config_param {
  const char *name;	/* name of the option */
  int type;		/* type (see below) */
  void *ptr;		/* pointer to value portion */
  void (*func)(struct config_param *opt, char *newval);
				/* function to set value */
};

/* types */
#define CONFIG_INT	0
#define CONFIG_FLOAT	1
  
void parse_options(char **argv, struct config_param options[], int nopt);
int config_parse(char **args, struct config_param cparams[], int nparams);
void dump_options(const char *str, struct config_param cparams[], int nparams);

#ifdef __cplusplus 
}
#endif


#endif
