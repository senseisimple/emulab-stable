// RatioCutPartition.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef RATIO_CUT_PARTITION_H_IP_ASSIGN_2
#define RATIO_CUT_PARTITION_H_IP_ASSIGN_2

// Occasionally, there will be a reference to a page number. These
// page numbers are from the paper "Ratio Cut Partitioning for
// Hierarchical Designs" from which the ratio-cut algorithm for
// division is derived.

#include "Partition.h"

class RatioCutPartition : public Partition
{
private:
    // RatioCut divides the graph into two partitions. One partition
    // has the source and the other has the sink. This enum is used as
    // an array index to discriminate between them. There are a few
    // auxilliary declarations here to make it easy to change this in
    // the future if need be.
    enum ANCHOR
    {
        ANCHOR_MIN = 0,
        SOURCE = 0,
        SINK = 1,
        ANCHOR_MAX = 1,
        ANCHOR_COUNT = 2
    };

    // Given one partition number, return the other.
    ANCHOR flip(ANCHOR num)
    {
        if (num == SOURCE)
        {
            return SINK;
        }
        else
        {
            return SOURCE;
        }
    }

    // A bucket is the element type for the AnchorDelta arrays. Each
    // index represents a delta for the number of cut edges. The
    // bucket has an array of indices which represent the vertices
    // which, if swapped, would change the number of cut edges by that
    // amount. The delta can be either positive or negative and the
    // vector can only be indexed with positive numbers. Therefore,
    // there is an auxilliary data structure anchorDeltaOrigin[] which
    // contains the number which represents a delta of 0. The other
    // indices can be found by adding the delta to that number.
    typedef std::list<size_t> Bucket;


    struct VertexInfo
    {
        VertexInfo() : delta(0), movable(false) {}
        int delta;
        bool movable;
        Bucket::iterator pos;
    };

    // This contains a score which represents the ratio-cut score and
    // a vertex number. This is used in different ways by different
    // functions. See initialize(), iterativeShifting(), and
    // groupSwapping() for details.
    struct ScoreVertex
    {
        ScoreVertex() : score(0.0), vertex(0) {}
        double score;
        size_t vertex;
        bool operator<(ScoreVertex const & right) const
        {
            return score < right.score;
        }
    };
public:
    RatioCutPartition()
    {
        reset();
    }

    void reset(void)
    {
        m_partitionCount = 0;
        m_indexes = NULL;
        m_neighbors = NULL;
        m_weights = NULL;
        vertexCount = 0;
        anchor[SOURCE] = 0;
        anchor[SINK] = 0;
        m_partitions = NULL;
        partitionSize[SOURCE] = 1;
        partitionSize[SINK] = 1;
        currentCut = 0;
        anchorMinIndex[SOURCE] = 0;
        anchorMinIndex[SINK] = 0;
        anchorDeltaOrigin[SOURCE] = 0;
        anchorDeltaOrigin[SINK] = 0;
        anchorDelta[SOURCE].clear();
        anchorDelta[SINK].clear();
        info.clear();
    }

    virtual void addLan(void)
    {
    }

    virtual void partition(std::vector<int> & indexes,
                           std::vector<int> & neighbors,
                           std::vector<int> & weights,
                           std::vector<int> & partitions)
    {
        // Get rid of any detritus from previous invocations.
        reset();

        if (indexes.size() <= 2)
        {
            m_partitionCount = 1;
            partitions.resize(1);
            partitions[0] = 0;
        }
        else if (indexes.size() == 3)
        {
            m_partitionCount = 2;
            partitions.resize(2);
            partitions[0] = 0;
            partitions[1] = 1;
        }
        else
        {
            // Unless the heuristic for determining ratio-cut makes
            // one or both partitions non-internally-contiguous, there
            // should be exactly two partitions.
            m_partitionCount = 2;
            vertexCount = indexes.size() - 1;
            partitions.resize(vertexCount);
            fill(partitions.begin(), partitions.end(), SOURCE);

            // The information about the graph is used internally.
            // TODO: Change RatioCut to use weights information
            m_indexes = &indexes;
            m_neighbors = &neighbors;
            m_weights = &weights;
            m_partitions = &partitions;

            ratioCut();

            // Split up non-internally-contiguous partitions.
            m_partitionCount = Partition::makeConnectedGraph(m_partitionCount,
                                                             *m_indexes,
                                                             *m_neighbors,
                                                             *m_weights,
                                                             *m_partitions);
        }
    }

