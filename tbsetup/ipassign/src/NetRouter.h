// NetRouter.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

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
    virtual void printStatistics(std::ostream & output) const;
private:
    struct RouteInfo
    {
        RouteInfo() : firstLan(0), firstNode(0), destination(0), distance(INT_MAX) {}
        size_t firstLan;
        size_t firstNode;
        size_t destination;
        int distance;
    };
    // map<pair<node number,destination>,route>
    typedef std::map<pair<size_t,size_t>,RouteInfo> NodeBorderMap;
private:
    void setup(void);
    bool isBorderLan(size_t level, size_t lan) const;
    void calculateBorderToBorderPaths(size_t level, size_t partition);
    void calculateNodeToBorderPaths(size_t level, size_t partition);
    void findNodeToBorder(NodeBorderMap::iterator searchPos,
                          NodeBorderMap::iterator searchLimit,
                          size_t sourceNode, size_t newPartition,
                          size_t destinationLan);
    void calculateBorders(size_t level, size_t partition);
    void calculatePartitionRoutes(size_t level, size_t partition);
    void findRouteToPartition(NodeBorderMap::iterator searchPos,
                              NodeBorderMap::iterator searchLimit,
                              size_t sourceNode, size_t sourcePartition,
                              size_t destPartition, size_t level);
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
