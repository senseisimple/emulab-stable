// Assigner.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

// This is the base class for all ip assignment algorithms. Using
// this class, the ip assignment algorithm can be selected dynamically.

#ifndef ASSIGNER_H_IP_ASSIGN_2
#define ASSIGNER_H_IP_ASSIGN_2

#include "Partition.h"

class Assigner
{
public:
    typedef std::vector< std::vector<size_t> > NodeLookup;
    typedef std::vector<int> MaskTable;
    typedef std::vector<IPAddress> PrefixTable;
    typedef std::vector< std::vector< size_t > > LevelLookup;
public:
    virtual ~Assigner();
    virtual std::auto_ptr<Assigner> clone(void) const=0;

    virtual void setPartition(Partition & newPartition)=0;
    // This is called repeatedly to form the graph. Raw parsing
    // of the command line is done outside.
    // Any numbers between [0..MaxNodeNumber] that are unused
    // should be represented with the constant INVALID_NODE
    virtual void addLan(int, int, std::vector<size_t>)=0;
    // The main processing function. After the graph has been populated,
    // assign IP addresses to the nodes.
    virtual void ipAssign(void)=0;
    // Output the network and associated IP addresses to the specified stream.
    virtual void print(std::ostream &) const=0;
    // Populate the argument vectors with our state so that routes can
    // be calculated.
    // for the explanation of these arguments, see Router.h
    virtual void graph(std::vector<NodeLookup> & nodeToLevel,
                       std::vector<MaskTable> & levelMaskSize,
                       std::vector<PrefixTable> & levelPrefix,
                       std::vector<LevelLookup> & levelMakeup,
                       std::vector<int> & lanWeights) const=0;
private:
};

#endif


