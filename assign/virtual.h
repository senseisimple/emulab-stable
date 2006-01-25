/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef __VIRTUAL_H
#define __VIRTUAL_H

/*
 * We have to do these includes differently depending on which version of gcc
 * we're compiling with
 */
#if __GNUC__ == 3 && __GNUC_MINOR__ > 0
#include <queue>
using namespace std;
#else
#include <queue>
#endif

#include <vector>
#include <list>
using namespace std;

#include "featuredesire.h"
#include "fstring.h"

#include <boost/config.hpp>
#include <boost/utility.hpp>
#include <boost/property_map.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
using namespace boost;

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
  typedef enum {LINK_UNMAPPED, LINK_DIRECT,
		LINK_INTRASWITCH, LINK_INTERSWITCH,
		LINK_TRIVIAL, LINK_DELAYED} linkType;
  linkType type_used;		// type of physical link used to satisfy this
  				// virtual link
  pedge_path plinks;		// the path of pedges
  pvertex_list switches;	// what switches were used

  tb_link_info() { ; };
  tb_link_info(linkType _type_used) : type_used(_type_used), plinks(), switches() { ; };
  tb_link_info(linkType _type_used, pedge_path _plinks) : type_used(_type_used), plinks(_plinks), switches() { ; };
  tb_link_info(linkType _type_used, pedge_path _plinks, pvertex_list _switches) : type_used(_type_used), plinks(_plinks), switches(_switches) { ; };
  
  friend ostream &operator<<(ostream &o, const tb_link_info& link)
  {
    o << "  Type: ";
    switch (link.type_used) {
    case LINK_UNMAPPED : o << "LINK_UNMAPPED"; break;
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
  tb_vnode(): vclass(NULL), fixed(false), assigned(false), subnode_of(NULL),
      subnode_of_name(""), typecount(1) {;}

  friend ostream &operator<<(ostream &o, const tb_vnode& node)
  {
    o << "tb_vnode: " << node.name << " (" << &node << ")" << endl;
    o << "  Type: " << node.type <<  " Count: " << node.typecount << endl;
    o << "  Desires:" << endl;
    for (node_desire_set::const_iterator it = node.desires.begin();
	 it!=node.desires.end();it++) 
      o << "    " << it->name() << " -> " << it->cost() << endl;
    o << " vclass=" << node.vclass << " fixed=" <<
      node.fixed << endl;
    return o;
  }

  // contains weight of each desire
  node_desire_set desires;

  fstring type;			// the current type of the node
  int typecount;		// How many slots of the type this vnode takes up
  tb_vclass *vclass;		// the virtual class of the node, if any
  fstring name;			// string name of the node
  bool fixed;			// is this node fixed
  bool assigned;		// is this node assigned?
  pvertex assignment;		// the physical vertex assigned to

#ifdef PER_VNODE_TT
  int num_links;
  int total_bandwidth;
#endif

  // For the case where we want to make sure that a vnode has all trivial
  // links, or no trivial links, but not a mix of both.
  bool disallow_trivial_mix;
  int nontrivial_links;
  int trivial_links;

  // For the subnode-of relationship - we need to keep track of not only our
  // own parent (if any), but any children we have, since our being assigned
  // will mean that we have to check them as well
  tb_vnode *subnode_of;
  typedef list<tb_vnode*> subnode_list;
  subnode_list subnodes;
  fstring subnode_of_name;

  // Counts how many links of each type this virtual node has
  typedef hash_map<fstring,int> link_counts_map;
  link_counts_map link_counts;

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
  fstring name;			// name
  fstring type;			// type of this link
  bool fix_src_iface;		// Did the user fix the name of the iface (src)?
  fstring src_iface;		// If so, this is what it must be named
  bool fix_dst_iface;		// Did the user fix the name of the iface (dst)?
  fstring dst_iface;		// If so, this is what it must be named

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

typedef hash_map<fstring,vvertex> name_vvertex_map;
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


