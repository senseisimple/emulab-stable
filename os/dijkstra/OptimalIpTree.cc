// OptimalIpTree.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "lib.h"
#include "OptimalIpTree.h"
#include "SetIterator.h"
#include "dijkstra.h"

using namespace std;


void OptimalIpTree::IntersectionOrUnion(set<int> const & left,
                                        set<int> const & right,
                                        set<int> & result)
{
    assert(&left != &result && &right != &result);
    result.clear();
    bool useIntersection = false;
    SetIterator pos(left, right);
    for ( ; pos.valid() && !useIntersection; pos.increment())
    {
        if (pos.getSet() == SetIterator::BOTH_SETS)
        {
            useIntersection = true;
        }
    }
    if (useIntersection)
    {
        Intersection(left, right, result);
    }
    else
    {
        Union(left, right, result);
    }
}

void OptimalIpTree::Intersection(set<int> const & left,
                                 set<int> const & right,
                                 set<int> & result)
{
    assert(result.empty());
    assert(&left != &result && &right != &result);
    SetIterator pos(left, right);
    for ( ; pos.valid(); pos.increment())
    {
        if (pos.getSet() == SetIterator::BOTH_SETS)
        {
            result.insert(pos.getValue());
        }
    }
}

void OptimalIpTree::Union(set<int> const & left,
                          set<int> const & right,
                          set<int> & result)
{
    assert(result.empty());
    assert(&left != &result && &right != &result);
    SetIterator pos(left, right);
    for ( ; pos.valid(); pos.increment())
    {
        result.insert(pos.getValue());
    }
}

OptimalIpTree::OptimalIpTree()
    : m_depth(0)
{
}

OptimalIpTree::~OptimalIpTree()
{
}

OptimalIpTree::OptimalIpTree(OptimalIpTree const & right)
{
    m_depth = right.m_depth;
    m_firstHops = right.m_firstHops;
    if (right.m_children[0].get() == NULL)
    {
        m_children[0].reset(NULL);
    }
    else
    {
        m_children[0].reset(new OptimalIpTree(*(right.m_children[0])));
    }

    if (right.m_children[1].get() == NULL)
    {
        m_children[1].reset(NULL);
    }
    else
    {
        m_children[1].reset(new OptimalIpTree(*(right.m_children[1])));
    }
}

template<class T>
void swap_modify(T & left, T & right)
{
    T temp;
    temp = left;
    left = right;
    right = temp;
}

OptimalIpTree & OptimalIpTree::operator=(OptimalIpTree const & right)
{
    OptimalIpTree swapper(right);
    m_firstHops.swap(swapper.m_firstHops);
    swap(m_depth, swapper.m_depth);
    swap_modify(m_children[0], swapper.m_children[0]);
    swap_modify(m_children[1], swapper.m_children[1]);
    return *this;
}

auto_ptr<IpTree> OptimalIpTree::exemplar(void) const
{
    return auto_ptr<IpTree>(new OptimalIpTree());
}

void OptimalIpTree::reset(void)
{
    m_depth = 0;
    m_firstHops.clear();
    m_children[0].reset(NULL);
    m_children[1].reset(NULL);
}

void OptimalIpTree::addRoute(IPAddress ip, int newFirstHop, int depth)
{
    m_depth = depth;
    if (depth < IP_SIZE)
    {
        IPAddress bit = (ip >> (IP_SIZE - depth - 1)) & 0x01;
        if (m_children[bit].get() == NULL)
        {
            m_children[bit].reset(new OptimalIpTree());
        }
        m_children[bit]->addRoute(ip, newFirstHop, depth + 1);
    }
    else
    {
        m_firstHops.clear();
        m_firstHops.insert(newFirstHop);
    }
}

void OptimalIpTree::joinSets(void)
{
    if (m_children[0].get() != NULL && m_children[1].get() != NULL)
    {
        m_children[0]->joinSets();
        m_children[1]->joinSets();
        IntersectionOrUnion(m_children[0]->m_firstHops,
                            m_children[1]->m_firstHops,
                            m_firstHops);
    }
    else if (m_children[0].get() != NULL)
    {
        m_children[0]->joinSets();
        m_firstHops = m_children[0]->m_firstHops;
    }
    else if (m_children[1].get() != NULL)
    {
        m_children[1]->joinSets();
        m_firstHops = m_children[1]->m_firstHops;
    }
    // If both are NULL, we are a leaf, and do nothing.
}

void OptimalIpTree::printRoutes(HostHostToIpMap const & ip, int source,
                                IPAddress subnet)
{
    if (m_children[0].get() != NULL || m_children[1].get() != NULL)
    {
        joinSets();
        assert(!(m_firstHops.empty()));
        HostEntry::const_iterator pos;
        pos  = ip[source].find(*(m_firstHops.begin()));
        if (pos != ip[source].end())
        {
            printRouteToSubnet(pos->second.first, pos->second.second,
                               ipToString(subnet << (IP_SIZE - m_depth)),
                               m_depth, 1);
        }
        printRoutes(*(m_firstHops.begin()), ip, source, subnet);
    }
}

void OptimalIpTree::printRoutes(int firstHop, HostHostToIpMap const & ip,
                                int source, IPAddress subnet)
{
    assert(!(m_firstHops.empty()));
    if (m_firstHops.count(firstHop) == 0)
    {
        firstHop = *(m_firstHops.begin());
        HostEntry::const_iterator pos = ip[source].find(firstHop);
        if (pos != ip[source].end())
        {
            printRouteToSubnet(pos->second.first, pos->second.second,
                               ipToString(subnet << (IP_SIZE - m_depth)),
                               m_depth, 1);
            // TODO: replace hardcoded '1' with cost metric
        }
    }
    if (m_children[0].get() != NULL)
    {
        m_children[0]->printRoutes(firstHop, ip, source, subnet << 1);
    }
    if (m_children[1].get() != NULL)
    {
        m_children[1]->printRoutes(firstHop, ip, source, (subnet << 1) + 1);
    }
}

int OptimalIpTree::size(void)
{
    int result = 1;
    if (m_children[0].get() != NULL)
    {
        result += m_children[0]->size();
    }
    if (m_children[1].get() != NULL)
    {
        result += m_children[1]->size();
    }
    return result;
}
