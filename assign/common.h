#ifndef __COMON_H
#define __COMON_H

#include "config.h"

const int MAX_PNODES = 1024;	/* maximum # of physical nodes */

/*
  To use these on the command line, each entry gets a
  <name>=<value> pair on the command line.
  Here we declare them all and give them defaults.
*/

static int init_temp = 100;
static int USE_OPTIMAL = 1;
static int temp_prob = 130;
static int temp_stop = 20;
static int CYCLES = 20;

static float temp_rate = 0.9;
static float opt_nodes_per_sw = 5.0;
static float SCORE_DIRECT_LINK = 0.01;/* Cost of a direct link */
static float SCORE_DIRECT_LINK_PENALTY = 0.5;/* Cost of overused direct link*/
static float SCORE_INTRASWITCH_LINK = 0.02;/* Cost of an intraswitch link*/
static float SCORE_INTERSWITCH_LINK = 0.05;/* Cost of an interswitch link*/
static float SCORE_NO_CONNECTION = 0.5;/* Cost of not filling a virt. link*/
static float SCORE_PNODE = 0.05;/* Cost of using a pnode*/
static float SCORE_PNODE_PENALTY = 0.5;/* Cost of overusing a pnode*/
static float SCORE_SWITCH = 0.5;/* Cost of using a switch.*/
static float SCORE_UNASSIGNED = 1;/* Cost of an unassigned node*/
static float SCORE_OVER_BANDWIDTH = 0.5;/* Cost of going over bandwidth*/
static float SCORE_DESIRE = 1;/* Multiplier for desire costs*/
static float SCORE_FEATURE = 1;/* Multiplier for feature weights*/
static float SCORE_PCLASS = 0.5; /* Cost of each pclass */
static float SCORE_VCLASS = 1;	/* vclass score multiplier */
				  
static struct config_param options[] = {
  { "IT",	CONFIG_INT,	&init_temp,			0 },
  { "OP",	CONFIG_INT,	&USE_OPTIMAL,			0 },
  { "TP",	CONFIG_INT,	&temp_prob,			0 },
  { "TS",	CONFIG_INT,	&temp_stop,			0 },
  { "CY",	CONFIG_INT,	&CYCLES,			0 },
  { "UN",	CONFIG_FLOAT,	&SCORE_UNASSIGNED,     		0 },
  { "DE",	CONFIG_FLOAT,	&SCORE_DESIRE,			0 },
  { "FE",	CONFIG_FLOAT,	&SCORE_FEATURE,			0 },
  { "1S",	CONFIG_FLOAT,	&SCORE_INTERSWITCH_LINK,	0 },
  { "2S",	CONFIG_FLOAT,	&SCORE_INTRASWITCH_LINK,	0 },
  { "NC",	CONFIG_FLOAT,	&SCORE_NO_CONNECTION,		0 },
  { "OB",	CONFIG_FLOAT,	&SCORE_OVER_BANDWIDTH,		0 },
  { "DL",	CONFIG_FLOAT,	&SCORE_DIRECT_LINK,		0 },
  { "DP",	CONFIG_FLOAT,	&SCORE_DIRECT_LINK_PENALTY,	0 },
  { "PN",	CONFIG_FLOAT,	&SCORE_PNODE,			0 },
  { "PP",	CONFIG_FLOAT,	&SCORE_PNODE_PENALTY,		0 },
  { "PC",	CONFIG_FLOAT,	&SCORE_PCLASS,		        0 },
  { "VC",       CONFIG_FLOAT,   &SCORE_VCLASS,                  0 },
  { "SW",	CONFIG_FLOAT,	&SCORE_SWITCH,			0 },
  { "ON",	CONFIG_FLOAT,	&opt_nodes_per_sw,		0 },
  { "TR",	CONFIG_FLOAT,	&temp_rate,			0 }
};

static int noptions = sizeof(options) / sizeof(options[0]);

#endif



