/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef __PHYSICAL_H
#define __PHYSICAL_H

#include "common.h"
#include <set>

// Icky, but I can't include virtual.h here
class tb_vnode;
typedef hash_set<tb_vnode*,hashptr<tb_vnode*> > tb_vnode_set;

// Class forward declarations - defined below
class tb_pclass;
class tb_pnode;
class tb_switch;
class tb_plink;
class tb_slink;

// typedefs
typedef property<vertex_data_t,tb_pnode*> PNodeProperty;
typedef property<edge_data_t,tb_plink*> PEdgeProperty;
typedef property<vertex_data_t,tb_switch*> SNodeProperty;
typedef property<edge_data_t,tb_slink*,
  property<edge_weight_t,long> > SEdgeProperty;

typedef adjacency_list<listS,listS,undirectedS,
  PNodeProperty,PEdgeProperty> tb_pgraph;
typedef adjacency_list<listS,vecS,undirectedS,
  SNodeProperty,SEdgeProperty> tb_sgraph;

typedef property_map<tb_pgraph,vertex_data_t>::type tb_pgraph_vertex_pmap;
typedef property_map<tb_pgraph,edge_data_t>::type tb_pgraph_edge_pmap;
typedef property_map<tb_sgraph,vertex_data_t>::type tb_sgraph_vertex_pmap;
typedef property_map<tb_sgraph,edge_data_t>::type tb_sgraph_edge_pmap;
typedef property_map<tb_sgraph,edge_weight_t>::type tb_sgraph_weight_pmap;

typedef graph_traits<tb_pgraph>::vertex_descriptor pvertex;
typedef graph_traits<tb_pgraph>::edge_descriptor pedge;
typedef graph_traits<tb_sgraph>::vertex_descriptor svertex;
typedef graph_traits<tb_sgraph>::edge_descriptor sedge;

typedef graph_traits<tb_pgraph>::vertex_iterator pvertex_iterator;
typedef graph_traits<tb_pgraph>::edge_iterator pedge_iterator;
typedef graph_traits<tb_pgraph>::out_edge_iterator poedge_iterator;
typedef graph_traits<tb_sgraph>::vertex_iterator svertex_iterator;
typedef graph_traits<tb_sgraph>::edge_iterator sedge_iterator;
typedef graph_traits<tb_sgraph>::out_edge_iterator soedge_iterator;

typedef set<pvertex> pvertex_set;
typedef hash_map<tb_pnode*,pvertex,hashptr<tb_pnode*> > pnode_pvertex_map;
typedef hash_map<crope,pvertex> name_pvertex_map;
typedef vector<svertex> switch_pred_map;
typedef hash_map<svertex,switch_pred_map*>switch_pred_map_map;
typedef vector<svertex> switch_dist_map;
typedef hash_map<svertex,switch_dist_map*>switch_dist_map_map;
typedef list<pedge> pedge_path;
typedef list<pvertex> pvertex_list;

// Globals, declared in assign.cc

extern tb_pgraph_vertex_pmap pvertex_pmap;
extern tb_pgraph_edge_pmap pedge_pmap;
extern tb_sgraph_vertex_pmap svertex_pmap;
extern tb_sgraph_edge_pmap sedge_pmap;

class tb_pnode {
public:
  tb_pnode() { tb_pnode("(unnamed)"); }
  tb_pnode(crope _name) : types(), features(), name(_name), typed(false),
  			  current_type_record(NULL), total_load(0),
			  switches(), sgraph_switch(), switch_used_links(0),
			  total_interfaces(0), used_interfaces(0),
			  total_bandwidth(0), my_class(NULL),
			  my_own_class(NULL), assigned_nodes(),
			  trivial_bw(0), trivial_bw_used(0) {;}

  class type_record {
      public:
	  type_record(int _max_load, bool _is_static) :
	      max_load(_max_load), current_load(0),
	      is_static(_is_static) { ; }
	  int max_load;		// maximum load for this type
	  int current_load;	// how many vnodes are assigned of this type
	  bool is_static;	// whether this type is static or dynamic

	  bool operator==(const type_record &b) {
	      return ((max_load == b.max_load) && (is_static == b.is_static));
	  }

	  friend ostream &operator<<(ostream &o, const type_record& node)
	  {
	      o << "max_load = " << node.max_load <<
		   " current_load = " << node.current_load <<
		   " is_static = " << node.is_static;
	  }
  };

  typedef hash_map<crope,type_record*> types_map;
  typedef hash_map<crope,double> features_map;

  // contains max nodes for each type
  types_map types;

  // contains cost of each feature
  features_map features;

  crope name;			// name of the node
  bool typed;			// has it been typed
  crope current_type;		// type the node is currently being used as
  type_record* current_type_record;
  int total_load;		// total load for all types
  //int max_load;			// maxmium load for current type
  //int current_load;		// how many vnodes are assigned to this pnode
  pvertex_set switches;		// what switches the node is attached to

  svertex sgraph_switch;	// only for switches, the corresponding
				// sgraph switch.
  int switch_used_links;	// only for switches, how many links are
				// in use.  Switch is in use whenever > 0

