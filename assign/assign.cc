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
#include "solution.h"
#include "maps.h"
#include "anneal.h"

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

// A simple list of physical types.
name_count_map ptypes;

// List of virtual types by name.
name_count_map vtypes;

// A list of all pclasses.
pclass_list pclasses;
 	
// Map from a pnode* to the the corresponding pvertex.
pnode_pvertex_map pnode2vertex;

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
// Whether or not assign is allowed to generate trivial links
bool allow_trivial_links = true;

// Whether or not assign should use pclasses
bool disable_pclasses = false;

// Whether or not assign should prune out pclasses that it knows can
// never be used
bool prune_pclasses = false;
  
// XXX - shouldn't be in this file
double absbest;
int absbestviolated, iters, iters_to_best;

/*
 * Internal functions
 */

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
 
// Perfrom a pre-cehck to make sure that there are enough free nodes of the
// proper types. Returns 1 if the proper types exist, 0 if they do not.
// TODO - move away from using global variables
int type_precheck() {
    cout << "Type precheck:" << endl;

    bool ok = true;

    for (name_count_map::iterator vtype_it=vtypes.begin();
	    vtype_it != vtypes.end();++vtype_it) {
    
	// Ignore LAN vnodes
	if (vtype_it->first == crope("lan")) {
	    continue;
	}

	// Check to see if there were any pnodes of the type at all
	name_count_map::iterator ptype_it = ptypes.find(vtype_it->first);
	if (ptype_it == ptypes.end()) {
	    cerr << "  *** No physical nodes of type " << vtype_it->first
		<< " found" << endl;
	    ok = false;
	} else {
	    // Okay, there are some - are there enough?
	    if (ptype_it->second < vtype_it->second) {
		cerr << "  *** " << vtype_it->second << " nodes of type " <<
		    vtype_it->first << " requested, but only "
		    << ptype_it->second << " found" << endl;
		ok = false;
	    }
	}
    }
    if (ok) {
      cout << "Type preecheck passed." << endl;
      return 1;
    } else {
      cout << "Type precheck failed!" << endl;
      return 0;
    }
}

// Perfrom a pre-cehck to make sure that every vnode has at least one possible
// mapping.  Returns 1 if this is the case, 0 if not.
// TODO - move away from using global variables
int mapping_precheck() {
#ifdef PER_VNODE_TT
    cout << "Node mapping precheck:" << endl;
    /*
     * Build up an entry in the type table for each vnode, by first looking at
     * the type table entry for the vnode's type, then checking each entry to
     * make sure that it:
     * (1) Has enough interfaces
     * (2) Has enough total bandwidth (for emulated links)
     * (3) Meets any 1.0-weight features and/or desires
     */
    vvertex_iterator vit,vendit;
    tie(vit,vendit) = vertices(VG);

    /*
     * Indicates whether all nodes have potential matches or not
     */
    bool ok = true;

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
			feature_it != pnode->features.end(); feature_it++) {
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

    if (ok) {
	cout << "Node mapping precheck succeeded" << endl;
	return 1;
    } else {
	cout << "Node mapping precheck failed!" << endl;
	return 0;
    }

#else // PER_VNODE_TT
    // PER_VNODE_TT is required for this check, just pretend it's OK.
    return 1;
#endif
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
      disable_pclasses = true; break;
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
  generate_pclasses(PG,disable_pclasses);
  cout << pclasses.size() << endl;

#ifdef PCLASS_DEBUG
  pclass_debug();
#endif

  read_virtual_topology(argv[1]);

  // Run the type precheck
  if (!type_precheck()) {
      exit(2);
  }

  // Run the mapping precheck
  if (!mapping_precheck()) {
      exit(2);
  }

#ifdef PER_VNODE_TT
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

  if ((get_score() > absbest) || (violated > absbestviolated)) {
    cerr << "Internal error: Invalid migration assumptions." << endl;
    cerr << "score:" << get_score() << " absbest:" << absbest <<
      " violated:" << violated << " absbestviolated:" <<
      absbestviolated << endl;
    cerr << "  Contact calfeld" << endl;
  }
  
  cout << "   BEST SCORE:  " << get_score() << " in " << iters <<
    " iters and " << timeend-timestart << " seconds" << endl;
  cout << "With " << violated << " violations" << endl;
  cout << "Iters to find best score:  " << iters_to_best << endl;
  cout << "Violations: " << violated << endl;
  cout << vinfo;

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

