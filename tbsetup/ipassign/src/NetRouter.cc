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
    // TODO: This no longer works because of the aut_ptr<>.
//    return auto_ptr<Router>(new NetRouter(*this));
    return auto_ptr<Router>(NULL);
}

void NetRouter::visitBranch(ptree::Branch & target)
{
    if (target.getChild() != NULL)
    {
        target.getChild()->accept(*this);
    }

    list<size_t> obsoleteLans;
    list<ptree::Node *> subPartitionList;

    calculateBorders(target, obsoleteLans, subPartitionList);
    calculateBorderToBorderPaths(target, obsoleteLans);
    calculateNodeToBorderPaths(target, subPartitionList);
    calculatePartitionRoutes(target, subPartitionList);
    cleanupBorderLans(obsoleteLans, subPartitionList);

    if (target.getSibling() != NULL)
    {
        target.getSibling()->accept(*this);
    }
}

void NetRouter::visitLeaf(ptree::Leaf & target)
{
    // This Leaf contains a LAN which might be a border
    // TODO: Setup LAN stuff.
    // Set initial data structures for LAN level
    size_t lanIndex = target.getLanIndex();
    ptree::LeafLan & currentLan = lans[lanIndex];

    setInternalHosts(target);

    // set up the border LANs for the LAN level.
    vector<size_t> & borderList(borderLans[&target]);
    borderList.clear();
    borderList.push_back(lanIndex);

    // set up nodeToBorder for the LAN level.
    NodeBorderMap & borderMap(nodeToBorder[&target]);
    borderMap.clear();
    RouteInfo route;
    route.firstLan = lanIndex;
    route.distance = currentLan.weight;
    route.destination = lanIndex;
    for (size_t i = 0; i < currentLan.hosts.size(); ++i)
    {
        size_t hostIndex = currentLan.hosts[i];
        route.firstNode = hostIndex;
        borderMap[make_pair(hostIndex, lanIndex)] = route;
    }

    if (target.getSibling() != NULL)
    {
        target.getSibling()->accept(*this);
    }
}

void NetRouter::calculateRoutes(void)
{
    setup();

    if (tree.get() != NULL)
    {
        tree->accept(*this);
    }
    else
    {
        throw NoGraphToRouteException("");
    }

/*    // for each level of the hierarchy, starting at one level above LANs
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
            calculateBorders(i, j, obsoleteLans);
            // For each border LAN, calculate the routes to all of the other
            // border LANs in this partition.
            calculateBorderToBorderPaths(i, j);
            // for each node in the partition, find the shortest routes
            // and first hops to each border in the partition.
            calculateNodeToBorderPaths(i, j);
            // for each node in the partition, and each subnet in
            // the partition, find a route from that node to that subnet
            calculatePartitionRoutes(i, j);
            removeObsoleteConnections(obsoleteLans);
        }
        }
*/
}

void NetRouter::setup(void)
{
    setupLeastCommonAncestors();

    // Remove any pre-existing connections between hosts.
    borderConnections.clear();
    reverseBorderConnections.clear();
    borderLans.clear();
    nodeToBorder.clear();
    nodeToPartition.clear();
    nodeToPartition.resize(hosts.size());

    // Set up lanLanNode structure
    for (size_t i = 0; i < lans.size(); ++i)
    {
        size_t sourceLanIndex = i;
        ptree::LeafLan & sourceLan(lans[sourceLanIndex]);
        for (size_t j = 0; j < sourceLan.hosts.size(); ++j)
        {
            size_t currentNodeIndex = sourceLan.hosts[j];
            Host & currentNode = hosts[currentNodeIndex];
            for (size_t k = 0; k < currentNode.lans.size(); ++k)
            {
                size_t destLanIndex = currentNode.lans[k];
                if (sourceLanIndex != destLanIndex)
                {
                    lanLanNode[make_pair(sourceLanIndex, destLanIndex)]
                        = currentNodeIndex;
                    // The connection (destLanIndex, sourceLanIndex)
                    // is done implicitly
                }
            }
        }
    }

    // Determines which pairs (or more) of LANs are connected
    // and add these to the connection list.
    calculateInterBorders();
}

