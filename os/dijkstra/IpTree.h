// IpTree.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

// IpTree is a sparse binary tree into which IP addresses are
// inserted. Each IP address inserted into the tree has an associated
// 'first hop' number. Each node knows which firstHop numbers it has
// encountered and how many of each there are.

#ifndef IP_TREE_H_DISTRIBUTED_DIJKSTRA_1
#define IP_TREE_H_DISTRIBUTED_DIJKSTRA_1

class IpTree
{
public:
    IpTree();
    ~IpTree();

    // Reset the state of the tree. Remove any state and put it back
    // to how it was just after construction.
    void reset(void);

    // Fill the tree with an IP address. The recursively added address
    // is limited by depth.
    void addRoute(IPAddress ip, int newFirstHop, int depth = 0);

    // Print a route based on the most popular first hop this object
    // has encountered. The object knows how many of each first hop
    // has visited it. The one which has visited it the most often is
    // the first hop which is selected. A route based on that first
    // hop is then printed. Note that if the parent's most popular
    // first hop is the same as the current first hop, no route need
    // be printed. The left/right branch selection of the path from
    // the root to this host represents the prefix. The depth
    // represents the netmask size.
    void printRoutes(int parentFirstHop, HostHostToIpMap const & ip,
                     int source, IPAddress subnet);

    // Which of the first hops in m_childHops have we encountered the
    // most?
    std::map<int,int>::iterator findMostPos(void);
private:
    IpTree(IpTree const &);
    IpTree & operator=(IpTree const &) { return *this; }
private:
    std::auto_ptr<IpTree> m_child[2];
    int m_firstHop;
    std::map<int, int> m_childHops;
    int m_depth;
};

#endif
