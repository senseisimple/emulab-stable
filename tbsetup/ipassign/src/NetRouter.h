// NetRouter.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

// This strategy takes advantage of all layers of hierarchy given to us by
// IP assignment. I use a method that is akin to dynamic programming. I
// Approximate a solution at the lowest level of the hierarchy, then use that
// approximation to get an approximate solution at the next higher level and
// so on until I reach the top.

// TODO: Change this so that disconnected graphs are accepted.

#ifndef NET_ROUTER_H_IP_ASSIGN_2
#define NET_ROUTER_H_IP_ASSIGN_2

#include "Router.h"

class NetRouter : public Router
{
public:
    NetRouter();
    virtual ~NetRouter();
    virtual std::auto_ptr<Router> clone(void) const;

    virtual void calculateRoutes(void);
    virtual void print(std::ostream & output) const;
private:
    struct RouteInfo
    {
        // The source in RouteInfo is always a host.
        RouteInfo() : firstLan(0), firstNode(0),
            destination(0), distance(INT_MAX) {}
        // The first LAN on the path to the destination (If there is only one
        // hop, this is the destination).

        size_t firstLan;

        // The first node on the path to the destination. If the destination
        // is a LAN connected to this host, the firstNode is this host.
        size_t firstNode;

        // In most cases, this is a border LAN number. But see nodeToPartition
        // for the exception.
        size_t destination;

        // Weight of the route to destination. node-lan routes include both
        // the first LAN weight and the destination weight. This meshes with
        // how lan-lan distances are stored. See below for details.
        int distance;
    };
    // This is a map associating (source-host,destination-LAN) pairs with
    // routing info.
    // map<pair<node number,destination>,route>
    typedef std::map<pair<size_t,size_t>,RouteInfo> NodeBorderMap;
private:
    // Set up the data structures for the LAN level. This is the lowest
    // level and represents a sort of base-case which is then used to
    // calculate the routes on each higher level in turn.
    void setup(void);

    // Is this LAN a border LAN at a particular level? A LAN may be a border
    // LAN at some level, but be an internal LAN at higher level. At the very
    // bottom level, each LAN is its own partition and therefore every LAN is
    // a border LAN.
    bool isBorderLan(size_t level, size_t lan) const;

    // We have a partition on a certain level. That partition is made
    // up of sub-partitions on the level below. Each sub-partition has
    // borders to other sub-partitions within the partition. This
    // method calculates the paths between all-pairs of borders on a
    // particular sub-partition.
    void calculateBorderToBorderPaths(size_t level, size_t partition);
    void calculateNodeToBorderPaths(size_t level, size_t partition);
    void findNodeToBorder(NodeBorderMap::iterator searchPos,
                          NodeBorderMap::iterator searchLimit,
                          size_t sourceNode, size_t newPartition,
                          size_t destinationLan);
    void calculateBorders(size_t level, size_t partition,
                          std::list<size_t> & obsoleteLans);
    void calculatePartitionRoutes(size_t level, size_t partition);
    void findRouteToPartition(NodeBorderMap::iterator searchPos,
                              NodeBorderMap::iterator searchLimit,
                              size_t sourceNode, size_t sourcePartition,
                              size_t destPartition, size_t level);
    // given a list of LANs which we no longer care about, remove all
    // connections that involve them from the ConnectionMap.
    void removeObsoleteConnections(std::list<size_t> const & obsoleteLans);
    void calculateInterBorders(void);
private:
    // partition< borderList >
    std::vector< std::vector<size_t> > oldBorderLans;
    std::vector< std::vector<size_t> > newBorderLans;

    // route from each node to each border LAN in its partition.
    // Contains all that we
    // need to route to that border.
    // Each element in the vector is the border routing for a
    // partition
    std::vector<NodeBorderMap> oldNodeToBorder;
    std::vector<NodeBorderMap> newNodeToBorder;

    std::vector<size_t> lanToPartition;

    // in each level, for each node, there are partitions that we route
    // to directly.
    std::vector< std::vector< std::map<size_t, RouteInfo> > > nodeToPartition;

    // lanLanNode data structure takes two Lans and returns the node which
    // connects them. If there are more than one nodes connecting the LANs,
    // then only one is returned (the last one used to populate this).
    std::map<std::pair<size_t,size_t>,size_t> lanLanNode;

    // border adjascency
    // map< pair<source,dest>,pair<firstHop,distance> >
    typedef std::map<std::pair<size_t,size_t>, std::pair<size_t,int> > ConnectionMap;
    void addBorderConnection(size_t first, size_t second, size_t firstHop,
                             int weight);
    void removeBorderConnection(size_t first, size_t second);
    void removeBorderLan(size_t lan);
    size_t getBorderInfo(size_t first, size_t second, bool getWeight) const;
    int getBorderWeight(size_t first, size_t second) const;
    size_t getBorderFirstHop(size_t first, size_t second) const;
    ConnectionMap::iterator borderBegin(size_t index);
    ConnectionMap::const_iterator borderBegin(size_t index) const;
    ConnectionMap::iterator borderEnd(size_t index);
    ConnectionMap::const_iterator borderEnd(size_t index) const;

    ConnectionMap borderConnections;
    // This is kept synchronized with borderConnections. Because
    // we have two copies of the map with the order of 'first' and 'second'
    // reversed from one to another, we can find all uses of a particular
    // node (either to or from) and remove them.
    ConnectionMap reverseBorderConnections;
};

#endif
