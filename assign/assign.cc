/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "port.h"

#include <hash_map>
#include <rope>
#include <queue>

#include <boost/config.hpp>
#include <boost/utility.hpp>
#include <boost/property_map.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/graphviz.hpp>

#include <fstream.h>
#include <iostream.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

using namespace boost;

#include "common.h"
#include "delay.h"
#include "physical.h"
#include "virtual.h"
#include "vclass.h"
#include "pclass.h"
#include "score.h"

/*
 * 'optimal' score - computes a lower bound on the optimal score for this
 * mapping, so that if we find this score, we know we're done and can exit
 * early.
 */
#ifdef USE_OPTIMAL
#define OPTIMAL_SCORE(edges,nodes) (nodes*SCORE_PNODE + \
                                    nodes/opt_nodes_per_sw*SCORE_SWITCH + \
                                    edges*((SCORE_INTRASWITCH_LINK+ \
                                    SCORE_DIRECT_LINK*2)*4+\
                                    SCORE_INTERSWITCH_LINK)/opt_nodes_per_sw)
#else
#define OPTIMAL_SCORE(edges,nodes) 0
#endif

#ifdef ROB_DEBUG
#define RDEBUG(a) a
#else
#define RDEBUG(a)
#endif

// Some defaults for #defines
#ifndef NO_REVERT
#define NO_REVERT 0
#endif

#ifndef REVERT_VIOLATIONS
#define REVERT_VIOLATIONS 1
#endif

#ifndef REVERT_LAST
#define REVERT_LAST 0
#endif

#ifdef PHYS_CHAIN_LEN
#define PHYSICAL(x) x
#else
#define PHYSICAL(x) 0
#endif

// Here we set up all our graphs.  Need to create the graphs
// themselves and then setup the property maps.
tb_pgraph PG;
tb_pgraph_vertex_pmap pvertex_pmap = get(vertex_data, PG);
tb_pgraph_edge_pmap pedge_pmap = get(edge_data, PG);
tb_sgraph SG;
tb_sgraph_vertex_pmap svertex_pmap = get(vertex_data, SG);
tb_sgraph_edge_pmap sedge_pmap = get(edge_data, SG);
tb_vgraph VG;
tb_vgraph_vertex_pmap vvertex_pmap = get(vertex_data, VG);
tb_vgraph_edge_pmap vedge_pmap = get(edge_data, VG);

// Map of physical node name to its vertex descriptor.
name_pvertex_map pname2vertex;

// A simple list of physical types.
name_count_map ptypes;

// Map of virtual node name to its vertex descriptor.
name_vvertex_map vname2vertex;

// This is a vector of all the nodes in the top file.  It's used
// to randomly choose nodes.
vvertex_vector virtual_nodes;

// Map of virtual node name to the physical node name it's fixed too.
// The domain is the set of all fixed virtual nodes and the range is
// the set of all fixed physical nodes.
name_name_map fixed_nodes;

// List of virtual types by name.
name_count_map vtypes;

// Priority queue of unassigned virtual nodes.  Basically a fancy way
// of randomly choosing a unassigned virtual node.  When nodes become
// unassigned they are placed in the queue with a random priority.
vvertex_int_priority_queue unassigned_nodes;

// Map from a pnode* to the the corresponding pvertex.
pnode_pvertex_map pnode2vertex;

// A list of all pclasses.
pclass_list pclasses;

// Map of a type to a tt_entry, a vector of pclasses and the size of
// the vector.
pclass_types type_table;
#ifdef PER_VNODE_TT
pclass_types vnode_type_table;
#endif

// This datastructure contains all the information needed to calculate
// the shortest path between any two switches.  Indexed by svertex,
// the value will be a predicate map (indexed by svertex as well) of
// the shortest paths for the given vertex.
switch_pred_map_map switch_preds;

// Same, but for distances 
switch_dist_map_map switch_dist;

// Time started, finished, and the time limit
double timestart, timeend, timelimit, timetarget;

#ifdef GNUPLOT_OUTPUT
FILE *scoresout, *tempout, *deltaout;
#endif

// A hash function for graph edges.
struct hashedge {
  size_t operator()(vedge const &A) const {
    hashptr<void *> ptrhash;
    return ptrhash(target(A,VG))/2+ptrhash(source(A,VG))/2;
  }
};

typedef hash_map<vvertex,pvertex,hashptr<void *> > node_map;
typedef hash_map<vvertex,bool,hashptr<void *> > assigned_map;
typedef hash_map<pvertex,crope,hashptr<void *> > type_map;
typedef hash_map<vedge,tb_link_info,hashedge> link_map;

// A scaling constant for the temperature in determining whether to
// accept a change.
//static double sensitivity = 0.1;
static double sensitivity = 1;

// The number of accepts of increase that took place during the annealing.
int accepts;

// The number of iterations that took place.
int iters;

// These variables store the best solution.
node_map absassignment;		// assignment field of vnode
assigned_map absassigned;	// assigned field of vnode
type_map abstypes;		// type field of vnode
double absbest;			// score
int absbestviolated;		// violated
int iters_to_best = 0;		// iters

int npnodes;

// Whether or not assign is allowed to generate trivial links
bool allow_trivial_links = true;

// Whether or not assign should use pclasses
bool use_pclasses = true;

// Whether or not assign should prune out pclasses that it knows can
// never be used
bool prune_pclasses = false;

// Determines whether to accept a change of score difference 'change' at
// temperature 'temperature'.
inline int accept(double change, double temperature)
{
  double p;
  int r;

  if (change == 0) {
    p = 1000 * temperature / temp_prob;
  } else {
    p = expf(change/(temperature*sensitivity)) * 1000;
  }
  r = std::random() % 1000;
  if (r < p) {
    //accepts++;
    return 1;
  }
  return 0;
}

// Return the CPU time (in seconds) used by this process
float used_time()
{
  struct rusage ru;
  getrusage(RUSAGE_SELF,&ru);
  return ru.ru_utime.tv_sec+ru.ru_utime.tv_usec/1000000.0+
    ru.ru_stime.tv_sec+ru.ru_stime.tv_usec/1000000.0;
}

// Read in the .ptop file
void read_physical_topology(char *filename)
{
  ifstream ptopfile;
  ptopfile.open(filename);
  if (!ptopfile.is_open()) {
      cerr << "Unable to open ptop file " << filename << endl;
      exit(2);
  }
  cout << "Physical Graph: " << parse_ptop(PG,SG,ptopfile) << endl;

#ifdef DUMP_GRAPH
  {
    cout << "Physical Graph:" << endl;
    
    pvertex_iterator vit,vendit;
    tie(vit,vendit) = vertices(PG);
    
    for (;vit != vendit;vit++) {
      tb_pnode *p = get(pvertex_pmap,*vit);
      cout << *vit << "\t" << *p;
    }
    
    pedge_iterator eit,eendit;
    tie(eit,eendit) = edges(PG);

    for (;eit != eendit;eit++) {
      tb_plink *p = get(pedge_pmap,*eit);
      cout << *eit << " (" << source(*eit,PG) << " <-> " <<
	target(*eit,PG) << ")\t" << *p;
    }
  }
#endif // DUMP_GRAPH

#ifdef GRAPH_DEBUG
  {
    cout << "Switch Graph:" << endl;
    

    svertex_iterator vit,vendit;
    tie(vit,vendit) = vertices(SG);
    
    for (;vit != vendit;vit++) {
      tb_switch *p = get(svertex_pmap,*vit);
      cout << *vit << "\t" << *p;
    }
    
    sedge_iterator eit,eendit;
    tie(eit,eendit) = edges(SG);

    for (;eit != eendit;eit++) {
      tb_slink *p = get(sedge_pmap,*eit);
      cout << *eit << " (" << source(*eit,SG) << " <-> " <<
	target(*eit,SG) << ")\t" << *p;
    }
  }
#endif // GRAPH_DEBUG

  // Set up pnode2vertex - a mapping between vertices in the physical graph and
  // the pnodes that we just read in from the ptop file
  pvertex_iterator pvit,pvendit;
  tie(pvit,pvendit) = vertices(PG);
  for (;pvit != pvendit;pvit++) {
    pnode2vertex[get(pvertex_pmap,*pvit)]=*pvit;
  }

}

