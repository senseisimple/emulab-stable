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
//    return auto_ptr<Router>(new HostRouter(*this));
    return auto_ptr<Router>(NULL);
}

void HostRouter::calculateRoutes(void)
{
    if (hosts.size() != 0 && lans.size() != 0)
    {
        FileWrapper file(coprocess(ROUTECALC).release());
        m_tableList.clear();
        m_tableList.resize(hosts.size());
        for (size_t i = 0; i < hosts.size(); ++i)
        {
            if (isValidNode(i))
            {
                m_tableList[i].resize(hosts.size());
                for (size_t j = 0; j < hosts[i].size(); ++j)
                {
                    size_t lan = hosts[i][j];
                    for (size_t k = 0; k < lans[lan].hosts.size(); ++k)
                    {
                        size_t node = lans[lan].hosts[k];
                        if (i != node)
                        {
                            write(file, 'i', i, node,
                                  static_cast<float>(lans[lan].weight));
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
    vector<size_t>::const_iterator lanPos = hosts[node].begin();
    for (; lanPos != hosts[node].end(); ++lanPos)
    {
        // in every node that is in one of those LANs
        vector<size_t> const & firstHopList = hosts[*lanPos];
        vector<size_t>::const_iterator pos = firstHopList.begin();
        for (; pos != firstHopList.end(); ++pos)
        {
            // Note that we'll be repeatedly overwriting an IPAddress for
            // the current node. But that is ok since the current node is
            // never used as a firstHop.
            // If this becomes an issue, put a conditional here.
            nodeToIP[*pos] = lans[*lanPos].partition->getPrefix() + *pos + 1;
        }
    }
}


void HostRouter::printTable(ostream & output, size_t node,
                            map<size_t, IPAddress> & nodeToIP) const
{
    size_t lanCount = lans.size();
    // Print the routing table itself
    output << "Routing table for node: " << node << endl;
    for (size_t destLan = 0; destLan < lanCount; ++destLan)
    {
        for (size_t destNodeIndex = 0;
             destNodeIndex < lans[destLan].hosts.size();
             ++destNodeIndex)
        {
            size_t destNode = lans[destLan].hosts[destNodeIndex];

            // if the destination LAN is not connected to the current node.
            if (find(hosts[node].begin(),
                     hosts[node].end(), destLan)
                == hosts[node].end())
            {
                output << "Destination: "
                       << ipToString(lans[destLan].partition->getPrefix()
                                     + destNodeIndex + 1)
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
