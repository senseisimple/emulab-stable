// FixedPartition.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

// This partitioning scheme uses a fixed number of partitions set on the
// command line. METIS is used.

#ifndef FIXED_PARTITION_H_IP_ASSIGN_2
#define FIXED_PARTITION_H_IP_ASSIGN_2

#include "Partition.h"

class FixedPartition : public Partition
{
public:
    // The number of partitions is set in the constructor.
    FixedPartition(int newCount = 0)
        : m_partitionCount(newCount)
    {
    }

    // This does nothing because the number of partitions is fixed.
    virtual void addLan(void)
    {
    }

    // Pass off the partitioning job to the common code in Partition.
    // We know how many partitions we are supposed to have.
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
