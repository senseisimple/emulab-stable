// NoneCompressor.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

// This function-object carries out route compression. Actually, the
// point of this object is to represent no-compression. This object
// simply prints out the routes generated without trying to compress
// anything. This is used if the user doesn't want any route
// compression or while testing a route-compression method.

#ifndef NONE_COMPRESSOR_H_DISTRIBUTED_DIJKSTRA_1
#define NONE_COMPRESSOR_H_DISTRIBUTED_DIJKSTRA_1

#include "Compressor.h"

class NoneCompressor : public Compressor
{
public:
    NoneCompressor();
    virtual ~NoneCompressor();

    virtual void printRoutes(SingleSource const & graph,
                             HostHostToIpMap const & ip);
private:
    // Print all routes from a particular host to every destination.
    void printRoutesFromHost(int source, SingleSource const & graph,
                             HostHostToIpMap const & ip);

    // Print all routes from a particular host to a particular host
    void printRoutesToHost(int source, int dest, SingleSource const & graph,
                           HostHostToIpMap const & ip);

    // Calculate the source ip address and the first hop ip address
    // between a particular pair of hosts.
    void calculateSourceInfo(int source, int dest,
                             SingleSource const & graph,
                             HostHostToIpMap const & ip,
                             std::string & OutSourceIp,
                             std::string & OutFirstHopIp);
private:
    NoneCompressor(NoneCompressor const &);
    NoneCompressor & operator=(NoneCompressor const &) { return *this; }
};

#endif
