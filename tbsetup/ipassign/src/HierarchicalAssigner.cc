// HierarchicalAssigner.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "lib.h"
#include "HierarchicalAssigner.h"
#include "bitmath.h"

using namespace std;

HierarchicalAssigner::HierarchicalAssigner(Partition & newPartition)
    : convertAdapter(lans, hosts, lanVertexNumbers)
    , m_partition(&newPartition)
{
}

HierarchicalAssigner::~HierarchicalAssigner()
{
}
/*
HierarchicalAssigner::HierarchicalAssigner(HierarchicalAssigner
                                           const & right)
{
}
*/

// TODO: Clean up spurious copy semantics
// Clone is disabled for now. This is not used.
auto_ptr<Assigner>  HierarchicalAssigner::clone(void) const
{
//    return auto_ptr<Assigner>(new HierarchicalAssigner(*this));
    return auto_ptr<Assigner>();
}

void HierarchicalAssigner::setPartition(Partition & newPartition)
{
    m_partition = &newPartition;
}

void HierarchicalAssigner::addLan(int /*bits*/, int weight,
                                  std::vector<size_t> nodes)
{
    ptree::LeafLan temp;
    temp.number = lans.size();
    temp.weight = weight;
    temp.hosts = nodes;
    lans.push_back(temp);
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        int hostNumber = nodes[i];
        if (hostNumber >= hosts.size())
        {
            hosts.resize(hostNumber + 1);
        }
        hosts[hostNumber].push_back(lans.back().number);
    }
}

void HierarchicalAssigner::ipAssign(void)
{
    auto_ptr<ptree::Branch> root;
    IPAddress subnet = prefix;
    int blockBits = totalBits - prefixBits;
    lanVertexNumbers.resize(lans.size());
    list<size_t> lanIndexes;
    for (size_t i = 0; i < lans.size(); ++i)
    {
        lanIndexes.push_back(i);
        lanVertexNumbers[i] = i;
        lans[i].partition = NULL;
    }
    divideAndConquer(root, subnet, blockBits, lanIndexes);
    tree = root;
}

void HierarchicalAssigner::divideAndConquer(auto_ptr<ptree::Branch> & parent,
                                            IPAddress subnet,
                                            int blockBits,
                                            list<size_t> & partitionList)
{
    vector< list<size_t> > components;

    initPartition(parent.get(), partitionList);

    bool goodPartition = partitionPartition(parent.get(), blockBits,
                                            partitionList, components);
    auto_ptr<ptree::Branch> current(new ptree::Branch());
    current->setNetworkEntry(subnet, totalBits - blockBits);
    if (goodPartition)
    {
        int newBits = blockBits - countToBlockBit(components.size());
        // For each component
        for (size_t i = 0; i < components.size(); ++i)
        {
            IPAddress newSubnet = (i << newBits) | subnet;
            // if that partition has a size greater than 1
            if (components[i].size() > 1)
            {
                // recurse
                divideAndConquer(current, newSubnet, newBits, components[i]);
            }
            else if (components[i].size() == 1)
            {
                // create a leaf node for the LAN.
                auto_ptr<ptree::Leaf> newLeaf(new ptree::Leaf());
                newLeaf->setNetworkEntry(newSubnet, totalBits - newBits);
                newLeaf->setLanIndex(components[i].front());
                lans[components[i].front()].partition = newLeaf.get();
                current->addChild(auto_ptr<ptree::Node>(newLeaf.release()));
            }
        }
    }
    else
    {
        // create a leaf node for each LAN.
        int newBits = blockBits - countToBlockBit(partitionList.size());
        list<size_t>::iterator pos = partitionList.begin();
        list<size_t>::iterator limit = partitionList.end();
        for (unsigned int num = 0 ; pos != limit; ++pos, ++num)
        {
            IPAddress newSubnet = (num << newBits) | subnet;
            auto_ptr<ptree::Leaf> newLeaf(new ptree::Leaf());
            newLeaf->setNetworkEntry(newSubnet, totalBits - newBits);
            newLeaf->setLanIndex(*pos);
            lans[*pos].partition = newLeaf.get();
            current->addChild(auto_ptr<ptree::Node>(newLeaf.release()));
        }
    }
    if (parent.get() != NULL)
    {
        parent->addChild(auto_ptr<ptree::Node>(current.release()));
    }
    else
    {
        parent.reset(current.release());
    }
}

