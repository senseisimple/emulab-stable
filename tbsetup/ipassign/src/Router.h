// Router.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

// This is the base class for the Router module. Any new routing method should
// inherit from Router. This defines the routing method interface.

#ifndef ROUTER_H_IP_ASSIGN_2
#define ROUTER_H_IP_ASSIGN_2

#include "Assigner.h"

class Router
{
public:
    virtual ~Router();

    // Polymorphic copy
    virtual std::auto_ptr<Router> clone(void) const=0;

    // Set up internal hierarchy based on an Assigner
    virtual void reset(Assigner const & newAssign);
    // calculate the routing tables for every node
    virtual void calculateRoutes(void)=0;
    // print out the routing tables for every node
    virtual void print(ostream & output) const=0;
protected:
    // Common code. Is this node valid?
    bool isValidNode(size_t node) const;
protected:
    // Each NodeLookup in the nodeToLevel vector is a mapping from nodes
    // to the partitioning on that level. Nodes to Lans is nodeToLevel[0]
    // and each additional level of hierarchy has a Nodes to X mapping
    std::vector<Assigner::NodeLookup> m_nodeToLevel;

    // Each MaskTable in levelMaskSize is the number of bits in the bitmask
    // for each partition in that level of the hierarchy. levelMaskSize[0]
    // is the masks for each LAN.
    std::vector<Assigner::MaskTable> m_levelMaskSize;

    // Each PrefixTable in levelPrefix is the IP address prefix used with
    // the netmask to identify that network. levelPrefix[0] is the LAN level
    std::vector<Assigner::PrefixTable> m_levelPrefix;

    // Each LevelLookup in levelMakeup is a table representing which
    // indices of the level below it make up each node of the current level
    // levelMakeup[0] is the LAN level
    std::vector<Assigner::LevelLookup> m_levelMakeup;

    // This is a vector whose elements represent the weight of each LAN
    std::vector<int> m_lanWeights;
};

#endif



