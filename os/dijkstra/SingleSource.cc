// SingleSource.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "lib.h"
#include "bitmath.h"
#include "SingleSource.h"

using namespace std;
using namespace boost;

SingleSource::SingleSource(int size)
    : adj_graph(size)
    , pred_map(size)
    , dist_map(size)
    , first_hop(size)
    , vertexCount(size)
    , source(0)
{
    weightmap = get(edge_weight, adj_graph);
}

SingleSource::~SingleSource()
{
}

void SingleSource::insertEdge(int edgeSource, int edgeDest, int cost)
{
    edge_descriptor edge;
    bool inserted = false;
    tie(edge, inserted) = add_edge(edgeSource, edgeDest, adj_graph);
    if (!inserted)
    {
        throw EdgeInsertException(edgeSource, edgeDest, cost);
    }
    weightmap[edge] = cost;
}

void SingleSource::route(int newSource)
{
    source = newSource;
    // Compute the single-source shortest-path tree rooted at this vertex.
    dijkstra_shortest_paths(adj_graph, vertex(source, adj_graph),
                            predecessor_map(&pred_map[0]).
                            distance_map(&dist_map[0]));
    // set up the first_hop vector
    first_hop[source] = source;
    for (int i = 0; i < static_cast<int>(first_hop.size()); ++i)
    {
        if (i != static_cast<int>(pred_map[i]))
        {
            int current = i;
            while(static_cast<int>(pred_map[current]) != source)
            {
                current = pred_map[current];
            }
            first_hop[i] = current;
        }
        else
        {
            first_hop[i] = i;
        }
    }
}

int SingleSource::getFirstHop(int index) const
{
    return first_hop[index];
}

int SingleSource::getPred(int index) const
{
    return pred_map[index];
}

int SingleSource::getDistance(int index) const
{
    return dist_map[index];
}

int SingleSource::getVertexCount(void) const
{
    return vertexCount;
}

int SingleSource::getSource(void) const
{
    return source;
}
