/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef __VIRTUAL_H
#define __VIRTUAL_H

#include <hash_set>

class tb_plink;
class tb_vnode;
class tb_vlink;
class tb_vclass;

typedef property<vertex_data_t,tb_vnode*> VNodeProperty;
typedef property<edge_data_t,tb_vlink*> VEdgeProperty;

typedef adjacency_list<listS,listS,undirectedS,
  VNodeProperty,VEdgeProperty> tb_vgraph;

typedef property_map<tb_vgraph,vertex_data_t>::type tb_vgraph_vertex_pmap;
typedef property_map<tb_vgraph,edge_data_t>::type tb_vgraph_edge_pmap;

typedef graph_traits<tb_vgraph>::vertex_descriptor vvertex;
typedef graph_traits<tb_vgraph>::edge_descriptor vedge;

typedef graph_traits<tb_vgraph>::vertex_iterator vvertex_iterator;
typedef graph_traits<tb_vgraph>::edge_iterator vedge_iterator;
typedef graph_traits<tb_vgraph>::out_edge_iterator voedge_iterator;

class tb_link_info {
public:
  typedef enum {LINK_UNKNOWN, LINK_DIRECT,
		LINK_INTRASWITCH, LINK_INTERSWITCH,
		LINK_TRIVIAL, LINK_DELAYED} linkType;
  linkType type;
  pedge_path plinks;		// the path of pedges
  pvertex_list switches;	// what switches were used

  friend ostream &operator<<(ostream &o, const tb_link_info& link)
  {
    o << "  Type: ";
    switch (link.type) {
    case LINK_UNKNOWN : o << "LINK_UNKNOWN"; break;
    case LINK_DIRECT : o << "LINK_DIRECT"; break;
    case LINK_INTRASWITCH : o << "LINK_INTRASWITCH"; break;
    case LINK_INTERSWITCH : o << "LINK_INTERSWITCH"; break;
    case LINK_TRIVIAL : o << "LINK_TRIVIAL"; break;
    }
    o << " Path: ";
    for (pedge_path::const_iterator it=link.plinks.begin();
	 it!=link.plinks.end();it++) {
      o << *it << " ";
    }
    o << " Switches: ";
    for (pvertex_list::const_iterator it=link.switches.begin();
	 it!=link.switches.end();it++) {
      o << *it << " ";
    }
    o << endl;
    return o;
  }
};

class tb_vnode {
public:
  tb_vnode() {;}

  friend ostream &operator<<(ostream &o, const tb_vnode& node)
  {
    o << "tb_vnode: " << node.name << " (" << &node << ")" << endl;
    o << "  Type: " << node.type << endl;
    o << "  Desires:" << endl;
    for (desires_map::const_iterator it = node.desires.begin();
	 it!=node.desires.end();it++) 
      o << "    " << (*it).first << " -> " << (*it).second << endl;
    o << " vclass=" << node.vclass << " fixed=" <<
      node.fixed << endl;
    return o;
  }

  typedef hash_map<crope,double> desires_map;
  typedef hash_map<crope,int> desires_count_map;

  // contains weight of each desire
  desires_map desires;

  crope type;			// the current type of the node
  tb_vclass *vclass;		// the virtual class of the node, if any
  crope name;			// string name of the node
  bool fixed;			// is this node fixed
  bool assigned;		// is this node assigned?
  pvertex assignment;		// the physical vertex assigned to

#ifdef PER_VNODE_TT
  int num_links;
  int total_bandwidth;
#endif

};

class tb_vlink {
public:
  tb_vlink() {;}

  friend ostream &operator<<(ostream &o, const tb_vlink& link)
  {
    o << "tb_vnode: " << link.name << " (" << &link << ")" << endl;
    o << " emulated=" << link.emulated << " allow_delayed=" <<
      link.allow_delayed << " no_connection=" << link.no_connection << endl;
    o << "delay_info: " << link.delay_info;
    o << link.link_info;
    return o;
  }

  tb_delay_info delay_info;	// the delay characteristics of the link
  tb_link_info link_info;	// what it's mapped to
  crope name;			// name
  bool emulated;		// is this an emulated link, i.e. can it
				// share a plink withouter emulated vlinks
  bool no_connection;		// true if this link should be satisfied
				// but isn't.
  bool allow_delayed;		// can this vlink by a delayed link
  bool allow_trivial;           // can this vlink be a trivial link?
  vvertex src, dst;		// Source and destination for this link
};

extern tb_vgraph_vertex_pmap vvertex_pmap;
extern tb_vgraph_edge_pmap vedge_pmap;

typedef hash_map<crope,vvertex> name_vvertex_map;
typedef pair<vvertex,int> vvertex_int_pair;
typedef vector<vvertex> vvertex_vector;

struct ltnodepq {
  bool operator()(const vvertex_int_pair &A,const vvertex_int_pair &B) const {
    return A.second < B.second;
  };
};

typedef priority_queue<vvertex_int_pair,
  vector<vvertex_int_pair>, ltnodepq> vvertex_int_priority_queue;
typedef list<vvertex> vvertex_list;

int parse_top(tb_vgraph &G, istream& i);

/*
 * Globals
 */

/* The virtual graph, defined in assign.cc */
extern tb_vgraph VG;

#endif


