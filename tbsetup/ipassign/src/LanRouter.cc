// LanRouter.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "lib.h"
#include "Exception.h"
#include "bitmath.h"
#include "LanRouter.h"
#include "coprocess.h"

using namespace std;

LanRouter::LanRouter()
    : routeCount(0)
{
}

LanRouter::~LanRouter()
{
}

auto_ptr<Router> LanRouter::clone(void) const
{
    return auto_ptr<Router>(new LanRouter(*this));
}

void LanRouter::calculateRoutes(void)
{
    size_t i = 0;
    size_t j = 0;
    size_t k = 0;
    Assigner::LevelLookup & lanToNode = m_levelMakeup[0];
    Assigner::NodeLookup & nodeToLan = m_nodeToLevel[0];
    size_t lanCount = lanToNode.size();
    size_t nodeCount = nodeToLan.size();

    // setup the routing table.
    routingTable.clear();
    routingTable.resize(nodeCount);

    // Create a vector of maps which yields a node for each pair of LANs which
    // are adjascent. This node is one of the nodes which connects them.
    vector< map<size_t,size_t> > lanConnectionList;
    lanConnectionList.resize(lanCount);
    for (i = 0; i < nodeCount; ++i)
    {
        // Each node connects every pair of LANs it touches together with
        // each other.
        connectLans(nodeToLan[i], i, lanConnectionList);
    }

    // vector< firstHop, weight >
    vector< vector< pair<size_t, int> > > lanShortestPaths;
    lanShortestPaths.resize(lanCount);

    // calculate lan shortest paths
    {
        FileWrapper file(coprocess(ROUTECALC).release());
        for (i = 0; i < lanCount; ++i)
        {
            lanShortestPaths[i].resize(lanCount);
            for (j = 0; j < lanToNode[i].size(); ++j)
            {
                size_t node = lanToNode[i][j];
                for (k = 0; k < nodeToLan[node].size(); ++k)
                {
                    size_t lan = nodeToLan[node][k];
                    if (i != lan)
                    {
                        write(file, 'i', i, lan, m_lanWeights[lan]);
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
            lanShortestPaths[source][dest] = make_pair(firstHop, distance);
            readCount = read(file, source, dest, firstHop, distance);
        }
    }
    // For every lan, run Dijkstra's algorithm.
    for (i = 0; i < lanCount; ++i)
    {
        calculateRoutesFrom(i, lanConnectionList, lanToNode, nodeToLan,
                            lanShortestPaths[i]);
    }

    // Now that we know the best paths between LANs, go through every node
    for (i = 0; i < nodeCount; ++i)
    {
        if (isValidNode(i))
        {
            vector<size_t> & currentNode = nodeToLan[i];
            // set up the routing table
            routingTable[i].resize(lanCount);
            for (j = 0; j < lanCount; ++j)
            {
                // and determine which node is the first on the path to that
                // LAN.
                routingTable[i][j] = chooseLan(i, j, nodeToLan[i],
                                               lanConnectionList,
                                               lanShortestPaths);
            }
        }
    }
}

void LanRouter::calculateRoutesFrom(
                   size_t lanNumber,
                   std::vector< std::map<size_t,size_t> > const & connections,
                   Assigner::LevelLookup const & lanToNode,
                   Assigner::NodeLookup const & nodeToLan,
                   std::vector< std::pair<size_t, int> > & shortestPaths)
{
    // Here is our default 'infinity' value use for Dijkstra.
    static const int MAX_DISTANCE = INT_MAX;

    // Set up the array which will hold the Lan which is the first hop
    // towards the destination
    vector<size_t> firstHop;
    firstHop.resize(lanToNode.size());
    fill(firstHop.begin(), firstHop.end(), 0);
    firstHop[lanNumber] = lanNumber;

    // This array will hold the distances to every node from lanNumber
    vector<int> distance;
    distance.resize(lanToNode.size());
    fill(distance.begin(), distance.end(), MAX_DISTANCE);
    distance[lanNumber] = 0;

    // This array holds the distances to every node, but each node which
    // has already been selected is 'infinity'.
    vector<int> unselectedDistance;
    unselectedDistance.resize(lanToNode.size());
    fill(unselectedDistance.begin(), unselectedDistance.end(), MAX_DISTANCE);
    unselectedDistance[lanNumber] = 0;


    // As long as minPos is not pointing to something that is infinity,
    // there is at least one unselected node.
    vector<int>::iterator minPos = min_element(unselectedDistance.begin(),
                                               unselectedDistance.end());
    while (*minPos != MAX_DISTANCE)
    {
        // Get the index using operator- on random access iterators.
        size_t minIndex = minPos - unselectedDistance.begin();
        // Set the selected node to infinity
        unselectedDistance[minIndex] = MAX_DISTANCE;
        vector<size_t> const & currentLan = lanToNode[minIndex];
        // For every node in the selected lan
        for (size_t i = 0; i < currentLan.size(); ++i)
        {
            vector<size_t> const & currentNode = nodeToLan[currentLan[i]];
            // For every Lan that node connects too
            for (size_t j = 0; j < currentNode.size(); ++j)
            {
                // If the Lan is not the selected Lan
                if (currentNode[j] != minIndex)
                {
                    // figure out what the new distance is if reached through
                    // the current node
                    int currentDistance = distance[minIndex]
                        + m_lanWeights[currentNode[j]];
                    // If the new distance is cheaper than the old distance
                    if (currentDistance < distance[currentNode[j]])
                    {
                        // Reset the distance.
                        distance[currentNode[j]] = currentDistance;
                        unselectedDistance[currentNode[j]] = currentDistance;
                        // If the selected Lan is the same as lanNumber
                        // that means that this is the first iteration
                        if (minIndex == lanNumber)
                        {
                            // On the first iteration, we are just looking
                            // at the Lans adjascent to the first Lan.
                            // In this case, the first hop to each of these
                            // Lans is simply the Lan itself.
                            firstHop[currentNode[j]] = currentNode[j];
                        }
                        else
                        {
                            // On other iterations, we are looking at
                            // distant relatives. In this instance, the
                            // first hop to the adjascent node is the
                            // same as the first hop to the current node.
                            firstHop[currentNode[j]] = firstHop[minIndex];
                        }
                    }
                }
            }
        }

        minPos = min_element(unselectedDistance.begin(),
                             unselectedDistance.end());
    }


    shortestPaths.resize(lanToNode.size());
    for (size_t i = 0; i < shortestPaths.size(); ++i)
    {
        shortestPaths[i] = make_pair(firstHop[i], distance[i]);
    }
}

void LanRouter::connectLans(std::vector<size_t> const & currentNode, size_t i,
                 std::vector< std::map<size_t,size_t> > & connectionList)
{
    for (size_t j = 0; j < currentNode.size(); ++j)
    {
        for (size_t k = 0; k < currentNode.size(); ++k)
        {
            if (currentNode[j] != currentNode[k])
            {
                connectionList[currentNode[j]][currentNode[k]] = i;
            }
        }
    }
}

std::pair<size_t,size_t> LanRouter::chooseLan(size_t nodeNumber,
                                              size_t lanNumber,
                        vector<size_t> const & currentNode,
                        vector< map<size_t,size_t> > const & connection,
                        vector<vector< pair<size_t, int> > > const & shortest)
{
    // Note that the current library doesn't have the numeric_limits
    // library that is mandated by the standard. Therefore, we
    // have to limp along using C limits and hope they work.

    // nodeNumber is the node which we are calculating a route from.
    // currentNode is a mapping between that node and the LANs which it
    //      connects to.
    // connection takes two LANs and returns the node that they share.
    // shortest is a list of the shortest paths between the LAN we are
    // routing to and all other LANs. The first part of the pair is the
    // first hop, and the second is the length of the path.

    // We use all of this information to determine the route from a
    // particular node to a particular LAN.

    size_t bestLan = UINT_MAX;
    size_t bestNode = UINT_MAX;
    size_t bestWeight = UINT_MAX;
    // For every LAN that the source node connects to...
    for (size_t i = 0; i < currentNode.size(); ++i)
    {
        size_t firstHopCandidate = currentNode[i];
        // calculate the weight to the destination
        int currentWeight = shortest[lanNumber][firstHopCandidate].second
            + m_lanWeights[lanNumber];
        if (currentWeight < bestWeight)
        {
            // Find a node which the two LANs share. This is the first hop
            // to get to the destination LAN.
            map<size_t,size_t>::const_iterator pos;
            pos = connection[firstHopCandidate].find(
                            shortest[firstHopCandidate][lanNumber].first);
            if (currentWeight == m_lanWeights[firstHopCandidate])
            // shortest[currentNode[i]].first == currentNode[i])
            {
                bestLan = firstHopCandidate;
                bestNode = nodeNumber;
                bestWeight = 0;
            }
            else if (pos != connection[firstHopCandidate].end())
            {
                bestLan = firstHopCandidate;
                bestNode = pos->second;
                bestWeight = currentWeight;
            }
        }
    }
    return make_pair(bestNode, bestLan);
}

void LanRouter::print(ostream & output) const
{
    routeCount = 0;
    output << "%%" << endl;
    for (size_t i = 0; i < routingTable.size(); ++i)
    {
        if (isValidNode(i))
        {
            output << "Routing table for node: " << i << endl;
            vector< pair<size_t,size_t> > const & currentRoutes
                                                         = routingTable[i];

            for (size_t j = 0; j < currentRoutes.size(); ++j)
            {
                size_t firstNode = currentRoutes[j].first;
                size_t firstLan = currentRoutes[j].second;
                if (firstNode != i)
                {
                    // The first node on the route is some other node
                    // so we've got work to do.
                    vector<size_t>::const_iterator pos;
                    // To find the IPAddress of the firstNode,
                    // we must find its position in the firstLan.
                    pos = find(m_levelMakeup[0][firstLan].begin(),
                               m_levelMakeup[0][firstLan].end(),
                               firstNode);
                    if (pos != m_levelMakeup[0][firstLan].end())
                    {
                        IPAddress increment = 1;
                        increment += pos - m_levelMakeup[0][firstLan].begin();
                        // Output the first node
                        output << "Destination: "
                               << ipToString(m_levelPrefix[0][j])
                               << "/" << m_levelMaskSize[0][j]
                               << " FirstHop: "
                               << ipToString(m_levelPrefix[0][firstLan]
                                             + increment)
                               << endl;
                        ++routeCount;
                    }

                }
                else
                {
                    // The first node on the route is us.
                    // This means that we are directly adjascent to this LAN.
                    // which means we do nothing.
                }
            }
            output << "%%" << endl;
        }
    }
}

void LanRouter::printStatistics(ostream & output) const
{
    size_t hostCount = m_nodeToLevel[0].size();
    output << "Total Number of Routes: " << routeCount << endl;

    // calculate route length from every node to every interface.
    // This should be equivalent to calculating a route length from every
    // interface to every node.
    int totalRouteLength = 0;
    // for each node
    for (size_t i = 0; i < hostCount; ++i)
    {
        if (isValidNode(i))
        {
            // for each LAN in the graph
            for (size_t j = 0; j < m_levelMakeup.size(); ++j)
            {
                totalRouteLength += distanceToLan(i, j);
            }
        }
    }
    output << "Total Route Length: " << totalRouteLength << endl;
}

int LanRouter::distanceToLan(size_t sourceNode, size_t destLan) const
{
    int routeLength = 0;
    // find a path length to that LAN.
    size_t current = sourceNode;
    int length = 0;
    while (current != routingTable[current][destLan].first)
    {
        length += m_lanWeights[routingTable[current][destLan].second];
        current = routingTable[current][destLan].first;
    }
    // for every node in that LAN
    for (size_t i = 0; i < m_levelMakeup[0][destLan].size(); ++i)
    {
        // if the destination node is not the source node
        if (sourceNode != i)
        {
            // add the path length to that interface to the
            // total route length
            routeLength += length;
        }
    }
    return routeLength;
}
