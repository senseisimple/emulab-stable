// SingleSource.h

#ifndef SINGLE_SOURCE_H_DISTRIBUTED_DIJKSTRA_1
#define SINGLE_SOURCE_H_DISTRIBUTED_DIJKSTRA_1

#include <boost/config.hpp>
#include <vector>
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

    void setMessage(string newSource, string newDest, string newCost)
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
    string message;
    int source;
    int dest;
    int cost;
};

class SingleSource
{
public:
    SingleSource(int size)
        : adj_graph(size)
        , pred_map(size)
        , dist_map(size)
        , first_hop(size)
        , vertexCount(size)
        , source(0)
    {
        using namespace boost;
        weightmap = get(edge_weight, adj_graph);
    }

    void insertEdge(int edgeSource, int edgeDest, int cost)
    {
        using namespace boost;
        edge_descriptor edge;
        bool inserted = false;
        tie(edge, inserted) = add_edge(edgeSource, edgeDest, adj_graph);
        if (!inserted)
        {
            throw EdgeInsertException(edgeSource, edgeDest, cost);
        }
        weightmap[edge] = cost;
    }

    void route(int newSource)
    {
        source = newSource;
        using namespace boost;
        // Compute the single-source shortest-path tree rooted at this vertex.
        dijkstra_shortest_paths(adj_graph, vertex(source, adj_graph),
                                predecessor_map(&pred_map[0]).
                                                   distance_map(&dist_map[0]));
        // set up the first_hop vector
        first_hop[source] = source;
        for (int i = 0; i < static_cast<int>(first_hop.size()); ++i)
        {
            if (i != source)
            {
                int current = i;
                while(pred_map[current] != source)
                {
                    current = pred_map[current];
                }
                first_hop[i] = current;
            }
        }
    }

    int getFirstHop(int index) const
    {
        return first_hop[index];
    }

    int getPred(int index) const
    {
        return pred_map[index];
    }

    int getDistance(int index) const
    {
        return dist_map[index];
    }

    int getVertexCount(void) const
    {
        return vertexCount;
    }

    int getSource(void) const
    {
        return source;
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
