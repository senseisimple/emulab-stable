// NetRouter.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "lib.h"
#include "Exception.h"
#include "NetRouter.h"
#include "bitmath.h"
#include "coprocess.h"

using namespace std;

NetRouter::NetRouter()
{
}

NetRouter::~NetRouter()
{
}

auto_ptr<Router> NetRouter::clone(void) const
{
    return auto_ptr<Router>(new NetRouter(*this));
}

void NetRouter::calculateRoutes(void)
{
    size_t i = 0;
    size_t j = 0;
    size_t k = 0;

    setup();

    // for each level of the hierarchy, starting at one level above LANs
    for (i = 1; i < m_levelMakeup.size(); ++i)
    {
        Assigner::LevelLookup & currentLevel = m_levelMakeup[i];
        oldNodeToBorder.swap(newNodeToBorder);
        newNodeToBorder.clear();
        newNodeToBorder.resize(m_levelMakeup[i].size());
        // Getting rid of connections to LANs that we don't care about anymore
        // is just an efficiency thing. TODO: do this if we care.
        oldBorderLans.swap(newBorderLans);
        newBorderLans.clear();
        newBorderLans.resize(m_levelMakeup[i].size());

        // Set up a vector to determine which border lans are in which
        // partition.
        fill(lanToPartition.begin(), lanToPartition.end(), UINT_MAX);
        for (j = 0; j < currentLevel.size(); ++j)
        {
            for (k = 0; k < currentLevel[j].size(); ++k)
            {
                vector<size_t> & borders = oldBorderLans[currentLevel[j][k]];
                for (size_t l = 0; l < borders.size(); ++l)
                {
                    lanToPartition[borders[l]] = j;
                }
            }
        }

         // for each partition in the current level
        for (j = 0; j < currentLevel.size(); ++j)
        {
            // Determine the border LANs for that partition
            calculateBorders(i, j);
            // For each border LAN, calculate the routes to all of the other
            // border LANs in this partition.
            calculateBorderToBorderPaths(i, j);
            // for each node in the partition, find the shortest routes
            // and first hops to each border in the partition.
            calculateNodeToBorderPaths(i, j);
            // for each node in the partition, and each subnet in
            // the partition, find a route from that node to that subnet
            calculatePartitionRoutes(i, j);
        }
    }
}

void NetRouter::setup(void)
{
    size_t i = 0;
    size_t j = 0;
    size_t k = 0;
    // check execution conditions
    if (m_nodeToLevel.size() == 0 || m_levelMaskSize.size() == 0
        || m_levelPrefix.size() == 0 || m_levelMakeup.size() == 0
        || m_lanWeights.size() == 0)
    {
        throw NoGraphToRouteException("");
    }

    // the top level should contain only a single partition
    if (m_levelMakeup.back().size() > 1)
    {
        throw NoTopLevelException("");
    }

    size_t total = 0;
    for (i = 0; i < m_levelMakeup[0].size(); ++i)
    {
        for (j = 0; j < m_levelMakeup[0][i].size(); ++j)
        {
            size_t node = m_levelMakeup[0][i][j];
            total += m_nodeToLevel[0][node].size() - 1;
        }
    }
    double result = static_cast<double>(total) / m_levelMakeup[0].size();
    cerr << "Average degree: " << result << endl;

/*    if (m_nodeToLevel[1][859].size() == 2)
    {
        cerr << "859 partitions: " << m_nodeToLevel[1][859][0] << " " << m_nodeToLevel[1][859][1] << endl;
        for (size_t i = 0; i < 2; ++i)
        {
            size_t currentPartition = m_nodeToLevel[1][859][i];
            cerr << "Lans in " << i << ": ";
            for (size_t j = 0; j < m_levelMakeup[1][currentPartition].size(); ++j)
            {
                cerr << m_levelMakeup[1][currentPartition][j] << " ";
            }
            cerr << endl;
        }
        }*/

    borderConnections.clear();
    reverseBorderConnections.clear();

    // Set initial data structures for LAN level

    // lanToCurrentLevel tells which partition on the current level a
    // LAN is.
    lanToPartition.clear();
    lanToPartition.resize(m_levelMakeup[0].size());

    // oldBorderLans is initialized implicitly by the first run-through
    // of the loop.
    newBorderLans.clear();
    // set up the border LANs for the LAN level. Basically,
    // every LAN is its own partition and therefore each LAN
    // is a border node for its own partition.
    newBorderLans.resize(m_levelMakeup[0].size());
    for (i = 0; i < m_levelMakeup[0].size(); ++i)
    {
        newBorderLans[i].push_back(i);
    }

    // oldNodeToBorder is initialized implicitly by the first iteration
    newNodeToBorder.clear();
    newNodeToBorder.resize(m_levelMakeup[0].size());
    for (i = 0; i < m_levelMakeup[0].size(); ++i)
    {
        RouteInfo temp;
        temp.firstLan = i;
        temp.distance = 0;
        temp.destination = i;
        for (j = 0; j < m_levelMakeup[0][i].size(); ++j)
        {
            size_t currentNode = m_levelMakeup[0][i][j];
            temp.firstNode = currentNode;
            newNodeToBorder[i][make_pair(currentNode,i)] = temp;
        }
    }

    nodeToPartition.clear();
    nodeToPartition.resize(m_nodeToLevel.size());
    for (size_t i = 0; i < nodeToPartition.size(); ++i)
    {
        nodeToPartition[i].resize(m_nodeToLevel[i].size());
    }

    // Set up lanLanNode structure
    for (i = 0; i < m_levelMakeup[0].size(); ++i)
    {
        for (j = 0; j < m_levelMakeup[0][i].size(); ++j)
        {
            size_t currentNode = m_levelMakeup[0][i][j];
            for (k = 0; k < m_nodeToLevel[0][currentNode].size(); ++k)
            {
                size_t destLan = m_nodeToLevel[0][currentNode][k];
                if (destLan != i)
                {
                    lanLanNode[make_pair(i, destLan)] = currentNode;
                    // The connection (destLan, i) is done implicitly
                }
            }
        }
    }

    // Determines which pairs (or more) of LANs are connected
    // and add these to the connection list.
    calculateInterBorders();
}