// Calculate the minimum spanning tree for the switches - we only consider one
// potential path between each pair of switches.
void calculate_switch_MST()
{
  cout << "Calculating shortest paths on switch fabric." << endl;

  // Set up the weight map for Dijkstra's
  tb_sgraph_weight_pmap sweight_pmap = get(edge_weight, SG);
  sedge_iterator seit,seendit;
  tie(seit,seendit) = edges(SG);
  for (;seit != seendit;seit++) {
    tb_slink *slink = get(sedge_pmap,*seit);
    // XXX should we make this more complicated depending on
    // latency/loss as well?
    put(sweight_pmap,*seit,
	100000000-get(pedge_pmap,slink->mate)->delay_info.bandwidth);
  }

  // Let boost do the Disjktra's for us, from each switch
  svertex_iterator svit,svendit;
  tie(svit,svendit) = vertices(SG);
  for (;svit != svendit;svit++) {
    switch_preds[*svit] = new switch_pred_map(num_vertices(SG));
    switch_dist[*svit] = new switch_dist_map(num_vertices(SG));
    dijkstra_shortest_paths(SG,*svit,
    			    predecessor_map(&((*switch_preds[*svit])[0])).
			    distance_map(&((*switch_dist[*svit])[0])));
  }

#ifdef GRAPH_DEBUG
  cout << "Shortest paths" << endl;
  tie(svit,svendit) = vertices(SG);
  for (;svit != svendit;svit++) {
    cout << *svit << ":" << endl;
    for (unsigned int i = 0;i<num_vertices(SG);++i) {
      cout << i << " " << (*switch_dist[*svit])[i] << endl;
    }
  }
#endif // GRAPH_DEBUG
}

// Read in the .top file
void read_virtual_topology(char *filename)
{
  ifstream topfile;
  topfile.open(filename);
  if (!topfile.is_open()) {
      cerr << "Unable to open top file " << filename << endl;
      exit(2);
  }
  cout << "Virtual Graph: " << parse_top(VG,topfile) << endl;

#ifdef DUMP_GRAPH
  {
    cout << "Virtual Graph:" << endl;
    

    vvertex_iterator vit,vendit;
    tie(vit,vendit) = vertices(VG);
    
    for (;vit != vendit;vit++) {
      tb_vnode *p = get(vvertex_pmap,*vit);
      cout << *vit << "\t" << *p;
    }
    
    vedge_iterator eit,eendit;
    tie(eit,eendit) = edges(VG);

    for (;eit != eendit;eit++) {
      tb_vlink *p = get(vedge_pmap,*eit);
      cout << *eit << " (" << source(*eit,VG) << " <-> " <<
	target(*eit,VG) << ")\t" << *p;
    }
  }  
#endif
}

tb_pnode *find_pnode(tb_vnode *vn)
{
#ifdef PER_VNODE_TT
  tt_entry tt = vnode_type_table[vn->name];
#else
  tt_entry tt = type_table[vn->type];
#endif
  int num_types = tt.first;
  pclass_vector *acceptable_types = tt.second;
  
  tb_pnode *newpnode;
  
  int i = std::random()%num_types;
  int first = i;
  for (;;) {
    i = (i+1)%num_types;
#ifdef PCLASS_SIZE_BALANCE
    int acceptchance = 1000 * (*acceptable_types)[i]->size * 1.0 /
	npnodes;
    //cout << "Chance was " << acceptchance << endl;
    if ((std::rand() % 1000) < acceptchance) {
	continue;
    }
#endif
#ifdef LOAD_BALANCE
REDO_SEARCH:
    tb_pnode* firstmatch = NULL;
#endif
#ifdef FIND_PNODE_SEARCH
#ifdef PER_VNODE_TT
    // If using PER_VNODE_TT and vclasses, it's possible that there are
    // some pclasses in this node's type table that can't be used right now,
    // becuase they contain entires that don't contain the vnodes _current_
    // type
    if ((*acceptable_types)[i]->members.find(vn->type) ==
	    (*acceptable_types)[i]->members.end()) {
	continue;
    }
#endif
    list<tb_pnode*>::iterator it = (*acceptable_types)[i]->members[vn->type]->L.begin();
#ifdef LOAD_BALANCE
    int skip = std::rand() % (*acceptable_types)[i]->members[vn->type]->L.size();
    // Skip the begging of the list
    for (int j = 0; j < skip; j++) {
	it++;
    }
#endif
    while (it != (*acceptable_types)[i]->members[vn->type]->L.end()) {
#ifdef LOAD_BALANCE
	if ((*it)->typed) {
	    if ((*it)->current_type.compare(vn->type)) {
		it++;
	    } else {
		if (firstmatch == NULL) {
		    firstmatch = *it;
		}
		double acceptchance = 1 - (*it)->current_load * 1.0
		    / (*it)->max_load;

	        int p = 1000 * acceptchance;
		if ((std::random() % 1000) < (1000 * acceptchance)) {
		    break;
		} else {
		    it++;
		}
	    }
	} else {
	    break;
	}
#else
	if ((*it)->typed && ((*it)->current_type.compare(vn->type) ||
			     ((*it)->current_load >= (*it)->max_load))) {
	    it++;
	} else {
	    break;
	}
#endif
    }
    if (it == (*acceptable_types)[i]->members[vn->type]->L.end()) {
#ifdef LOAD_BALANCE
	if (firstmatch) {
	    //newpnode = firstmatch;
	    goto REDO_SEARCH;
	} else {
	    newpnode = NULL;
	}
#else
	newpnode = NULL;
#endif
    } else {
	newpnode = *it;
    }
#else
    newpnode = (*acceptable_types)[i]->members[vn->type]->front();
#endif
#ifdef PCLASS_DEBUG
    cerr << "Found pclass: " <<
      (*acceptable_types)[i]->name << " and node " <<
      (newpnode == NULL ? "NULL" : newpnode->name) << "\n";
#endif
    if (newpnode != NULL) {
      RDEBUG(cout << " to " << newpnode->name << endl;)
      return newpnode;
    }
    
#ifndef PCLASS_SIZE_BALANCE
    if (i == first) {
	// couldn't find one
	return NULL;
    }
#endif
  }
}

/*
 * Make a pass through the pclasses, looking for ones that no node can use, and
 * nuking them.
 */
void prune_unusable_pclasses() {
    cout << "Pruning pclasses." << endl;
    int pruned = 0;
    pclass_list::iterator pclass_iterator = pclasses.begin();
    while (pclass_iterator != pclasses.end()) {
	if ((*pclass_iterator)->refcount == 0) {
	    pclass_list::iterator nukeme = pclass_iterator;
	    pclass_iterator++;
#ifdef PCLASS_DEBUG
	    cout << "Pruning " << (*nukeme)->name << endl;
#endif
	    pruned++;
	    delete *nukeme;
	    pclasses.erase(nukeme);
	} else {
	    pclass_iterator++;
	}
    }
    cout << "pclass pruning complete: removed " << pruned << " pclasses, " <<
	pclasses.size() << " remain." << endl;
}

