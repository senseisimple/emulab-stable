/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2007 University of Utah and the Flux Group.
 * All rights reserved.
 *  
 * forwarding.h - data structures for recording information about what
 * protocol(s) a node can forward
 */
#ifndef __FORWARDING_H
#define __FORWARDING_H

#include "port.h"
#include "fstring.h"

#include <boost/config.hpp>
#include <boost/utility.hpp>
#include <boost/property_map.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/named_function_params.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
using namespace boost;

#ifdef NEW_GCC
#include <ext/hash_set>
using namespace __gnu_cxx;
#else
#include <hash_set>
#endif

#include <list>
#include <vector>
using namespace std;

class forwarding_info {
public:
    forwarding_info(): protocol_set() { ; }

    /*
     * Indicate that the node can forward the given protocol
     */
    void add_protocol(fstring protocol);
    
    /*
     * Return true if this object forwards the given protocol, and false if it
     * does not
     */
    inline bool forwards_for_protocol(const fstring protocol) const;
    
    /*
     * Returns a list of all protocols in the system than someone can forward.
     * Caller must free the list returned
     */
    typedef list<fstring> protocol_list;
    static protocol_list *global_protocol_list();

    typedef hash_set<fstring> protocol_set_t;

private:
	     
    /*
     * Simply holds the names of protocols forwarded by this node. In the
     * future, we might use a more complicated structure to record things like
     * whether it can do true unicast, broadcast, etc.
     */
    protocol_set_t protocol_set;
    
    /*
     * We use this to keep track of the global list of prototols that anyone in
     * the whole system can forward - this is used to compute spaning trees for
     * each protocol.     
     * XXX: This is a global in the .cpp file for now, 'cause I'm getting some
     * linker errors, and my C++-fu is not strong enough
     */
    //static protocol_set_t global_protocol_set;
};

// This is down here because we need forwarding_info for physical.h, but some
// of the stuff in physical.h before the routing tables
#include "physical.h"

class protocol_routing_table {
public:
    /*
     * This constructor takes the full physical graph, and computes shortest
     * path routes for the given protocol.
     */
    protocol_routing_table(tb_pgraph pg, fstring _protocol);
    
    /*
     * Return the protocol this routing table is for
     */
    fstring get_protocol() const;
private:
    /*
     * Graph holding the set of nodes that will forward this protocol, and the
     * links between them.
     * Question: Should we be storing pnodes or vertices as the bundled
     * properties?
     */
    struct vertex_properties { tb_pnode *pnode; };
    struct edge_properties { tb_plink *plink; };
    // OutEdgeList, VertexList, Directed, VertexProperties, EdgeProperties,
    // GraphProperties, EdgeList
    typedef adjacency_list<listS,listS,undirectedS,
	vertex_properties, edge_properties> protograph_t;
    typedef graph_traits<protograph_t>::vertex_descriptor protovertex;
    typedef graph_traits<protograph_t>::edge_descriptor protoedge;
    protograph_t graph;
    
    /*
     * Predecessor and distance graphs for the routes
     */
    typedef vector<protovertex> predecessor_graph_t;
    typedef vector<protovertex> distance_graph_t;
    // TODO: Need to decide if these are per-vertex, or per-pnode
    typedef hash_map<protovertex, predecessor_graph_t*> pred_map_t;
    typedef hash_map<protovertex, distance_graph_t*> dist_map_t;
    pred_map_t pred_map;
    dist_map_t dist_map;
    
    fstring protocol;
};

#endif /* Header guard */
