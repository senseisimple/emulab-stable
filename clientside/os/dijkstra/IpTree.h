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
    IpTree() {}
    virtual ~IpTree() {}

    // return a new default-constructed IpTree of the same type as the
    // current object is.
    virtual std::auto_ptr<IpTree> exemplar(void) const=0;

    // Reset the state of the tree. Remove any state and put it back
    // to how it was just after construction.
    virtual void reset(void)=0;


    // Fill the tree with an IP address. The recursively added address
    // is limited by depth.
    void addRoute(IPAddress ip, int newFirstHop)
    {
        addRoute(ip, newFirstHop, 0);
    }
    virtual void addRoute(IPAddress ip, int newFirstHop, int depth)=0;

    // Print out the routes for this subtree
    virtual void printRoutes(HostHostToIpMap const & ip, int source,
                             IPAddress subnet)=0;
private:
    IpTree(IpTree const &);
    IpTree & operator=(IpTree const &) { return *this; }
};

#endif
