// HostRouter.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

// This module sets up individual routes from every host to every interface.
// Because of this, the routes are guaranteed to the the shortest path.
// Unfortunately, this also means that the routing tables are going to be
// huge and the speed will be glacial.

#ifndef HOST_ROUTER_H_IP_ASSIGN_2
#define HOST_ROUTER_H_IP_ASSIGN_2

#include "Router.h"

class HostRouter : public Router
{
public:
    HostRouter();
    virtual ~HostRouter();
    virtual std::auto_ptr<Router> clone(void) const;

    virtual void calculateRoutes(void);
    virtual void print(std::ostream & output) const;
private:
    // pre-processing for print. Each host has adjascent interfaces.
    // The first hop information in m_tableList provides hops in terms of
    // nodes. But the first hop info we are to output is in terms of the
    // IP addresses of adjascent interfaces.
    // This function calculates those IP addresses.
    void findAdjascentInterfaces(size_t node,
                                 map<size_t, IPAddress> & nodeToIP) const;

    // Print out the actual table for a particular node.
    void printTable(ostream & output, size_t node,
                    map<size_t, IPAddress> & nodeToIP) const;
private:
    // For each node, which is the first node on the shortest path to it?
    typedef std::vector< size_t > RouteTable;
    // A routing table for every node. The first index is which node's routing
    // table. The second index is the destination. The result is the next
    // hop.
    std::vector<RouteTable> m_tableList;
};

#endif


