// Compressor.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

// This is a function-object which carries out route compression. This
// relies on the helper class IpTree. See that class for details of
// the algorithm. Of note is that there are three bitspaces which can
// be compressed: 10.0.0.0/8, 172.16.0.0/12, and 192.168.0.0/16. These
// must be individually compressed because otherwise a subnet might
// cover a real internet address. Any ip address not in one of these
// blocks is not compressed and a single route is printed to that
// address.

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

    // Run the algorithm. Note that no state is kept between invocations.
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
