// HostRouter.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

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
    virtual void printStatistics(std::ostream & output) const;
private:
    int getWeight(size_t first, size_t second) const;
    void calculateRoutesFrom(size_t node);
    template <typename RandomAccessIterator>
    size_t minIndex(RandomAccessIterator startPos, RandomAccessIterator endPos)
    {
        return min_element(startPos, endPos) - startPos;
    }

    template <typename T>
    size_t findIndex(vector<T>::const_iterator startPos,
                     vector<T>::const_iterator endPos, T const & item)
    {
        return find(startPos, endPos, item) - startPos;
    }
private:
    // For each node, which is the first node on the shortest path to it?
    typedef std::vector< size_t > RouteTable;
    // A routing table for every node. The first index is which node's routing
    // table. The second index is the destination. The result is the next
    // hop.
    std::vector<RouteTable> m_tableList;
    mutable int routeCount;
};

#endif


