// IpTree.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef IP_TREE_H_DISTRIBUTED_DIJKSTRA_1
#define IP_TREE_H_DISTRIBUTED_DIJKSTRA_1

class IpTree
{
public:
    IpTree();
    ~IpTree();

    void reset(void);
    void addRoute(IPAddress ip, int newFirstHop, int depth = 0);
    void printRoutes(int parentFirstHop, HostHostToIpMap const & ip,
                     int source, IPAddress subnet);
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
