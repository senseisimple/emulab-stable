// ConservativeAssigner.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "lib.h"
#include "Exception.h"
#include "ConservativeAssigner.h"
#include "bitmath.h"

using namespace std;

ConservativeAssigner::ConservativeAssigner(Partition & newPartition)
    : m_partitionCount(0)
    , m_largestLan(0)
    , m_lanMaskSize(0)
    , m_netMaskSize(0)
    , m_partition(&newPartition)
    // m_lanList, m_nodeToLan, and m_partitionIPList initialize themselves.
{
}

ConservativeAssigner::~ConservativeAssigner()
{
}

auto_ptr<Assigner> ConservativeAssigner::clone(void) const
{
    return auto_ptr<Assigner>(new ConservativeAssigner(*this));
}

void ConservativeAssigner::setPartition(Partition & newPartition)
{
    m_partition = &newPartition;
}

void ConservativeAssigner::addLan(int bits, int weight, vector<size_t> nodes)
{
    // add the new LAN to m_lanList
    Lan temp;
    temp.number = m_lanList.size();
    temp.weight = weight;
    temp.nodes = nodes;
    m_lanList.push_back(temp);

    // if this LAN is the largest we've encountered, note that fact.
    m_largestLan = max(m_largestLan, static_cast<int>(nodes.size()));

    // Update m_nodeToLan such that the nodes with interfaces in this LAN
    // know that fact.
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        if (nodes[i] >= m_nodeToLan.size())
        {
            m_nodeToLan.resize(nodes[i] + 1);
        }
        vector<size_t> & currentNode = m_nodeToLan[nodes[i]];
        currentNode.push_back(m_lanList.back().number);
    }
}

void ConservativeAssigner::ipAssign(void)
{
    // These hold a METIS-ready conversion of the LAN-graph data.
    std::vector<int> partitionArray;
    std::vector<int> graphIndexArray;
    std::vector<int> graphNeighborArray;
    std::vector<int> weightArray;

    // convert to METIS graph
    convert(graphIndexArray, graphNeighborArray, weightArray);

    // partition the graph
    m_partition->partition(graphIndexArray, graphNeighborArray, weightArray,
                          partitionArray);
    m_partitionCount = m_partition->getPartitionCount();

// This is the code for calling METIS and changing its tolerances.
/*    if (numPartitions > 1)
    {
        int numConstraints = 2;
        weightOption = 1;
        vector<int> vertexWeights;
        vertexWeights.resize(numConstraints*numVertices);
        fill(vertexWeights.begin(), vertexWeights.end(), 1);
        vector<float> tolerances;
        tolerances.resize(numConstraints);
        fill(tolerances.begin(), tolerances.end(), static_cast<float>(1.5));
        options[0] = 0;
        METIS_mCPartGraphKway(
            &numVertices,              // # of vertices in graph
            &numConstraints,           // Number of constraints
            &(graphIndexArray[0]),     // xadj
            &(graphNeighborArray[0]),  // adjncy
            &(vertexWeights[0]),       // vertex weights
            &(weightArray[0]),         // edge weights
            &weightOption,             // Weights on edges only
            &indexOption,              // 0-based index
            &numPartitions,
            &(tolerances[0]),
            options,                   // set options to nonrandom
            &edgesCut,                 // return the # of edges cut
            &(partitionArray[0]));     // return the partition of each vertex
    }
*/

    // Each number in the partitions vector is the partition that
    // the associated Lan in m_lanList belongs too.
    // Let each Lan know where it belongs.
    vector<Lan>::iterator pos = m_lanList.begin();
    vector<int>::iterator partPos = partitionArray.begin();
    for ( ; pos != m_lanList.end() && partPos != partitionArray.end();
          ++pos, ++partPos)
    {
        pos->partition = *partPos;
    }

    // If there are disconnected partitions, turn them into different
    // partitions.
    // m_partitionCount may increase during this loop
    int loop = 0;
    while (loop < m_partitionCount)
    {
        // Each time a breadth-first walk from a LAN in a partition fails
        // to reach every LAN in that partition, the remainder is slopped
        // together into a new partition which is m_partitionCount+1.
        // Then m_partitionCount is incremented.
        makeConnected(loop);
        ++loop;
    }

    // Calculate the list of super-partitions. These are the
    // fully-connected subgraphs in the possibly disconnected
    // super-graph. There should usually only be one super-partition.
    calculateSuperPartitions();

    //////////////////////////////////////////////////////
    // find out which bits go to which subnet
    //////////////////////////////////////////////////////

    // Add two to the largest lan because we need can't use an address
    // which has a hostname of all 0s or all 1s

    int bitsForNodes = getBits(m_largestLan + 2);
    int bitsForNetwork = getBits(m_partitionCount);
    if (bitsForNetwork < 0)
    {
        bitsForNetwork = 0;
    }

    int bitsForLans = getLanBits(partitionArray, m_partitionCount);
    checkBitOverflow(bitsForNodes + bitsForLans + bitsForNetwork);

    //////////////////////////////////////////////////////
    // assign IP addresses
    //////////////////////////////////////////////////////

    assignNumbers(bitsForNetwork, bitsForLans, bitsForNodes, m_partitionCount);
}

