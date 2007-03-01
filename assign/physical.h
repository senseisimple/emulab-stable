/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef __PHYSICAL_H
#define __PHYSICAL_H

#include "common.h"
#include "delay.h"

#include <set>
#include <list>
using namespace std;

#include <boost/config.hpp>
#include <boost/utility.hpp>
#include <boost/property_map.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
using namespace boost;

#include <string>
using namespace std;

/*
 * We have to do these includes differently depending on which version of gcc
 * we're compiling with
 */
#ifdef NEW_GCC
#include <ext/hash_set>
#include <ext/hash_map>
using namespace __gnu_cxx;
#else
#include <hash_set>
#include <hash_map>
#endif

#include "featuredesire.h"

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
typedef hash_map<fstring,pvertex> name_pvertex_map;
typedef vector<svertex> switch_pred_map;
typedef hash_map<svertex,switch_pred_map*>switch_pred_map_map;
typedef vector<svertex> switch_dist_map;
typedef hash_map<svertex,switch_dist_map*>switch_dist_map_map;
typedef list<pedge> pedge_path;
typedef list<pvertex> pvertex_list;

typedef hash_map<fstring,int> link_type_count_map;

// Globals, declared in assign.cc

extern tb_pgraph_vertex_pmap pvertex_pmap;
extern tb_pgraph_edge_pmap pedge_pmap;
extern tb_sgraph_vertex_pmap svertex_pmap;
extern tb_sgraph_edge_pmap sedge_pmap;

/*
 * Represents a physical type
 */
class tb_ptype {
    public:
	tb_ptype(fstring _name) : users(0), max_users(0), my_name(_name), slots(0)
	    { ; }
	inline fstring name() const { return my_name; };
	inline int pnode_slots() const { return slots; };
	inline int maxusers() const { return max_users; };
	inline int add_users(int count = 1) {
	    int oldusers = users;
	    users = users + count;
	    if (max_users && (oldusers <= max_users) && (users > max_users)) {
		return 1;
	    } else {
		return 0;
	    }
	}
	inline int remove_users(int count = 1) {
	    int oldusers = users;
	    users = users - count;
	    assert(users >= 0);
	    if (max_users && (oldusers > max_users) && (users <= max_users)) {
		return 1;
	    } else {
		return 0;
	    }
	}
	inline void set_max_users(int users) {
	    max_users = users;
	}
	inline void add_slots(int additional_slots) {
	    slots += additional_slots;
	}
	inline void remove_slots(int slots_to_remove) {
	    slots -= slots_to_remove;
	}
    private:
	fstring my_name;
	/* How many users are using this type right now */
	int users;
	/* The maximum number of nodes of this type we're allowed to use */
	int max_users;
	/* How many slots of this type are available in the physical topology */
	int slots;
};


class tb_pnode {
public:
  tb_pnode() { tb_pnode("(unnamed)"); }
  tb_pnode(fstring _name) : types(), features(), name(_name), typed(false),
  			  current_type_record(NULL), total_load(0),
			  switches(), sgraph_switch(), switch_used_links(0),
			  total_interfaces(0), used_interfaces(0),
			  total_bandwidth(0), nontrivial_bw_used(0),
			  my_class(NULL), my_own_class(NULL), assigned_nodes(),
			  trivial_bw(0), trivial_bw_used(0), subnode_of(NULL),
			  subnode_of_name(""), has_subnode(false),
			  unique(false), is_switch(false) {;}

  class type_record {
      public:
	  type_record(int _max_load, bool _is_static, tb_ptype *_ptype) :
	      max_load(_max_load), current_load(0),
	      is_static(_is_static), ptype(_ptype) { ; }
	  int max_load;		// maximum load for this type
	  int current_load;	// how many vnodes are assigned of this type
	  bool is_static;	// whether this type is static or dynamic

	  tb_ptype *ptype;	// Pointer to the global ptype strucutre for
	  			// type

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

