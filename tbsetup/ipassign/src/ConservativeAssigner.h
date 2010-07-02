// ConservativeAssigner.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

// This ip assignment algorithm divides the graph into a set number of
// partitions. It uses conservative bit assignment rather than dynamic
// bit assignment.

#ifndef CONSERVATIVE_ASSIGNER_H_IP_ASSIGN_2
#define CONSERVATIVE_ASSIGNER_H_IP_ASSIGN_2

#include "Assigner.h"

class ConservativeAssigner : public Assigner
{
public:
    ConservativeAssigner(Partition & newPartition);
    virtual ~ConservativeAssigner();

    // For explanation of the purpose of these functions, see 'Assigner.h'
    virtual std::auto_ptr<Assigner> clone(void) const;

    virtual void setPartition(Partition & newPartition);

    virtual void addLan(int bits, int weight, std::vector<size_t> nodes);
    virtual void ipAssign(void);
    virtual void print(std::ostream & output) const;

    virtual void getGraph(std::auto_ptr<ptree::Node> & tree,
                          std::vector<ptree::LeafLan> & lans);
    virtual NodeLookup & getHosts(void);

/* TODO: Remove this when changes are debugged
    virtual void graph(std::vector<NodeLookup> & nodeToLevel,
                       std::vector<MaskTable> & levelMaskSize,
                       std::vector<PrefixTable> & levelPrefix,
                       std::vector<LevelLookup> & levelMakeup,
                       std::vector<int> & lanWeights) const;

*/

private:
    void populateSuperPartitionTree(size_t superPartition,
                                    ptree::Branch * tree,
                                    std::vector<ptree::LeafLan> & outLan);

    void populatePartitionTree(size_t partition,
                               ptree::Branch * tree,
                               std::vector<ptree::LeafLan> & outLan);

    // Given a number, how many bits are required to represent up to and
    // including that number?
    static int getBits(int num);
    // How many bits are required to represent each LAN in each partition?
    static int getLanBits(std::vector<int> const & partitionArray,
                          int numPartitions);
    static int roundUp(double num);
    // If we don't have enough bitspace to represent every partition, LAN,
    // and interface, throw an exception and abort.
    static void checkBitOverflow(int numBits);

    // Actually give ip prefixes to each LAN and partition.
    void assignNumbers(int networkSize, int lanSize, int hostSize,
                       int numPartitions);
    // Do the LANs withing a particular partition have a path to each other?
    bool isConnected(int whichPartition);

    // If a particular partition is disconnected, seperate it into two
    // partitions.
    void makeConnected(int whichPartition);

    void calculateSuperPartitions(void);
private:
    // Information about each LAN, indexed by the number assigned to each
    // LAN.
    std::vector<Lan> m_lanList;
    // At this point, we only have to worry about one level of hierarchy.
    // Given a node number, what are the LANs to which it belongs?
    // See Assigner.h for a definition of 'NodeLookup'
    NodeLookup m_nodeToLan;
    // The number of partitions the LAN-graph was divided up into.
    int m_partitionCount;
    // The number of nodes in the largest LAN.
    int m_largestLan;
    // The number of bits in the netmask of each LAN.
    int m_lanMaskSize;
    // The number of bits in the netmask of each partition.
    int m_netMaskSize;
    // IP prefixes for every partition.
    PrefixTable m_partitionIPList;
    // This pointer is used polymorphically to determine which partitioning
    // method to use.
    Partition * m_partition;
    // The super-partition table. Each vector contains all of the
    // partitions inside the super-partition. The number of
    // super-partitions is the number of internally connected
    // sub-graphs in the (possibly disconnected) graph.
    std::vector< std::vector<size_t> > m_superPartitionList;
};

#endif