void ConservativeAssigner::print(ostream & output) const
{
    vector<Lan>::const_iterator lanPos = m_lanList.begin();
    for ( ; lanPos != m_lanList.end(); ++lanPos)
    {
        size_t i = 0;
        for ( ; i < lanPos->nodes.size(); ++i)
        {
            output << lanPos->number << " ";
            output << lanPos->nodes[i] << " ";
            // The ip address of a particular interface is just an offset
            // from the prefix associated with its LAN.
            output << ipToString(lanPos->ip + i + 1);
            output << endl;
        }
    }
}

void ConservativeAssigner::graph(vector<Assigner::NodeLookup> & nodeToLevel,
                           vector<Assigner::MaskTable> & levelMaskSize,
                           vector<Assigner::PrefixTable> & levelPrefix,
                           vector<Assigner::LevelLookup> & levelMakeup,
                           vector<int> & lanWeights) const
{
    // we only have two layers of hierarchies with this method (LAN and
    // partition), so the first element of all of these vectors will
    // represent the LAN level and the second element is the partition layer.
    size_t i;

    // make sure that the output stuff is in a sane state.
    nodeToLevel.clear();
    levelMaskSize.clear();
    levelPrefix.clear();
    levelMakeup.clear();
    lanWeights.clear();

    // set up the nodeToLan table in the output
    nodeToLevel.push_back(m_nodeToLan);

    // set up the nodeToPartition table in the output
    nodeToLevel.push_back(NodeLookup());
    nodeToLevel.back().resize(m_nodeToLan.size());
    for (i = 0; i < nodeToLevel.back().size(); ++i)
    {
        set<size_t> partitions;
        for (size_t j = 0; j < m_nodeToLan[i].size(); ++j)
        {
            partitions.insert(m_lanList[m_nodeToLan[i][j]].partition);
        }
        (nodeToLevel.back())[i].reserve(partitions.size());
        set<size_t>::iterator pos = partitions.begin();
        for ( ; pos != partitions.end(); ++pos)
        {
            (nodeToLevel.back())[i].push_back(*pos);
        }
    }

    nodeToLevel.push_back(NodeLookup());
    nodeToLevel.back().resize(m_nodeToLan.size());
    for (i = 0; i < nodeToLevel.back().size(); ++i)
    {
        (nodeToLevel.back())[i].push_back(0);
    }

    // set up the netmasks. Since we are being conservative, they will
    // all be the same.
    levelMaskSize.push_back(MaskTable());
    levelMaskSize.back().resize(m_lanList.size());
    fill(levelMaskSize.back().begin(), levelMaskSize.back().end(),
         m_lanMaskSize);
    levelMaskSize.push_back(MaskTable());
    levelMaskSize.back().resize(m_partitionCount);
    fill(levelMaskSize.back().begin(), levelMaskSize.back().end(),
         m_netMaskSize);

    levelMaskSize.push_back(MaskTable());
    levelMaskSize.back().push_back(0);

    // set up the prefixes. These are the ip numbers of the LANS and
    // partitions, respectively.
    levelPrefix.push_back(PrefixTable());
    levelPrefix.back().resize(m_lanList.size());
    for (i = 0; i < m_lanList.size(); ++i)
    {
        (levelPrefix.back())[i] = m_lanList[i].ip;
    }
    levelPrefix.push_back(m_partitionIPList);
    levelPrefix.push_back(PrefixTable());
    levelPrefix.back().push_back(0);

    // set up levelMakeup. This is which nodes make up each LAN and which LANs
    // make up each partition in the partition-graph, respectively.
    levelMakeup.push_back(LevelLookup());
    levelMakeup.back().resize(m_lanList.size());
    for (i = 0; i < m_lanList.size(); ++i)
    {
        (levelMakeup.back())[i] = m_lanList[i].nodes;
    }
    levelMakeup.push_back(LevelLookup());
    levelMakeup.back().resize(m_partitionCount);
    for (i = 0; i < m_lanList.size(); ++i)
    {
        (levelMakeup.back())[m_lanList[i].partition].push_back(i);
    }

    // Add our super-partitions to the hierarchy. Each super-partition
    // is a fully-connected sub-graph in a possibly disconnected
    // graph.
    levelMakeup.push_back(m_superPartitionList);

    // set up the lan weights.
    lanWeights.resize(m_lanList.size());
    for (size_t i = 0; i < lanWeights.size(); ++i)
    {
        lanWeights[i] = m_lanList[i].weight;
    }
}