/* When this is finished the state will reflect the best solution found. */
void anneal()
{
  cout << "Annealing." << endl;

  double newscore = 0;
  double bestscore = 0;
  
  iters = 0;
  iters_to_best =0;
  accepts = 0;
  
  double scorediff;

  int nnodes = num_vertices(VG);
  npnodes = num_vertices(PG);
  int npclasses = pclasses.size();
  
  float cycles = CYCLES*(float)(nnodes + num_edges(VG) + PHYSICAL(npnodes));
  float optimal = OPTIMAL_SCORE(num_edges(VG),nnodes);
    
#ifdef STATS
  cout << "STATS_OPTIMAL = " << optimal << endl;
#endif

  int mintrans = (int)cycles;
  int trans;
  int naccepts = 20*(nnodes + PHYSICAL(npnodes));
  pvertex oldpos;
  bool oldassigned;
  int bestviolated;
  int num_fixed=0;
  double meltedtemp;
  double temp = init_temp;
  double deltatemp, deltaavg;

#ifdef VERBOSE
  cout << "Initialized to cycles="<<cycles<<" optimal="<<optimal<<" mintrans="
       << mintrans<<" naccepts="<<naccepts<< endl;
#endif

  /* Set up the initial counts */
  init_score();

  /* Set up fixed nodes */
  for (name_name_map::iterator fixed_it=fixed_nodes.begin();
       fixed_it!=fixed_nodes.end();++fixed_it) {
    if (vname2vertex.find((*fixed_it).first) == vname2vertex.end()) {
      cerr << "Fixed node: " << (*fixed_it).first <<
	"does not exist." << endl;
      exit(2);
    }
    vvertex vv = vname2vertex[(*fixed_it).first];
    if (pname2vertex.find((*fixed_it).second) == pname2vertex.end()) {
      cerr << "Fixed node: " << (*fixed_it).second <<
	" not available." << endl;
      exit(2);
    }
    pvertex pv = pname2vertex[(*fixed_it).second];
    tb_vnode *vn = get(vvertex_pmap,vv);
    tb_pnode *pn = get(pvertex_pmap,pv);
    if (vn->vclass != NULL) {
      cerr << "Can not have fixed nodes be in a vclass!.\n";
      exit(2);
    }
    if (add_node(vv,pv,false) == 1) {
      cerr << "Fixed node: Could not map " << vn->name <<
	" to " << pn->name << endl;
      exit(2);
    }
    vn->fixed = true;
    num_fixed++;
  }

  bestscore = get_score();
  bestviolated = violated;

#ifdef VERBOSE
  cout << "Problem started with score "<<bestscore<<" and "<< violated
       << " violations." << endl;
#endif

  absbest = bestscore;
  absbestviolated = bestviolated;

  vvertex_iterator vit,veit;
  tie(vit,veit) = vertices(VG);
  for (;vit!=veit;++vit) {
    tb_vnode *vn = get(vvertex_pmap,*vit);
    absassigned[*vit] = vn->assigned;
    if (vn->assigned) {
      assert(vn->fixed);
      absassignment[*vit] = vn->assignment;
      abstypes[*vit] = vn->type;
    } else {
      unassigned_nodes.push(vvertex_int_pair(*vit,std::random()));
    }
  }

  int neighborsize;
  neighborsize = nnodes * npclasses;
  if (neighborsize < min_neighborhood_size) {
    neighborsize = min_neighborhood_size;
  }
#ifdef CHILL
  double scores[neighborsize];
#endif

  if (num_fixed == nnodes) {
    cout << "All nodes are fixed.  No annealing." << endl;
    goto DONE;
  }
  
  // Annealing loop!
  vvertex vv;
  tb_vnode *vn;

  // Crap added by ricci
  bool melting;
  int nincreases, ndecreases;
  double avgincrease;
  double avgscore;
  double initialavg;
  double stddev;
  bool finished;
  bool forcerevert;
  finished = forcerevert = false;
  int tsteps;
  int mintsteps;
  double meltstart;

#define MAX_AVG_HIST 16
  double avghist[MAX_AVG_HIST];
  int hstart, nhist;
  hstart = nhist = 0;
  double lasttemp;
  double smoothedavg, lastsmoothed;
  lastsmoothed = 500000.0f;
  lasttemp = 5000.0f;
  int melttrials;
  melttrials = 0;

  bool finishedonce;
  finishedonce = false;

  tsteps = 0;
  mintsteps = MAX_AVG_HIST;
  tsteps = 0;
  mintsteps = MAX_AVG_HIST;
  tsteps = 0;
  mintsteps = MAX_AVG_HIST;

  // Make sure the last two don't prevent us from running!
  avgscore = initialavg = 1.0;

  stddev = 0;

#ifdef MELT
  melting = true;
#ifdef TIME_TARGET
  meltstart = used_time();
#endif
#else
  melting = false;
#endif

  melt_trans = neighborsize;
#ifdef EPSILON_TERMINATE
  while(1) {
#else
  while (temp >= temp_stop) {
#endif
#ifdef VERBOSE
    cout << "Temperature:  " << temp << " AbsBest: " << absbest <<
      " (" << absbestviolated << ")" << endl;
#endif
    trans = 0;
    accepts = 0;
    nincreases = ndecreases = 0;
    avgincrease = 0.0;
    avgscore = bestscore;
#ifdef CHILL
    scores[0] = bestscore;
#endif

    if (melting) {
      cout << "Doing melting run" << endl;
    }

    while ((melting && (trans < melt_trans))
#ifdef NEIGHBOR_LENGTH
	    || (trans < neighborsize)) {
#else
	    || (!melting && (trans < mintrans && accepts < naccepts))) {
#endif

#ifdef STATS
      cout << "STATS temp:" << temp << " score:" << get_score() <<
	" violated:" << violated << " trans:" << trans <<
	" accepts:" << accepts << " current_time:" <<
	used_time() << endl;
#endif 
      pvertex newpos;
      trans++;
      iters++;

      bool freednode = false;
      if (! unassigned_nodes.empty()) {
	vv = unassigned_nodes.top().first;
	assert(!get(vvertex_pmap,vv)->assigned);
	unassigned_nodes.pop();
      } else {
	int start = std::random()%nnodes;
	int choice = start;
#if defined(FIX_LAN_NODES) || defined(AUTO_MIGRATE)
	while (get(vvertex_pmap,virtual_nodes[choice])->fixed ||
		!get(vvertex_pmap,virtual_nodes[choice])->type.compare("lan")) {
#else
	while (get(vvertex_pmap,virtual_nodes[choice])->fixed) {
#endif
	  choice = (choice +1) % nnodes;
	  if (choice == start) {
	      choice = -1;
	      break;
	  }
	}
	if (choice >= 0) {
	    vv = virtual_nodes[choice];
	} else {
	    cout << "All nodes are fixed or LANs.  No annealing." << endl;
	    goto DONE;
	}
      }
      
      vn = get(vvertex_pmap,vv);
      RDEBUG(cout << "Reassigning " << vn->name << endl;)
      oldassigned = vn->assigned;
      oldpos = vn->assignment;
      
#ifdef FREE_IMMEDIATELY
      if (oldassigned) {
	remove_node(vv);
	RDEBUG(cout << "Freeing up " << vn->name << endl;)
      }
#endif
      
      if (vn->vclass != NULL) {
	vn->type = vn->vclass->choose_type();
#ifdef SCORE_DEBUG
	cerr << "vclass " << vn->vclass->name  << ": choose type for " <<
	    vn->name << " = " << vn->type << " dominant = " <<
	    vn->vclass->dominant << endl;
#endif
      }
      if (vn->type.compare("lan") == 0) {
	// LAN node
	pvertex lanv = make_lan_node(vv);
#ifndef FREE_IMMEDIATELY
	if (oldassigned) {
	  RDEBUG(cout << "removing: lan,oldassigned" << endl;)
	  remove_node(vv);
	}
#endif
	if (add_node(vv,lanv,false) != 0) {
	  delete_lan_node(lanv);
	  unassigned_nodes.push(vvertex_int_pair(vv,std::random()));
	  continue;
	}
      } else {
	tb_pnode *newpnode = find_pnode(vn);
#ifndef FREE_IMMEDIATELY
	if (oldassigned) {
	  RDEBUG(cout << "removing: !lan, oldassigned" << endl;)
	  remove_node(vv);
	}
#endif
	if (newpnode == NULL) {
	  // We're not going to be re-assigning this one
#ifndef SMART_UNMAP
	  unassigned_nodes.push(vvertex_int_pair(vv,std::random()));
#endif
	  // need to free up nodes
#ifdef SMART_UNMAP
     	  // XXX: Should probably randomize this
	  // XXX: Add support for not using PER_VNODE_TT
	  // XXX: Not very robust

	  freednode = true;

	  tt_entry tt = vnode_type_table[vn->name];
	  int size = tt.first;
	  pclass_vector *acceptable_types = tt.second;
	  // Find a node to kick out
	  bool foundnode = false;
	  int offi = std::rand();
	  int index;
	  for (int i = 0; i < size; i++) {
	      index = (i + offi) % size;
	      if ((*acceptable_types)[index]->used_members.find(vn->type) ==
		      (*acceptable_types)[index]->used_members.end()) {
		  continue;
	      }
	      if ((*acceptable_types)[index]->used_members[vn->type]->size() == 0) {
		  continue;
	      }
	      foundnode = true;
	      break;
	  }

	  if (foundnode) {
	      assert((*acceptable_types)[index]->used_members[vn->type]->size());
	      tb_pclass::tb_pnodeset::iterator it = (*acceptable_types)[index]->used_members[vn->type]->begin();
	      int j = std::rand() % (*acceptable_types)[index]->used_members[vn->type]->size();
	      while (j > 0) {
		  it++;
		  j--;
	      }
	      tb_vnode_set::iterator it2 = (*it)->assigned_nodes.begin();
	      int k = std::rand() % (*it)->assigned_nodes.size();
	      while (k > 0) {
		  it2++;
		  k--;
	      }
	      tb_vnode *kickout = *it2;
	      assert(kickout->assigned);
	      vvertex toremove = vname2vertex[kickout->name];
	      newpnode = *it;
	      remove_node(toremove);
	      unassigned_nodes.push(vvertex_int_pair(toremove,
			  std::random()));
	  } else {
	      cerr << "Failed to find a replacement!" << endl;
	  }

#else
	  int start = std::random()%nnodes;
	  int toremove = start;
#if defined(FIX_LAN_NODES) || defined(AUTO_MIGRATE)
	  while (get(vvertex_pmap,virtual_nodes[toremove])->fixed ||
		 (!get(vvertex_pmap,virtual_nodes[toremove])->assigned) ||
		 (get(vvertex_pmap,virtual_nodes[toremove])->type.compare("lan"))) {
#else
#ifdef SMART_UNMAP

#ifdef PER_VNODE_TT
          tt_entry tt = vnode_type_table[vn->name];
#else
	  tt_entry tt = type_table[vn->type];
#endif
	  pclass_vector *acceptable_types = tt.second;

	  while (1) {
	      bool keepgoing = false;
	      if (get(vvertex_pmap,virtual_nodes[toremove])->fixed) {
		  keepgoing = true;
	      } else if (! get(vvertex_pmap,virtual_nodes[toremove])->assigned) {
		  keepgoing = true;
	      } else {
		  pvertex pv = get(vvertex_pmap,virtual_nodes[toremove])->assignment;
		  tb_pnode *pn = get(pvertex_pmap,pv);
		  int j;
		  for (j = 0; j < acceptable_types->size(); j++) {
		      if ((*acceptable_types)[j] == pn->my_class) {
			  break;
		      }
		  }
		  if (j == acceptable_types->size()) {
		      keepgoing = true;
		  }
	      }

	      if (!keepgoing) {
		  break;
	      }
		  
#else
	  while (get(vvertex_pmap,virtual_nodes[toremove])->fixed ||
		 (! get(vvertex_pmap,virtual_nodes[toremove])->assigned)) {
#endif
#endif
	      toremove = (toremove +1) % nnodes;
	      if (toremove == start) {
		  toremove = -1;
		  break;
	      }
	  }
	  if (toremove >= 0) {
	      RDEBUG(cout << "removing: freeing up nodes" << endl;)
	      remove_node(virtual_nodes[toremove]);
	      unassigned_nodes.push(vvertex_int_pair(virtual_nodes[toremove],
						     std::random()));
	  }
	  continue;
#endif /* SMART_UNMAP */
#ifndef SMART_UNMAP
	} else {
#else
	}
#endif
	  if (newpnode != NULL) {
	      newpos = pnode2vertex[newpnode];
	      if (add_node(vv,newpos,false) != 0) {
		  unassigned_nodes.push(vvertex_int_pair(vv,std::random()));
		  continue;
	      }
	  } else {
#ifdef SMART_UNMAP
		  unassigned_nodes.push(vvertex_int_pair(vv,std::random()));
#endif
	      if (freednode) {
		  continue;
	      }
	  }
#ifndef SMART_UNMAP
	}
#endif
      }

#ifdef FIX_LAN_NODES
      // OK, we're going to do something silly here: Migrate LAN nodes!
      vvertex_iterator lanvertex_it,end_lanvertex_it;
      vvertex_list migrate_lan_nodes;
      tie(lanvertex_it,end_lanvertex_it) = vertices(VG);
      for (;lanvertex_it!=end_lanvertex_it;++lanvertex_it) {
	  tb_vnode *vnode = get(vvertex_pmap,*lanvertex_it);
	  if (vnode->assigned) {
	      if (vnode->type.compare("lan") == 0) {
		  migrate_lan_nodes.push_front(*lanvertex_it);
	      }
	  }
      }
      while (migrate_lan_nodes.size() > 0) {
	  vvertex lanv = migrate_lan_nodes.front();
	  migrate_lan_nodes.pop_front();
	  RDEBUG(cout << "removing: migration" << endl;)
	  remove_node(lanv);
	  pvertex lanpv = make_lan_node(lanv);
	  add_node(lanv,lanpv,true);
      }

#endif

      newscore = get_score();
      assert(newscore >= 0);

      // Negative means bad
      scorediff = bestscore - newscore;
      // This looks funny, because < 0 means worse, which means an increase in
      // score
      if (scorediff < 0) {
	nincreases++;
	avgincrease = avgincrease * (nincreases -1) / nincreases +
	  (-scorediff)  / nincreases;
      } else {
	ndecreases++;
      }
      
      bool accepttrans = false;
      if (newscore < optimal) {
	  accepttrans = true;
	  RDEBUG(cout << "accept: optimal (" << newscore << "," << optimal
		  << ")" << endl;)
      } else if (melting) {
	  accepttrans = true;
	  RDEBUG(cout << "accept: melting" << endl;)
      } else
#ifdef NO_VIOLATIONS
	  if (newscore < bestscore) {
	      accepttrans = true;
	      RDEBUG(cout << "accept: better (" << newscore << "," << bestscore
		      << ")" << endl;)
	  } else if (accept(scorediff,temp)) {
	      accepttrans = true;
	      RDEBUG(cout << "accept: metropolis (" << newscore << ","
		      << bestscore << "," << expf(scorediff/(temp*sensitivity))
		      << ")" << endl;)
	  }
#else
          if ((violated == bestviolated) && (newscore < bestscore)) {
	      accepttrans = true;
	      RDEBUG(cout << "accept: better (" << newscore << "," << bestscore
		      << ")" << endl;)
	  } else if (violated < bestviolated) {
	      accepttrans = true;
	      RDEBUG(cout << "accept: better (violations) (" << newscore << ","
		      << bestscore << "," << violated << "," << bestviolated
		      << ")" << endl;
	          cout << "Violations: (new) " << violated << endl;
		  cout << "  unassigned: " << vinfo.unassigned << endl;
		  cout << "  pnode_load: " << vinfo.pnode_load << endl;
		  cout << "  no_connect: " << vinfo.no_connection << endl;
		  cout << "  link_users: " << vinfo.link_users << endl;
		  cout << "  bandwidth:  " << vinfo.bandwidth << endl;
		  cout << "  desires:    " << vinfo.desires << endl;
		  cout << "  vclass:     " << vinfo.vclass << endl;
		  cout << "  delay:      " << vinfo.delay << endl;)
	  } else if (accept(scorediff,temp)) {
	      accepttrans = true;
	      RDEBUG(cout << "accept: metropolis (" << newscore << ","
		      << bestscore << "," << expf(scorediff/(temp*sensitivity))
		      << ")" << endl;)
	  }
#endif

      if (accepttrans) {
	bestscore = newscore;
	bestviolated = violated;
#ifdef GNUPLOT_OUTPUT
	fprintf(tempout,"%f\n",temp);
	fprintf(scoresout,"%f\n",newscore);
	fprintf(deltaout,"%f\n",-scorediff);
#endif
	avgscore += newscore;

	accepts++;

#ifdef CHILL
	 if (!melting) {
	     scores[accepts] = newscore;
	 }
#endif

#ifdef NO_VIOLATIONS
	if (newscore < absbest) {
#else
	if ((violated < absbestviolated) ||
	    ((violated == absbestviolated) &&
	     (newscore < absbest))) {
#endif
#ifdef SCORE_DEBUG
	  cerr << "New best solution." << endl;
#endif
	  tie(vit,veit) = vertices(VG);
	  for (;vit!=veit;++vit) {
	    absassignment[*vit] = get(vvertex_pmap,*vit)->assignment;
	    absassigned[*vit] = get(vvertex_pmap,*vit)->assigned;
	    abstypes[*vit] = get(vvertex_pmap,*vit)->type;
	  }
	  absbest = newscore;
	  absbestviolated = violated;
	  iters_to_best = iters;
#ifdef SCORE_DEBUG
	  cerr << "New best recorded" << endl;
#endif
	}
	if (newscore < optimal) {
	  cout << "OPTIMAL ( " << optimal << ")" << endl;
	  goto DONE;
	}
	// Accept change
      } else {
	// Reject change
	RDEBUG(cout << "removing: rejected change" << endl;)
	remove_node(vv);
	if (oldassigned) {
	  if (vn->type.compare("lan") == 0) {
	    oldpos = make_lan_node(vv);
	  }
	  add_node(vv,oldpos,false);
	}
      }

      if (melting) {
	temp = avgincrease /
	  log(nincreases/ (nincreases * X0 - ndecreases * (1 - X0)));
	if (!(temp > 0.0)) {
	    temp = 0.0;
	}
      }
#ifdef TIME_TERMINATE
      if (timelimit && ((used_time() - timestart) > timelimit)) {
	printf("Reached end of run time, finishing\n");
	forcerevert = true;
	finished = true;
	goto NOTQUITEDONE;
      }
#endif

    }

#ifdef RANDOM_ASSIGNMENT
      if (violated == 0) {
	  finished = true;
      }
#endif

#ifdef REALLY_RANDOM_ASSIGNMENT
      if (unassigned_nodes.size() == 0) {
	  finished = true;
      }
#endif

NOTQUITEDONE:
      RDEBUG(printf("avgscore: %f = %f / %i\n",avgscore / (accepts +1),avgscore,accepts+1);)
      avgscore = avgscore / (accepts +1);

    if (melting) {
      melting = false;
      initialavg = avgscore;
      meltedtemp = temp;
      RDEBUG(cout << "Melting finished with a temperature of " << temp
	<< " avg score was " << initialavg << endl;)
      if (!(meltedtemp > 0.0)) { // This backwards expression to catch NaNs
	cout << "Finished annealing while melting!" << endl;
	finished = true;
	forcerevert = true;
      }
#ifdef TIME_TARGET
      if (timetarget) {
	double melttime = used_time() - meltstart;
	double timeleft = timetarget - melttime;
	double stepsleft = timeleft / melttime;
	cout << "Melting took " << melttime << " seconds, will try for "
	  << stepsleft << " temperature steps" << endl;
	temp_rate = pow(temp_stop/temp,1/stepsleft);
	cout << "Timelimit: " << timelimit << " Timeleft: " << timeleft
	  << " temp_rate: " << temp_rate << endl;
      }
#endif
    } else {
#ifdef CHILL
      if (!melting) {
	  stddev = 0;
	  for (int i = 0; i <= accepts; i++) {
	    stddev += pow(scores[i] - avgscore,2);
	  }
	  stddev /= (accepts +1);
	  stddev = sqrt(stddev);
	  temp = temp / (1 + (temp * log(1 + delta))/(3  * stddev));
      }
#else
      temp *= temp_rate;
#endif
    }


#ifdef DEBUG_TSTEP
#ifdef EPSILON_TERMINATE
#ifdef CHILL
    RDEBUG(printf("temp_end: %f %f %f\n",temp,temp * avgscore / initialavg,stddev);)
#else
    RDEBUG(printf("temp_end: %f %f\n",temp,temp * avgscore / initialavg);)
#endif
#else
    printf("temp_end: %f ",temp);
    if (trans >= mintrans) {
	if (accepts >= naccepts) {
	    printf("both");
	} else {
	    printf("trans %f",accepts*1.0/naccepts);
	}
    } else {
	printf("accepts %f",trans*1.0/mintrans);
    }
    printf("\n");
#endif
#endif
    
    // Revert to best found so far - do link/lan migration as well
#ifdef SCORE_DEBUG
    cerr << "Reverting to best known solution." << endl;
#endif


    // Add this to the history, and computed a smoothed average
    smoothedavg = avgscore / (nhist + 1);
    for (int j = 0; j < nhist; j++) {
      smoothedavg += avghist[(hstart + j) % MAX_AVG_HIST] / (nhist + 1);
    }

    avghist[(hstart + nhist) % MAX_AVG_HIST] = avgscore;
    if (nhist < MAX_AVG_HIST) {
      nhist++;
    } else {
      hstart = (hstart +1) % MAX_AVG_HIST;
    }

#ifdef LOCAL_DERIVATIVE
    deltaavg = lastsmoothed - smoothedavg;
    deltatemp = lasttemp - temp;
#else
    deltaavg = initialavg - smoothedavg;
    deltatemp = meltedtemp - temp;
#endif

    lastsmoothed = smoothedavg;
    lasttemp = temp;

#ifdef EPSILON_TERMINATE
    RDEBUG(
       printf("avgs: real: %f, smoothed %f, initial: %f\n",avgscore,smoothedavg,initialavg);
       printf("epsilon: (%f) %f / %f * %f / %f < %f (%f)\n", fabs(deltaavg), temp, initialavg,
	   deltaavg, deltatemp, epsilon,(temp / initialavg) * (deltaavg/ deltatemp));
    )
    if ((tsteps >= mintsteps) &&
#ifdef ALLOW_NEGATIVE_DELTA
	((fabs(deltaavg) < 0.0000001)
	 || (fabs((temp / initialavg) * (deltaavg/ deltatemp)) < epsilon))) {
#else
	(deltaavg > 0) && ((temp / initialavg) * (deltaavg/ deltatemp) < epsilon)) {
#endif
#ifdef FINISH_HILLCLIMB
        if (!finishedonce && ((absbestviolated <= violated) && (absbest < bestscore))) {
	    // We don't actually stop, we just go do a hill-climb (basically) at the best
	    // one we previously found
	    finishedonce = true;
	    printf("Epsilon Terminated, but going back to a better solution\n");
	} else {
	    finished = true;
	}
#else
	finished = true;
#endif
	forcerevert = true;
    }
#endif

    bool revert = false;
    if (forcerevert) {
	cout << "Reverting: forced" << endl;
	revert = true;
    }

#ifndef NO_REVERT
    if (REVERT_VIOLATIONS && (absbestviolated < violated)) {
	cout << "Reverting: REVERT_VIOLATIONS" << endl;
	revert = true;
    }
    if (absbest < bestscore) {
	cout << "Reverting: best score" << endl;
	revert = true;
    }
#endif

    if (REVERT_LAST && (temp < temp_stop)) {
	cout << "Reverting: REVERT_LAST" << endl;
	revert = true;
    }

    // Only revert if the best configuration has better violations
    vvertex_list lan_nodes;
    vvertex_iterator vvertex_it,end_vvertex_it;
    if (!revert) {
      // Just find LAN nodes, for migration
      tie(vvertex_it,end_vvertex_it) = vertices(VG);
      for (;vvertex_it!=end_vvertex_it;++vvertex_it) {
	tb_vnode *vnode = get(vvertex_pmap,*vvertex_it);
	if (vnode->assigned) {
	  if (vnode->type.compare("lan") == 0) {
	    lan_nodes.push_front(*vvertex_it);
	  }
	}
      } 
    } else {
      cout << "Reverting to best solution\n";
      // Do a full revert
      tie(vvertex_it,end_vvertex_it) = vertices(VG);
      for (;vvertex_it!=end_vvertex_it;++vvertex_it) {
	tb_vnode *vnode = get(vvertex_pmap,*vvertex_it);
	if (vnode->fixed) continue;
	if (vnode->assigned) {
	  RDEBUG(cout << "removing: revert " << vnode->name << endl;)
	  remove_node(*vvertex_it);
	} else {
	  RDEBUG(cout << "not removing: revert " << vnode->name << endl;)
	}
      }
      tie(vvertex_it,end_vvertex_it) = vertices(VG);
      for (;vvertex_it!=end_vvertex_it;++vvertex_it) {
	tb_vnode *vnode = get(vvertex_pmap,*vvertex_it);
	if (vnode->fixed) continue;
	if (absassigned[*vvertex_it]) {
	  if (vnode->type.compare("lan") == 0) {
	    lan_nodes.push_front(*vvertex_it);
	  } else {
	    if (vnode->vclass != NULL) {
	      vnode->type = abstypes[*vvertex_it];
	    }
	    assert(!add_node(*vvertex_it,absassignment[*vvertex_it],true));
	  }
	}
      }
    }

    // Do LAN migration
    RDEBUG(cout << "Doing LAN migration" << endl;)
    while (lan_nodes.size() > 0) {
      vvertex lanv = lan_nodes.front();
      lan_nodes.pop_front();
      if (!revert) { // If reverting, we've already done this
	  RDEBUG(cout << "removing: migration" << endl;)
	  remove_node(lanv);
      }
      pvertex lanpv = make_lan_node(lanv);
      add_node(lanv,lanpv,true);
    }

    tsteps++;

    if (finished) {
      goto DONE;
    }
  }
 DONE:
  cout << "Done" << endl;
}

void print_solution()
{
  vvertex_iterator vit,veit;
  tb_vnode *vn;
  
  cout << "Nodes:" << endl;
  tie(vit,veit) = vertices(VG);
  for (;vit != veit;++vit) {
    vn = get(vvertex_pmap,*vit);
    if (! vn->assigned) {
      cout << "unassigned: " << vn->name << endl;
    } else {
      cout << vn->name << " "
	   << get(pvertex_pmap,vn->assignment)->name << endl;
    }
  }
  cout << "End Nodes" << endl;
  cout << "Edges:" << endl;
  vedge_iterator eit,eendit;
  tie(eit,eendit) = edges(VG);
  for (;eit!=eendit;++eit) {
    tb_vlink *vlink = get(vedge_pmap,*eit);
    cout << vlink->name;
    if (vlink->link_info.type == tb_link_info::LINK_DIRECT) {
      tb_plink *p = get(pedge_pmap,vlink->link_info.plinks.front());
      cout << " direct " << p->name << " (" <<
	p->srcmac << "," << p->dstmac << ")" << endl;
    } else if (vlink->link_info.type == tb_link_info::LINK_INTRASWITCH) {
      tb_plink *p = get(pedge_pmap,vlink->link_info.plinks.front());
      tb_plink *p2 = get(pedge_pmap,vlink->link_info.plinks.back());
      cout << " intraswitch " << p->name << " (" <<
	p->srcmac << "," << p->dstmac << ") " <<
	p2->name << " (" << p2->srcmac << "," << p2->dstmac <<
	")" << endl;
    } else if (vlink->link_info.type == tb_link_info::LINK_INTERSWITCH) {
      cout << " interswitch ";
      for (pedge_path::iterator it=vlink->link_info.plinks.begin();
	   it != vlink->link_info.plinks.end();++it) {
	tb_plink *p = get(pedge_pmap,*it);
	cout << " " << p->name << " (" << p->srcmac << "," <<
	  p->dstmac << ")";
      }
      cout << endl;
    } else if (vlink->link_info.type == tb_link_info::LINK_TRIVIAL) {
      cout << " trivial" << endl;
    } else {
      cout << " Mapping Failed" << endl;
    }
  }
  cout << "End Edges" << endl;
  cout << "End solution" << endl;
}

struct pvertex_writer {
  void operator()(ostream &out,const pvertex &p) const {
    tb_pnode *pnode = get(pvertex_pmap,p);
    out << "[label=\"" << pnode->name << "\"";
    crope style;
    if (pnode->types.find("switch") != pnode->types.end()) {
      out << " style=dashed";
    } else if (pnode->types.find("lan") != pnode->types.end()) {
      out << " style=invis";
    }
    out << "]";
  }
};

struct vvertex_writer {
  void operator()(ostream &out,const vvertex &v) const {
    tb_vnode *vnode = get(vvertex_pmap,v);
    out << "[label=\"" << vnode->name << " ";
    if (vnode->vclass == NULL) {
      out << vnode->type;
    } else {
      out << vnode->vclass->name;
    }
    out << "\"";
    if (vnode->fixed) {
      out << " style=dashed";
    }
    out << "]";
  }
};

struct pedge_writer {
  void operator()(ostream &out,const pedge &p) const {
    out << "[";
    tb_plink *plink = get(pedge_pmap,p);
#ifdef VIZ_LINK_LABELS
    out << "label=\"" << plink->name << " ";
    out << plink->delay_info.bandwidth << "/" <<
      plink->delay_info.delay << "/" << plink->delay_info.loss << "\"";
#endif
    if (plink->type == tb_plink::PLINK_INTERSWITCH) {
      out << " style=dashed";
    }
    tb_pnode *src = get(pvertex_pmap,source(p,PG));
    tb_pnode *dst = get(pvertex_pmap,target(p,PG));
    if ((src->types.find("lan") != src->types.end()) ||
	(dst->types.find("lan") != dst->types.end())) {
      out << " style=invis";
    }
    out << "]";
  }
};

struct sedge_writer {
  void operator()(ostream &out,const sedge &s) const {
    tb_slink *slink = get(sedge_pmap,s);
    pedge_writer pwriter;
    pwriter(out,slink->mate);
  }
};
struct svertex_writer {
  void operator()(ostream &out,const svertex &s) const {
    tb_switch *snode = get(svertex_pmap,s);
    pvertex_writer pwriter;
    pwriter(out,snode->mate);
  }
};

struct vedge_writer {
  void operator()(ostream &out,const vedge &v) const {
    tb_vlink *vlink = get(vedge_pmap,v);
    out << "[";
#ifdef VIZ_LINK_LABELS
    out << "label=\"" << vlink->name << " ";
    out << vlink->delay_info.bandwidth << "/" <<
      vlink->delay_info.delay << "/" << vlink->delay_info.loss << "\"";
#endif
    if (vlink->emulated) {
      out << "style=dashed";
    }
    out <<"]";
  }
};

struct graph_writer {
  void operator()(ostream &out) const {
    out << "graph [size=\"1000,1000\" overlap=\"false\" sep=0.1]" << endl;
  }
};

struct solution_edge_writer {
  void operator()(ostream &out,const vedge &v) const {
    tb_link_info &linfo = get(vedge_pmap,v)->link_info;
    out << "[";
    crope style;
    crope color;
    crope label;
    switch (linfo.type) {
    case tb_link_info::LINK_UNKNOWN: style="dotted";color="red"; break;
    case tb_link_info::LINK_DIRECT: style="dashed";color="black"; break;
    case tb_link_info::LINK_INTRASWITCH:
      style="solid";color="black";
      label=get(pvertex_pmap,linfo.switches.front())->name;
      break;
    case tb_link_info::LINK_INTERSWITCH:
      style="solid";color="blue";
      label="";
      for (pvertex_list::const_iterator it=linfo.switches.begin();
	   it!=linfo.switches.end();++it) {
	label += get(pvertex_pmap,*it)->name;
	label += " ";
      }
      break;
    case tb_link_info::LINK_TRIVIAL: style="dashed";color="blue"; break;
    }
    out << "style=" << style << " color=" << color;
    if (label.size() != 0) {
      out << " label=\"" << label << "\"";
    }
    out << "]";
  }
};

struct solution_vertex_writer {
  void operator()(ostream &out,const vvertex &v) const {
    tb_vnode *vnode = get(vvertex_pmap,v);
    crope label=vnode->name;
    crope color;
    if (absassigned[v]) {
      label += " ";
      label += get(pvertex_pmap,absassignment[v])->name;
      color = "black";
    } else {
      color = "red";
    }
    crope style;
    if (vnode->fixed) {
      style="dashed";
    } else {
      style="solid";
    }
    out << "[label=\"" << label << "\" color=" << color <<
      " style=" << style << "]";
  }
};

void print_help()
{
  cerr << "assign [options] ptopfile topfile [config params]" << endl;
  cerr << "Options: " << endl;
#ifdef TIME_TERMINATE
  cerr << "  -l <time>   - Limit runtime." << endl;
#endif
  cerr << "  -s <seed>   - Set the seed." << endl;
  cerr << "  -v <viz>    - Produce graphviz files with given prefix." <<
    endl;
  cerr << "  -r          - Don't allow trivial links." << endl;
  cerr << "  -p          - Disable pclasses." << endl;
#ifdef PER_VNODE_TT
  cerr << "  -P          - Prune unusable pclasses." << endl;
#endif
  exit(2);
}

int main(int argc,char **argv)
{
  int seed = 0;
  crope viz_prefix;
  
  // Handle command line
  char ch;
  timelimit = 0.0;
  timetarget = 0.0;
  while ((ch = getopt(argc,argv,"s:v:l:t:rpP")) != -1) {
    switch (ch) {
    case 's':
      if (sscanf(optarg,"%d",&seed) != 1) {
	print_help();
      }
      break;
    case 'v':
      viz_prefix = optarg;
      break;
#ifdef TIME_TERMINATE
    case 'l':
      if (sscanf(optarg,"%lf",&timelimit) != 1) {
	print_help();
      }
      break;
#endif
#ifdef TIME_TARGET
    case 't':
      if (sscanf(optarg,"%lf",&timetarget) != 1) {
	print_help();
      }
      break;
#endif
    case 'r':
      allow_trivial_links = false; break;
    case 'p':
      use_pclasses = false; break;
#ifdef PER_VNODE_TT
    case 'P':
      prune_pclasses = true; break;
#endif
    default:
      print_help();
    }
  }
  argc -= optind;
  argv += optind;
  
  if (seed == 0) {
    if (getenv("ASSIGN_SEED") != NULL) {
      sscanf(getenv("ASSIGN_SEED"),"%d",&seed);
    } else {
      seed = time(NULL)+getpid();
    }
  }

  if (viz_prefix.size() == 0) {
    if (getenv("ASSIGN_GRAPHVIZ") != NULL) {
      viz_prefix = getenv("ASSIGN_GRAPHVIZ");
    }
  }

  if (argc == 0) {
    print_help();
  }

  // Convert options to the common.h parameters.
  parse_options(argv, options, noptions);
#ifdef SCORE_DEBUG
  dump_options("Configuration options:", options, noptions);
#endif

#ifdef GNUPLOT_OUTPUT
  scoresout = fopen("scores.out","w");
  tempout = fopen("temp.out","w");
  deltaout = fopen("delta.out","w");
#endif

  cout << "seed = " << seed << endl;
  std::srandom(seed);

  read_physical_topology(argv[0]);
  calculate_switch_MST();
  
  cout << "Generating physical equivalence classes:";
  generate_pclasses(PG);
  cout << pclasses.size() << endl;

#ifdef PCLASS_DEBUG
  pclass_debug();
#endif

  read_virtual_topology(argv[1]);
 
  cout << "Type preecheck:" << endl;
  // Type precheck
  bool ok=true;
  for (name_count_map::iterator vtype_it=vtypes.begin();
       vtype_it != vtypes.end();++vtype_it) {
    
    // Ignore LAN vnodes
    if (vtype_it->first == crope("lan")) {
	continue;
    }

    // Check to see if there were any pnodes of the type at all
    name_count_map::iterator ptype_it = ptypes.find(vtype_it->first);
    if (ptype_it == ptypes.end()) {
      cerr << "  *** No physical nodes of type " << vtype_it->first << " found"
        << endl;
      ok = false;
    } else {
      // Okay, there are some - are there enough?
      if (ptype_it->second < vtype_it->second) {
	cerr << "  *** " << vtype_it->second << " nodes of type " <<
	  vtype_it->first << " requested, but only " << ptype_it->second <<
	  " found" << endl;
	ok = false;
      }
    }
  }
  if (! ok) {
      cout << "Type preecheck failed!" << endl;
      exit(2);
  }

  cout << "Type preecheck passed." << endl;

#ifdef PER_VNODE_TT
  cout << "Node mapping precheck:" << endl;
  /*
   * Build up an entry in the type table for each vnode, by first looking at the
   * type table entry for the vnode's type, then checking each entry to make
   * sure that it:
   * (1) Has enough interfaces
   * (2) Has enough total bandwidth (for emulated links)
   * (3) Meets any 1.0-weight features and/or desires
   */
  vvertex_iterator vit,vendit;
  tie(vit,vendit) = vertices(VG);

  /*
   * Indicates whether all nodes have potential matches or not
   */
  ok = true;

  for (;vit != vendit;vit++) {
      tb_vnode *v = get(vvertex_pmap,*vit);
      //
      // No reason to do this work for LAN nodes!
      if (!v->type.compare("lan")) {
	  continue;
      }

      pclass_vector *vec = new pclass_vector();
      vnode_type_table[v->name] = tt_entry(0,vec);
      
      // This constitutes a list of the number of ptypes that matched the
      // criteria. We use to guess what's wrong with the vnode.
      int matched_links = 0;
      int matched_bw = 0;
      // Keep track of desires had how many 'hits', so that we can tell
      // if any simply were not matched
      tb_vnode::desires_count_map matched_desires;

      tb_vclass *vclass = v->vclass;
      tb_vclass::members_map::iterator mit;
      if (vclass) {
	  mit = vclass->members.begin();
      }
      for (;;) {
	  // Loop over all types this node can take on, which might be only
	  // one, if it's not part of a vclass
	  crope this_type;
	  if (vclass) {
	      this_type = mit->first;
	  } else {
	      this_type = v->type;
	  }

	  for (pclass_vector::iterator it = type_table[this_type].second->begin();
		  it != type_table[this_type].second->end(); it++) {

	      bool potential_match = true;
	      // Grab the first node of the pclass as a representative sample
	      tb_pnode *pnode = *((*it)->members[this_type]->L.begin());

	      // Check the number of interfaces
	      if (pnode->total_interfaces >= v->num_links) {
		  matched_links++;
	      } else {
		  potential_match = false;
	      }

	      // Check bandwidth on emulated links
	      if (pnode->total_bandwidth >= v->total_bandwidth) {
		  matched_bw++;
	      } else {
		  potential_match = false;
	      }

	      //
	      // Check features and desires
	      //

	      // Desires first
	      for (tb_vnode::desires_map::iterator desire_it = v->desires.begin();
		      desire_it != v->desires.end();
		      desire_it++) {
		  crope name = (*desire_it).first;
		  float value = (*desire_it).second;
		  // Only check for desires that would result in a violation if
		  // unsatisfied
		  if (value >= FD_VIOLATION_WEIGHT) {
		      if (matched_desires.find(name) == matched_desires.end()) {
			  matched_desires[name] = 0;
		      }
		      tb_pnode::features_map::iterator feature_it =
			  pnode->features.find(name);
		      if (feature_it != pnode->features.end()) {
			  matched_desires[name]++;
		      } else {
			  potential_match = false;
		      }
		  }
	      }

	      // Next, features
	      for (tb_pnode::features_map::iterator feature_it = pnode->features.begin();
		      feature_it != pnode->features.end();
		      feature_it++) {
		  crope name = (*feature_it).first;
		  float value = (*feature_it).second;
		  // Only check for feature that would result in a violation if
		  // undesired
		  if (value >= FD_VIOLATION_WEIGHT) {
		      tb_vnode::desires_map::iterator desire_it =
			  v->desires.find(name);
		      if (desire_it == v->desires.end()) {
			  potential_match = false;
		      }
		  }
	      }


	      if (potential_match) {
		  vec->push_back(*it);
		  vnode_type_table[v->name].first++;
		  (*it)->refcount++;
#ifdef PCLASS_DEBUG
		  cerr << v->name << " can map to " << (*it)->name << endl;
#endif
	      }
	  }

	  if (vclass) { 
	      mit++;
	      if (mit == vclass->members.end()) {
		  break;
	      }
	  } else {
	      // If not a vtype, we only had to do this once
	      break;
	  }
      }

      if (vnode_type_table[v->name].first == 0) {
	  cerr << "  *** No possible mapping for " << v->name << endl;
	  // Make an attempt to figure out why it didn't match
	  if (!matched_links) {
	      cerr << "      Too many links!" << endl;
	  }

	  if (!matched_bw) {
	      cerr << "      Too much bandwidth on emulated links!" << endl;
	  }

	  for (tb_vnode::desires_count_map::iterator dit = matched_desires.begin();
		  dit != matched_desires.end();
		  dit++) {
	      if (dit->second == 0) {
		  cerr << "      No physical nodes have feature " << dit->first
		      << "!" << endl;
	      }
	  }
	  ok = false;
      }
#ifdef PCLASS_DEBUG
      cerr << v->name << " can map to " << vnode_type_table[v->name].first << " pclasses"
	  << endl;
#endif

  }

  if (!ok) {
      cout << "Node mapping precheck failed!" << endl;
      exit(2);
  }
  cout << "Node mapping precheck succeeded" << endl;

  if (prune_pclasses) {
      prune_unusable_pclasses();
  }

#endif

  // Output graphviz if necessary
  if (viz_prefix.size() != 0) {
    crope vviz = viz_prefix + "_virtual.viz";
    crope pviz = viz_prefix + "_physical.viz";
    crope sviz = viz_prefix + "_switch.viz";
    ofstream vfile,pfile,sfile;
    vfile.open(vviz.c_str());
    write_graphviz(vfile,VG,vvertex_writer(),vedge_writer(),graph_writer());
    vfile.close();
    pfile.open(pviz.c_str());
    write_graphviz(pfile,PG,pvertex_writer(),pedge_writer(),graph_writer());
    pfile.close();
    sfile.open(sviz.c_str());
    write_graphviz(sfile,SG,svertex_writer(),sedge_writer(),graph_writer());
    sfile.close();
  }
 
  timestart = used_time();
  anneal();
  timeend = used_time();
#ifdef GNUPLOT_OUTPUT
  fclose(scoresout);
  fclose(tempout);
  fclose(deltaout);
#endif

  if ((score > absbest) || (violated > absbestviolated)) {
    cerr << "Internal error: Invalid migration assumptions." << endl;
    cerr << "score:" << score << " absbest:" << absbest <<
      " violated:" << violated << " absbestviolated:" <<
      absbestviolated << endl;
    cerr << "  Contact calfeld" << endl;
  }
  
  cout << "   BEST SCORE:  " << score << " in " << iters <<
    " iters and " << timeend-timestart << " seconds" << endl;
  cout << "With " << violated << " violations" << endl;
  cout << "Iters to find best score:  " << iters_to_best << endl;
  cout << "Violations: " << violated << endl;
  cout << "  unassigned: " << vinfo.unassigned << endl;
  cout << "  pnode_load: " << vinfo.pnode_load << endl;
  cout << "  no_connect: " << vinfo.no_connection << endl;
  cout << "  link_users: " << vinfo.link_users << endl;
  cout << "  bandwidth:  " << vinfo.bandwidth << endl;
  cout << "  desires:    " << vinfo.desires << endl;
  cout << "  vclass:     " << vinfo.vclass << endl;
  cout << "  delay:      " << vinfo.delay << endl;
#ifdef FIX_PLINK_ENDPOINTS
  cout << "  endpoints:  " << vinfo.incorrect_endpoints << endl;
#endif

  print_solution();

  if (viz_prefix.size() != 0) {
    crope aviz = viz_prefix + "_solution.viz";
    ofstream afile;
    afile.open(aviz.c_str());
    write_graphviz(afile,VG,solution_vertex_writer(),
		   solution_edge_writer(),graph_writer());
    afile.close();
  }
  
  if (violated > 0) {
      return 1;
  } else {
      return 0;
  }
}