void HierarchicalAssigner::initPartition(ptree::Node * parent,
                                         list<size_t> & partitionList)
{
    // Set up vertex numbers & partition
    int currentVertexNumber = 0;
    list<size_t>::iterator initPos = partitionList.begin();
    list<size_t>::iterator initLimit = partitionList.end();
    for ( ; initPos != initLimit; ++initPos)
    {
        lans[*initPos].partition = parent;
        lanVertexNumbers[*initPos] = currentVertexNumber;
        ++currentVertexNumber;
    }
}

bool HierarchicalAssigner::partitionPartition(ptree::Node * parent,
                                           int blockBits,
                                           list<size_t> & partitionList,
                                           vector< list<size_t> > & components)
{
    // Convert the graph to the proper format
    std::vector<int> partitioning;
    convertAdapter.reset(parent, partitionList);
    converter.reset(convertAdapter);

    // Partition the graph.
    m_partition->partition(converter.getIndexes(),
                           converter.getNeighbors(),
                           converter.getWeights(),
                           partitioning);

    // Ensure that all sub-partitions fit in the blockspace.
    bool goodPartition = partitioningIsOk(blockBits, partitioning,
                                          partitionList);
    if (goodPartition)
    {
        // divide up partitionList amongst the children.
        components.resize(m_partition->getPartitionCount());
        list<size_t>::iterator dividePos = partitionList.begin();
        list<size_t>::iterator divideLimit = partitionList.end();
        list<size_t>::iterator divideNext = dividePos;
        vector<int>::iterator partPos = partitioning.begin();
        while (dividePos != divideLimit)
        {
            // init
            ++divideNext;

            components[*partPos].splice(components[*partPos].end(),
                                        partitionList, dividePos);

            // get ready for next iteration
            dividePos = divideNext;
            ++partPos;
        }
    }
    return goodPartition;
}

bool HierarchicalAssigner::partitioningIsOk(int blockBits,
                                            vector<int> & partitioning,
                                            list<size_t> & partitionList)
{
    int partitionBits = countToBlockBit(m_partition->getPartitionCount());
    vector<int> lanCount;
    lanCount.resize(m_partition->getPartitionCount());
    fill(lanCount.begin(), lanCount.end(), 0);

    vector<int> hostCount;
    hostCount.resize(m_partition->getPartitionCount());
    fill(hostCount.begin(), hostCount.end(), 0);

    list<size_t>::iterator pos = partitionList.begin();
    list<size_t>::iterator limit = partitionList.end();
    vector<int>::iterator partitionNumberPos = partitioning.begin();
    for ( ; pos != limit; ++pos, ++partitionNumberPos)
    {
        int partNumber = *partitionNumberPos;
        ++(lanCount[partNumber]);
        hostCount[partNumber] = max(hostCount[partNumber],
                                    static_cast<int>(lans[*pos].hosts.size()));
    }
    bool ok = true;
    for (int i = 0; i < lanCount.size(); ++i)
    {
        if (blockBits < partitionBits + countToBlockBit(lanCount[i])
                                      + countToBlockBit(hostCount[i]))
        {
            ok = false;
        }
    }
    return ok;
}

void HierarchicalAssigner::print(std::ostream & output) const
{
    vector<ptree::LeafLan>::const_iterator lanPos = lans.begin();
    for ( ; lanPos != lans.end(); ++lanPos)
    {
        size_t i = 0;
        for ( ; i < lanPos->hosts.size(); ++i)
        {
            output << lanPos->number << " ";
            output << lanPos->hosts[i] << " ";
            output << ipToString(lanPos->partition->getPrefix() + i + 1);
            output << endl;
        }
    }
}

void HierarchicalAssigner::getGraph(std::auto_ptr<ptree::Node> & outTree,
                                    std::vector<ptree::LeafLan> & outLans)
{
    outTree = tree;
    outLans = lans;
}

Assigner::NodeLookup & HierarchicalAssigner::getHosts(void)
{
    return hosts;
}