namespace
{
    class TouchParentVisitor : public ptree::ParentVisitor
    {
    public:
        virtual ~TouchParentVisitor() {}
        virtual bool visitNode(ptree::Node & target)
        {
            target.addVisit();
            return true;
        }
    };

    class CleanParentVisitor : public ptree::ParentVisitor
    {
    public:
        virtual ~CleanParentVisitor() {}
        virtual bool visitNode(ptree::Node & target)
        {
            target.removeVisit();
            return true;
        }
    };

    class CheckParentVisitor : public ptree::ParentVisitor
    {
    public:
        CheckParentVisitor(size_t newHostNumber, int newExpectedVisit)
            : hostNumber(newHostNumber)
            , expectedVisit(newExpectedVisit)
        {
        }

        virtual ~CheckParentVisitor() {}
        virtual bool visitNode(ptree::Node & target)
        {
            bool foundParent = false;
            if (target.getVisit() == expectedVisit)
            {
                target.addNewInternalHost(hostNumber);
                foundParent = true;
            }
            return !foundParent;
        }
    private:
        size_t hostNumber;
        int expectedVisit;
    };
}

void NetRouter::setupLeastCommonAncestors(void)
{
    // for every host
    for (size_t i = 0; i < hosts.size(); ++i)
    {
        vector<size_t> & targetLans = hosts[i].lans;
        size_t j = 0;
        int expectedCount = 0;

        TouchParentVisitor touchVisit;
        // For each LAN, add one to the counter of each ancestor.
        // Skip the first LAN. We will use this one to check.
        for (j = 1; j < targetLans.size(); ++j)
        {
            expectedCount += runParentVisitor(targetLans[j], touchVisit);
        }

        CheckParentVisitor checkVisit(i, expectedCount);
        if (targetLans.size() > 0)
        {
            runParentVisitor(targetLans[0], checkVisit);
        }

        CleanParentVisitor cleanVisit;
        // Erase the counter of each ancestor. Skip the first LAN because
        // it didn't change any counters.
        for (j = 1; j < targetLans.size(); ++j)
        {
            runParentVisitor(targetLans[j], cleanVisit);
        }
    }
}

int NetRouter::runParentVisitor(size_t lanIndex, ptree::ParentVisitor & visit)
{
    int result = 0;
    ptree::Node * currentNode = lans[lanIndex].partition;
    if (currentNode != NULL)
    {
        currentNode->accept(visit);
        result = 1;
    }
    return result;
}

void NetRouter::calculateBorders(ptree::Branch & target,
                                 list<size_t> & obsoleteLans,
                                 list<ptree::Node *> & subPartitionList)
{
    setInternalHosts(target);

    // Use a visitor to get a linked list of handles to all the
    // sub-partitions in this partition.
    subPartitionList.clear();
    if (target.getChild() != NULL)
    {
        ptree::SiblingListVisitor sibFinder(subPartitionList);
        target.getChild()->accept(sibFinder);
    }

    {
        vector<size_t> & borders = borderLans[&target];
        borders.clear();
        obsoleteLans.clear();
        // Iterate through the handles to sub-partitions.
        // Use each handle to find out the border LANs in that
        // sub-partition and find out which subset of them are
        // still border LANs with respect to the target partition.
        std::list<ptree::Node *>::iterator pos = subPartitionList.begin();
        std::list<ptree::Node *>::iterator limit = subPartitionList.end();
        for ( ; pos != limit; ++pos)
        {
            vector<size_t> & lanList(borderLans[*pos]);
            for (size_t i = 0; i < lanList.size(); ++i)
            {
                // lanList[i] is the index of the current LAN
                // Use that index to get a handle to the LAN itself.
                ptree::LeafLan & currentLan(lans[lanList[i]]);
                if (isBorderLan(currentLan))
                {
                    // If this is still a border LAN, put it in the
                    // list of borders.
                    borders.push_back(lanList[i]);
                }
                else
                {
                    // If this is not a border LAN anymore, put it in
                    // the list of LANs to be cleaned up at the end.
                    obsoleteLans.push_back(lanList[i]);
                }
            }
        }
    }

/* TODO: Remove this when tree-code works
    obsoleteLans.clear();
    // For every element in the sub-partition list of [level][partition]
    for (size_t i = 0; i < m_levelMakeup[level][partition].size(); ++i)
    {
        // oldPartition is the number of a sub-partition
        size_t oldPartition = m_levelMakeup[level][partition][i];
        for (size_t j = 0; j < oldBorderLans[oldPartition].size(); ++j)
        {
            size_t currentLanIndex = oldBorderLans[oldPartition][j];
            if (isBorderLan(level, currentLanIndex))
            {
                newBorderLans[partition].push_back(currentLanIndex);
            }
            else
            {
                obsoleteLans.push_back(currentLanIndex);
            }
        }
        }*/
}

