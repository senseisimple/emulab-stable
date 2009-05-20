/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "forwarding.h"
#include "physical.h"

// XXX: This is supposed to be in the header
static forwarding_info::protocol_set_t global_protocol_set;

void forwarding_info::add_protocol(fstring protocol) {
    protocol_set.insert(protocol);
    global_protocol_set.insert(protocol);
}

bool forwarding_info::forwards_for_protocol(const fstring protocol) const {
    if (protocol_set.find(protocol) != protocol_set.end()) {
	return true;
    } else {
	return false;
    }
}

forwarding_info::protocol_list *forwarding_info::global_protocol_list() {
    protocol_list *plist = new protocol_list;
    for (protocol_set_t::const_iterator it = global_protocol_set.begin();
	 it != global_protocol_set.end(); it++) {
	plist->push_back(*it);
    }
    
    return plist;
}

/* Given the physical graph, and the forwarding information for each node,
 * calculate the shortest routes for every protcol in the system.
 * TODO:
 *   Allow user to specify routes, instead of calculating shortest ones
 *   Does this work for disconnected graphs?
 */
void calculate_shortest_routes(tb_pgraph pg) {
#if 0
    // Have to iterate through all protocols that anyone in the system can forward
    forwarding_info::protocol_list *plist = forwarding_info::global_protocol_list();
    for (forwarding_info::protocol_list::const_iterator pit = plist->begin();
	 pit != plist->end(); pit++) {
	fstring protocol = *pit;
	// Make a graph for this protocol - we take the physical graph, and
	// include only edges of the proper type, and only nodes that can
	// forward that protocol.
	// TODO: Can we/should we include edges of the proper type between
	// nodes that won't forward that type?
	tb_pgraph protograph;
	pertex_iterator pit, pendit;
	tie(pit, pendit) = vertices(pg);
	for (;pit != pendit; pit++) {
	    tb_pnode *pnode = get(pvertex_map,*pit);
	    if (pit->forwarding.forwards_for_protocol(protocol)) {
		// XXX: How to we map between verticies in the original and in
		// the copy?
	    }
	}
    }
    
    delete plist;
#endif
}

protocol_routing_table::protocol_routing_table(tb_pgraph pg, fstring _protocol) {
    this->protocol = _protocol;
    
    tb_pgraph_vertex_pmap pvertex_pmap = get(vertex_data, pg);
    tb_pgraph_edge_pmap pedge_pmap = get(edge_data, pg);
    
    /*
     * Add in all verticies in the physical map that can forward this
     * protocol.
     */
    pvertex_iterator pit, pendit;
    tie(pit, pendit) = vertices(pg);
    for (;pit != pendit; pit++) {
	tb_pnode *pnode = get(pvertex_pmap,*pit);
	if (pnode->forwarding.forwards_for_protocol(protocol)) {
	    protovertex v = add_vertex(graph);
	    graph[v].pnode = pnode;
	}
    }
    
    /*
     * Add in all links that can run this protocol with two endpoints that can
     * forward it.
     */
    pedge_iterator eit, eendit;
    tie (eit, eendit) = edges(pg);
    for (;eit != eendit; eit++) {
	tb_plink *plink = get(pedge_pmap,*eit);
	if (plink->has_type(protocol)) {
	    pvertex src = source(*eit,pg);
	    pvertex dst = target(*eit,pg);
	    tb_pnode *srcnode = get(pvertex_pmap,src);
	    tb_pnode *dstnode = get(pvertex_pmap,dst);
	    if (srcnode->forwarding.forwards_for_protocol(protocol) &&
		dstnode->forwarding.forwards_for_protocol(protocol)) {
		// Add this edge to the protcol-specific graph - we have to
		// find the vertices in the protograph that correspond to the
		// pnodes in the physical graph
		// TODO: We could obviously make this faster!
		// XXX: Should assert that we find them, somehow
		protovertex srcvertex, dstvertex;
		graph_traits<protograph_t>::vertex_iterator vit, vendit;
		tie(vit, vendit) = vertices(graph);
		for (;vit != vendit; vit++) {
		    if (graph[*vit].pnode == srcnode) {
			srcvertex = *vit;
		    }
		    if (graph[*vit].pnode == dstnode) {
			dstvertex = *vit;
		    }
		    
		}
		add_edge(srcvertex,dstvertex,pg);
	    }
	}
    }
    
    // Okay, now that we've built the graph, run Dijkstra's on every vertex!
    graph_traits<protograph_t>::vertex_iterator vit, vendit;
    tie(vit, vendit) = vertices(graph);
    for (;vit != vendit; vit++) {
	// XXX: Broken!
	//pred_map[*vit] = new predecessor_graph_t(num_vertices(graph));
	//dist_map[*vit] = new distance_graph_t(num_vertices(graph));
	//dijkstra_shortest_paths(pg, *vit,
	//			predecessor_map(&((*pred_map[*vit])[0])).
	//			distance_map(&((*dist_map[*vit])[0])));

    }
}