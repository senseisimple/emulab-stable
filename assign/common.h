/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef __COMON_H
#define __COMON_H

#include "port.h"

/*
 * We have to do these includes differently depending on which version of gcc
 * we're compiling with
 */
#ifdef NEW_GCC
#include <ext/hash_map>
#include <ext/hash_fun.h>
using namespace __gnu_cxx;
#define RANDOM() random()
#else
#include <hash_map>
#define RANDOM() std::random()
#endif

#include "config.h"
#include <utility>
#include "port.h"
#include "fstring.h"

#include <boost/graph/adjacency_list.hpp>

/*
 * Exit vaules from assign
 */

// A solution with no violations was found
#define EXIT_SUCCESS 0

// No valid solution was found, but one may exist
// No violation-free solution was found after annealing.
#define EXIT_RETRYABLE 1

// It is not possible to map the given top file into the given ptop file,
// so there's no point in re-running assign.
#define EXIT_UNRETRYABLE 2

// An internal error occured, or there was a problem with the input - for
// example, the top or ptop file does not exist or cannot be parsed
#define EXIT_FATAL -1

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

// The weight at which a feature or desire triggers a violations if
// unstatisfied or unused
static float FD_VIOLATION_WEIGHT = 1.0;

// Number of runs to spend melting
static int melt_trans = 1000;
static int min_neighborhood_size = 1000;


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
static float SCORE_MISSING_LOCAL_FEATURE = 1.0;
static float SCORE_OVERUSED_LOCAL_FEATURE = 0.5;
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
static float SCORE_TRIVIAL_PENALTY = 0.5; /* Cost of over-using a trivial link */

static float SCORE_TRIVIAL_MIX = 0.5; /* Cost of mixing trivial and non-trivial
					 links */

static float SCORE_SUBNODE = 0.5; /* Cost of not properly assigning subnodes */
static float SCORE_MAX_TYPES = 0.15; /* Cost of going over type limits - low 
                                      * enough that assign ususally picks this
				      * over leaving a node unassigned */

// The following are used to weight possible link resolutions.  Higher
// numbers mean a more likely resolution.
static float LINK_RESOLVE_TRIVIAL = 8.0;
static float LINK_RESOLVE_DIRECT = 4.0;
static float LINK_RESOLVE_INTRASWITCH = 2.0;
static float LINK_RESOLVE_INTERSWITCH = 1.0;

// The amount that each violation contributes to the total score
static float VIOLATION_SCORE = 1.0;

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

#ifdef NEW_GCC
namespace __gnu_cxx
{
#endif
    template<> struct hash< std::string >
    {
        size_t operator()( const std::string& x ) const
        {
            return hash< const char* >()( x.c_str() );
        }
    };
#ifdef NEW_GCC
};
#endif


enum edge_data_t {edge_data};
enum vertex_data_t {vertex_data};

namespace boost {
  BOOST_INSTALL_PROPERTY(edge,data);
  BOOST_INSTALL_PROPERTY(vertex,data);
}

/*
 * Used to count the number of nodes in each ptype and vtype
 */
typedef hash_map<fstring,int> name_count_map;
typedef hash_map<fstring,vector<fstring> > name_list_map;

/*
 * A hash function for pointers
 */
template <class T> struct hashptr {
  size_t operator()(T const &A) const {
    return (size_t) A;
  }
};

/*
 * Misc. debugging stuff
 */
#ifdef ROB_DEBUG
#define RDEBUG(a) a
#else
#define RDEBUG(a)
#endif

/*
 * Needed for the transition from gcc 2.95 to 3.x - the new gcc puts some
 * non-standard (ie. SGI) STL extensions in different place
 */
#ifdef NEW_GCC
#define HASH_MAP <ext/hash_map>
#else
#define HASH_MAP
#endif

// For use in functions that want to return a score/violations pair
typedef pair<double,int> score_and_violations;

#endif