void NetRouter::calculateBorders(size_t level, size_t partition)
{
    for (size_t i = 0; i < m_levelMakeup[level][partition].size(); ++i)
    {
        size_t oldPartition = m_levelMakeup[level][partition][i];
        for (size_t j = 0; j < oldBorderLans[oldPartition].size(); ++j)
        {
            size_t currentLanIndex = oldBorderLans[oldPartition][j];
            if (isBorderLan(level, currentLanIndex))
            {
                newBorderLans[partition].push_back(currentLanIndex);
            }
        }
    }
}

bool NetRouter::isBorderLan(size_t level, size_t lan) const
{
    bool isBorder = false;
    for (size_t i = 0; i < m_levelMakeup[0][lan].size() && !isBorder; ++i)
    {
        size_t node = m_levelMakeup[0][lan][i];
        isBorder = isBorder || m_nodeToLevel[level][node].size() > 1;
    }
/*    if (isBorder && level == 1)
    {
        cerr << "IsBorderLan " << lan << ": ";
        for (size_t i = 0; i < m_levelMakeup[0][lan].size(); ++i)
        {
            size_t node = m_levelMakeup[0][lan][i];
            cerr << node << ':' << m_nodeToLevel[level][node].size() << ' ';
        }
        cerr << endl;
        }*/
    return isBorder;
}

namespace
{
    template<typename T>
    vector<T> & operator+=(vector<T> & left, vector<T> const & right)
    {
        size_t oldLeftSize = left.size();
        left.resize(left.size() + right.size());
        copy(right.begin(), right.end(), left.begin() + oldLeftSize);
        return left;
    }
}

