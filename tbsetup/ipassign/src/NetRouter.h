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

// Each partition is a node on a tree. This tree represents the
// hierarchical relationships between partitions. Each partition has
// sub-partitions (children) which join together to form that
// partition. Each partition also has co-partitions (siblings) which,
// when joined to it form a super-partition (parent). Each partition
// is made up of LANs which are the quanta of partition.


#ifndef NET_ROUTER_H_IP_ASSIGN_2
#define NET_ROUTER_H_IP_ASSIGN_2

#include "Router.h"

class NetRouter : public Router
{
public:
// inherited from ptree::Visitor
    virtual void visitBranch(ptree::Branch & target);
    virtual void visitLeaf(ptree::Leaf & target);

// inherited from Router
    NetRouter();
    virtual ~NetRouter();
    virtual std::auto_ptr<Router> clone(void) const;

    virtual void calculateRoutes(void);
    virtual void print(std::ostream & output) const;
    virtual std::auto_ptr<ptree::Node> & getTree(void);
    virtual std::vector<ptree::LeafLan> & getLans(void);
    virtual void setHosts(Assigner::NodeLookup & source);
private:
    struct Host
    {
        Host() : internalHost(false) {}
        std::vector<size_t> lans;
        bool internalHost;
    };

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

    // Every host is adjascent to a set of LANs. Each LAN belongs to a
    // Leaf partition. This function calculates the least common
    // ancestor of all those LANs in the partition hierarchy. This is
    // used to determine at which point in the hierarchy a host stops
    // being on some border.
    void setupLeastCommonAncestors(void);

    // This is a helper function for setupLeastCommonAncestors(). It
    // runs a given visitor on the Leaf partition for a particular
    // LAN.
    int runParentVisitor(size_t lanIndex, ptree::ParentVisitor & visit);

    // Find out which LANs are on the borders of a particular
    // partition.  This is defined in terms of LANs and
    // partitions. See the comment for 'isBorderLan()' below for a
    // definition of what a border is. obsoleteLans is an output
    // variable. All of the LANs which were border LANs of the
    // sub-partitions but are no longer border LANS of the target
    // partition are put here. subPartitionList is also an output
    // variable. It yields a list of handles to the sub-partitions of
    // the target partition (direct children).
    void calculateBorders(ptree::Branch & target,
                          std::list<size_t> & obsoleteLans,
                          std::list<ptree::Node *> & subPartitionList);

    // Given a partition, flag as internal those hosts whose least
    // common ancestor is that partition.
    void setInternalHosts(ptree::Node & target);


    // Is this LAN a border LAN in a particular partition?  Borders
    // between partitions are quantified in terms of which LANs from
    // which partition are adjascent to which LANs of other
    // partitions. A LAN may be a border LAN in some partition, but be
    // an internal LAN in a super-partition. At the very bottom of the
    // partition hierarchy, every Leaf partition contains one and only
    // one LAN. Therefore, a LAN can be thought of as the quanta of
    // the partition space.
    bool isBorderLan(ptree::LeafLan & currentLan) const;

    // Calculate the shortest-path between all-pairs of vertices on a
    // graph. The vertices of this graph are the border LANs of the
    // sub-partitions of a given partition. Think of the partition as
    // a boundary on the size of the algorithm and the sub-partitions
    // inside the partition as what we are actually interested in
    // thinking about.
    void calculateBorderToBorderPaths(ptree::Node & target,
                                      list<size_t> const & obsoleteLans);

    void calculateNodeToBorderPaths(ptree::Node & target,
                                    list<ptree::Node *> & subPartitionList);
    void findNodeToBorder(NodeBorderMap::iterator searchPos,
                          NodeBorderMap::iterator searchLimit,
                          size_t sourceNode, ptree::Node & superPartition,
                          size_t destinationLan);
    void calculatePartitionRoutes(ptree::Node & target,
                                  list<ptree::Node *> & subPartitionList);

    void findRouteToPartition(NodeBorderMap::iterator searchPos,
                              NodeBorderMap::iterator searchLimit,
                              size_t sourceNode,
                              ptree::Node * sourcePartition,
                              ptree::Node * destPartition);

    // given a list of LANs which we no longer care about, remove all
    // connections that involve them from the ConnectionMap.
    void cleanupBorderLans(list<size_t> const & obsoleteLans,
                           list<ptree::Node *> const & subPartitionList);
    void calculateInterBorders(void);

private:
    // The structure to hold all the data we need is public so that the
    // Visitors can all be passed a handle to it if they need to look at it
    // or change it.
    std::auto_ptr<ptree::Node> tree;
    std::vector<ptree::LeafLan> lans;
    std::vector<Host> hosts;

    // partition< borderList >
/*    std::vector< std::vector<size_t> > oldBorderLans;
      std::vector< std::vector<size_t> > newBorderLans;*/
    // The pointer represents the partition to which these borderLans belong
    // The vector is the border LANs which belong to that partition
    std::map<ptree::Node *, std::vector<size_t> > borderLans;

    // route from each node to each border LAN in its partition.
    // Contains all that we
    // need to route to that border.
    // Each element in the vector is the border routing for a
    // partition
//        std::vector<NodeBorderMap> oldNodeToBorder;
//        std::vector<NodeBorderMap> newNodeToBorder;
    std::map<ptree::Node *, NodeBorderMap> nodeToBorder;

//        std::vector<size_t> lanToPartition;

    // in each level, for each node, there are partitions that we route
    // to directly.
//        std::vector< std::vector< std::map<size_t, RouteInfo> > > nodeToPartition;

    // Every node has a set of partitions which it routes to directly.
    std::vector< std::map<ptree::Node *, RouteInfo> > nodeToPartition;

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
