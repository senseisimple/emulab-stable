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
    static void partitionN(int partitionCount,
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

        if (partitionCount > 1 && partitionCount <= 8)
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
        else if (partitionCount > 1)
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
    }
private:
};

#endif
