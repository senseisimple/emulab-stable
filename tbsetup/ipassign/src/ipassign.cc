// ipassign.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

// Please note that if
// log_2(maxNodesPerLan) + log_2(numLans) + log_2(sqrt(numLans)) > 24
// then the bit division code will flag an error. To get around this, you'd
// have to write a less naive algorithm to divide the bits of an IP address
// into subnets.

// This program processes from stdin to stdout.
// Input and output specifications can be found in README

#include "lib.h"
#include "Exception.h"
#include "Graph.h"

using namespace std;

// constants
// totalBits should go here, but because of compiler restrictions,
// I have to use the enum hack. See Graph.h
const int prefixBits = 8;
const int postfixBits = totalBits - prefixBits;
const bitset<totalBits> prefix(bitset<totalBits>(10) << postfixBits);
const bitset<totalBits> prefixMask(bitset<totalBits>().set() << postfixBits);
const bitset<totalBits> postfixMask(bitset<totalBits>().set() >> prefixBits);

// does everything :)
// Can throw bad_alloc, BitOverflowError
void execute(void);

// return the number of bits required to store anything in [0,num]
// Cannot throw
int getBits(int num);

// return the number of bits required to store the largest lan
int getLanBits(vector<int> const & partitionArray, int numPartitions);

// return the smallest integer greater than a double
// Cannot throw
int roundUp(double num);

// if there are more bits allocated for the hierarchy than there are bits
// available, then we can't assign IP addresses.
// Can throw BitOverflowError
void checkBitOverflow(int numBits);

int main()
{
    int errorCode = 0;

    try
    {
        execute();
    }
    catch (std::exception const & error)
    {
        std::cerr << "ipassign: " << error.what() << endl;
        errorCode = 1;
    }

    return errorCode;
}

void execute(void)
{
    //////////////////////////////////////////////////////
    // read in the graph
    //////////////////////////////////////////////////////

    Graph web(cin);

    //////////////////////////////////////////////////////
    // find out which bits go to which subnet
    //////////////////////////////////////////////////////

    // Add two to the largest lan because we need can't use an address
    // which has a hostname of all 0s or all 1s

    int bitsForNodes = getBits(web.getLargestLan() + 2);
    int bitsForNetwork = getBits(roundUp(sqrt(web.getLanCount())));
    if (bitsForNetwork < 0)
    {
        bitsForNetwork = 0;
    }

    std::vector<int> partitionArray;
    std::vector<int> graphIndexArray;
    std::vector<int> graphNeighborArray;
    std::vector<int> weightArray;

    //////////////////////////////////////////////////////
    // convert to METIS graph
    //////////////////////////////////////////////////////

    web.convert(graphIndexArray, graphNeighborArray, weightArray);
    partitionArray.resize(graphIndexArray.size() - 1);

    //////////////////////////////////////////////////////
    // partition the graph
    //////////////////////////////////////////////////////

    int numVertices = graphIndexArray.size() - 1;
    int weightOption = 1; // Weights on edges only
    int indexOption = 0; // 0-based index
    int numPartitions = static_cast<int>(pow(2, bitsForNetwork));
    int options[5];
    int edgesCut = 0;

    if (numPartitions > 1 && numPartitions <= 8)
    {
        options[0] = 0; // use defaults. Ignore other options.
        options[1] = 3; // (default) Sorted Heavy-Edge matching
        options[2] = 1; // (default) Region Growing
        options[3] = 1; // (default) Early-Exit Boundary FM refinement
        options[4] = 0; // (default) Always set to 0

        METIS_PartGraphRecursive(
            &numVertices,              // # of vertices in graph
            &(graphIndexArray[0]),     // xadj
            &(graphNeighborArray[0]),  // adjncy
            NULL,                      // vertex weights
            &(weightArray[0]),         // edge weights
            &weightOption,             // Weights on edges only
            &indexOption,              // 0-based index
            &numPartitions,
            options,                   // set options to default
            &edgesCut,                 // return the # of edges cut
            &(partitionArray[0]));     // return the partition of each vertex
    }
    else if (numPartitions > 1)
    {
        options[0] = 1; // ignore defaults. Use other options.
        options[1] = 3; // (default) Sorted Heavy-Edge Matching
        options[2] = 1; // (default) Multilevel recursive bisection
        options[3] = 2; // Greedy boundary refinement
        options[4] = 0; // (default) Always set to 0
        METIS_PartGraphKway(
            &numVertices,              // # of vertices in graph
            &(graphIndexArray[0]),     // xadj
            &(graphNeighborArray[0]),  // adjncy
            NULL,                      // vertex weights
            &(weightArray[0]),         // edge weights
            &weightOption,             // Weights on edges only
            &indexOption,              // 0-based index
            &numPartitions,
            options,                   // set options to nonrandom
            &edgesCut,                 // return the # of edges cut
            &(partitionArray[0]));     // return the partition of each vertex
    }

    web.partition(partitionArray);

    //////////////////////////////////////////////////////
    // Find out how many bits we need for the LANs
    //////////////////////////////////////////////////////

    int bitsForLans = getLanBits(partitionArray, numPartitions);
    checkBitOverflow(bitsForNodes + bitsForLans + bitsForNetwork);

    //////////////////////////////////////////////////////
    // assign IP addresses
    //////////////////////////////////////////////////////

    web.assign(cout, bitsForNetwork, bitsForLans, bitsForNodes, numPartitions);
}

int getBits(int num)
{
    return roundUp(log(static_cast<double>(num))/log(2.0));
}

int getLanBits(vector<int> const & partitionArray, int numPartitions)
{
    vector<int> counter;
    int current;
    counter.resize(numPartitions);
    fill(counter.begin(), counter.end(), 0);
    for (size_t i = 0; i < partitionArray.size(); ++i)
    {
        current = partitionArray[i];
        if (current >= 0 && current < numPartitions)
        {
            ++(counter[current]);
        }
    }
    return getBits(*(max_element(counter.begin(), counter.end())));
}

int roundUp(double num)
{
    return static_cast<int>(ceil(num) + 0.5);
}

void checkBitOverflow(int numBits)
{
    if (numBits <= 0 || numBits > totalBits)
    {
        throw BitOverflowError("Too many hosts/lans.");
    }
}