void ConservativeAssigner::convert(std::vector<int> & indexes,
                                   std::vector<int> & neighbors,
                                   std::vector<int> & weights) const
{
    // ensure that there is no garbage in the output references.
    indexes.clear();
    neighbors.clear();
    weights.clear();
    indexes.reserve(m_lanList.size() + 1);

    // fill them up with the graph.
    // See the METIS manual, section [5.1 Graph Data Structure], page 18
    vector<Lan>::const_iterator lanPos = m_lanList.begin();
    for ( ; lanPos != m_lanList.end(); ++lanPos)
    {
        // neighbors.size() is the total number of edges pushed on so
        // far. This is one greater than the max index. Which is the
        // starting index for that LAN's set of edges.
        indexes.push_back(neighbors.size());
        convertAddLan(*lanPos, neighbors, weights);
    }
    indexes.push_back(neighbors.size());
}

// The following two functions are auxilliary to the 'convert' function
// They were seperated out into different functions for conceptual simplicity.

void ConservativeAssigner::convertAddLan(Lan const & lanToAdd,
                                         std::vector<int> & neighbors,
                                         std::vector<int> & weights) const
{
    vector<size_t>::const_iterator nodePos = lanToAdd.nodes.begin();
    for ( ; nodePos != lanToAdd.nodes.end(); ++nodePos)
    {
        convertAddNode(lanToAdd, *nodePos, neighbors, weights);
    }
}

void ConservativeAssigner::convertAddNode(Lan const & info, int currentNode,
                           std::vector<int> & neighbors,
                           std::vector<int> & weights) const
{
    vector<size_t>::const_iterator pos = m_nodeToLan[currentNode].begin();
    for ( ; pos != m_nodeToLan[currentNode].end(); ++pos)
    {
        if (*pos != info.number)
        {
            neighbors.push_back(*pos);
            weights.push_back(info.weight + m_lanList[*pos].weight);
        }
    }
}

int ConservativeAssigner::getBits(int num)
{
    return roundUp(log(static_cast<double>(num))/log(2.0));
}

int ConservativeAssigner::getLanBits(vector<int> const & partitionArray,
                                     int numPartitions)
{
    // Create an array of counters, adding to the appropriate counter
    // whenever we encounter a LAN in that partition. Then take the largest
    // and voila! instant largest partition!
    vector<int> counter;
    int current;
    counter.resize(numPartitions);
    fill(counter.begin(), counter.end(), 0);
    for (size_t i = 0; i < partitionArray.size(); ++i)
    {
        current = partitionArray[i];
        if (current >= 0 && current < numPartitions)
        {
            ++(counter[current]);
        }
    }
    vector<int>::iterator maxPos = max_element(counter.begin(), counter.end());
    if (maxPos != counter.end())
    {
        return getBits(*maxPos);
    }
    else
    {
        return 0;
    }
}

