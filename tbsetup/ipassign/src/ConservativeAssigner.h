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
private:
    struct Lan
    {
        Lan()
            : weight(0), partition(0), number(0), ip(0)
        {
        }
        // The partition number associated with this LAN. This is the index
        // of the LAN in m_lanList. Not to be confused with 'partition'.
        size_t number;
        // The weight of this LAN.
        int weight;
        // The partition number of the super-partition this LAN is a part of.
        int partition;
        // The IP Address prefix that this LAN provides. Note that with
        // conservative IP assignment, every LAN has the same netmask.
        IPAddress ip;
        // The list of nodes which have interfaces in this LAN.
        std::vector<size_t> nodes;
    };
public:
    ConservativeAssigner(Partition & newPartition);
    virtual ~ConservativeAssigner();

    // For explanation of the purpose of these functions, see 'Assigner.h'
    virtual std::auto_ptr<Assigner> clone(void) const;

    virtual void setPartition(Partition & newPartition);

    virtual void addLan(int bits, int weight, std::vector<size_t> nodes);
    virtual void ipAssign(void);
    virtual void print(std::ostream & output) const;
    virtual void graph(std::vector<NodeLookup> & nodeToLevel,
                       std::vector<MaskTable> & levelMaskSize,
                       std::vector<PrefixTable> & levelPrefix,
                       std::vector<LevelLookup> & levelMakeup,
                       std::vector<int> & lanWeights) const;
private:
    // populate arguments with the METIS graph format
    void convert(std::vector<int> & indexes, std::vector<int> & neighbors,
                 std::vector<int> & weights) const;
    // Given a lan, add the appropriate adjacency and weight information
    // to arrays.
    void convertAddLan(Lan const & lanToAdd, std::vector<int> & neighbors,
                       std::vector<int> & weights) const;

    // Given the number of a node, calculate the adjacencies and weights
    // and put them into arrays.
    void convertAddNode(Lan const & info, int currentNode,
                        std::vector<int> & neighbors,
                        std::vector<int> & weights) const;
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
};

#endif