  int total_interfaces;		// total number of links leaving the node
  int used_interfaces;		// number of links that are currently in use
  int total_bandwidth;		// total bandwidth of all this nodes' links

  tb_pclass *my_class;		// the pclass this node belongs to

  tb_pclass *my_own_class;	// if using DYNAMIC_PCLASSES, a pointer to the
  				// node's own class

  tb_vnode_set assigned_nodes;	// the set of vnodes currently assigned

  int trivial_bw;		// the maximum amount of trivial bandwidth
  				// available
  int trivial_bw_used;		// the amount of bandwidth currently used by
  				// trivial links
	
  bool set_current_type(crope type) {
      if (types.find(type) == types.end()) {
	  //cout << "Failed to find type " << type << endl;
	  return false;
      }
      current_type = type;
      typed = true;
      current_type_record = types[type];
      return true;
  }

  void remove_current_type() {
      typed = false;
      current_type_record = NULL;
  }

  // Output operator for debugging
  friend ostream &operator<<(ostream &o, const tb_pnode& node)
    {
      o << "tb_pnode: " << node.name << " (" << &node << ")" << endl;
      o << "  Types:" << endl;
      for (types_map::const_iterator it = node.types.begin();
	   it!=node.types.end();it++) 
	o << "    " << (*it).first << " -> " << (*it).second << endl;
      o << "  Features:" << endl;
      for (features_map::const_iterator it = node.features.begin();
	   it!=node.features.end();it++) 
	cout << "    " << (*it).first << " -> " << (*it).second << endl;
      o << "  Current Type: " << node.current_type << endl; /* <<
	" (" << node.current_load << "/" << node.max_load << ")" <<  endl; */
      o << "  switches=";
      for (pvertex_set::const_iterator it = node.switches.begin();
	   it != node.switches.end();++it) {
	o << " " << get(pvertex_pmap,*it)->name;
      }
      o << endl;
      o << " sgraph_switch=" << node.sgraph_switch
	  << " my_class=" << node.my_class << endl;
      return o;
    }

};

class tb_switch {
public:
  friend ostream &operator<<(ostream &o, const tb_switch& node)
  {
    o << "tb_switch: " << &node << endl;
    o << "  mate=" << node.mate << endl;
    return o;
  }
  
  tb_switch() {;}
  pvertex mate;			// match in PG
};

// Hasher for pairs
template <class T> struct pairhash { 
    size_t operator()(pair<T,T> const &A) const {
	hash<T> H;
	return (H(A.first) | H(A.second));
    }
};

typedef pair<crope,crope> nodepair;
typedef hash_map<nodepair,int,pairhash<crope> > nodepair_count_map;

class tb_plink {
public:
  typedef enum {PLINK_NORMAL,PLINK_INTERSWITCH,PLINK_LAN} plinkType;

  tb_plink(crope _name, plinkType _type, crope _srcmac, crope _dstmac)
    : name(_name), srcmac(_srcmac), dstmac(_dstmac), type(_type),
      delay_info(), bw_used(0), emulated(0), nonemulated(0),
      penalty(0.0), fixends(false), current_endpoints(), current_count(0),
      vedge_counts() {;}

  crope name;			// the name
  crope srcmac,dstmac;		// source and destination MAC addresses.

  plinkType type;		// type of the link
  tb_delay_info delay_info;	// the delay characteristics of this link
  int bw_used;			// how much is used

  int emulated;			// number of emulated vlinks
  int nonemulated;		// number of nonemulated vlinks
  float penalty;		// penaly for bandwidth used

  bool fixends;                 // if using as a emulated link, fix endpoints
  nodepair current_endpoints;	// pnodes at the endpoints of the vlink using
  				// this plink
  int current_count;		// number of mapped vlinks that share these
				// pnode endpoints
  nodepair_count_map vedge_counts; // list, and count, of all pairs of pnode
				   // endpoints sharing this link
				   
  friend ostream &operator<<(ostream &o, const tb_plink& link)
  {
    o << "tb_plink: " << link.name << " (" << &link << ")" << endl;
    o << "  type: ";
    switch (link.type) {
    case tb_plink::PLINK_NORMAL:
      o << "normal" << endl;
      break;
    case tb_plink::PLINK_INTERSWITCH:
      o << "interswitch" << endl;
      break;
    case tb_plink::PLINK_LAN:
      o << "lan" << endl;
      break;
    }
    o << "  bw_used=" << link.bw_used <<
      " srcmac=" << link.srcmac << " dstmac=" << link.dstmac <<
      " emulated=" << link.emulated << " nonemulated=" <<
      link.nonemulated << endl;
    o << link.delay_info;
    return o;
  }
};

class tb_slink {
public:
  tb_slink() {;}

  friend ostream &operator<<(ostream &o, const tb_slink &link)
  {
    o << "tb_slink: " << &link << endl;
    o << "  mate=" << link.mate << endl;
    return o;
  }
  pedge mate;			// match in PG
};

int parse_ptop(tb_pgraph &PG, tb_sgraph &SG, istream& i);

/*
 * Globals
 */

/* The physical graph, defined in assign.cc */
extern tb_pgraph PG;

#endif