    virtual std::auto_ptr<Partition> clone(void)
    {
        return std::auto_ptr<Partition>(new RatioCutPartition(*this));
    }

    virtual int getPartitionCount(void)
    {
        return m_partitionCount;
    }
private:
    void ratioCut(void)
    {
        std::vector<ScoreVertex> cost;
        cost.resize(vertexCount);

        // There are three phases of the process. See page 914, section 3.1.
        initialize(cost);
        iterativeShifting(cost);
        groupSwapping();
    }

    void initialize(std::vector<ScoreVertex> & sinkToSource)
    {
        // sinkToSource should have the correct size when we get it.

        // sinkToSource is used in the first pass where every vertex starts
        // in the sink partition and one by one they are moved to the source
        // partition.

        // sourceToSink is used in the second pass where the reverse is done.
        std::vector<ScoreVertex> sourceToSink;
        sourceToSink.resize(vertexCount);

        // Both sinkToSource and sourceToSink use the ScoreVertexes in
        // the same way. The score at a particular partition is the
        // ratio-cut score that would result if that position and
        // every position left (less than) of it were part of the
        // source partition and all of the positions right of it were
        // in the sink partition.

        size_t i = 0;

        // Calculate both source and sink. Right now, we use the
        // method on page 914, section 3.1.1. This will probably be
        // tweaked later on.
        findSink();

        // Initialize all of the data structures used in the algorithm.
        initializeDelta();

        {
            // Every vertex should start out part of the sink
            // partition except for the source vertex.
            for (i = 0; i < info.size(); ++i)
            {
                if ((*m_partitions)[i] == SOURCE && i != anchor[SOURCE])
                {
                    // We know that the SINK anchor will not be swapped
                    // here, because every vertex is initialized to SINK
                    // except for the SOURCE anchor.
                    swapVertex(i);
                }
            }
            // The left (index 0) side of the vector represents the
            // source.  The score is calculated assuming every vertex
            // is in the sink partition except for the source.
            sinkToSource.front().vertex = anchor[SOURCE];
            sinkToSource.front().score = calculateScore(currentCut,
                                                        partitionSize[SOURCE],
                                                        partitionSize[SINK]);
            // The right (index size() - 1) of the vector represents
            // the sink. This position should never be used because by
            // the definition of the data structure above, that would
            // mean that the sink is in the source partition. This
            // must never be.
            sinkToSource.back().vertex = anchor[SINK];
//            sinkToSource.back().score = 1e37;

            std::vector<ScoreVertex>::iterator anchorPos[ANCHOR_COUNT];
            anchorPos[SOURCE] = sinkToSource.begin();
            anchorPos[SINK] = sinkToSource.end();
            --(anchorPos[SINK]);
            // Shift takes vertexes one by one from a particular
            // partition and transfers them to the other partition.
            shift(SINK, anchorPos, sinkToSource.begin());
        }

        {
            for (i = 1; i < info.size(); ++i)
            {
                if ((*m_partitions)[i] == SINK && i != anchor[SINK])
                {
                    swapVertex(i);
                }
            }
            sourceToSink.front().vertex = anchor[SOURCE];
            sourceToSink.back().vertex = anchor[SINK];
            sourceToSink.back().score =  calculateScore(currentCut,
                                                        partitionSize[SOURCE],
                                                        partitionSize[SINK]);
            std::vector<ScoreVertex>::iterator anchorPos[ANCHOR_COUNT];
            anchorPos[SOURCE] = sourceToSink.begin();
            anchorPos[SINK] = sourceToSink.end();
            --(anchorPos[SINK]);
            std::vector<ScoreVertex>::iterator lastSource;
            lastSource = sourceToSink.end();
            advance(lastSource, -2);
            shift(SOURCE, anchorPos, lastSource);
        }

        // We now have two vectors, each representing successive
        // swaps. One transfers from the source to the sink, the other
        // from the sink to the source. We want to choose the best
        // partitioning of these two.
        std::vector<ScoreVertex>::iterator sinkBest;
        sinkBest = min_element(sinkToSource.begin(), sinkToSource.end());
        std::vector<ScoreVertex>::iterator sourceBest;
        sourceBest = min_element(sourceToSink.begin(), sourceToSink.end());
        if (sourceBest->score > sinkBest->score)
        {
            setPartitioning(sourceToSink, sourceBest);
            sinkToSource.swap(sourceToSink);
        }
        else
        {
            setPartitioning(sinkToSource, sinkBest);
        }
    }