int ConservativeAssigner::roundUp(double num)
{
    return static_cast<int>(ceil(num) + 0.5);
}

void ConservativeAssigner::checkBitOverflow(int numBits)
{
    if (numBits <= 0 || numBits > totalBits)
    {
        throw BitOverflowException("");
    }
}

void ConservativeAssigner::assignNumbers(int networkSize, int lanSize,
                                         int hostSize, int numPartitions)
{
    // set up a counter for each partition
    vector<int> counter;
    counter.resize(numPartitions);
    fill(counter.begin(), counter.end(), 0);

    // we want to set up human readable bit boundaries if possible
    if (networkSize <= 8 && lanSize <= 8 && hostSize <= 8)
    {
        networkSize = 8;
        lanSize = 8;
        hostSize = 8;
    }

    // set up netmask sizes
    m_lanMaskSize = totalBits - hostSize;
    m_netMaskSize = totalBits - lanSize - hostSize;

    int networkShift = lanSize + hostSize;
    int lanShift = hostSize;

    // For each lan, set the IP address. The IP for nodes in a LAN are implicit
    vector<Lan>::iterator pos = m_lanList.begin();
    for ( ; pos != m_lanList.end(); ++pos)
    {
        pos->ip = prefix | (pos->partition << networkShift)
                         | (counter[pos->partition] << lanShift);
        ++(counter[pos->partition]);
    }

    // set up the partition numbers. Since we don't have to worry about
    // a partition using up more than half a bit of space, we just
    // use the partition number to set the IP.
    m_partitionIPList.resize(numPartitions);
    for (size_t i = 0; i < numPartitions; ++i)
    {
        m_partitionIPList[i] = prefix | (i << networkShift);
    }
}

bool ConservativeAssigner::isConnected(int whichPartition)
{
    // Use a queue for a breadth-first search
    queue<size_t> nextConnection;
    size_t first = 0;
    size_t numInPartition = 0;
    // This extra scoping is to make sure that i can be re-used.
    {
        // Find out how many LANs are in whichPartition and find an arbitrary
        // starting LAN for the search.
        for (size_t i = 0; i < m_lanList.size(); ++i)
        {
            if (m_lanList[i].partition == whichPartition)
            {
                first = i;
                ++numInPartition;
            }
        }
    }
    // trivial case
    if (numInPartition == 0 || m_lanList.size() == 0)
    {
        return true;
    }

    nextConnection.push(first);
    vector<bool> connected;
    connected.resize(m_lanList.size());
    fill(connected.begin(), connected.end(), false);
    connected[first] = true;

    // breadth-first search
    while (!(nextConnection.empty()))
    {
        size_t current = nextConnection.front();
        nextConnection.pop();

        // add all adjascent LANs in this partition
        for (size_t i = 0; i < m_lanList[current].nodes.size(); ++i)
        {
            size_t currentNode = m_lanList[current].nodes[i];
            for (size_t j = 0; j < m_nodeToLan[currentNode].size(); ++j)
            {
                size_t destLan = m_nodeToLan[currentNode][j];
                if (m_lanList[destLan].partition == whichPartition
                    && !(connected[destLan]))
                {
                        connected[destLan] = true;
                        nextConnection.push(destLan);
                }
            }
        }
    }
    vector<bool>::difference_type num = count(connected.begin(),
                                              connected.end(),
                                              true);
    // iff we got to every node in this partition during our search,
    // then the partition is connected.
    return num == numInPartition;
}

