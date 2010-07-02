// SingleSource.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

// This data structure represents a graph and single-source
// shortest-path information for a particular source.

#ifndef SINGLE_SOURCE_H_DISTRIBUTED_DIJKSTRA_1
#define SINGLE_SOURCE_H_DISTRIBUTED_DIJKSTRA_1

#include <climits>
#include <boost/config.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

class EdgeInsertException : public std::exception
{
public:
    EdgeInsertException(int newSource, int newDest, int newCost)
        : source(newSource)
        , dest(newDest)
        , cost(newCost)
    {
        setMessage(intToString(source), intToString(dest), intToString(cost));
    }

    virtual ~EdgeInsertException() throw() {}

    void setMessage(std::string newSource, std::string newDest,
                    std::string newCost)
    {
        message = "Could not insert edge: source=" + newSource + ", dest="
            + newDest + ", cost=" + newCost;
    }

    virtual char const * what(void) const throw()
    {
        return message.c_str();
    }

    int getSource(void)
    {
        return source;
    }

    int getDest(void)
    {
        return dest;
    }

    int getCost(void)
    {
        return cost;
    }
private:
    std::string message;
    int source;
    int dest;
    int cost;
};

class SingleSource
{
public:
    SingleSource(int size);
    ~SingleSource();

    // Add an edge to the graph. Every edge should be inserted before
    // running route().
    void insertEdge(int edgeSource, int edgeDest, int cost);
    // Calculate shortest path info from a particular source.
    void route(int newSource);

    // Find out what the first hop is from the source to a particular
    // host.
    int getFirstHop(int index) const;
    // Find out what the predecessor is in the tree extending from
    // source.
    int getPred(int index) const;
    // Find out the distance between source and a particular host.
    int getDistance(int index) const;
    // How many vertices are in the graph?
    int getVertexCount(void) const;
    // What is the index of the source host?
    int getSource(void) const;
    // Get the vector of first hops
    std::vector<int> const & getAllFirstHops(void) const
    {
        return first_hop;
    }
private:
    // Use an adjacency_list graph instead of an adjacency_matrix
    // graph, based on the assumption that there will be many fewer
    // than V**2/2 edges.  Represent the adjacencies with vectors
    // instead of lists for traversal speed since the graph is not
    // going to be changing a lot.
    typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
        boost::no_property, boost::property < boost::edge_weight_t, int > >
        Graph;
    typedef boost::graph_traits < Graph >::vertex_descriptor vertex_descriptor;
    typedef boost::graph_traits < Graph >::edge_descriptor edge_descriptor;
    typedef std::pair<int, int> Edge;

private:
  Graph adj_graph;
  boost::property_map<Graph, boost::edge_weight_t>::type weightmap;
  std::vector<vertex_descriptor> pred_map;
  std::vector<int> dist_map;
  std::vector<int> first_hop;
  int vertexCount;
  int source;
};

#endif