    void iterativeShifting(std::vector<ScoreVertex> & scoreList)
    {
        // TODO: Remove this spurious search by changing method call
        // parameters.

        // Find the partitioning that provides the best score (so far).
        std::vector<ScoreVertex>::iterator anchorPos[ANCHOR_COUNT];
        anchorPos[SOURCE] = scoreList.begin();
        anchorPos[SINK] = scoreList.end();
        --(anchorPos[SINK]);

        std::vector<ScoreVertex>::iterator least;
        least = min_element(anchorPos[SOURCE], anchorPos[SINK]);

        bool done = false;

        std::vector<ScoreVertex>::iterator before;
        shift(SOURCE, anchorPos, least);
        before = min_element(scoreList.begin(), scoreList.end());
        setPartitioning(scoreList, before);

        std::vector<ScoreVertex>::iterator after;
        shift(SINK, anchorPos, before);
        after = min_element(scoreList.begin(), scoreList.end());
        setPartitioning(scoreList, after);
        while (before != after)
        {
            shift(SOURCE, anchorPos, least);
            before = min_element(scoreList.begin(), scoreList.end());
            setPartitioning(scoreList, before);

            shift(SINK, anchorPos, before);
            after = min_element(scoreList.begin(), scoreList.end());
            setPartitioning(scoreList, after);

            least = after;
        }
    }

    void groupSwapping(void)
    {
        // At this point, a local minima has been reached. There is no
        // single swap which will reduce the score. Now we want to
        // find groups of swaps which, when taken together, reduce the
        // score.

        // operations is where we record each swap in the order that
        // the swap occurs. This way, we can backtrack to the best
        // place.
        std::vector<ScoreVertex> operations;
        operations.resize(vertexCount - 1);
        operations.front().vertex = 0;
        bool done = false;
        while (!done)
        {
            operations.front().score = calculateScore(currentCut,
                                                      partitionSize[SOURCE],
                                                      partitionSize[SINK]);

            // Swap every vertex. This is to get out of the local minima we
            // describe above.
            swapAllVertices(operations);

            // Pick the position with the least score. If the first
            // position is picked, the original score is
            // best. Otherwise, there is some sequence of swaps which
            // improve the score. We undo all of the swaps after that
            // point.
            std::vector<ScoreVertex>::iterator least;
            least = min_element(operations.begin(), operations.end());
            if (least == operations.begin())
            {
                done = true;
            }
            backtrack(least, operations);
        }
    }

    void swapAllVertices(std::vector<ScoreVertex> & operations)
    {
        // We want to swap every vertex except the source and sink
        // vertices. We want to do this in reverse order of delta score.
        for (size_t i = 1; i < operations.size(); ++i)
        {
            ANCHOR part = ANCHOR_MIN;
            // Choose the partition with the smallest delta.
            if (anchorMinIndex[SOURCE] < anchorMinIndex[SINK])
            {
                part = SOURCE;
            }
            else
            {
                part = SINK;
            }
            size_t minIndex = anchorMinIndex[part];
            size_t vertex = anchorDelta[part][minIndex].front();
            swapVertex(vertex);
            // The score associated with this operation is the total score
            // once the swap has occurred.
            operations[i].score = calculateScore(currentCut,
                                                 partitionSize[SOURCE],
                                                 partitionSize[SINK]);
            operations[i].vertex = vertex;
            removeVertex(vertex);
        }
    }

    void backtrack(std::vector<ScoreVertex>::iterator backPos,
                   std::vector<ScoreVertex> & operations)
    {
        // backPos starts out pointing to the last operation which we
        // don't want to undo. Therefore, move it to the next one.
        if (backPos != operations.end())
        {
            ++backPos;
        }

        // Add every vertex back to the index.
        for (size_t i = 1; i < operations.size(); ++i)
        {
            addVertex(operations[i].vertex);
        }

        // undo every operation we've done.
        for ( ; backPos != operations.end(); ++backPos)
        {
            swapVertex(backPos->vertex);
        }
    }