void NetRouter::setInternalHosts(ptree::Node & target)
{
    // Find all hosts whose least common ancestor is target, set
    // their internalHost flag to reflect the fact that they are
    // no longer on any border.
    std::list<size_t>::const_iterator pos;
    pos = target.getNewInternalHostsBegin();
    std::list<size_t>::const_iterator limit;
    limit = target.getNewInternalHostsEnd();
    for ( ; pos != limit; ++pos)
    {
        hosts[*pos].internalHost = true;
    }
}

bool NetRouter::isBorderLan(ptree::LeafLan & currentLan) const
{
    // Assume that the LAN is not a border. If just one host
    // which is attached to this LAN is not an internal host,
    // then this LAN is still a border LAN.
    bool isBorder = false;
    for (size_t i = 0; i < currentLan.hosts.size() && !isBorder; ++i)
    {
        // currentLan.hosts[i] is the index of the current host
        size_t currentHostIndex(currentLan.hosts[i]);
        if (!(hosts[currentHostIndex].internalHost))
        {
            isBorder = true;
        }
    }
    return isBorder;
}

// TODO: Remove this when tree code works.
//bool NetRouter::isBorderLan(size_t level, size_t lan) const
//{
//    bool isBorder = false;
//    for (size_t i = 0; i < m_levelMakeup[0][lan].size() && !isBorder; ++i)
//    {
//        size_t node = m_levelMakeup[0][lan][i];
//        isBorder = isBorder || m_nodeToLevel[level][node].size() > 1;
//    }
////    if (isBorder && level == 1)
////    {
////        cerr << "IsBorderLan " << lan << ": ";
////        for (size_t i = 0; i < m_levelMakeup[0][lan].size(); ++i)
////        {
////            size_t node = m_levelMakeup[0][lan][i];
////            cerr << node << ':' << m_nodeToLevel[level][node].size() << ' ';
////        }
////        cerr << endl;
////    }
//    return isBorder;
//}
/*
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
*/
void NetRouter::calculateBorderToBorderPaths(ptree::Node & target,
                                             list<size_t> const & obsoleteLans)
{
    // The routecalc program requires a contiguous block of indices starting
    // at 0.
    vector<size_t> indexToLan;
    map<size_t, int> lanToIndex;

    {
        // The union of borderLans[&target] and obsoleteLans is the set of
        // LANs which we need to calculate shortest path on.
        list<size_t>::const_iterator pos = obsoleteLans.begin();
        list<size_t>::const_iterator limit = obsoleteLans.end();
        for ( ; pos != limit; ++pos)
        {
            lanToIndex[*pos] = static_cast<int>(indexToLan.size());
            indexToLan.push_back(*pos);
        }

        vector<size_t> & borders = borderLans[&target];
        for (size_t i = 0; i < borders.size(); ++i)
        {
            lanToIndex[borders[i]] = static_cast<int>(indexToLan.size());
            indexToLan.push_back(borders[i]);
        }

// TODO: Remove when tree code works
//        for (size_t i = 0; i < lanToPartition.size(); ++i)
//        {
//            if (lanToPartition[i] == partition)
//            {
//                lanToIndex[i] = static_cast<int>(indexToLan.size());
//                indexToLan.push_back(i);
//            }
//        }
    }

    cerr << ".";
    FileWrapper file(coprocess(ROUTECALC).release());
    char command = 'i';
    {
        map<size_t, int>::iterator pos = lanToIndex.begin();
        map<size_t, int>::iterator limit = lanToIndex.end();
        for ( ; pos != limit; ++pos)
        {
            size_t source = pos->first;
            ConnectionMap::iterator connectionPos = borderBegin(source);
            ConnectionMap::iterator connectionLimit = borderEnd(source);
            for ( ; connectionPos != connectionLimit; ++connectionPos)
            {
                size_t destination = connectionPos->first.second;
                map<size_t, int>::iterator search;
                search = lanToIndex.find(destination);
                if (search != lanToIndex.end())
                {
                    // pos->second is the index defined for the first LAN
                    // above (remember that these are new numbers created
                    // for their contiguity.
                    // search->second is the index for the second LAN above.
                    write(file, 'i', pos->second, search->second,
                          // connectionPos->second.second is the weight of
                          // the LAN.
                          static_cast<float>(connectionPos->second.second));
                }
            }
        }
    }
/* TODO: Remove when tree code works.
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
*/

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

void NetRouter::calculateNodeToBorderPaths(ptree::Node & target,
                                        list<ptree::Node *> & subPartitionList)
{
    vector<size_t> & superBorders = borderLans[&target];
    list<ptree::Node *>::iterator subPartition = subPartitionList.begin();
    list<ptree::Node *>::iterator subPartitionLimit = subPartitionList.end();
    for ( ; subPartition != subPartitionLimit; ++subPartition)
    {

//    for (size_t i = 0; i < m_levelMakeup[level][partition].size(); ++i)
//    {
//        size_t oldPartition = m_levelMakeup[level][partition][i];
        // We want to send the range of [(x,0)-(x,MAX)] at a time to
        // findNodeToBorder to be searched. Therefore we start at the
        // beginning. Each call to upper_bound() brings us one past
        // the current range (and the first one of the next)
        // and only when there is no next range (end()) do we stop.
        NodeBorderMap & subBorderMap = nodeToBorder[*subPartition];
        NodeBorderMap::iterator pos = subBorderMap.begin();
        NodeBorderMap::iterator limit;
        while(pos != subBorderMap.end())
        {
            limit = subBorderMap.upper_bound(make_pair(pos->first.first,
                                                       UINT_MAX));
            for (size_t i = 0; i < superBorders.size(); ++i)
            {
                findNodeToBorder(pos, limit, pos->first.first, target,
                                 superBorders[i]);
            }
            pos = limit;
        }

/* TODO: Remove this when tree code works
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
*/
    }
}

void NetRouter::findNodeToBorder(NodeBorderMap::iterator searchPos,
                                 NodeBorderMap::iterator searchLimit,
                                 size_t sourceNode,
                                 ptree::Node & superPartition,
                                 size_t destinationLan)
{
    static const int MAX_DISTANCE = INT_MAX;

    RouteInfo & bestRoute = nodeToBorder[&superPartition]
                                       [make_pair(sourceNode, destinationLan)];
    bestRoute.destination = destinationLan;
    // searchPos is a handle to border LAN from the sub-partition
    for ( ; searchPos != searchLimit; ++searchPos)
    {
        // if the border LAN pointed to by searchPos is not
        // the destination LAN
        if (searchPos->second.destination != destinationLan)
        {
            int currentDistance = searchPos->second.distance +
                getBorderWeight(searchPos->second.destination,
                                destinationLan);
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
            // The destination border LAN is the same as the border LAN
            // pointed to by searchPos. Therefore, the routing info is
            // unchanged.
            if (searchPos->second.distance < bestRoute.distance)
            {
                bestRoute = searchPos->second;
            }
        }
    }
}

void NetRouter::calculatePartitionRoutes(ptree::Node & target,
                                        list<ptree::Node *> & subPartitionList)
{
    // for each sub-partition in the partition
    list<ptree::Node *>::iterator sourcePartition = subPartitionList.begin();
    list<ptree::Node *>::iterator sourcePartitionLimit;
    sourcePartitionLimit = subPartitionList.end();
    for ( ; sourcePartition != sourcePartitionLimit; ++sourcePartition)
    {
        NodeBorderMap & sourceBorderMap(nodeToBorder[*sourcePartition]);
        NodeBorderMap::iterator pos = sourceBorderMap.begin();
        NodeBorderMap::iterator limit;
        while (pos != sourceBorderMap.end())
        {
            limit = sourceBorderMap.upper_bound(make_pair(pos->first.first,
                                                          UINT_MAX));
            // for each sub-partition in the partition
            list<ptree::Node *>::iterator destPartition;
            destPartition = subPartitionList.begin();
            list<ptree::Node *>::iterator destPartitionLimit;
            destPartitionLimit = subPartitionList.end();
            for ( ; destPartition != destPartitionLimit; ++destPartition)
            {
                NodeBorderMap::iterator foundBegin;
                NodeBorderMap::iterator foundEnd;
                foundBegin = nodeToBorder[*destPartition].lower_bound(
                    make_pair(pos->first.first, 0));
                foundEnd = nodeToBorder[*destPartition].upper_bound(
                    make_pair(pos->first.first, UINT_MAX));
                // if the node is not part of the destination partition
                if (foundBegin == foundEnd)
                {
                    // find a route from that node to this partition.
                    findRouteToPartition(pos, limit, pos->first.first,
                                         *sourcePartition, *destPartition);
                }
            }
            pos = limit;
        }
    }
//      // for each node in the partition, sorted by sub-partition
//      for (size_t i = 0; i < m_levelMakeup[level][partition].size(); ++i)
//      {
//          size_t sourcePartition = m_levelMakeup[level][partition][i];
//          // see calculateNodeToBorderPaths for an explanation of this looping
//          // idiom.
//          NodeBorderMap::iterator pos = oldNodeToBorder[sourcePartition].begin();
//          NodeBorderMap::iterator limit;
//          while (pos != oldNodeToBorder[sourcePartition].end())
//          {
//              limit = oldNodeToBorder[sourcePartition].upper_bound(
//                  make_pair(pos->first.first, UINT_MAX));
//              // for each sub-partition in the partition
//              for (size_t j = 0; j < m_levelMakeup[level][partition].size(); ++j)
//              {
//                  size_t destPartition = m_levelMakeup[level][partition][j];
//                  NodeBorderMap::iterator foundBegin;
//                  NodeBorderMap::iterator foundEnd;
//                  foundBegin = oldNodeToBorder[destPartition].lower_bound(
//                      make_pair(pos->first.first, 0));
//                  foundEnd = oldNodeToBorder[destPartition].upper_bound(
//                      make_pair(pos->first.first, UINT_MAX));
//                  // if the node is not part of the destination partition
//                  if (foundBegin == foundEnd)
//                  {
//                      // find a route from that node to this partition.
//                      findRouteToPartition(pos, limit, pos->first.first,
//                                           sourcePartition, destPartition,
//                                           level);
//                  }
//              }
//              pos = limit;
//          }
//    }
}

void NetRouter::findRouteToPartition(NodeBorderMap::iterator searchPos,
                                     NodeBorderMap::iterator searchLimit,
                                     size_t sourceNode,
                                     ptree::Node * sourcePartition,
                                     ptree::Node * destPartition)
{
    map<ptree::Node *,RouteInfo> & currentRoute = nodeToPartition[sourceNode];

    map<ptree::Node *,RouteInfo>::iterator found = currentRoute.find(destPartition);
    if (found == currentRoute.end())
    {
        found = currentRoute.insert(make_pair(destPartition, RouteInfo()))
                                                                        .first;
    }

    // for every border in the sub-partition of the sourceNode
    for ( ; searchPos != searchLimit; ++searchPos)
    {
        // for every border in the destination sub-partition
        vector<size_t> & destPartitionLans(borderLans[destPartition]);
        for (size_t i = 0; i < destPartitionLans.size(); ++i)
        {
            // If the route using those two borders is shorter than the
            // current route
            size_t destLan = destPartitionLans[i];
            int currentDistance = searchPos->second.distance +
                getBorderWeight(searchPos->second.destination,
                                destLan);
/*            if (ipToString((found->first)->getPrefix()) == "10.20.0.0"
                && sourceNode == 2)
            {
                cerr << "Judgement: " << currentDistance << " < " << found->second.distance << endl;
                cerr << "LANS: " << searchPos->second.firstLan << " < " << found->second.firstLan << endl;
                }*/
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
/*                    if ((firstLan == 1768 || secondLan == 1768)
                        && destLan != secondLan && firstLan != destLan)
                    {
                        cerr << "!!!! 1768 !!!! -- " << firstLan << ":"
                             << secondLan << " "
                             << ipToString((found->first)->getPrefix())
                             << " " << sourceNode << endl;
                             }*/
                    found->second.firstNode = lanLanNode[make_pair(firstLan,
                                                                   secondLan)];
                }
            }
        }
    }
