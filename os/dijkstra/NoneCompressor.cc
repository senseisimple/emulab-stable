// NoneCompressor.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "lib.h"
#include "NoneCompressor.h"

using namespace std;

NoneCompressor::NoneCompressor()
{
}

NoneCompressor::~NoneCompressor()
{
}

void NoneCompressor::printRoutes(SingleSource const & graph,
                                 HostHostToIpMap const & ip)
{
    printRoutesFromHost(graph.getSource(), graph, ip);
}

void NoneCompressor::printRoutesFromHost(int source,
                                         SingleSource const & graph,
                                         HostHostToIpMap const & ip)
{
    for (int i = 0; i < graph.getVertexCount(); ++i)
    {
        if (i != source)
        {
            printRoutesToHost(source, i, graph, ip);
        }
    }
}

void NoneCompressor::printRoutesToHost(int source, int dest,
                                       SingleSource const & graph,
                                       HostHostToIpMap const & ip)
{
    string sourceIp;
    string firstHopIp;
    calculateSourceInfo(source, dest, graph, ip, sourceIp, firstHopIp);

    multimap< int, pair<string, string> >::const_iterator pos;
    pos = ip[dest].begin();
    multimap< int, pair<string, string> >::const_iterator limit;
    limit = ip[dest].end();
    string previous;

    for ( ; pos != limit; ++pos)
    {
        string const & destIp = pos->second.first;
        if (destIp != previous)
        {
            printRouteToIp(sourceIp, firstHopIp, destIp,
                           graph.getDistance(dest));
            previous = destIp;
        }
    }
}

void NoneCompressor::calculateSourceInfo(int source, int dest,
                                         SingleSource const & graph,
                                         HostHostToIpMap const & ip,
                                         string & outSourceIp,
                                         string & outFirstHopIp)
{
    multimap< int, pair<string, string> >::const_iterator sourcePos;
    sourcePos = ip[source].find(graph.getFirstHop(dest));
    if (sourcePos == ip[source].end())
    {
        throw RouteNotFoundException(source, dest, graph.getFirstHop(dest));
    }
    outSourceIp = sourcePos->second.first;
    outFirstHopIp = sourcePos->second.second;
}
