// Partition.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef PARTITION_H_IP_ASSIGN_2
#define PARTITION_H_IP_ASSIGN_2

class Partition
{
public:
    // Notify this partition that there is another LAN. Used in schemes
    // that depend on the number of LANs in a graph. Now that I think about
    // it, this is redundant. The number of LANs can be garnerd from the
    // size of indexes at partition time.
    // TODO: Remove this function and the need to use it.
    virtual void addLan(void)=0;

    // Partition up the graph. indexes, neighbors, and weights are the
    // converted form of the LAN-graph.
    virtual void partition(std::vector<int> & indexes,
                           std::vector<int> & neighbors,
                           std::vector<int> & weights,
                           std::vector<int> & partitions)=0;

    // Get a polymorphic copy of this object.
    virtual std::auto_ptr<Partition> clone(void)=0;

    // After partitioning, how many partitions were decided upon?
    virtual int getPartitionCount(void)=0;
public:
    // Most partitioning schemes (at least in the near future) will presumably
    // use METIS. This is the common code to use METIS in the standard fashion.
    static int partitionN(int partitionCount,
                           std::vector<int> & indexes,
                           std::vector<int> & neighbors,
                           std::vector<int> & weights,
                           std::vector<int> & partitions /* output */)
    {
        if (neighbors.size() != weights.size())
        {
            throw StringException("Partition: Array size mismatch");
        }
        int numVertices = indexes.size() - 1;
        int weightOption = 1; // Weights on edges only
        int indexOption = 0; // 0-based index
        int options[5];
        int edgesCut = 0;

        if (partitionCount > 1 && partitionCount <= 8
            && numVertices >= partitionCount)
        {
            options[0] = 0; // use defaults. Ignore other options.
            options[1] = 3; // (default) Sorted Heavy-Edge matching
            options[2] = 1; // (default) Region Growing
            options[3] = 1; // (default) Early-Exit Boundary FM refinement
            options[4] = 0; // (default) Always set to 0

            METIS_PartGraphRecursive(
                &numVertices,              // # of vertices in graph
                &(indexes[0]),     // xadj
                &(neighbors[0]),  // adjncy
                NULL,                      // vertex weights
                &(weights[0]),         // edge weights
                &weightOption,             // Weights on edges only
                &indexOption,              // 0-based index
                &partitionCount,
                options,                   // set options to default
                &edgesCut,                 // return the # of edges cut
                &(partitions[0]));     // return the partitions
        }
        else if (partitionCount > 1 && numVertices >= partitionCount)
        {
            options[0] = 1; // ignore defaults. Use other options.
            options[1] = 3; // (default) Sorted Heavy-Edge Matching
            options[2] = 1; // (default) Multilevel recursive bisection
            options[3] = 2; // Greedy boundary refinement
            options[4] = 0; // (default) Always set to 0
            METIS_PartGraphKway(
                &numVertices,              // # of vertices in graph
                &(indexes[0]),     // xadj
                &(neighbors[0]),  // adjncy
                NULL,                      // vertex weights
                &(weights[0]),         // edge weights
                &weightOption,             // Weights on edges only
                &indexOption,              // 0-based index
                &partitionCount,
                options,                   // set options to nonrandom
                &edgesCut,                 // return the # of edges cut
                &(partitions[0]));     // return the partitions
        }
        else
        {
            fill(partitions.begin(), partitions.end(), 0);
        }
        return edgesCut;
    }

    // Goes through every partition in the graph and checks to see if
    // that partition is connected. If not, the partition is
    // separated. Returns the new partitionCount after these new
    // partitions are created.
    static int makeConnectedGraph(int partitionCount,
                                  std::vector<int> & indexes,
                                  std::vector<int> & neighbors,
                                  std::vector<int> & weights,
                                  std::vector<int> & partitions)
    {
        // If there are disconnected partitions, turn them into
        // different partitions.
        // partitionCount may increase during this loop
        int loop = 0;
        while (loop < partitionCount)
        {
            // Each time a breadth-first walk from a LAN in a
            // partition fails to reach every LAN in that partition,
            // the remainder is slopped together into a new partition
            // which is m_partitionCount+1.  Then m_partitionCount is
            // incremented.
            partitionCount = makeConnectedPartition(loop, partitionCount,
                                                    indexes, neighbors,
                                                    weights, partitions);
            ++loop;
        }
        return partitionCount;
    }