/*    if (sourceNode == 2)
    {
        cerr << "Routing to: "
             << ipToString((found->first)->getPrefix())
             << "/" << (found->first)->getMaskSize()
             << " FirstLan: " << found->second.firstLan << " FirstNode: "
             << found->second.firstNode << " Distance: "
             << found->second.distance << endl;
             }*/
}

void NetRouter::cleanupBorderLans(list<size_t> const & obsoleteLans,
                                  list<ptree::Node *> const & subPartitionList)
{
    // Clear the ConnectionMap of references to LANs which we don't
    // care about anymore.
    {
        list<size_t>::const_iterator pos = obsoleteLans.begin();
        list<size_t>::const_iterator limit = obsoleteLans.end();
        for ( ; pos != limit; ++pos)
        {
            removeBorderLan(*pos);
        }
    }

    // We won't need to refer to the subPartitions anymore. So we remove
    // the clutter from the borderLans and nodeToBorder maps.
    {
        list<ptree::Node *>::const_iterator pos = subPartitionList.begin();
        list<ptree::Node *>::const_iterator limit = subPartitionList.end();
        for ( ; pos != limit; ++pos)
        {
            borderLans.erase(*pos);
            nodeToBorder.erase(*pos);
        }
    }
}

void NetRouter::calculateInterBorders(void)
{
    for (size_t i = 0; i < hosts.size(); ++i)
    {
        Host & currentHost(hosts[i]);
        for (size_t j = 0; j < currentHost.lans.size(); ++j)
        {
            size_t currentSource = currentHost.lans[j];
            for (size_t k = j + 1; k < currentHost.lans.size(); ++k)
            {
                size_t currentDest = currentHost.lans[k];
                addBorderConnection(currentSource, currentDest,
                                    currentDest,
                                    lans[currentDest].weight);
                addBorderConnection(currentDest, currentSource,
                                    currentSource,
                                    lans[currentSource].weight);
            }
        }
    }
}

