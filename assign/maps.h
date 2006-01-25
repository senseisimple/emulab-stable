/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * A simple header that provides definitions of some maps used in assign
 */

#ifndef __MAPS_H
#define __MAPS_H

#include "fstring.h"

/*
 * A hash function for graph edges
 */
struct hashedge {
  size_t operator()(vedge const &A) const {
    hashptr<void *> ptrhash;
    return ptrhash(target(A,VG))/2+ptrhash(source(A,VG))/2;
  }
};

/*
 * Map types
 */
typedef hash_map<vvertex,pvertex,hashptr<void *> > node_map;
typedef hash_map<vvertex,bool,hashptr<void *> > assigned_map;
typedef hash_map<pvertex,fstring,hashptr<void *> > type_map;
typedef hash_map<vedge,tb_link_info,hashedge> link_map;


#endif
