// Compressor.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef COMPRESSOR_H_DISTRIBUTED_DIJKSTRA_1
#define COMPRESSOR_H_DISTRIBUTED_DIJKSTRA_2

#include "SingleSource.h"
#include "IpTree.h"

class Compressor
{
private:
    static const int shift_10 = IP_SIZE - 8;
    static const IPAddress ip_10 = 10;

    static const int shift_172_16 = IP_SIZE - 12;
    static const IPAddress ip_172_16 = (172 << 8) + 16;

    static const int shift_192_168 = IP_SIZE - 16;
    static const IPAddress ip_192_168 = (192 << 8) + 168;
public:
    Compressor();
    ~Compressor();

    void compress(SingleSource const & newGraph,
                  HostHostToIpMap const & newIp);
private:
    void add(int dest, IPAddress destIp);
private:
    Compressor(Compressor const &);
    Compressor & operator=(Compressor const &) { return *this; }
private:
    IpTree root_10;
    IpTree root_172_16;
    IpTree root_192_168;

    SingleSource const * graph;
    HostHostToIpMap const * ipMap;
};

#endif
