// FixedPartition.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef FIXED_PARTITION_H_IP_ASSIGN_2
#define FIXED_PARTITION_H_IP_ASSIGN_2

#include "Partition.h"

class FixedPartition : public Partition
{
public:
    FixedPartition(int newCount = 0)
        : m_partitionCount(newCount)
    {
    }

    virtual void addLan(void)
    {
    }

    virtual void partition(std::vector<int> & indexes,
                           std::vector<int> & neighbors,
                           std::vector<int> & weights,
                           std::vector<int> & partitions)
    {
        partitions.resize(indexes.size() - 1);
        fill(partitions.begin(), partitions.end(), 0);
        Partition::partitionN(m_partitionCount, indexes, neighbors, weights,
                              partitions);
    }

    virtual std::auto_ptr<Partition> clone(void)
    {
        return std::auto_ptr<Partition>(new FixedPartition(*this));
    }

    virtual int getPartitionCount(void)
    {
        return m_partitionCount;
    }
private:
    int m_partitionCount;
};

#endif
