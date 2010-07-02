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
#include "GraphConverter.h"

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

namespace
{
    // This is an adapter which presents data to the graph converter algorithm
    // This is constructed in accordance with the specification for
    // the GraphData representation. See GraphConverter.h
    class ConservativeGraphData
    {
    public:
        typedef std::vector<Assigner::Lan>::const_iterator LanIterator;
        typedef std::vector<size_t>::const_iterator HostInsideLanIterator;
        typedef std::vector<size_t>::const_iterator LanInsideHostIterator;
    public:
        ConservativeGraphData(std::vector<Assigner::Lan> const & newLans,
                              Assigner::NodeLookup const & newHosts)
            : lans(newLans)
            , hosts(newHosts)
        {
        }

        ~ConservativeGraphData()
        {
        }

        LanIterator getLanBegin(void)
        {
            return lans.begin();
        }

        LanIterator getLanEnd(void)
        {
            return lans.end();
        }

        int getLanNumber(LanIterator & currentLan)
        {
            return static_cast<int>(currentLan->number);
        }

        int getLanNumber(LanInsideHostIterator & currentLan)
        {
            return static_cast<int>(lans[*currentLan].number);
        }

        int getLanWeight(LanIterator & currentLan)
        {
            return currentLan->weight;
        }

        int getLanWeight(LanInsideHostIterator & currentLan)
        {
            return static_cast<int>(lans[*currentLan].weight);
        }

        HostInsideLanIterator getHostInsideLanBegin(LanIterator & currentLan)
        {
            return currentLan->nodes.begin();
        }

        HostInsideLanIterator getHostInsideLanEnd(LanIterator & currentLan)
        {
            return currentLan->nodes.end();
        }

        LanInsideHostIterator getLanInsideHostBegin(
                                           HostInsideLanIterator & currentHost)
        {
            return hosts[*currentHost].begin();
        }

        LanInsideHostIterator getLanInsideHostEnd(
                                           HostInsideLanIterator & currentHost)
        {
            return hosts[*currentHost].end();
        }
    private:
        ConservativeGraphData(ConservativeGraphData const &);
        ConservativeGraphData & operator=(ConservativeGraphData const &)
            { return *this; }
    private:
        std::vector<Assigner::Lan> const & lans;
        Assigner::NodeLookup const & hosts;
    };
}

void ConservativeAssigner::ipAssign(void)
{
    // These hold a METIS-ready conversion of the LAN-graph data.
    std::vector<int> partitionArray;

    // convert to METIS graph
    ConservativeGraphData convertGraphData(m_lanList, m_nodeToLan);
    GraphConverter<ConservativeGraphData, ConservativeGraphData::LanIterator,
                   ConservativeGraphData::HostInsideLanIterator,
                   ConservativeGraphData::LanInsideHostIterator>
        convert;
    convert.reset(convertGraphData);

    // partition the graph
    m_partition->partition(convert.getIndexes(), convert.getNeighbors(),
                           convert.getWeights(), partitionArray);
    m_partitionCount = m_partition->getPartitionCount();

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

void ConservativeAssigner::getGraph(auto_ptr<ptree::Node> & tree,
                                    vector<ptree::LeafLan> & lans)
{
    using namespace ptree;
    auto_ptr<Branch> newBranch;

    // outLan is an output reference. We reset it before we do anything else.
    lans.clear();
    lans.resize(m_lanList.size());

    // This auto_ptr represents the entire tree. There is always at least
    // one super-partition.
    newBranch.reset(new Branch());
    populateSuperPartitionTree(0, newBranch.get(), lans);
    tree.reset(newBranch.release());

    // If there are any other superPartitions, add them as siblings and
    // populate their trees as well.
    for (size_t i = 1; i < m_superPartitionList.size(); ++i)
    {
        // Create a new top-level branch.
        newBranch.reset(new Branch());
        populateSuperPartitionTree(i, newBranch.get(), lans);

        // Top-level partitions do not route to one another. They represent
        // sub-graphs. which are disconnected from one another. Therefore
        // the routing information about such domains doesn't matter.
        // Nonetheless, we set the information to a sane state.
        newBranch->setNetworkEntry(prefix, prefixBits);

        // Add the current partition to the tree as a sibling of the original
        // tree.
        auto_ptr<ptree::Node> tempAutoPtr(newBranch.release());
        tree->addSibling(tempAutoPtr);
    }
}

void ConservativeAssigner::populateSuperPartitionTree(size_t superPartition,
                                                      ptree::Branch * tree,
                                               vector<ptree::LeafLan> & outLan)
{
    using namespace ptree;

    if (   superPartition < m_superPartitionList.size()
        && superPartition >= 0 && tree != NULL)
    {
        for (size_t i = 0; i < m_superPartitionList[superPartition].size();
             ++i)
        {
            auto_ptr<Branch> newBranch(new Branch());
            newBranch->setParent(tree);
            size_t partition = m_superPartitionList[superPartition][i];
            populatePartitionTree(partition, newBranch.get(), outLan);
            newBranch->setNetworkEntry(m_partitionIPList[partition],
                                       m_netMaskSize);
            auto_ptr<ptree::Node> tempAutoPtr(newBranch.release());
            tree->addChild(tempAutoPtr);
        }
    }
}

void ConservativeAssigner::populatePartitionTree(size_t partition,
                                                 ptree::Branch * tree,
                                               vector<ptree::LeafLan> & outLan)
{
    using namespace ptree;

    cerr << "Partition: " << partition << endl;

    if (partition < m_partitionCount && partition >= 0 && tree != NULL)
    {
        for (size_t i = 0; i < m_lanList.size(); ++i)
        {
            if (m_lanList[i].partition == partition)
            {
                auto_ptr<Leaf> newLeaf(new Leaf());
                newLeaf->setParent(tree);
                newLeaf->setLanIndex(i);
                newLeaf->setNetworkEntry(m_lanList[i].ip, m_lanMaskSize);
                outLan[i].number = m_lanList[i].number;
                outLan[i].weight = m_lanList[i].weight;
                outLan[i].partition = newLeaf.get();
                outLan[i].hosts = m_lanList[i].nodes;
                auto_ptr<ptree::Node> tempAutoPtr(newLeaf.release());
                tree->addChild(tempAutoPtr);
            }
        }
    }
}

Assigner::NodeLookup & ConservativeAssigner::getHosts(void)
{
    return m_nodeToLan;
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
