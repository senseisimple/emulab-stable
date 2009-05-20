/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 1999-2003 The University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include "common.h"
#include "score.h"
#include "anneal.h"

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


/*
 * To use these on the command line, each entry gets a
 * <name>=<value> pair on the command line.
 */
static struct config_param options[] = {
    { "IT",	CONFIG_INT,	&init_temp,			0 },
    { "TP",	CONFIG_INT,	&temp_prob,			0 },
    { "TS",	CONFIG_INT,	&temp_stop,			0 },
    { "CY",	CONFIG_INT,	&CYCLES,			0 },
    { "UN",	CONFIG_FLOAT,	&SCORE_UNASSIGNED,     		0 },
    { "1S",	CONFIG_FLOAT,	&SCORE_INTERSWITCH_LINK,	0 },
    { "2S",	CONFIG_FLOAT,	&SCORE_INTRASWITCH_LINK,	0 },
    { "NC",	CONFIG_FLOAT,	&SCORE_NO_CONNECTION,		0 },
    { "DL",	CONFIG_FLOAT,	&SCORE_DIRECT_LINK,		0 },
    { "DP",	CONFIG_FLOAT,	&SCORE_DIRECT_LINK_PENALTY,	0 },
    { "PN",	CONFIG_FLOAT,	&SCORE_PNODE,			0 },
    { "PP",	CONFIG_FLOAT,	&SCORE_PNODE_PENALTY,		0 },
    { "PC",	CONFIG_FLOAT,	&SCORE_PCLASS,		        0 },
    { "VC",     CONFIG_FLOAT,   &SCORE_VCLASS,                  0 },
    { "SW",	CONFIG_FLOAT,	&SCORE_SWITCH,			0 },
    { "EL",	CONFIG_FLOAT,	&SCORE_EMULATED_LINK,		0 },
    { "ON",	CONFIG_FLOAT,	&opt_nodes_per_sw,		0 },
    { "TR",	CONFIG_FLOAT,	&temp_rate,			0 },
    { "LD",     CONFIG_FLOAT,   &LINK_RESOLVE_DIRECT,           0 },
    { "LI",     CONFIG_FLOAT,   &LINK_RESOLVE_INTRASWITCH,      0 },
    { "LT",     CONFIG_FLOAT,   &LINK_RESOLVE_INTERSWITCH,      0 },
    { "OD",     CONFIG_FLOAT,   &SCORE_OUTSIDE_DELAY,           0 }
};

const int noptions = sizeof(options) / sizeof(options[0]);

#endif
