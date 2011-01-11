/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef __COMMON_H
#define __COMMON_H

#include "port.h"

/*
 * We have to do these includes differently depending on which version of gcc
 * we're compiling with
 */
#ifdef NEW_GCC
#include <ext/hash_map>
#ifdef NEWER_GCC
  #include <backward/hash_fun.h>
#else
  #include <ext/hash_fun.h>
#endif

using namespace __gnu_cxx;
#define RANDOM() random()
#else
#include <hash_map>
#define RANDOM() std::random()
#endif

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
}
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