    void setPartitioning(std::vector<ScoreVertex> & partitioning,
                         std::vector<ScoreVertex>::iterator lastSource)
    {
        // Make sure we don't go beyond the end.
        if (lastSource != partitioning.end())
        {
            ++lastSource;
        }

        std::vector<ScoreVertex>::iterator pos;
        std::vector<ScoreVertex>::iterator limit;

        // Everything to the left of lastSource should be part of the
        // SOURCE partition.
        pos = partitioning.begin();
        limit = lastSource;
        for ( ; pos != limit; ++pos)
        {
            if ((*m_partitions)[pos->vertex] == SINK
                && pos->vertex != anchor[SOURCE]
                && pos->vertex != anchor[SINK])
            {
                swapVertex(pos->vertex);
            }
        }

        // Everything to the right of lastSource should be part of the
        // SINK partition.
        pos = lastSource;
        limit = partitioning.end();
        for ( ; pos != limit; ++pos)
        {
            if ((*m_partitions)[pos->vertex] == SOURCE
                && pos->vertex != anchor[SOURCE]
                && pos->vertex != anchor[SINK])
            {
                swapVertex(pos->vertex);
            }
        }
    }
private:
    void findSink(void)
    {
        std::queue<size_t> cache;
        std::vector<bool> touched;
        touched.resize(vertexCount);
        fill(touched.begin(), touched.end(), false);

        size_t source = 0;
        size_t sink = 0;
        cache.push(source);
        while (!(cache.empty()))
        {
            sink = cache.front();
            cache.pop();
            for (size_t i = (*m_indexes)[sink]; i < (*m_indexes)[sink + 1];
                                                                           ++i)
            {
                size_t neighbor = (*m_neighbors)[i];
                if (!(touched[neighbor]))
                {
                    touched[neighbor] = true;
                    cache.push(touched[neighbor]);
                }
            }
        }
        anchor[SOURCE] = source;
        anchor[SINK] = sink;
    }

    void initializeDelta(void)
    {
        int maxDegree = 0;
        size_t i = 0;
        for (i = 0; i < vertexCount; ++i)
        {
            maxDegree = std::max(maxDegree,
                                 (*m_indexes)[i + 1] - (*m_indexes)[i]);
        }
        anchorDeltaOrigin[SOURCE] = maxDegree;
        anchorDeltaOrigin[SINK] = maxDegree;
        anchorDelta[SOURCE].resize((2 * maxDegree) + 1);
        anchorDelta[SINK].resize((2 * maxDegree) + 1);

        fill(m_partitions->begin(), m_partitions->end(), SINK);
        (*m_partitions)[anchor[SOURCE]] = SOURCE;

        anchorMinIndex[SOURCE] = anchorDelta[SOURCE].size() - 1;
        anchorMinIndex[SINK] = anchorDelta[SINK].size() - 1;

        info.resize(vertexCount);
        for (i = 0; i < vertexCount; ++i)
        {
            if (i != anchor[SOURCE] && i != anchor[SINK])
            {
                addVertex(i);
            }
            info[i].delta = calculateDelta(i);
        }

        currentCut = (*m_indexes)[anchor[SOURCE] + 1]
                                                - (*m_indexes)[anchor[SOURCE]];
        partitionSize[SOURCE] = 1;
        partitionSize[SINK] = vertexCount - 1;
    }

    void shift(ANCHOR fromPartition,
               std::vector<ScoreVertex>::iterator anchorPos[ANCHOR_COUNT],
               std::vector<ScoreVertex>::iterator lastSourcePos)
    {
        int step = 0;
        std::vector<ScoreVertex>::iterator pos = lastSourcePos;
        if (fromPartition == SOURCE)
        {
            step = -1;
        }
        else if (pos != anchorPos[fromPartition])
        {
            step = 1;
            // pos now points to a vertex which is part of the source
            // partition. It should point one past that.
            ++pos;
        }
        while (pos != anchorPos[fromPartition])
        {
            size_t deltaIndex = anchorMinIndex[fromPartition];
            size_t bestVertex = anchorDelta[fromPartition][deltaIndex].front();
            swapVertex(bestVertex);
            pos->score = calculateScore(currentCut, partitionSize[SOURCE],
                                        partitionSize[SINK]);
            pos->vertex = bestVertex;
            advance(pos, step);
        }
    }

    double calculateScore(int cutScore, int sourceCount, int sinkCount)
    {
        return static_cast<double>(cutScore) /
           (static_cast<double>(sourceCount) * static_cast<double>(sinkCount));
    }

