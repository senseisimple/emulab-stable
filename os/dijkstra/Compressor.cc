// Compressor.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "lib.h"
#include "bitmath.h"
#include "dijkstra.h"
#include "Compressor.h"

using namespace std;

Compressor::Compressor()
    : graph(NULL)
    , ipMap(NULL)
{
}

Compressor::~Compressor()
{
}

void Compressor::compress(SingleSource const & newGraph,
                          HostHostToIpMap const & newIp)
{
    graph = &newGraph;
    ipMap = &newIp;
    root_10.reset();
    root_172_16.reset();
    root_192_168.reset();

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
    root_10.printRoutes(INT_MAX, *ipMap, graph->getSource(), ip_10);
    root_172_16.printRoutes(INT_MAX, *ipMap, graph->getSource(),
                            ip_172_16);
    root_192_168.printRoutes(INT_MAX, *ipMap, graph->getSource(),
                             ip_192_168);
}

void Compressor::add(int dest, IPAddress destIp)
{
    int firstHop = graph->getFirstHop(dest);
    HostEntry::const_iterator pos;
    pos = (*ipMap)[graph->getSource()].find(firstHop);
    if (pos != (*ipMap)[graph->getSource()].end())
    {
        if ((destIp >> shift_10) == ip_10)
        {
            root_10.addRoute(destIp, firstHop, IP_SIZE - shift_10);
        }
        else if ((destIp >> shift_172_16) == ip_172_16)
        {
            root_172_16.addRoute(destIp, firstHop, IP_SIZE - shift_172_16);
        }
        else if ((destIp >> shift_192_168) == ip_192_168)
        {
            root_192_168.addRoute(destIp, firstHop, IP_SIZE - shift_192_168);
        }
        else
        {
            // TODO: Figure out what cost means
            printRouteToIp(pos->second.first, pos->second.second,
                           ipToString(destIp), 1);
        }
    }
}

