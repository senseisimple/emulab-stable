/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef __COMON_H
#define __COMON_H

#include "config.h"

const int MAX_PNODES = 1024;	/* maximum # of physical nodes */

/*
  To use these on the command line, each entry gets a
  <name>=<value> pair on the command line.
  Here we declare them all and give them defaults.
*/

//static int init_temp = 100;
static int init_temp = 10;
static int USE_OPTIMAL = 1;
static int temp_prob = 130;
#ifdef LOW_TEMP_STOP
//static int temp_stop = 1;
static float temp_stop = .005;
#else
static float temp_stop = 2;
//static int temp_stop = 20;
#endif
static int CYCLES = 20;

// The following are basically arbitrary constants
// Initial acceptance ratio for melting
static float X0 = .95;
//static float epsilon = 0.1;
#ifdef LOCAL_DERIVATIVE
static float epsilon = 0.0001;
#else
static float epsilon = 0.01;
#endif
static float delta = 2;
//static float delta = 1;
//static float min_temp_end = 0.01;
//static float min_temp_end = 10000000.0;


// Number of runs to spend melting
static int melt_trans = 500;
static int min_neighborhood_size = 500;


static float temp_rate = 0.9;
static float opt_nodes_per_sw = 5.0;
#ifdef PENALIZE_BANDWIDTH
static float SCORE_DIRECT_LINK = 0.0;/* Cost of a direct link */
static float SCORE_INTRASWITCH_LINK = 0.0;/* Cost of an intraswitch link*/
static float SCORE_INTERSWITCH_LINK = 0.0;/* Cost of an interswitch link*/
#else
static float SCORE_DIRECT_LINK = 0.01;/* Cost of a direct link */
static float SCORE_INTRASWITCH_LINK = 0.02;/* Cost of an intraswitch link*/
static float SCORE_INTERSWITCH_LINK = 0.2;/* Cost of an interswitch link*/
#endif
static float SCORE_DIRECT_LINK_PENALTY = 0.5;/* Cost of overused direct link*/
static float SCORE_NO_CONNECTION = 0.5;/* Cost of not filling a virt. link*/
static float SCORE_PNODE = 0.2;/* Cost of using a pnode*/
static float SCORE_PNODE_PENALTY = 0.5;/* Cost of overusing a pnode*/
static float SCORE_SWITCH = 0.5;/* Cost of using a switch.*/
static float SCORE_UNASSIGNED = 1.0;/* Cost of an unassigned node*/
static float SCORE_DESIRE = 1.0;/* Multiplier for desire costs*/
static float SCORE_FEATURE = 1.0;/* Multiplier for feature weights*/
#ifdef NO_PCLASS_PENALTY
static float SCORE_PCLASS = 0.0; /* Cost of each pclass */
#else
static float SCORE_PCLASS = 0.5; /* Cost of each pclass */
#endif
static float SCORE_VCLASS = 1.0; /* vclass score multiplier */
static float SCORE_EMULATED_LINK = 0.01; /* cost of an emualted link */
static float SCORE_OUTSIDE_DELAY = 0.5;	/* penalty for going out of delay
					   requirements */
static float SCORE_DELAY = 10.0; /* multiplier to distance for delay scoring */
#ifdef PENALIZE_UNUSED_INTERFACES
static float SCORE_UNUSED_INTERFACE = 0.04;
#endif

// The following are used to weight possible link resolutions.  Higher
// numbers mean a more likely resolution.  Trivial resolutions are always
// used if possible.
static float LINK_RESOLVE_DIRECT = 4.0;
static float LINK_RESOLVE_INTRASWITCH = 2.0;
static float LINK_RESOLVE_INTERSWITCH = 1.0;

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
  { "DL",	CONFIG_FLOAT,	&SCORE_DIRECT_LINK,		0 },
  { "DP",	CONFIG_FLOAT,	&SCORE_DIRECT_LINK_PENALTY,	0 },
  { "PN",	CONFIG_FLOAT,	&SCORE_PNODE,			0 },
  { "PP",	CONFIG_FLOAT,	&SCORE_PNODE_PENALTY,		0 },
  { "PC",	CONFIG_FLOAT,	&SCORE_PCLASS,		        0 },
  { "VC",       CONFIG_FLOAT,   &SCORE_VCLASS,                  0 },
  { "SW",	CONFIG_FLOAT,	&SCORE_SWITCH,			0 },
  { "EL",	CONFIG_FLOAT,	&SCORE_EMULATED_LINK,		0 },
  { "ON",	CONFIG_FLOAT,	&opt_nodes_per_sw,		0 },
  { "TR",	CONFIG_FLOAT,	&temp_rate,			0 },
  { "LD",       CONFIG_FLOAT,   &LINK_RESOLVE_DIRECT,           0 },
  { "LI",       CONFIG_FLOAT,   &LINK_RESOLVE_INTRASWITCH,      0 },
  { "LT",       CONFIG_FLOAT,   &LINK_RESOLVE_INTERSWITCH,      0 },
  { "OD",       CONFIG_FLOAT,   &SCORE_OUTSIDE_DELAY,           0 },
  { "DM",       CONFIG_FLOAT,   &SCORE_DELAY,                   0 }
};

static int noptions = sizeof(options) / sizeof(options[0]);

void parse_options(char **argv, struct config_param options[], int nopt);

struct eqstr
{
  bool operator()(const char* A, const char* B) const
  {
    return (! strcmp(A, B));
  }
};

enum edge_data_t {edge_data};
enum vertex_data_t {vertex_data};

namespace boost {
  BOOST_INSTALL_PROPERTY(edge,data);
  BOOST_INSTALL_PROPERTY(vertex,data);
}

typedef hash_map<crope,crope> name_name_map;
typedef slist<crope> name_slist;

/*
 * Used to count the number of nodes in each ptype and vtype
 */
typedef hash_map<crope,int> name_count_map;

template <class T> struct hashptr {
  size_t operator()(T const &A) const {
    return (size_t) A;
  }
};

#endif
