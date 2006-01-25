/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Contains the actual functions to do simulated annealing
 */

#ifndef __ANNEAL_H
#define __ANNEAL_H

#include "port.h"

#include <boost/graph/adjacency_list.hpp>
using namespace boost;

#include <iostream>
using namespace std;

/*
 * We have to do these includes differently depending on which version of gcc
 * we're compiling with
 */
#if __GNUC__ == 3 && __GNUC_MINOR__ > 0
#include <ext/hash_map>
#include <ext/slist>
using namespace __gnu_cxx;
#else
#include <hash_map>
#include <slist>
#endif

#include <math.h>

#include "delay.h"
#include "physical.h"
#include "pclass.h"
#include "fstring.h"

// Some defaults for #defines
#ifndef NO_REVERT
#define NO_REVERT 0
#endif

#ifndef REVERT_VIOLATIONS
#define REVERT_VIOLATIONS 1
#endif

#ifndef REVERT_LAST
#define REVERT_LAST 0
#endif

#ifdef PHYS_CHAIN_LEN
#define PHYSICAL(x) x
#else
#define PHYSICAL(x) 0
#endif

/*
 * Globals - XXX made non-global!
 */
/* From assign.cc */
extern pclass_types type_table;
extern pclass_list pclasses;
extern pnode_pvertex_map pnode2vertex;
extern double absbest;
extern int absbestviolated, iters, iters_to_best;
extern bool allow_overload;

#ifdef PER_VNODE_TT
extern pclass_types vnode_type_table;
#endif

/* Decides based on the temperature if a new score should be accepted or not */
inline bool accept(double change, double temperature);

/* Find a pnode that can satisfy the give vnode */
tb_pnode *find_pnode(tb_vnode *vn);

/* The big guy! */
void anneal(bool scoring_selftest, double scale_neighborhood,
    double *initial_temperature, double use_connected_pnode_find);

typedef hash_map<fstring,fstring> name_name_map;
typedef slist<fstring> name_slist;

#endif