void NetRouter::calculateBorderToBorderPaths(size_t level, size_t partition)
{
    // The routecalc program requires a contiguous block of indices starting
    // at 0.
    vector<size_t> indexToLan;
    map<size_t, int> lanToIndex;
    for (size_t i = 0; i < lanToPartition.size(); ++i)
    {
        if (lanToPartition[i] == partition)
        {
            lanToIndex[i] = static_cast<int>(indexToLan.size());
            indexToLan.push_back(i);
        }
    }
    cerr << ".";
    FileWrapper file(coprocess("bin/routecalc").release());
    char command = 'i';
    for (size_t i = 0; i < m_levelMakeup[level][partition].size(); ++i)
    {
        size_t currentSubPartition = m_levelMakeup[level][partition][i];
        for (size_t j = 0; j < oldBorderLans[currentSubPartition].size(); ++j)
        {
            size_t currentBorder = oldBorderLans[currentSubPartition][j];
            ConnectionMap::iterator pos = borderBegin(currentBorder);
            ConnectionMap::iterator limit = borderEnd(currentBorder);
            for ( ; pos != limit; ++pos)
            {
                if (lanToPartition[pos->first.first]
                    == lanToPartition[pos->first.second])
                {
                    write(file, 'i', lanToIndex[pos->first.first],
                          lanToIndex[pos->first.second],
                          static_cast<float>(pos->second.second));
                }
            }
        }
    }
    cerr << "W" << lanToIndex.size();
    write(file, 'C');

    cerr << "C";

    int readCount = 0;
    int source = 0;
    int dest = 0;
    int firstHop = 0;
    int distance = 0;
    readCount = read(file, source, dest, firstHop, distance);
    cerr << "R";
    // There should always be 4 items read and assigned. If there are
    // more or less or EOF happens, we should stop.
    while (readCount == 4)
    {
        size_t sourceSize = indexToLan[source];
        size_t destSize = indexToLan[dest];
        size_t firstHopSize = indexToLan[firstHop];
        addBorderConnection(sourceSize, destSize,
                            getBorderFirstHop(sourceSize, firstHopSize),
                            distance);
        readCount = 0;
        readCount = read(file, source, dest, firstHop, distance);
    }
    cerr << "+\t";
}

void NetRouter::calculateNodeToBorderPaths(size_t level, size_t partition)
{
    for (size_t i = 0; i < m_levelMakeup[level][partition].size(); ++i)
    {
        size_t oldPartition = m_levelMakeup[level][partition][i];
        // We want to send the range of [(x,0)-(x,MAX)] at a time to
        // findNodeToBorder to be searched. Therefore we start at the
        // beginning. Each call to upper_limit() brings us one past
        // the current range (and the first one of the next)
        // and only when there is no next range (end()) do we stop.
        NodeBorderMap::iterator pos = oldNodeToBorder[oldPartition].begin();
        NodeBorderMap::iterator limit;
        while(pos != oldNodeToBorder[oldPartition].end())
        {
            limit = oldNodeToBorder[oldPartition].
                upper_bound(make_pair(pos->first.first, UINT_MAX));
            for (size_t j = 0; j < newBorderLans[partition].size(); ++j)
            {
                findNodeToBorder(pos, limit, pos->first.first, partition,
                                 newBorderLans[partition][j]);
            }
            pos = limit;
        }
    }
}

void NetRouter::findNodeToBorder(NodeBorderMap::iterator searchPos,
                                 NodeBorderMap::iterator searchLimit,
                                 size_t sourceNode, size_t newPartition,
                                 size_t destinationLan)
{
    static const int MAX_DISTANCE = INT_MAX;

    RouteInfo & bestRoute =
        newNodeToBorder[newPartition][make_pair(sourceNode, destinationLan)];
    bestRoute.destination = destinationLan;
    for ( ; searchPos != searchLimit; ++searchPos)
    {
        if (searchPos->second.destination != destinationLan)
        {
            int currentDistance = searchPos->second.distance +
                getBorderWeight(searchPos->second.destination, destinationLan);
            if (currentDistance < bestRoute.distance)
            {
                bestRoute.distance = currentDistance;
                bestRoute.firstLan = searchPos->second.firstLan;
                if (searchPos->second.firstNode != sourceNode)
                {
                    // Since the node which is the first hop is
                    // not equal to ourselves, then we are not directly
                    // adjascent to this border node. Therefore since
                    // the shortest path leads through this LAN, our
                    // first hop node to the destination is the same
                    // as the first hop node to this LAN.
                    bestRoute.firstNode = searchPos->second.firstNode;
                }
                else
                {
                    // The node is directly adjascent to the old border LAN.
                    // We can use the 'getBorderFirstHop'
                    // function to find the next LAN on the path, and
                    // use the two LANs to determine the node between
                    // them.
                    size_t firstLan = searchPos->second.destination;
                    size_t secondLan = getBorderFirstHop(firstLan,
                                                         destinationLan);
                    bestRoute.firstNode = lanLanNode[make_pair(firstLan,
                                                               secondLan)];
                }
            }
        }
        else
        {
            bestRoute = searchPos->second;
        }
    }
}

