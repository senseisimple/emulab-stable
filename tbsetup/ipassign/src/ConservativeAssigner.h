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
        size_t number;
        int weight;
        int partition;
        IPAddress ip;
        std::vector<size_t> nodes;
    };
public:
    ConservativeAssigner();
    virtual ~ConservativeAssigner();
    virtual std::auto_ptr<Assigner> clone(void) const;

    virtual void setPartitionCount(size_t newCount);
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
    static int getBits(int num);
    static int getLanBits(std::vector<int> const & partitionArray,
                          int numPartitions);
    static int roundUp(double num);
    static void checkBitOverflow(int numBits);
    void assignNumbers(int networkSize, int lanSize, int hostSize,
                       int numPartitions);
    bool isConnected(int whichPartition);
    void makeConnected(int whichPartition);
private:
    std::vector<Lan> m_lanList;
    NodeLookup m_nodeToLan;
    int m_partitionCount;
    int m_largestLan;
    int m_lanMaskSize;
    int m_netMaskSize;
    PrefixTable m_partitionIPList;
};

#endif

