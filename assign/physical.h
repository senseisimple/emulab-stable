#ifndef __PHYSICAL_H
#define __PHYSICAL_H

#include "common.h"
// Icky, but I can't include virtual.h here
class tb_vnode;
typedef hash_set<tb_vnode*,hashptr<tb_vnode*> > tb_vnode_set;

class tb_pclass;
class tb_pnode;
class tb_switch;
class tb_plink;
class tb_slink;

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

typedef hash_set<pvertex,hashptr<void*> > pvertex_set;
typedef hash_map<tb_pnode*,pvertex,hashptr<tb_pnode*> > pnode_pvertex_map;
typedef hash_map<crope,pvertex> name_pvertex_map;
typedef vector<svertex> switch_pred_map;
typedef hash_map<svertex,switch_pred_map*>switch_pred_map_map;
typedef vector<svertex> switch_dist_map;
typedef hash_map<svertex,switch_dist_map*>switch_dist_map_map;
typedef list<pedge> pedge_path;
typedef list<pvertex> pvertex_list;

extern tb_pgraph_vertex_pmap pvertex_pmap;
extern tb_pgraph_edge_pmap pedge_pmap;
extern tb_sgraph_vertex_pmap svertex_pmap;
extern tb_sgraph_edge_pmap sedge_pmap;

class tb_pnode {
public:
  tb_pnode() {;}

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
      o << "  Current Type: " << node.current_type <<
	" (" << node.current_load << "/" << node.max_load << ")" <<  endl;
      o << "  switches=";
      for (pvertex_set::const_iterator it = node.switches.begin();
	   it != node.switches.end();++it) {
	o << " " << get(pvertex_pmap,*it)->name;
      }
      o << endl;
      o << " pnodes_used="<< node.pnodes_used << " sgraph_switch=" <<
	node.sgraph_switch << " my_class=" << node.my_class << endl;
      return o;
    }

  typedef hash_map<crope,int> types_map;
  typedef hash_map<crope,double> features_map;

  // contains max nodes for each type
  types_map types;

  // contains cost of each feature
  features_map features;

  crope current_type;
  bool typed;			// has it been typed
  int max_load;			// maxmium load for current type
  int current_load;		// how many vnodes are assigned to this pnode
  crope name;
  pvertex_set switches;		// what switches the node is attached to
  int pnodes_used;		// for switch nodes
  svertex sgraph_switch;	// only for switches, the corresponding
				// sgraph switch.
  int switch_used_links;	// only for switches, how many links are
				// in use.  Switch is in use whenever > 0
  int total_interfaces;
#ifdef PENALIZE_UNUSED_INTERFACES
  int used_interfaces;
#endif

  tb_pclass *my_class;

#ifdef SMART_UNMAP
  tb_vnode_set assigned_nodes;
#endif
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

#ifdef FIX_PLINK_ENDPOINTS
// Hasher for pairs
template <class T> struct pairhash { 
    size_t operator()(pair<T,T> const &A) const {
	hash<T> H;
	return (H(A.first) | H(A.second));
    }
};

typedef pair<crope,crope> nodepair;
typedef hash_map<nodepair,int,pairhash<crope> > nodepair_count_map;
#endif

class tb_plink {
public:
#ifdef FIX_PLINK_ENDPOINTS
  tb_plink():current_count(0) {;}
#else
  tb_plink() {;}
#endif

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

  typedef enum {PLINK_NORMAL,PLINK_INTERSWITCH,PLINK_LAN} plinkType;

  plinkType type;
  tb_delay_info delay_info;	// the delay characteristics of this link
  int bw_used;			// how much is used
  crope srcmac,dstmac;		// source and destination MAC addresses.
  crope name;			// The name
  int emulated;			// number of emulated vlinks
  int nonemulated;		// number of nonemulated vlinks
  bool interswitch;		// is this an interswitch link
#ifdef PENALIZE_BANDWIDTH
  float penalty;
#endif
#ifdef FIX_PLINK_ENDPOINTS
  bool fixends;                 // If using as a emulated link, fix endpoints
  nodepair_count_map vedge_counts;
  nodepair current_endpoints;
  int current_count;
#endif
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

#endif
