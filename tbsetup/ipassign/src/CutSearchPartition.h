// CutSearchPartition.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef CUT_SEARCH_PARTITION_H_IP_ASSIGN_2
#define CUT_SEARCH_PARTITION_H_IP_ASSIGN_2

#include "Partition.h"

class CutSearchPartition : public Partition
{
public:
    CutSearchPartition()
        : m_lanCount(0)
        , m_finalCount(0)
    {
    }

    std::auto_ptr<Partition> CutSearchPartition::clone()
    {
        return std::auto_ptr<Partition>(new CutSearchPartition(*this));
    }

    virtual void addLan()
    {
    }

    virtual void partition(std::vector<int> & indexes,
                           std::vector<int> & neighbors,
                           std::vector<int> & weights,
                           std::vector<int> & partitions)
    {
        m_lanCount = static_cast<int>(indexes.size() - 1);
        partitions.resize(indexes.size() - 1);
        fill(partitions.begin(), partitions.end(), 0);
        int limit = static_cast<int>(sqrt(m_lanCount));
        double bestScore = 1e37;
        int bestCount = 1;

        std::vector<int> current;
        current.resize(partitions.size());
        double currentScore = 0;

        for (int i = 2; i < limit; ++i)
        {
            int partitionCount = i;
            int temp = Partition::partitionN(partitionCount, indexes,
                                             neighbors, weights,
                                             current);
            partitionCount = Partition::makeConnectedGraph(partitionCount,
                                                           indexes, neighbors,
                                                           weights,
                                                           current);
            currentScore = static_cast<double>(temp) / partitionCount;
            if (currentScore < bestScore)
            {
                bestCount = partitionCount;
                bestScore = currentScore;
                partitions = current;
                std::cerr << "NewBest: " << i << ":" << partitionCount
                          << std::endl;
            }
        }
        m_finalCount = bestCount;
    }

    virtual int getPartitionCount(void)
    {
        return m_finalCount;
    }
private:
    int m_lanCount;
    int m_finalCount;
};

#endif


