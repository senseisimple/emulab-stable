// SearchPartition.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef SEARCH_PARTITION_H_IP_ASSIGN_2
#define SEARCH_PARTITION_H_IP_ASSIGN_2

#include "Partition.h"

class SearchPartition : public Partition
{
public:
    SearchPartition()
        : m_lanCount(0)
        , m_finalCount(0)
    {
    }

    virtual void addLan(void)
    {
        ++m_lanCount;
    }

    virtual void partition(std::vector<int> & indexes,
                           std::vector<int> & neighbors,
                           std::vector<int> & weights,
                           std::vector<int> & partitions)
    {
        partitions.resize(indexes.size() - 1);
        fill(partitions.begin(), partitions.end(), 0);
        int limit = static_cast<int>(sqrt(m_lanCount));
        long long bestScore = cube(indexes.size() - 1);
        int bestCount = 1;

        std::vector<int> current;
        current.resize(partitions.size());
        long long currentScore = 0;

        for (int i = 2; i < limit; ++i)
        {
            int partitionCount = i;
            Partition::partitionN(partitionCount, indexes, neighbors, weights,
                                  current);
            currentScore = score(partitionCount, indexes, neighbors, current);
            partitionCount = Partition::makeConnectedGraph(partitionCount,
                                                           indexes, neighbors,
                                                           weights,
                                                           current);
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

    long long score(int partitionCount,
              std::vector<int> const & indexes,
              std::vector<int> const & neighbors,
              std::vector<int> const & partitions)
    {
        std::vector<int> partitionSizes;
        partitionSizes.resize(partitionCount);
        fill(partitionSizes.begin(), partitionSizes.end(), 0);
        int borderLanCount = 0;
        size_t i = 0;
        for (i = 0; i < indexes.size() - 1; ++i)
        {
            int sourceLan = static_cast<int>(i);
            int sourcePartition = partitions[sourceLan];
            ++(partitionSizes[sourcePartition]);
            int loop = indexes[sourceLan];
            int limit = indexes[sourceLan + 1];
            bool isBorder = false;
            for ( ; loop != limit && !isBorder; ++loop)
            {
                int destLan = neighbors[loop];
                int destPartition = partitions[destLan];
                if (sourcePartition != destPartition)
                {
                    isBorder = true;
                }
            }
            if (isBorder)
            {
                ++borderLanCount;
            }
        }
        long long result = 0;
        result += cube(borderLanCount);
        for (i = 0; i < partitionSizes.size(); ++i)
        {
            result += cube(partitionSizes[i]);
        }
        return result;
    }

    long long cube(long long num)
    {
        return num * num * num;
    }

    virtual std::auto_ptr<Partition> clone(void)
    {
        return std::auto_ptr<Partition>(new SearchPartition(*this));
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


