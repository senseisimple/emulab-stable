// VoteIpTree.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

// VoteIpTree is a sparse binary tree into which IP addresses are
// inserted. Each IP address inserted into the tree has an associated
// 'first hop' number. Each node knows which firstHop numbers it has
// encountered and how many of each there are. After every route has
// been added to that table, each internal node takes a 'vote' amongst
// the first-hops which have touched it. The one which touched it the
// most is the one selected. If the vote on a node is the same as its
// parent, no route is printed because that route is already covered
// by the parent. If the vote is different, a route is printed because
// this route represents a more specific 'hole' punched in the parents
// route.

#ifndef VOTE_IP_TREE_H_DISTRIBUTED_DIJKSTRA_1
#define VOTE_IP_TREE_H_DISTRIBUTED_DIJKSTRA_1

#include <climits>
#include "bitmath.h"
#include "IpTree.h"

class VoteIpTree : public IpTree
{
public:
    VoteIpTree();
    virtual ~VoteIpTree();

    virtual std::auto_ptr<IpTree> exemplar(void) const;
    virtual void reset(void);
    virtual void addRoute(IPAddress ip, int newFirstHop, int depth);

    // Print out the routes for this subtree. This function is called
    // from the outside world and invokes the other printRoutes
    // function to do the actual work.
    virtual void printRoutes(HostHostToIpMap const & ip, int source,
                             IPAddress subnet);
private:
    // Which of the first hops in m_childHops have we encountered the
    // most?
    std::map<int,int>::iterator findMostPos(void);

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
private:
    VoteIpTree(VoteIpTree const &);
    VoteIpTree & operator=(VoteIpTree const &) { return *this; }
private:
    std::auto_ptr<VoteIpTree> m_child[2];
    int m_firstHop;
    std::map<int, int> m_childHops;
    int m_depth;
};

#endif