    void swapVertex(size_t vertex)
    {
        if (info[vertex].movable)
        {
            ANCHOR startAnchor = static_cast<ANCHOR>((*m_partitions)[vertex]);

            // Adjust global statistics
            currentCut += info[vertex].delta;
            --(partitionSize[startAnchor]);
            ++(partitionSize[flip(startAnchor)]);

            // Adjust neighbor delta scores
            int newVertexScore = 0;
            for (size_t i = (*m_indexes)[vertex]; i < (*m_indexes)[vertex + 1];
                                                                           ++i)
            {
                size_t neighbor = (*m_neighbors)[i];
                if ((*m_partitions)[neighbor] == startAnchor)
                {
                    --newVertexScore;
                    moveVertex(neighbor, info[neighbor].delta - 1,
                               startAnchor);
                    --(info[neighbor].delta);
                }
                else
                {
                    ++newVertexScore;
                    moveVertex(neighbor, info[neighbor].delta + 1,
                               flip(startAnchor));
                    ++(info[neighbor].delta);
                }
            }

            // Change the partition of of the vertex
            moveVertex(vertex, newVertexScore, flip(startAnchor));
            (*m_partitions)[vertex] = flip(startAnchor);
        }
    }

    void normalizeDelta(ANCHOR target)
    {
        while (anchorMinIndex[target] < anchorDelta[target].size() - 1
               && anchorDelta[target][anchorMinIndex[target]].empty())
        {
            ++(anchorMinIndex[target]);
        }
    }

    void addVertex(size_t vertex)
    {
        if (!(info[vertex].movable))
        {
            info[vertex].movable = true;
            size_t currentAnchor = (*m_partitions)[vertex];
            info[vertex].delta = calculateDelta(vertex);
            size_t index = info[vertex].delta
                           + anchorDeltaOrigin[currentAnchor];
            anchorDelta[currentAnchor][index].push_front(vertex);
            info[vertex].pos = anchorDelta[currentAnchor][index].begin();
            anchorMinIndex[currentAnchor]
                                      = std::min(anchorMinIndex[currentAnchor],
                                                 index);
        }
    }

    int calculateDelta(int vertex)
    {
        ANCHOR currentAnchor = static_cast<ANCHOR>((*m_partitions)[vertex]);
        int delta = 0;
        for (size_t i = (*m_indexes)[vertex]; i < (*m_indexes)[vertex + 1];
             ++i)
        {
            size_t currentDest = (*m_neighbors)[i];
            if ((*m_partitions)[currentDest] == currentAnchor)
            {
                ++delta;
            }
            else
            {
                --delta;
            }
        }
        return delta;
    }

    void removeVertex(size_t vertex)
    {
        if (info[vertex].movable)
        {
            info[vertex].movable = false;
            ANCHOR currentAnchor
                                = static_cast<ANCHOR>((*m_partitions)[vertex]);
            size_t index = info[vertex].delta
                           + anchorDeltaOrigin[currentAnchor];
            anchorDelta[currentAnchor][index].erase(info[vertex].pos);
            normalizeDelta(currentAnchor);
        }
    }

    void moveVertex(size_t vertex, int newDelta, ANCHOR newPartition)
    {
        if (info[vertex].movable)
        {
            ANCHOR oldPartition = static_cast<ANCHOR>((*m_partitions)[vertex]);
            size_t oldDelta = info[vertex].delta;
            size_t oldIndex = anchorDeltaOrigin[oldPartition] + oldDelta;
            size_t newIndex = anchorDeltaOrigin[newPartition] + newDelta;

            Bucket & originList = anchorDelta[oldPartition][oldIndex];
            Bucket & destList = anchorDelta[newPartition][newIndex];
            originList.erase(info[vertex].pos);
            destList.push_front(vertex);
//        destList.splice(destList.begin(), originList, info[vertex].pos);
            info[vertex].pos = destList.begin();
            info[vertex].delta = newDelta;
            anchorMinIndex[newPartition]
                                     = std::min(anchorMinIndex[newPartition],
                                                newIndex);
            normalizeDelta(oldPartition);
        }
    }
private:
    int m_partitionCount;
    std::vector<int> * m_indexes;
    std::vector<int> * m_neighbors;
    std::vector<int> * m_weights;

    size_t vertexCount;
    size_t anchor[ANCHOR_COUNT];

    std::vector<int> * m_partitions;
    int partitionSize[ANCHOR_COUNT];
    int currentCut;

    size_t anchorMinIndex[ANCHOR_COUNT];
    size_t anchorDeltaOrigin[ANCHOR_COUNT];
    std::vector<Bucket> anchorDelta[ANCHOR_COUNT];
    std::vector<VertexInfo> info;
};

#endif

