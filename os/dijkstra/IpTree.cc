// IpTree.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "lib.h"
#include "bitmath.h"
#include "dijkstra.h"
#include "IpTree.h"

using namespace std;

IpTree::IpTree()
    : m_firstHop(0)
    , m_depth(0)
{
}

IpTree::~IpTree()
{
}

void IpTree::reset(void)
{
    m_child[0].reset(NULL);
    m_child[1].reset(NULL);
    m_firstHop = 0;
    m_childHops.clear();
    m_depth = 0;
}

void IpTree::addRoute(IPAddress ip, int newFirstHop, int depth)
{
    m_depth = depth;
    if (depth < IP_SIZE)
    {
        IPAddress bit = (ip >> (IP_SIZE - depth - 1)) & 0x01;
        if (m_child[bit].get() == NULL)
        {
            m_child[bit].reset(new IpTree());
        }
        m_child[bit]->addRoute(ip, newFirstHop, depth + 1);
    }
    else
    {
        m_firstHop = newFirstHop;
    }
    ++(m_childHops[newFirstHop]);
}

void IpTree::printRoutes(int parentFirstHop, HostHostToIpMap const & ip,
                 int source, IPAddress subnet)
{
    if (m_childHops.empty())
    {
        return;
    }

    map<int,int>::iterator mostPos = findMostPos();

    if (mostPos->first != parentFirstHop)
    {
        m_firstHop = mostPos->first;
        HostEntry::const_iterator pos;
        pos = ip[source].find(m_firstHop);
        if (pos != ip[source].end())
        {
            printRouteToSubnet(pos->second.first, pos->second.second,
                               ipToString(subnet << (IP_SIZE - m_depth)),
                               m_depth, 1);
            // TODO: replace hardwired '1' with actual cost metric
        }
        else
        {
            throw StringException("Internal error: Corruption in data structures: Compressor::add()");
        }
    }

    if (m_child[0].get() != NULL)
    {
        m_child[0]->printRoutes(mostPos->first, ip, source, subnet << 1);
    }

    if (m_child[1].get() != NULL)
    {
        m_child[1]->printRoutes(mostPos->first, ip, source,
                                (subnet << 1) + 1);
    }
}

map<int,int>::iterator IpTree::findMostPos(void)
{
    map<int,int>::iterator pos = m_childHops.begin();
    map<int,int>::iterator mostPos = pos;
    int mostScore = mostPos->second;
    ++pos;
    while (pos != m_childHops.end())
    {
        if (pos->second > mostScore)
        {
            mostPos = pos;
            mostScore = mostPos->second;
        }
        ++pos;
    }
    return mostPos;
}