void NetRouter::calculatePartitionRoutes(size_t level, size_t partition)
{
    // for each node in the partition, sorted by sub-partition
    for (size_t i = 0; i < m_levelMakeup[level][partition].size(); ++i)
    {
        size_t sourcePartition = m_levelMakeup[level][partition][i];
        // see calculateNodeToBorderPaths for an explanation of this looping
        // idiom.
        NodeBorderMap::iterator pos = oldNodeToBorder[sourcePartition].begin();
        NodeBorderMap::iterator limit;
        while (pos != oldNodeToBorder[sourcePartition].end())
        {
            limit = oldNodeToBorder[sourcePartition].upper_bound(
                make_pair(pos->first.first, UINT_MAX));
            // for each sub-partition in the partition
            for (size_t j = 0; j < m_levelMakeup[level][partition].size(); ++j)
            {
                size_t destPartition = m_levelMakeup[level][partition][j];
                NodeBorderMap::iterator foundBegin;
                NodeBorderMap::iterator foundEnd;
                foundBegin = oldNodeToBorder[destPartition].lower_bound(
                    make_pair(pos->first.first, 0));
                foundEnd = oldNodeToBorder[destPartition].upper_bound(
                    make_pair(pos->first.first, UINT_MAX));
                // if the node is not part of the destination partition
                if (foundBegin == foundEnd)
                {
                    // find a route from that node to this partition.
                    findRouteToPartition(pos, limit, pos->first.first,
                                         sourcePartition, destPartition,
                                         level);
                }
            }
            pos = limit;
        }
    }
}

void NetRouter::findRouteToPartition(NodeBorderMap::iterator searchPos,
                                     NodeBorderMap::iterator searchLimit,
                                     size_t sourceNode, size_t sourcePartition,
                                     size_t destPartition, size_t level)
{
    map<size_t,RouteInfo> & currentRoute =
                                        nodeToPartition[level-1][sourceNode];

    map<size_t,RouteInfo>::iterator found = currentRoute.find(destPartition);
    if (found == currentRoute.end())
    {
        found = currentRoute.insert(make_pair(destPartition, RouteInfo()))
                                                                        .first;
    }

    // for every border in the sub-partition of the sourceNode
    for ( ; searchPos != searchLimit; ++searchPos)
    {
        // for every border in the destination sub-partition
        for (size_t i = 0; i < oldBorderLans[destPartition].size(); ++i)
        {
            // If the route using those two borders is shorter than the
            // current route
            size_t destLan = oldBorderLans[destPartition][i];
            int currentDistance = searchPos->second.distance +
                m_lanWeights[searchPos->second.firstLan] +
                getBorderWeight(searchPos->second.destination,
                                destLan);
            if (currentDistance < found->second.distance)
            {
                // replace the current route with the new route,
                found->second.firstNode = searchPos->second.firstNode;
                found->second.firstLan = searchPos->second.firstLan;
                // our destination is the border lan of the destination
                // sub-partition
                found->second.destination = destLan;
                // making sure that we use the new distance
                found->second.distance = currentDistance;
                if (searchPos->second.firstNode == sourceNode)
                {
                    // The node is part of the border LAN.
                    size_t firstLan = searchPos->second.firstLan;
                    size_t secondLan = getBorderFirstHop(firstLan, destLan);
                    found->second.firstNode = lanLanNode[make_pair(firstLan,
                                                                   secondLan)];
                }
            }
        }
    }
}

void NetRouter::calculateInterBorders(void)
{
    for (size_t i = 0; i < m_nodeToLevel[0].size(); ++i)
    {
        for (size_t j = 0; j < m_nodeToLevel[0][i].size(); ++j)
        {
            size_t currentSource = m_nodeToLevel[0][i][j];
            for (size_t k = j; k < m_nodeToLevel[0][i].size(); ++k)
            {
                size_t currentDest = m_nodeToLevel[0][i][k];
                if (currentSource == currentDest)
                {
                    addBorderConnection(currentSource, currentDest,
                                        currentSource, 0);
                }
                else
                {
                    addBorderConnection(currentSource, currentDest,
                                        currentDest,
                                        m_lanWeights[currentDest]);
                    addBorderConnection(currentDest, currentSource,
                                        currentSource,
                                        m_lanWeights[currentSource]);
                }
            }
        }
    }
}