  // Contains max nodes for each type
  // NOTE: Parallel data strucure, see below!
  typedef hash_map<fstring,type_record*> types_map;
  types_map types;
  
  // Same as above, but a list for fast iteration
  // If you touch the above list, you must touch this one too
  typedef list<type_record*> types_list;
  types_list type_list;

  // contains cost of each feature
  node_feature_set features;

  fstring name;			// name of the node
  bool typed;			// has it been typed
  fstring current_type;		// type the node is currently being used as
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
  int nontrivial_bw_used;	// amount of non-trivial bandwidth in use on
  				// this node - for debugging only

  tb_pclass *my_class;		// the pclass this node belongs to

  tb_pclass *my_own_class;	// if using DYNAMIC_PCLASSES, a pointer to the
  				// node's own class

  tb_vnode_set assigned_nodes;	// the set of vnodes currently assigned

  int trivial_bw;		// the maximum amount of trivial bandwidth
  				// available
  int trivial_bw_used;		// the amount of bandwidth currently used by
  				// trivial links

  tb_pnode *subnode_of;		// the pnode, if any, that this node is a
  				// subnode of
  fstring subnode_of_name;        // name of the pnode this node is a subnode of -
                                // used to do late bindind

  bool has_subnode;		// whether or not this node has any subnodes

  bool unique;		// says that this pnode should never go into a
  				// pclass with other nodes, because of some
				// characteristic that is not known to assign

  bool is_switch;		// Indicates whether or not this pnode is a
                                // switch
				//
  link_type_count_map link_counts; // Counts how many links of each type this
  				   // node has 
	
  bool set_current_type(fstring type) {
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
      current_type = "";
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
      for (node_feature_set::const_iterator it = node.features.begin();
	   it != node.features.end(); it++) 
	cout << "    " << it->name() << " -> " << it->cost() << endl;
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

typedef pair<fstring,fstring> nodepair;
typedef hash_map<nodepair,int,pairhash<fstring> > nodepair_count_map;

class tb_plink {
public:
  typedef enum {PLINK_NORMAL,PLINK_INTERSWITCH,PLINK_LAN} plinkType;
  typedef hash_set<fstring> type_set;

  tb_plink(fstring _name, plinkType _is_type, fstring _type, fstring _srcmac, fstring
      _dstmac, fstring _srciface, fstring _dstiface)
    : name(_name), srcmac(_srcmac), dstmac(_dstmac), is_type(_is_type),
      srciface(_srciface), dstiface(_dstiface),
      delay_info(), bw_used(0), emulated(0), nonemulated(0),
      penalty(0.0), fixends(false), current_endpoints(), current_count(0),
      vedge_counts() {
	  types.insert(_type);
      }

  fstring name;			// the name
  fstring srcmac,dstmac;	// source and destination MAC addresses.
  fstring srciface, dstiface;	// source and destination interface names

  plinkType is_type;		// inter-switch type of the link
  type_set types;		// type (ie. ethernet) of the link
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
    switch (link.is_type) {
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

  // Return true if the two plinks are equivalent, in terms of type and
  // bandwidth.
  // NOTE: should probably use a helper function in delay_info, but right now,
  // we only care about bandwidth
  const bool is_equiv(const tb_plink& link) {
#ifdef PCLASS_DEBUG_TONS
      cerr << "        Comparing " << delay_info.bandwidth 
          << " and " << link.delay_info.bandwidth << endl;
#endif
      if (types != link.types) {
#ifdef PCLASS_DEBUG_TONS
          cerr << "            No, types" << endl;
#endif
	  return false;
      }
      if (delay_info.bandwidth != link.delay_info.bandwidth) {
#ifdef PCLASS_DEBUG_TONS
          cerr << "            No, bandwidth" << endl;
#endif
	  return false;
      }

#ifdef PCLASS_DEBUG_TONS
          cerr << "            Yes" << endl;
#endif
      return true;
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

/* A map of all tb_ptypes currently in existance */
typedef map<fstring,tb_ptype*> tb_ptype_map;
extern tb_ptype_map ptypes;

#endif
