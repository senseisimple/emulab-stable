// LanRouter.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

// This routing strategy finds routes from every host to every LAN. Non-optimal
// routes are possible, but routes are stretched by at most one hop. If a few
// large LANs are used, this strategy is optimal. If many small LANs are used,
// this peforms worse time-wise than HostRouter.

#ifndef LAN_ROUTER_H_IP_ASSIGN_2
#define LAN_ROUTER_H_IP_ASSIGN_2

#include "Router.h"

class LanRouter : public Router
{
public:
    LanRouter();
    virtual ~LanRouter();
    virtual std::auto_ptr<Router> clone(void) const;

    virtual void calculateRoutes(void);
    virtual void print(std::ostream & output) const;
private:
    void calculateRoutesFrom(
                size_t lanNumber,
                std::vector< std::map<size_t,size_t> > const & connections,
                Assigner::LevelLookup const & lanToNode,
                Assigner::NodeLookup const & nodeToLan,
                std::vector< std::pair<size_t, int> > & shortestPaths);

    void connectLans(std::vector<size_t> const & currentNode, size_t i,
                     std::vector< std::map<size_t,size_t> > & connectionList);
    std::pair<size_t,size_t> chooseLan(size_t nodeNumber, size_t lanNumber,
                     std::vector<size_t> const & currentNode,
                     std::vector< std::map<size_t,size_t> > const & connection,
         std::vector< std::vector< std::pair<size_t,int> > > const & shortest);
    int distanceToLan(size_t sourceNode, size_t destLan) const;
private:
    // Outside vector is the node, inside vector is the route to that LAN
    // and the data is the first hop
    // vector< vector< pair<FirstNode,FirstLan> > >
    std::vector< std::vector< std::pair<size_t,size_t> > > routingTable;
};

#endif