void ConservativeAssigner::makeConnected(int whichPartition)
{
    // breadth first search.
    queue<size_t> nextConnection;
    size_t first = 0;
    size_t numInPartition = 0;
    size_t i = 0;
    vector<bool> connected;
    connected.resize(m_lanList.size());

    // Set all LANs in whichPartition to false. All others to true.
    for (i = 0; i < connected.size(); ++i)
    {
        if (m_lanList[i].partition == whichPartition)
        {
            connected[i] = false;
            first = i;
            ++numInPartition;
        }
        else
        {
            connected[i] = true;
        }
    }

    // Starting with a random LAN in whichPartition, set all neighbors
    // to true. Repeat until all connected LANs are true.
    nextConnection.push(first);
    connected[first] = true;
    if (m_lanList.size() > 0)
    {
        while (!(nextConnection.empty()))
        {
            size_t current = nextConnection.front();
            nextConnection.pop();
            for (i = 0; i < m_lanList[current].nodes.size(); ++i)
            {
                size_t currentNode = m_lanList[current].nodes[i];
                for (size_t j = 0; j < m_nodeToLan[currentNode].size(); ++j)
                {
                    size_t destLan = m_nodeToLan[currentNode][j];
                    if (m_lanList[destLan].partition == whichPartition)
                    {
                        if (!(connected[destLan]))
                        {
                            connected[destLan] = true;
                            nextConnection.push(destLan);
                        }
                    }
                }
            }
        }
    }
    // If any LANs are false, put them in a new partition because
    // they are not connected.
    bool newPartition = false;
    for (i = 0; i < connected.size(); ++i)
    {
        if (!(connected[i]))
        {
            newPartition = true;
            m_lanList[i].partition = m_partitionCount;
        }
    }
    if (newPartition)
    {
        ++m_partitionCount;
    }
}
namespace
{
    enum TouchedT
    {
        NotTouched,
        Touched,
        OldTouched
    };
}

void ConservativeAssigner::calculateSuperPartitions(void)
{
    vector<TouchedT> touch;
    queue<size_t> nextConnection;
    set<size_t> currentResult;
    touch.resize(m_lanList.size());
    fill(touch.begin(), touch.end(), NotTouched);

    // Initialize first. 'first' is the arbitrary initial node in the
    // breadth-first search.
    size_t first = 0;
    while (first < touch.size()
           && (touch[first] == Touched || touch[first] == OldTouched))
    {
        ++first;
    }

    while (first < touch.size())
    {
        // Run a breadth-first search through the graph, marking the
        // LANs we find as Touched. The partition of each LAN is added
        // to the list of partitions in the super-partition we are
        // constructing.
        touch[first] = Touched;
        nextConnection.push(first);
        currentResult.clear();
        while (!(nextConnection.empty()))
        {
            size_t current = nextConnection.front();
            nextConnection.pop();
            for (size_t i = 0; i < m_lanList[current].nodes.size(); ++i)
            {
                size_t currentNode = m_lanList[current].nodes[i];
                for (size_t j = 0; j < m_nodeToLan[currentNode].size(); ++j)
                {
                    size_t destLan = m_nodeToLan[currentNode][j];
                    if (touch[destLan] == NotTouched)
                    {
                        touch[destLan] = Touched;
                        nextConnection.push(destLan);
                        currentResult.insert(m_lanList[destLan].partition);
                    }
                }
            }
        }

        // The currentResult holds all of the partitions in the fully
        // connected super-partition. So we add the list to the
        // super-partition list.
        if (!(currentResult.empty()))
        {
            m_superPartitionList.push_back(vector<size_t>());
            m_superPartitionList.back().reserve(currentResult.size());
            set<size_t>::iterator pos = currentResult.begin();
            set<size_t>::iterator limit = currentResult.end();
            for ( ; pos != limit; ++pos)
            {
                m_superPartitionList.back().push_back(*pos);
            }
        }

        // We change all of the elements that were 'Touched' into
        // 'OldTouched' to distinguish between the current iteration
        // of the graph and previous ones.
        for (size_t i = 0; i < touch.size(); ++i)
        {
            if (touch[i] == Touched)
            {
                touch[i] = OldTouched;
            }
        }

        // get next first
        while (first < touch.size()
               && (touch[first] == Touched || touch[first] == OldTouched))
        {
            ++first;
        }
    }
}