    // Checks to ensure that a particular partition is internally
    // connected.  If it is not, it leaves the connected portion in
    // whichPartition and puts the rest into a newly created
    // partition. Returns the number of partitions there are after the
    // potential split. This is so that a function using it can call
    // it again on the new partition to ensure that the new one is
    // connected.
    static int makeConnectedPartition(int whichPartition,
                                      int partitionCount,
                                      std::vector<int> & indexes,
                                      std::vector<int> & neighbors,
                                      std::vector<int> & weights,
                                      std::vector<int> & partitions)
    {
        // breadth first search.
        std::queue<size_t> nextConnection;
        size_t first = 0;
        size_t numInPartition = 0;
        size_t i = 0;
        bool hasFirst = false;
        std::vector<bool> connected;
        connected.resize(partitions.size());

        // Set all LANs in whichPartition to false. All others to true.
        for (i = 0; i < connected.size(); ++i)
        {
            if (partitions[i] == whichPartition)
            {
                connected[i] = false;
                first = i;
                ++numInPartition;
                hasFirst = true;
            }
            else
            {
                connected[i] = true;
            }
        }

        // If we found no vertices in whichPartition, there is nothing to
        // do. An empty partition is a connected one.
        if (!hasFirst)
        {
            return partitionCount;
        }

        // Starting with a random LAN in whichPartition, set all neighbors
        // to true. Repeat until all connected LANs are true.
        nextConnection.push(first);
        connected[first] = true;
        while (!(nextConnection.empty()))
        {
            size_t current = nextConnection.front();
            nextConnection.pop();
            // indexes[current] is the first index into neighbors which
            // yields the edge list. indexes[current + 1] is the first
            // index of the edges of the next vertex.

            // See METIS documentation for description of the
            // indexes,neighbors,weights,partitions format specification.
            for (i = indexes[current]; i < indexes[current + 1]; ++i)
            {
                size_t destLan = neighbors[i];
                if (partitions[destLan] == whichPartition
                    && !(connected[destLan]))
                {
                    connected[destLan] = true;
                    nextConnection.push(destLan);
                }
            }
        }
        // If any LANs are false, put them in a new partition because
        // they are not connected.
        bool newPartition = false;
        for (i = 0; i < connected.size(); ++i)
        {
            if (!(connected[i]))
            {
                newPartition = true;
                partitions[i] = partitionCount;
            }
        }
        if (newPartition)
        {
            ++partitionCount;
        }

        return partitionCount;
    }
private:
};


// info
// This is the code for calling METIS and changing its tolerances.
/*    if (numPartitions > 1)
    {
        int numConstraints = 2;
        weightOption = 1;
        vector<int> vertexWeights;
        vertexWeights.resize(numConstraints*numVertices);
        fill(vertexWeights.begin(), vertexWeights.end(), 1);
        vector<float> tolerances;
        tolerances.resize(numConstraints);
        fill(tolerances.begin(), tolerances.end(), static_cast<float>(1.5));
        options[0] = 0;
        METIS_mCPartGraphKway(
            &numVertices,              // # of vertices in graph
            &numConstraints,           // Number of constraints
            &(graphIndexArray[0]),     // xadj
            &(graphNeighborArray[0]),  // adjncy
            &(vertexWeights[0]),       // vertex weights
            &(weightArray[0]),         // edge weights
            &weightOption,             // Weights on edges only
            &indexOption,              // 0-based index
            &numPartitions,
            &(tolerances[0]),
            options,                   // set options to nonrandom
            &edgesCut,                 // return the # of edges cut
            &(partitionArray[0]));     // return the partition of each vertex
    }
*/

#endif
