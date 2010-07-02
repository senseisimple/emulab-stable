// lib.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef LIB_H_DISTRIBUTED_DIJKSTRA_1
#define LIB_H_DISTRIBUTED_DIJKSTRA_1

#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <memory>
#include <set>
#include <cassert>

// A convenient converter
inline std::string intToString(int num)
{
    std::ostringstream stream;
    stream << num;
    return stream.str();
}


// Sparse array of hosts. The first string is the IP address of an
// interface on the host represented by the whole multimap. The second
// string is the IP address of an interface on the host represented by
// the key. The two interfaces are linked.
typedef std::multimap<int,std::pair<std::string,std::string> > HostEntry;

// Map every pair of adjascent hosts to the IP addresses of their interfaces.
typedef std::vector<HostEntry> HostHostToIpMap;

enum { IP_SIZE = 32 };

#endif
