// TreeCompressor.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

// This is a function-object which carries out route compression. It
// provides the framework to do various tree-based routing schemese
// based on the IpTree interface. The constructor provides a type
// which is used as an exemplar to create a separate tree for each
// bitspace that needs checked.

// Of note is that there are three bitspaces which can be compressed:
// 10.0.0.0/8, 172.16.0.0/12, and 192.168.0.0/16. These must be
// individually compressed because otherwise a subnet might cover a
// real internet address. Any ip address not in one of these blocks is
// not compressed and a single route is printed to that address.

#ifndef TREE_COMPRESSOR_H_DISTRIBUTED_DIJKSTRA_1
#define TREE_COMPRESSOR_H_DISTRIBUTED_DIJKSTRA_1

#include "Compressor.h"
#include "IpTree.h"

class TreeCompressor : public Compressor
{
private:
public:
    TreeCompressor(IpTree const & type);
    virtual ~TreeCompressor();

    // Run the algorithm. Note that no state is kept between invocations.
    virtual void printRoutes(SingleSource const & newGraph,
                             HostHostToIpMap const & newIp);
private:
    void add(int dest, IPAddress destIp);
private:
    TreeCompressor();
    TreeCompressor(TreeCompressor const &);
    TreeCompressor & operator=(TreeCompressor const &) { return *this; }
private:
    std::auto_ptr<IpTree> root[PRIVATE_SUBNET_COUNT];
    SingleSource const * graph;
    HostHostToIpMap const * ipMap;
};

#endif