void NetRouter::print(std::ostream & output) const
{
    output << "%%" << endl;
//    size_t nodeCount = m_nodeToLevel[0].size();
//    size_t levelCount = m_nodeToLevel.size() - 1;
    for (size_t i = 0; i < hosts.size(); ++i)
    {
        map<ptree::Node *, RouteInfo> const & hostMap(nodeToPartition[i]);
        map<ptree::Node *, RouteInfo>::const_iterator partitionPos;
        partitionPos = hostMap.begin();
        map<ptree::Node *, RouteInfo>::const_iterator partitionLimit;
        partitionLimit = hostMap.end();
        output << "Routing table for node: " << i << endl;
        for ( ; partitionPos != partitionLimit; ++partitionPos)
        {
            ptree::Node & partition = *(partitionPos->first);
            if (partitionPos->second.firstNode != i)
            {
                vector<size_t>::const_iterator found;
                size_t lanIndex = partitionPos->second.firstLan;
                size_t hostIndex = partitionPos->second.firstNode;
                found = find(lans[lanIndex].hosts.begin(),
                             lans[lanIndex].hosts.end(),
                             hostIndex);
                if (found != lans[lanIndex].hosts.end())
                {
                    IPAddress increment = 1;
                    // increment = 1 + indexOfHostWithRespectToLan
                    increment += found - lans[lanIndex].hosts.begin();
                    output << "Destination: "
                           << ipToString(partition.getPrefix())
                           << "/"
                           << partition.getMaskSize()
                           << " FirstHop: "
                           << ipToString(lans[lanIndex].partition->getPrefix()
                                         + increment)
                           << endl;
                }
            }
            else
            {
                // if the first node on the route is the originating
                // node, then that node is adjascent to the
                // destination.  Hence no routing at this level.
            }
        }
        output << "%%" << endl;
    }
}

auto_ptr<ptree::Node> & NetRouter::getTree(void)
{
    return tree;
}

vector<ptree::LeafLan> & NetRouter::getLans(void)
{
    return lans;
}

void NetRouter::setHosts(Assigner::NodeLookup & source)
{
    hosts.clear();
    hosts.resize(source.size());
    for (size_t i = 0; i < hosts.size(); ++i)
    {
        hosts[i].lans = source[i];
    }
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
