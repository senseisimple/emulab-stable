// TreeCompressor.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "lib.h"
#include "bitmath.h"
#include "dijkstra.h"
#include "TreeCompressor.h"

using namespace std;

TreeCompressor::TreeCompressor(IpTree const & type)
    : graph(NULL)
    , ipMap(NULL)
{
    for (PRIVATE_SUBNET loop = PRIVATE_SUBNET_MIN;
         loop < PRIVATE_SUBNET_COUNT;
         loop = static_cast<PRIVATE_SUBNET>(loop + PRIVATE_SUBNET_UNIT))
    {
        // We want to setup the three private subnets to 'IpTree' of
        // the same type as the argument.

        // This is simple assignment. Complicated by a weird error in
        // either g++ or the standard. This is caused by the fact that
        // the copy semantics of auto_ptr are not normal.
        root[loop].reset(type.exemplar().release());
    }
}

TreeCompressor::~TreeCompressor()
{
}

void TreeCompressor::printRoutes(SingleSource const & newGraph,
                                 HostHostToIpMap const & newIp)
{
    graph = &newGraph;
    ipMap = &newIp;
    root[SUB_10]->reset();
    root[SUB_172_16]->reset();
    root[SUB_192_168]->reset();

    // Figure out which IP addresses are directly adjascent to the
    // source host. These IP addresses are routed to implicitly and
    // therefore explicit routes should not be printed for them.
    set<string> implicitIp;
    HostEntry::const_iterator implicitPos;
    HostEntry::const_iterator implicitLimit;
    implicitPos = (*ipMap)[graph->getSource()].begin();
    implicitLimit = (*ipMap)[graph->getSource()].end();
    for ( ; implicitPos != implicitLimit; ++implicitPos)
    {
        implicitIp.insert(implicitPos->second.second);
    }

    for (int i = 0; i < graph->getVertexCount(); ++i)
    {
        int dest = i;
        if (dest != graph->getSource())
        {
            HostEntry::const_iterator pos;
            HostEntry::const_iterator limit;
            pos = (*ipMap)[dest].begin();
            limit = (*ipMap)[dest].end();
            for ( ; pos != limit; ++pos)
            {
                if (implicitIp.find(pos->second.first) == implicitIp.end())
                {
                    IPAddress destIp = stringToIP(pos->second.first);
                    add(dest, destIp);
                }
            }
        }
    }
    for (PRIVATE_SUBNET loop = PRIVATE_SUBNET_MIN;
         loop < PRIVATE_SUBNET_COUNT;
         loop = static_cast<PRIVATE_SUBNET>(loop + PRIVATE_SUBNET_UNIT))
    {
        root[loop]->printRoutes(*ipMap, graph->getSource(),
#ifdef ROCKETFUEL_TEST
 0
#else
prefix[loop]
#endif
);
    }
}

void TreeCompressor::add(int dest, IPAddress destIp)
{
    int firstHop = graph->getFirstHop(dest);
    HostEntry::const_iterator pos;
    pos = (*ipMap)[graph->getSource()].find(firstHop);
    if (pos != (*ipMap)[graph->getSource()].end())
    {
        PRIVATE_SUBNET subnet =
#ifdef ROCKETFUEL_TEST
SUB_10;
#else
whichSubnet(destIp);
#endif
        if (subnet == PRIVATE_SUBNET_INVALID)
        {
            // TODO: Figure out what cost means
            printRouteToIp(pos->second.first, pos->second.second,
                           ipToString(destIp), 1);
        }
        else
        {
            // TODO: Replace this
            root[subnet]->addRoute(destIp, firstHop,
#ifdef ROCKETFUEL_TEST
0
#else
IP_SIZE - shift[subnet]
#endif
);
        }
    }
}
