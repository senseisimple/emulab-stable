// HostRouter.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "lib.h"
#include "Exception.h"
#include "HostRouter.h"
#include "Assigner.h"
#include "bitmath.h"
#include "coprocess.h"

using namespace std;

HostRouter::HostRouter()
{
}

HostRouter::~HostRouter()
{
}

auto_ptr<Router> HostRouter::clone(void) const
{
    return auto_ptr<Router>(new HostRouter(*this));
}

void HostRouter::calculateRoutes(void)
{
    if (m_nodeToLevel.size() != 0 && m_levelMaskSize.size() != 0
        && m_levelPrefix.size() != 0 && m_levelMakeup.size() != 0
        && m_lanWeights.size() != 0)
    {
        FileWrapper file(coprocess(ROUTECALC).release());
        m_tableList.clear();
        m_tableList.resize(m_nodeToLevel[0].size());
        for (size_t i = 0; i < m_nodeToLevel[0].size(); ++i)
        {
            if (isValidNode(i))
            {
                m_tableList[i].resize(m_nodeToLevel[0].size());
                for (size_t j = 0; j < m_nodeToLevel[0][i].size(); ++j)
                {
                    size_t lan = m_nodeToLevel[0][i][j];
                    for (size_t k = 0; k < m_levelMakeup[0][lan].size(); ++k)
                    {
                        size_t node = m_levelMakeup[0][lan][k];
                        if (i != node)
                        {
                            write(file, 'i', i, node,
                                  static_cast<float>(m_lanWeights[lan]));
                        }
                    }
                }
            }
        }
        write(file, 'C');

        int readCount = 0;
        int source = 0;
        int dest = 0;
        int firstHop = 0;
        int distance = 0;
        readCount = read(file, source, dest, firstHop, distance);
        while (readCount == 4)
        {
            if (isValidNode(source))
            {
                m_tableList[source][dest] = firstHop;
            }
            readCount = read(file, source, dest, firstHop, distance);
        }
    }
}

void HostRouter::print(ostream & output) const
{
    // Preprocessing. Figure out which IP addresses to use for all
    // adjascent interfaces. We have the first hop in terms of hosts,
    // and this structure will provide a link from the host to the interface
    // IP address.
    // This maps adjascent nodes to IP addresses.
    map<size_t, IPAddress> nodeToIP;

    output << "%%" << endl;
    for (size_t i = 0; i < m_tableList.size(); ++i)
    {
        if (m_tableList[i].size() != 0)
        {
            findAdjascentInterfaces(i, nodeToIP);
            printTable(output, i, nodeToIP);
        }
    }
}

void HostRouter::findAdjascentInterfaces(size_t node,
                                map<size_t, IPAddress> & nodeToIP) const
{
    nodeToIP.clear();
    // in every LAN that we're connected to
    vector<size_t>::const_iterator lanPos = m_nodeToLevel[0][node].begin();
    for (; lanPos != m_nodeToLevel[0][node].end(); ++lanPos)
    {
        // in every node that is in one of those LANs
        vector<size_t> const & firstHopList = m_levelMakeup[0][*lanPos];
        vector<size_t>::const_iterator pos = firstHopList.begin();
        for (; pos != firstHopList.end(); ++pos)
        {
            // Note that we'll be repeatedly overwriting an IPAddress for
            // the current node. But that is ok since the current node is
            // never used as a firstHop.
            // If this becomes an issue, put a conditional here.
            nodeToIP[*pos] = m_levelPrefix[0][*lanPos] + *pos + 1;
        }
    }
}


void HostRouter::printTable(ostream & output, size_t node,
                            map<size_t, IPAddress> & nodeToIP) const
{
    size_t lanCount = m_levelMakeup[0].size();
    // Print the routing table itself
    output << "Routing table for node: " << node << endl;
    for (size_t destLan = 0; destLan < lanCount; ++destLan)
    {
        for (size_t destNodeIndex = 0;
             destNodeIndex < m_levelMakeup[0][destLan].size();
             ++destNodeIndex)
        {
            size_t destNode = m_levelMakeup[0][destLan][destNodeIndex];

            // if the destination LAN is not connected to the current node.
            if (find(m_nodeToLevel[0][node].begin(),
                     m_nodeToLevel[0][node].end(), destLan)
                == m_nodeToLevel[0][node].end())
            {
                output << "Destination: "
                       << ipToString(m_levelPrefix[0][destLan] + destNodeIndex
                                                               + 1)
                       << "/32" // host routing means no subnets
                       << " FirstHop: "
                       << ipToString(
                           nodeToIP[m_tableList[node][destNode]])
                       << endl;
            }
        }
    }
    // Use '%%' to divide routing tables from another.
    output << "%%" << endl;
}
