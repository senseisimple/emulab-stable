// Assigner.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

// This is the base class for all ip assignment algorithms. Using
// this class, the ip assignment algorithm can be selected dynamically.

// To create a new ip assignment module, create a class which inherits from
// Assigner and implements all of the below methods as described.
// Don't forget to change the command-line interface to actually use your
// new ip assignment strategy.

#ifndef ASSIGNER_H_IP_ASSIGN_2
#define ASSIGNER_H_IP_ASSIGN_2

#include "PTree.h"
#include "Partition.h"

class Assigner
{
public:
    // Note that I refer to 'partitions' in general. Each level is a
    // partitioning up of the level below. On the lowest level, each LAN is
    // a partition of its own.

    // A NodeLookup table allows one to find the partitions which a particular
    // node belongs too. The outer vector is a selector based on the node
    // number. The inner vector is a list of partition numbers that node
    // belongs too. In order to have node referencing to multiple levels of
    // hierarchy, there should be a vector of these.
    typedef std::vector< std::vector<size_t> > NodeLookup;

    // A MaskTable associates each partition number with the size of the
    // subnet mask for that partition. For instance, a LAN with 8 bits of
    // bitspace allocated to it would have '24' as its entry in the MaskTable.
    // To have Masks on different levels, it is necessary to have a vector of
    // these tables.
    typedef std::vector<int> MaskTable;

    // Each partition has a prefix associated with it. This prefix represents
    // the the first n bits of any IP address for all interfaces that are
    // contained within the partition. 'n', in this case, is the size of the
    // subnet mask for that partition. To have more than one level of prefixes,
    // a vector of PrefixTables is required.
    typedef std::vector<IPAddress> PrefixTable;

    // A partition on each level is made up of sub-partitions on the level
    // below. (On the bottom level, each LAN is made up of nodes). This table
    // relates each partition number to a list of partition numbers for the
    // level below. To represent many levels of partitioning, a vector of
    // LevelLookups is necessary.
    typedef std::vector< std::vector< size_t > > LevelLookup;

    // As you can see, the final data structure representing one part of the
    // hierarchy can seem pretty complex. One can have vectors of vectors of
    // vectors. Because of this, whenever possible, you should use references
    // as named temporaries to try to make this conceptually simple.

public:
    Assigner() {}
    virtual ~Assigner() {}

    // Create a copy of the current object polymorphically.
    virtual std::auto_ptr<Assigner> clone(void) const=0;

    // This may be called more than once to set the partitioning method
    // selected by the command line arguments. Note that only a handle is
    // being passed in. Ownership remains outside of this object.
    virtual void setPartition(Partition & newPartition)=0;

    // This is called repeatedly to form the graph. Raw parsing
    // of the command line is done outside.
    virtual void addLan(int bits, int weight, std::vector<size_t> nodes)=0;

    // The main processing function. After the graph has been populated,
    // assign IP addresses to the nodes.
    virtual void ipAssign(void)=0;

    // Output the network and associated IP addresses to the specified stream.
    virtual void print(std::ostream & output) const=0;

    virtual void getGraph(std::auto_ptr<ptree::Node> & tree,
                          std::vector<ptree::LeafLan> & lans)=0;

    virtual NodeLookup & getHosts(void)=0;

public:
    struct Lan
    {
        Lan()
            : weight(0), partition(0), number(0), ip(0)
        {
        }
        // The partition number associated with this LAN. This is the index
        // of the LAN in m_lanList. Not to be confused with 'partition'.
        size_t number;
        // The weight of this LAN.
        int weight;
        // The partition number of the super-partition this LAN is a part of.
        int partition;
        // The IP Address prefix that this LAN provides. Note that with
        // conservative IP assignment, every LAN has the same netmask.
        IPAddress ip;
        // The list of nodes which have interfaces in this LAN.
        std::vector<size_t> nodes;
    };
private:
};

#endif
