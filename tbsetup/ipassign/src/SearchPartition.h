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
            Partition::partitionN(i, indexes, neighbors, weights, current);
            currentScore = score(i, indexes, neighbors, current);
            if (currentScore < bestScore)
            {
                bestCount = i;
                bestScore = currentScore;
                partitions = current;
                cerr << "NewBest: " << i << endl;
            }
        }
        m_finalCount = bestCount;

/*        int lowerCount = 1;
        std::vector<int> lowerPartitions = partitions;
        long long lowerScore = cube(indexes.size() - 1);
        int upperCount = static_cast<int>(sqrt(m_lanCount));
        // This way we have a 50% chance of not having to do a copy at the end
        std::vector<int> & upperPartitions = partitions;
        Partition::partitionN(upperCount, indexes, neighbors, weights,
                              upperPartitions);
        long long upperScore = score(upperCount, indexes, neighbors,
                               upperPartitions);
        int middleCount = ((upperCount - lowerCount)/2) + lowerCount;

        while (middleCount != upperCount && middleCount != lowerCount)
        {
            cerr << "Upper: count " << upperCount << " score " << upperScore << endl;
            cerr << "Lower: count " << lowerCount << " score " << lowerScore << endl;
            if (lowerScore < upperScore)
            {
                // Replace upper with the middle
                upperCount = middleCount;
                Partition::partitionN(upperCount, indexes, neighbors, weights,
                                      upperPartitions);
                upperScore = score(upperCount, indexes, neighbors,
                                   upperPartitions);
            }
            else
            {
                // Replace lower with the middle
                lowerCount = middleCount;
                Partition::partitionN(lowerCount, indexes, neighbors, weights,
                                      lowerPartitions);
                lowerScore = score(lowerCount, indexes, neighbors,
                                   lowerPartitions);
            }
            middleCount = ((upperCount - lowerCount)/2) + lowerCount;
        }
        if (lowerScore < upperScore)
        {
            partitions = lowerPartitions;
            m_finalCount = lowerCount;
        }
        else
        {
            m_finalCount = upperCount;
            }*/
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


