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
    , m_partition(newPartition)
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
}

void ConservativeAssigner::addLan(int bits, int weight, vector<size_t> nodes)
{
    Lan temp;
    temp.number = m_lanList.size();
    temp.weight = weight;
    temp.nodes = nodes;
    m_lanList.push_back(temp);
    m_largestLan = max(m_largestLan, static_cast<int>(nodes.size()));
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        if (nodes[i] >= m_nodeToLan.size())
        {
            m_nodeToLan.resize(nodes[i] + 1);
        }
        m_nodeToLan[nodes[i]].push_back(m_lanList.back().number);
    }
}

void ConservativeAssigner::ipAssign(void)
{
    if (!(isConnected(0)))
    {
        cerr << "Graph is not connected" << endl;
    }

    std::vector<int> partitionArray;
    std::vector<int> graphIndexArray;
    std::vector<int> graphNeighborArray;
    std::vector<int> weightArray;

    // convert to METIS graph
    convert(graphIndexArray, graphNeighborArray, weightArray);

    // partition the graph
    m_partition.partition(graphIndexArray, graphNeighborArray, weightArray,
                          partitionArray);
    m_partitionCount = m_partition.getPartitionCount();

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

    // m_partitionCount may increase during this loop
    int loop = 0;
    while (loop < m_partitionCount)
    {
        makeConnected(loop);
        ++loop;
    }

    for (int i = 0; i < m_partitionCount; ++i)
    {
        if (!(isConnected(i)))
        {
            cerr << "Not connected " << i << endl;
        }
    }

    // output how many border LANs there are.
    vector<bool> border;
    border.resize(m_lanList.size());
    fill(border.begin(), border.end(), false);
    for (size_t i = 0; i < border.size(); ++i)
    {
        for (size_t j = 0; j < m_lanList[i].nodes.size(); ++j)
        {
            size_t node = (m_lanList[i].nodes)[j];
            for (size_t k = 0; k < m_nodeToLan[node].size(); ++k)
            {
                size_t destLan = m_nodeToLan[node][k];
                if (m_lanList[i].partition != m_lanList[destLan].partition)
                {
                    border[i] = true;
                }
            }
        }
    }
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
/*        for (size_t j = 0; j < m_nodeToLan[i].size(); ++j)
          {
          (nodeToLevel.back())[i].push_back(m_lanList[m_nodeToLan[i][j]].partition);
          }*/
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

    levelMakeup.push_back(LevelLookup());
    levelMakeup.back().resize(1);
    for (i = 0; i < levelMakeup[1].size(); ++i)
    {
        levelMakeup.back().back().push_back(i);
    }

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

    // we want to set up intuitive bit boundaries if possible
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
//    size_t orig = m_partitionCount;
//    makeConnected(whichPartition);
//    return orig >= m_partitionCount;
    queue<size_t> nextConnection;
    size_t first = 0;
    size_t numInPartition = 0;
    {
        for (size_t i = 0; i < m_lanList.size(); ++i)
        {
            if (m_lanList[i].partition == whichPartition)
            {
                first = i;
                ++numInPartition;
            }
        }
    }
    if (whichPartition == 0)
    {
        cerr << whichPartition << ':' << numInPartition << endl;
    }
    if (numInPartition == 0)
    {
        cerr << "Empty partition " << whichPartition << endl;
        return true;
    }
    nextConnection.push(first);
    vector<bool> connected;
    connected.resize(m_lanList.size());
    fill(connected.begin(), connected.end(), false);
    connected[first] = true;
    if (m_lanList.size() > 0)
    {
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
    vector<bool>::difference_type num = count(connected.begin(),
                                              connected.end(),
                                              true);
    return num == numInPartition;
}

void ConservativeAssigner::makeConnected(int whichPartition)
{
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