void NetRouter::print(std::ostream & output) const
{
    output << "%%" << endl;
    size_t nodeCount = m_nodeToLevel[0].size();
    size_t levelCount = m_nodeToLevel.size() - 1;
    for (size_t i = 0; i < nodeCount; ++i)
    {
        output << "Routing table for node: " << i << endl;
        for (size_t j = 0; j < levelCount; ++j)
        {
            map<size_t,RouteInfo>::const_iterator pos;
            pos = nodeToPartition[j][i].begin();
            map<size_t,RouteInfo>::const_iterator limit;
            limit = nodeToPartition[j][i].end();
            for ( ; pos != limit; ++pos)
            {
                if (i == 22 && (pos->first == 3 || pos->first == 2))
                {
//                    cerr << "destPartition: " << pos->first << " firstNode: " << pos->second.firstNode << " firstLan " << pos->second.firstLan << endl;
                }
                if (pos->second.firstNode != i)
                {
                    vector<size_t>::const_iterator found;
                   found = find(m_levelMakeup[0][pos->second.firstLan].begin(),
                                 m_levelMakeup[0][pos->second.firstLan].end(),
                                 pos->second.firstNode);
                    if (found != m_levelMakeup[0][pos->second.firstLan].end())
                    {
                        IPAddress increment = 1;
                        increment += found -
                            m_levelMakeup[0][pos->second.firstLan].begin();
                        output << "Destination: "
                               << ipToString(m_levelPrefix[j][pos->first])
                               << "/"
                               << m_levelMaskSize[j][pos->first]
                               << " FirstHop: "
                               << ipToString(m_levelPrefix[0]
                                             [pos->second.firstLan]
                                             + increment)
                               << endl;
                    }
                }
                else
                {
                    // if the first node on the route is the originating
                    // node, then that node is adjascent to the destination
                    // hence no routing at this level.
                }
            }
        }
        output << "%%" << endl;
    }
}

void NetRouter::printStatistics(std::ostream & output) const
{
}

void NetRouter::addBorderConnection(size_t first, size_t second,
                                    size_t firstHop, int weight)
{
    borderConnections[make_pair(first, second)] = make_pair(firstHop, weight);
    reverseBorderConnections[make_pair(second, first)] =
                                                 make_pair(firstHop, weight);
}

void NetRouter::removeBorderConnection(size_t first, size_t second)
{
    borderConnections.erase(make_pair(first, second));
    reverseBorderConnections.erase(make_pair(second, first));
}

void NetRouter::removeBorderLan(size_t lan)
{
    ConnectionMap::iterator start;
    ConnectionMap::iterator finish;
    start = borderConnections.lower_bound(make_pair(lan, 0));
    finish = borderConnections.upper_bound(make_pair(lan, UINT_MAX));
    for ( ; start != finish; ++start)
    {
        reverseBorderConnections.erase(make_pair(start->first.second,
                                                 start->first.first));
    }

    start = reverseBorderConnections.lower_bound(make_pair(lan, 0));
    finish = reverseBorderConnections.upper_bound(make_pair(lan, UINT_MAX));
    for ( ; start != finish; ++start)
    {
        borderConnections.erase(make_pair(start->first.second,
                                          start->first.first));
    }
}

size_t NetRouter::getBorderInfo(size_t first, size_t second,
                                bool getWeight) const
{
    size_t result = 0;
    ConnectionMap::const_iterator pos;
    pos = borderConnections.find(make_pair(first, second));
    if (pos != borderConnections.end())
    {
        if (getWeight)
        {
            result = static_cast<size_t>(pos->second.second);
        }
        else
        {
            result = pos->second.first;
        }
    }
    else
    {
        ostringstream buffer;
        buffer << "From " << first << " To " << second;
        throw NoConnectionException(buffer.str());
    }
    return result;
}

int NetRouter::getBorderWeight(size_t first, size_t second)
    const
{
    return static_cast<int>(getBorderInfo(first, second, true));
}

size_t NetRouter::getBorderFirstHop(size_t first, size_t second) const
{
    return getBorderInfo(first, second, false);
}

NetRouter::ConnectionMap::iterator NetRouter::borderBegin(size_t index)
{
    return borderConnections.lower_bound(make_pair(index, 0));
}

NetRouter::ConnectionMap::const_iterator NetRouter::borderBegin(size_t index)
    const
{
    return borderConnections.lower_bound(make_pair(index, 0));
}

NetRouter::ConnectionMap::iterator NetRouter::borderEnd(size_t index)
{
    return borderConnections.upper_bound(make_pair(index, UINT_MAX));
}

NetRouter::ConnectionMap::const_iterator NetRouter::borderEnd(size_t index)
    const
{
    return borderConnections.upper_bound(make_pair(index, UINT_MAX));
}
