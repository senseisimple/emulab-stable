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
    : routeCount(0)
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
    routeCount = 0;
    // Use '%%' to divide routing tables from another.
    output << "%%" << endl;
    for (size_t i = 0; i < m_tableList.size(); ++i)
    {
        if (m_tableList[i].size() != 0)
        {
            // Preprocessing. Figure out which IP addresses to use for all
            // adjascent interfaces.
            // This maps adjascent nodes to IP addresses.
            map<size_t, IPAddress> NodeToIP;

            // in every LAN that we're connected to
            vector<size_t>::const_iterator lanPos =
                                                 m_nodeToLevel[0][i].begin();
            for (; lanPos != m_nodeToLevel[0][i].end(); ++lanPos)
            {
                // in every node that is in one of those LANs
                size_t pos = 0;
                for (; pos < m_levelMakeup[0][*lanPos].size(); ++pos)
                {
                    NodeToIP[m_levelMakeup[0][*lanPos][pos]]
                        = m_levelPrefix[0][*lanPos] + pos + 1;
                }
            }

            // Print the routing table itself
            output << "Routing table for node: " << i << endl;
            for (size_t j = 0; j < m_levelMakeup[0].size(); ++j)
            {
                for (size_t k = 0; k < m_levelMakeup[0][j].size(); ++k)
                {
                    if (NodeToIP.count(m_levelMakeup[0][j][k]) == 0
                        || find(m_nodeToLevel[0][i].begin(),
                                m_nodeToLevel[0][i].end(), j)
                        == m_nodeToLevel[0][i].end())
                    {
                    output << "Destination: "
                           << ipToString(m_levelPrefix[0][j] + k + 1)
                           << "/32" // host routing means no subnets
                           << " FirstHop: "
                           << ipToString(
                              NodeToIP[m_tableList[i][m_levelMakeup[0][j][k]]])
                           << endl;
                    ++routeCount;
                    }
                }
            }
            output << "%%" << endl;
        }
    }
}

void HostRouter::printStatistics(ostream & output) const
{
    size_t i = 0;
    output << "Total Number of Routes: " << routeCount << endl;

    // calculate route length from every interface to every node
    // this should be equivalent to getting a route length from every node
    // to every interface.
    int totalRouteLength = 0;
    for (i = 0; i < m_levelMakeup[0].size(); ++i)
    {
        for (size_t j = 0; j < m_levelMakeup[0][i].size(); ++i)
        {
            for (size_t k = 0; k < m_tableList[j].size(); ++k)
            {
                size_t current = j;
                while (current != k)
                {
                    totalRouteLength += getWeight(current,
                                                  m_tableList[current][k]);
                    current = m_tableList[current][k];
                }
            }
        }
    }
    output << "Total Route Length: " << totalRouteLength << endl;
}

int HostRouter::getWeight(size_t first, size_t second) const
{
    // TODO. Fix this so that it reports weights accurately.
    return 1;
}


void HostRouter::calculateRoutesFrom(size_t node)
{
    static const int MAX_DISTANCE = INT_MAX;
    // The minimum distance discovered to every node
    vector<int> distance;
    distance.resize(m_nodeToLevel[0].size());
    fill(distance.begin(), distance.end(), MAX_DISTANCE);

    // The minimum distance to every node, except that
    // Nodes which have already been selected are MAX_DISTANCE
    vector<int> unselectedDistance;
    unselectedDistance.resize(m_nodeToLevel[0].size());
    fill(unselectedDistance.begin(), unselectedDistance.end(),
         MAX_DISTANCE);

    // Distance to ourselves is 0
    distance[node] = 0;
    unselectedDistance[node] = 0;

    // Keep track of the first hop used to get to minimum distance
    m_tableList[node].resize(m_nodeToLevel[0].size());
    m_tableList[node][node] = node;

    // Dijkstra
    size_t distanceIndex = minIndex(unselectedDistance.begin(),
                                    unselectedDistance.end());
    while (unselectedDistance[distanceIndex] != MAX_DISTANCE)
    {
        unselectedDistance[distanceIndex] = MAX_DISTANCE;
        // in every LAN that we're connected to
        vector<size_t>::iterator lanPos = m_nodeToLevel[0][distanceIndex].begin();
        for (; lanPos != m_nodeToLevel[0][distanceIndex].end(); ++lanPos)
        {
            // in every node that is in one of those LANs
            vector<size_t>::iterator pos = m_levelMakeup[0][*lanPos].begin();
            for (; pos != m_levelMakeup[0][*lanPos].end(); ++pos)
            {
                int newDistance = distance[distanceIndex]
                                + m_lanWeights[*lanPos];
                if (newDistance < distance[*pos])
                {
                    distance[*pos] = newDistance;
                    unselectedDistance[*pos] = newDistance;
                    // if this is the original node, we don't want to
                    // set the first hop to us. Rather, we should set
                    // the first hop to the node itself.
                    if (distanceIndex == node)
                    {
                        m_tableList[node][*pos] = *pos;
                    }
                    else
                    {
                        m_tableList[node][*pos] =
                                           m_tableList[node][distanceIndex];
                    }
                }
            }
        }
        distanceIndex = minIndex(unselectedDistance.begin(),
                                 unselectedDistance.end());
    }
}








