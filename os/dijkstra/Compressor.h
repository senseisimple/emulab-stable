// Compressor.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

// This is a parent class for a group of functors. Which functor is
// actually called is dependant upon command-line arguments and is set
// in the 'processArgs()' function in 'dijkstra.cc'. Each of these
// functors prints out all the routes for a particular source using
// the graph and IP map given. The different types of functors use
// different techniques to compress the routing table as it is
// printed. See 'NoneCompressor.h', 'VoteCompressor.h', and
// 'OptimalCompressor.h' for details on the particulars.

#ifndef COMPRESSOR_H_DISTRIBUTED_DIJKSTRA_1
#define COMPRESSOR_H_DISTRIBUTED_DIJKSTRA_1

#include "dijkstra.h"
#include "bitmath.h"
#include "SingleSource.h"

class Compressor
{
protected:
    // These constants are used to distinguish between the three
    // non-routable prefixes that can be used to build a private
    // network.
    enum PRIVATE_SUBNET
    {
        PRIVATE_SUBNET_MIN = 0,
        PRIVATE_SUBNET_UNIT = 1,
        PRIVATE_SUBNET_MAX = 2,
        PRIVATE_SUBNET_COUNT = 3,
        PRIVATE_SUBNET_INVALID = 3,
        SUB_10 = 0,
        SUB_172_16 = 1,
        SUB_192_168 = 2
    };

    static const int shift[PRIVATE_SUBNET_COUNT];// = {IP_SIZE - 8,
//                                                    IP_SIZE - 12,
//                                                    IP_SIZE - 16};

    static const IPAddress prefix[PRIVATE_SUBNET_COUNT];// = {10,
//                                                           (172 << 8) + 16,
//                                                         (192 << 8) + 168};

    static PRIVATE_SUBNET whichSubnet(IPAddress destIp);

//    static const int shift_10 = IP_SIZE - 8;
//    static const IPAddress ip_10 = 10;
//
//    static const int shift_172_16 = IP_SIZE - 12;
//    static const IPAddress ip_172_16 = (172 << 8) + 16;
//
//    static const int shift_192_168 = IP_SIZE - 16;
//    static const IPAddress ip_192_168 = (192 << 8) + 168;
public:
    Compressor() {}
    virtual ~Compressor() {}

    virtual void printRoutes(SingleSource const & graph,
                             HostHostToIpMap const & ip)=0;
private:
    Compressor(Compressor const &);
    Compressor & operator=(Compressor const &) { return *this; }
};

#endif
