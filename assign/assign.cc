/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "port.h"

#include <boost/config.hpp>
#include <boost/utility.hpp>
#include <boost/property_map.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#ifdef GRAPHVIZ_SUPPORT
#include <boost/graph/graphviz.hpp>
#endif

#include <fstream>
#include <iostream>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>
#include <sys/signal.h>
#include <queue>
#include <algorithm>

using namespace boost;

#include <stdio.h>

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
#include "config.h"

#ifdef WITH_XML
#include "parse_ptop_xml.h"
#include "parse_vtop_xml.h"
#include "parse_advertisement_rspec.h"
#include "parse_request_rspec.h"
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

// List of virtual types by name.
name_count_map vtypes;
name_list_map vclasses;

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

// An amount to scale the neighborhood size by
double scale_neighborhood = 1.0;

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

// Whether or not we should use the experimental support for dynamic pclasses
bool dynamic_pclasses = false;

// Whether or not to allow assign to temporarily over-subscribe pnodes
bool allow_overload = false;

// Forces assign to do greedy link assignment, by chosing the first link in its
// list, which is usually the lowest-cost
bool greedy_link_assignment = false;

// Forces assign to skip melting, and, instead, use the temperature given as
// the initial temperature
bool no_melting = false;
double initial_temperature = 0.0f;

// Print out a summary of the solution in addition to the solution itself
bool print_summary = false;

// Use the 'connected' find algorithm
double use_connected_pnode_find = 0.0f;

// Whether or not to perform all checks on fixed nodes
bool check_fixed_nodes = false;

// Use XML for file input
// bool xml_input = false;
#ifdef WITH_XML
bool ptop_xml_input = false;
bool vtop_xml_input = false;
bool ptop_rspec_input = false;
bool vtop_rspec_input = false;
#endif

// XXX - shouldn't be in this file
double absbest;
int absbestviolated, iters, iters_to_best;

// Map of all physical types in use in the system
tb_ptype_map ptypes;

/*
 * Internal functions
 */

// Return the CPU time (in seconds) used by this process
float used_time() {
  struct rusage ru;
  getrusage(RUSAGE_SELF,&ru);
  return ru.ru_utime.tv_sec+ru.ru_utime.tv_usec/1000000.0+
    ru.ru_stime.tv_sec+ru.ru_stime.tv_usec/1000000.0;
}

// Constructs the output file name for the annotated rspec
string annotated_filename (const char* filepath)
{
	string input_filepath = string(filepath);
	int pos_last_backslash = input_filepath.find_last_of("/");
	string input_filename = input_filepath.substr(pos_last_backslash+1, input_filepath.length()-pos_last_backslash-1);
	string output_filepath = input_filepath.substr(0, pos_last_backslash+1);
	output_filepath.append("annotated-");
	output_filepath.append(input_filename);
	return (output_filepath);
}

// Read in the .ptop file
void read_physical_topology(char *filename) {
  ifstream ptopfile;
  ptopfile.open(filename);
  if (!ptopfile.is_open()) {
      cout << "*** Unable to open ptop file " << filename << endl;
      exit(EXIT_FATAL);
  }
  
#ifdef WITH_XML
  if (ptop_xml_input) {
		cout << "Physical Graph: " << parse_ptop_xml(PG,SG,filename) << endl;
  } else if (ptop_rspec_input) {
	  cout << "Physical Graph: " << parse_ptop_rspec (PG, SG, filename) << endl;
	}
	else {
      cout << "Physical Graph: " << parse_ptop(PG,SG,ptopfile) << endl;
  }
#else
	cout << "Physical Graph: " << parse_ptop(PG,SG,ptopfile) << endl;
#endif

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
// XXX: Should soon be replaced by calculate_shortest_routes()
void calculate_switch_MST() {
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
void read_virtual_topology(char *filename) {
  ifstream topfile;
  topfile.open(filename);
  if (!topfile.is_open()) {
      cout << "*** Unable to open top file " << filename << endl;
      exit(EXIT_FATAL);
  }
  
#ifdef WITH_XML  
  if (vtop_xml_input) {
	cout << "Virtual Graph: " << parse_vtop_xml(VG,filename) << endl;
  }
  else if (vtop_rspec_input){
  	  cout << "Virtual Graph: " << parse_vtop_rspec (VG, filename) << endl ;
  }
  else {
      cout << "Virtual Graph: " << parse_top(VG,topfile) << endl;
  }
#else
	cout << "Virtual Graph: " << parse_top(VG,topfile) << endl;
#endif

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
            /*
             * Remove the nodes in the pclass we're removing from the slot
             * counts for their ptypes
             */
            tb_pclass::pclass_members_map::iterator ptype_iterator;
            ptype_iterator = (*pclass_iterator)->members.begin();
            while (ptype_iterator != (*pclass_iterator)->members.end()) {
                /*
                 * Find the recort for this type in the ptypes structure
                 */
                fstring this_type = ptype_iterator->first;
                tb_ptype_map::iterator ptype = ptypes.find(this_type);
                assert(ptype != ptypes.end());
                tb_ptype *this_type_p = ptype->second;

                /*
                 * For every node with this type, we want to remove its slot
                 * count from the total slot count for the type, since we know
                 * we will never use this particular node.
                 * Note: We only have to do this for pclasses that are "real",
                 * not dynamic ones.
                 */
                if (!(*pclass_iterator)->is_dynamic) {
                    tb_pnodelist::list_iter pnode_iterator =
                        ptype_iterator->second->L.begin();
                    while (pnode_iterator != ptype_iterator->second->L.end()) {
                        /*
                         * Get the slotcount for this ptype
                         */
                        tb_pnode::types_map::iterator tm_iterator;
                        tm_iterator = (*pnode_iterator)->types.find(this_type);
                        assert(tm_iterator != (*pnode_iterator)->types.end());

                        /*
                         * Remove it from the current ptype
                         */
                        this_type_p->remove_slots(
                                tm_iterator->second->get_max_load());

                        /*
                         * Move on to the next node
                         */
                        pnode_iterator++;
                    }
                }
                ptype_iterator++;
            }

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

void print_help() {
  cout << "assign [options] ptopfile topfile [config params]" << endl;
  cout << "Options: " << endl;
#ifdef TIME_TERMINATE
  cout << "  -l <time>   - Limit runtime." << endl;
#endif
  cout << "  -s <seed>   - Set the seed." << endl;
#ifdef GRAPHVIZ_SUPPORT
  cout << "  -v <viz>    - Produce graphviz files with given prefix." <<
    endl;
#endif
  cout << "  -r          - Don't allow trivial links." << endl;
  cout << "  -p          - Disable pclasses." << endl;
  cout << "  -d          - Enable dynamic pclasses." << endl;
#ifdef PER_VNODE_TT
  cout << "  -P          - Prune unusable pclasses." << endl;
#endif
  cout << "  -T          - Doing some scoring self-testing." << endl;
  cout << "  -H <float>  - Try <float> times harder." << endl;
  cout << "  -o          - Allow overloaded pnodes to be considered." << endl;
  cout << "  -t <float>  - Start the temperature at <float> instead of melting."
      << endl;
  cout << "  -u          - Print a summary of the solution." << endl;
  cout << "  -c <float>  - Use the 'connected' pnode finding algorithm " <<
      "<float>*100% of the time." << endl;
  cout << "  -n          - Don't anneal - just do the prechecks." << endl;

  cout << "  -x <file>   - Specify a text ptop file" << endl;
#ifdef WITH_XML
  cout << "  -X <file>   - Specify a XML ptop file" << endl;
#endif
  cout << "  -y <file>   - Specify a text top file" << endl;
#ifdef WITH_XML
  cout << "  -Y <file>   - Specify a XML vtop file" << endl;
#endif
#ifdef WITH_XML
  cout << "  -q <file>   - Specify a rspec ptop file" << endl;
  cout << "  -w <file>   - Specify a rspec vtop file" << endl;
#endif
  cout << "  -F          - Apply additional checking to fixed noded" << endl;
  exit(EXIT_FATAL);
}
 
// Perfrom a pre-cehck to make sure that there are enough free nodes of the
// proper types. Returns 1 if the proper types exist, 0 if they do not.
// TODO - move away from using global variables
int type_precheck(int round) {
    cout << "Type precheck:" << endl;

    bool ok = true;

    /*
     * Tailor our error messages, depending on the round - the first round of
     * the precheck is looking for available pnodes, the second is looking for
     * sutiable nodes (ie. at least one vnode could map to it)
     */
    char *round_str;
    if (round == 1) {
        round_str = "available";
    } else {
        round_str = "suitable";
    }
    // First, check the regular types
    for (name_count_map::iterator vtype_it=vtypes.begin();
	    vtype_it != vtypes.end();++vtype_it) {

	// Check to see if there were any pnodes of the type at all
	tb_ptype_map::iterator ptype_it = ptypes.find(vtype_it->first);
	if (ptype_it == ptypes.end()) {
	    cout << "  *** No " << round_str << " physical nodes of type "
                << vtype_it->first << " found (" << vtype_it->second
                << " requested)" << endl;
	    ok = false;
	} else {
	    // Okay, there are some - are there enough?
	    if (ptype_it->second->pnode_slots() < vtype_it->second) {
		cout << "  *** " << vtype_it->second << " nodes of type " <<
                    vtype_it->first << " requested, but only " <<
                    ptype_it->second->pnode_slots() << " " << round_str <<
                    " nodes of type " << vtype_it->first<< " found" << endl;
		ok = false;
	    }
	    // Okay, there are enough - but are we allowed to use them?
	    if (ptype_it->second->maxusers() &&
		    (ptype_it->second->maxusers() < vtype_it->second)) {
		cout << "  *** " << vtype_it->second << " nodes of type " <<
		    vtype_it->first << " requested, but you are only " <<
		    "allowed to use " << ptype_it->second->maxusers() << endl;
		ok = false;
	    }
	}
    }

    // Check the vclasses, too
    for (name_list_map::iterator vclass_it = vclasses.begin();
	    vclass_it != vclasses.end(); ++vclass_it) {
	bool found_match = false;
        // Make sure we actually use this vclass
        name_vclass_map::iterator dit = vclass_map.find(vclass_it->first);
        if (dit == vclass_map.end()) {
            cout << "***: Internal error - unable to find vtype " <<
                vclass_it->first << endl;
            exit(EXIT_FATAL);
        } else {
            if (dit->second->empty()) {
                // Nobody uses it, don't check
                continue;
            }
        }

	for (vector<fstring>::iterator vtype_it = vclass_it->second.begin();
		vtype_it != vclass_it->second.end(); vtype_it++) {
	    tb_ptype_map::iterator mit = ptypes.find(*vtype_it);
	    if ((mit != ptypes.end()) && (mit->second->pnode_slots() != 0)) {
		found_match = true;
		break;
	    }
	}

	if (!found_match) {
	    cout << "  *** No " << round_str <<
                " physical nodes can satisfy vclass " << vclass_it->first << endl;
	    ok = false;
	}
    }

    if (ok) {
      cout << "Type precheck passed." << endl;
      return 1;
    } else {
      cout << "*** Type precheck failed!" << endl;
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

	pclass_vector *vec = new pclass_vector();
	vnode_type_table[v->name] = tt_entry(0,vec);

	// This constitutes a list of the number of ptypes that matched the
	// criteria. We use to guess what's wrong with the vnode.
	int matched_bw = 0;
	// Keep track of desires had how many 'hits', so that we can tell
	// if any simply were not matched
	map<fstring,int> matched_desires;

	// Keep track of which link types had how many 'hits', so that we can
	// tell which type(s) caused this node to fail
	tb_vnode::link_counts_map matched_link_counts;
        hash_map<fstring,int> max_links;
        hash_map<fstring,int> desired_links;
	map<fstring,bool> matched_links;

	tb_vclass *vclass = v->vclass;
	tb_vclass::members_map::const_iterator mit;
	if (vclass) {
	    mit = vclass->get_members().begin();
	}
	for (;;) {
	    // Loop over all types this node can take on, which might be only
	    // one, if it's not part of a vclass
	    fstring this_type;
	    if (vclass) {
		this_type = mit->first;
	    } else {
		this_type = v->type;
	    }

            // Check to make sure there are actually nodes of this type in the
            // physical topology
            if (type_table.find(this_type) == type_table.end()) {
                // Yes, I know, goto is evil. But I'm not gonna indent the next
                // 100 lines of code for this error case
                goto nosuchtype;
            }

	    for (pclass_vector::iterator it = type_table[this_type].second->begin();
		    it != type_table[this_type].second->end(); it++) {

		bool potential_match = true;
		// Grab the first node of the pclass as a representative sample
	 	tb_pnode *pnode = *((*it)->members[this_type]->L.begin());

		// Check to see if any of the link that this pnode has are of
		// the correct type for the virtual node

		// Check bandwidth on emulated links
		if (pnode->total_bandwidth >= v->total_bandwidth) {
		    matched_bw++;
		} else {
			potential_match = false;
		}

		// Check to see if if the pnode has enough slots of the
		// appropriate type available
		tb_pnode::types_map::iterator type_iterator =
		    pnode->types.find(v->type);
		if (type_iterator == pnode->types.end()) {
		    // Must have been a vtype to get here - ignore it
		} else {
		    if (v->typecount > type_iterator->second->get_max_load()) {
			// Nope, this vnode is too demanding
			potential_match = false;
		    }
		}

		/*
		 * Check features and desires
		*/
		tb_featuredesire_set_iterator
		    fdit(v->desires.begin(),v->desires.end(),
			 pnode->features.begin(),pnode->features.end());
		for (;!fdit.done();fdit++) {
		    // Skip 'local' and 'global' features
		    if (fdit->is_global() || fdit->is_local()) {
			continue;
		    }
		    // Only check for FDs that would result in a violation if
		    // unmatched.
		    if (fdit.either_violateable()) {
			if (fdit.membership() !=
				tb_featuredesire_set_iterator::BOTH) {
			    potential_match = false;
			}

			// We look for violateable desires on vnodes so that we
			// can report them to the user
			if ((fdit.membership() ==
				tb_featuredesire_set_iterator::FIRST_ONLY ||
			    fdit.membership() ==
				 tb_featuredesire_set_iterator::BOTH)
				&& fdit.first_iterator()->is_violateable()) {
			    if (matched_desires.find(fdit->name()) ==
				    matched_desires.end()) {
				matched_desires[fdit->name()] = 0;
			    }
			}
			if (fdit.membership() ==
				tb_featuredesire_set_iterator::BOTH) {
			    matched_desires[fdit->name()]++;
			}
		    }
		}
		
		// Check link types - right now, we treat LANs specially, which
                // I am not happy about, but it seems to be necessary.
                // Otherwise, we can get a false negative when there are few
                // ports on a switch available, but we could map by using
                // the trunk links
                if (this_type != "lan") {
                  tb_vnode::link_counts_map::iterator link_it;
                  for (link_it = v->link_counts.begin();
		       link_it != v->link_counts.end();
                       link_it++) {
                    fstring type = link_it->first;
                    int count = link_it->second;
                    desired_links[type] = count;
                    if (pnode->link_counts.find(type) !=
                          pnode->link_counts.end()) {
                      // Found at least one link of this type
                      matched_links[type] = true;
                      if (pnode->link_counts[type] > max_links[type]) {
                        max_links[type] = pnode->link_counts[type];
                      }
                      if (pnode->link_counts[type] >= count) {
                        // Great, there are enough, too
                        matched_link_counts[type]++;
                      } else {
                        potential_match = false;
                      }
                    } else {
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

nosuchtype:
	    if (vclass) { 
		mit++;
		if (mit == vclass->get_members().end()) {
		    break;
		}
	    } else {
		// If not a vtype, we only had to do this once
		break;
	    }
	}

	if (vnode_type_table[v->name].first == 0) {
	    cout << "  *** No possible mapping for " << v->name << endl;
	    // Make an attempt to figure out why it didn't match
	    
	    // Check all of its link types
	    tb_vnode::link_counts_map::iterator lit;
	    for (lit = v->link_counts.begin(); lit != v->link_counts.end();
		lit++) {
	      fstring type = lit->first;
	      if (!matched_links[type]) {
		cout << "      No links of type " << type << " found! (" <<
                  desired_links[type] << " requested)" << endl;
	      } else {
		if (!matched_link_counts[type]) {
		  cout << "      Too many links of type " << type << "! (" <<
                    desired_links[type] << " requested, " << max_links[type] <<
                    " found)" << endl;
		}
	      }
	    }

	    if (!matched_bw) {
		cout << "      Too much bandwidth on emulated links!" << endl;
	    }

	    for (map<fstring,int>::iterator dit = matched_desires.begin();
		    dit != matched_desires.end();
		    dit++) {
		if (dit->second == 0) {
		    cout << "      No physical nodes have feature " << dit->first
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
	cout << "*** Node mapping precheck failed!" << endl;
	return 0;
    }

#else // PER_VNODE_TT
    // PER_VNODE_TT is required for this check, just pretend it's OK.
    return 1;
#endif
}

// Perfrom a pre-cehck to make sure that polices that are checkable at precheck
// time are not violated. Returns 1 if everything is A-OK, 0 otherwise
// TODO - move away from using global variables
int policy_precheck() {
  cout << "Policy precheck:" << endl;
  if (tb_featuredesire::check_desire_policies()) {
    cout << "Policy precheck succeeded" << endl;
    return 1;
  } else {
    cout << "*** Policy precheck failed!" << endl;
    return 0;
  }
}  

// Signal handler - add a convneint way to kill assign and make it return an
// unretryable error
void exit_unretryable(int signal) {
  cout << "*** Killed with signal " << signal << " - exiting!" << endl;
  _exit(EXIT_UNRETRYABLE);
}

// Singal handler - add a way for a user to get some status information
extern double temp;
void status_report(int signal) {
  cout << "I: " << iters << " T: " << temp << " S: " << get_score() << " V: "
    << violated << " (Best S: " << absbest << " V:" << absbestviolated << ")"
    << endl;
  cout.flush();
}

// From anneal.cc - the best solution found
extern solution best_solution;

int main(int argc,char **argv) {
  int seed = 0;
#ifdef GRAPHVIZ_SUPPORT
  fstring viz_prefix;
#endif
  bool scoring_selftest = false;
  bool prechecks_only = false;
  
  // Handle command line
  char ch;
  timelimit = 0.0;
  timetarget = 0.0;
  
  char* ptopFilename = "";
  char* vtopFilename = "";
  
  while ((ch = getopt(argc,argv,"s:v:l:t:rpPTdH:oguc:nx:X:y:Y:q:w:F")) != -1) {
    switch (ch) {
    case 's':
      if (sscanf(optarg,"%d",&seed) != 1) {
		print_help();
      }
      break;
#ifdef GRAPHVIZ_SUPPORT
    case 'v':
      viz_prefix = optarg;
      break;
#endif
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
    case 'T':
      scoring_selftest = true; break;
    case 'd':
      dynamic_pclasses = true; break;
    case 'H':
      if (sscanf(optarg,"%lf",&scale_neighborhood) != 1) {
	print_help();
      }
      break;
    case 'o':
      allow_overload = true; break;
    case 'g':
      greedy_link_assignment = true; break;
    case 't':
      if (sscanf(optarg,"%lf",&initial_temperature) != 1) {
	print_help();
      }
      no_melting = true; break;
    case 'u':
      print_summary = true; break;
    case 'c':
      if (sscanf(optarg,"%lf",&use_connected_pnode_find) != 1) {
	print_help();
      }
      break;
    case 'n':
      prechecks_only = true;
      cout << "Doing only prechecks, exiting early" << endl;
      break;
    case 'x':
#ifdef WITH_XML
      ptop_xml_input = false;
#endif
      if (strcmp(optarg, "") == 0) {
      	print_help();
	  }
	  ptopFilename = optarg;
      break;
#ifdef WITH_XML      
    case 'X':
      ptop_xml_input = true;
      if (strcmp(optarg, "") == 0) {
      	print_help();
	  }
	  ptopFilename = optarg;
    break;
#endif
    case 'y':
#ifdef WITH_XML
      vtop_xml_input = false;
#endif
      if (strcmp(optarg, "") == 0) {
      	print_help();
	  }
	  vtopFilename = optarg;
    break;
#ifdef WITH_XML
    case 'Y':
      vtop_xml_input = true;
      if (strcmp(optarg, "") == 0) {
      	print_help();
	  }
	  vtopFilename = optarg;
    break;
#endif
#ifdef WITH_XML
	case 'q':
	  ptop_rspec_input = true;
	  if (strcmp(optarg, "") == 0) {
	  	print_help();
	  }
	  ptopFilename = optarg;
    break;

	case 'w':
	  vtop_rspec_input = true;
	  if (strcmp(optarg, "") == 0) {
	  	print_help();
	  }
	  vtopFilename = optarg;
    break;
#endif
        case 'F':
          check_fixed_nodes = true;
    break;

    default:
      print_help();
    }
  }
  argc -= optind;
  argv += optind;
  
  if (argc == 2)
  {
	  ptopFilename = argv[0];
	  vtopFilename = argv[1];
  }
  
  if (strcmp(ptopFilename, "") == 0)
	  print_help();
  	
  if (strcmp(vtopFilename, "") == 0)
	  print_help();	
  
  if (seed == 0) {
    if (getenv("ASSIGN_SEED") != NULL) {
      sscanf(getenv("ASSIGN_SEED"),"%d",&seed);
    } else {
      seed = time(NULL)+getpid();
    }
  }

#ifdef GRAPHVIZ_SUPPORT
  if (viz_prefix.size() == 0) {
    if (getenv("ASSIGN_GRAPHVIZ") != NULL) {
      viz_prefix = getenv("ASSIGN_GRAPHVIZ");
    }
  }
#endif

//   if (argc == 0) {
//       print_help();
//   }

  // Set up a signal handler for USR1 that exits with an unretryable error
  struct sigaction action;
  action.sa_handler = exit_unretryable;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  sigaction(SIGUSR1,&action,NULL);
  sigaction(SIGINT,&action,NULL);

  // Set up a signal handler for control+T
  struct sigaction action2;
  action2.sa_handler = status_report;
  sigemptyset(&action2.sa_mask);
  action2.sa_flags = 0;
#ifdef __FreeBSD__
  sigaction(SIGINFO,&action2,NULL);
#endif 
  
  // Convert options to the common.h parameters.
  //parse_options(argv, options, noptions);
#ifdef SCORE_DEBUG
  //dump_options("Configuration options:", options, noptions);
#endif

#ifdef GNUPLOT_OUTPUT
  scoresout = fopen("scores.out","w");
  tempout = fopen("temp.out","w");
  deltaout = fopen("delta.out","w");
#endif

  cout << "seed = " << seed << endl;
  srandom(seed);

  read_physical_topology(ptopFilename);
  calculate_switch_MST();

  read_virtual_topology(vtopFilename);

  // Time while we make pclasses and do the type prechecks
  timestart = used_time();
  
  cout << "Generating physical equivalence classes:";
  generate_pclasses(PG,disable_pclasses,dynamic_pclasses);
  cout << pclasses.size() << endl;

#ifdef PCLASS_DEBUG
  pclass_debug();
#endif

  /*
   * There is a reason for the ordering of the prechecks. The mapping precheck
   * basically assumes that the type precheck has already run, so it doesn't
   * have to worry about nodes which cannot map due to type.
   */
  
  // Run the type precheck
  if (!type_precheck(1)) {
      exit(EXIT_UNRETRYABLE);
  }

  // Run the mapping precheck
  if (!mapping_precheck()) {
      exit(EXIT_UNRETRYABLE);
  }

#ifdef PER_VNODE_TT
  if (prune_pclasses) {
      prune_unusable_pclasses();
      /*
       * Run the type precheck again, this time without the pclasses we just
       * pruned. Yes, it's a bit redundant, but for the reasons stated above,
       * we mave to run the type precheck before the mapping precheck. And,
       * the type precheck is very fast.
       */
      if (!type_precheck(2)) {
          exit(EXIT_UNRETRYABLE);
      }
  }
#endif
    
  // Run the policy precheck - the idea behind running this last is that some
  // policy violations might become more clear after doing pruning
    if (!policy_precheck()) {
	exit(EXIT_UNRETRYABLE);
    }
    
  // Bomb out early if we're only doing the prechecks
  if (prechecks_only) {
      exit(EXIT_SUCCESS);
  }

  // Output graphviz if necessary
#ifdef GRAPHVIZ_SUPPORT
  if (viz_prefix.size() != 0) {
    fstring vviz = viz_prefix + "_virtual.viz";
    fstring pviz = viz_prefix + "_physical.viz";
    fstring sviz = viz_prefix + "_switch.viz";
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
#endif

  // Handle the initial temperature, if one was given - a NULL initial temp.
  // means that we should start with the normal melting procedure
  double *initial_temperature_pointer;
  if (no_melting) {
    initial_temperature_pointer = &initial_temperature;
  } else {
    initial_temperature_pointer = NULL;
  }
 
  // Note, time is started earlier now, up by where we make pclasses
  anneal(scoring_selftest, check_fixed_nodes, scale_neighborhood,
          initial_temperature_pointer, use_connected_pnode_find);
  timeend = used_time();

#ifdef GNUPLOT_OUTPUT
  fclose(scoresout);
  fclose(tempout);
  fclose(deltaout);
#endif

  if ((!compare_scores(get_score(),absbest)) || (violated > absbestviolated)) {
    cout << "WARNING: Internal scoring inconsistency." << endl;
    cout << "score:" << get_score() << " absbest:" << absbest <<
      " violated:" << violated << " absbestviolated:" <<
      absbestviolated << endl;
  }
  
  cout << "   BEST SCORE:  " << get_score() << " in " << iters <<
    " iters and " << timeend-timestart << " seconds" << endl;
  cout << "With " << violated << " violations" << endl;
  cout << "Iters to find best score:  " << iters_to_best << endl;
  cout << "Violations: " << violated << endl;
  cout << vinfo;

#ifdef WITH_XML
  if (vtop_rspec_input || vtop_xml_input)
  {
	  print_solution(best_solution, annotated_filename(vtopFilename).c_str());
  }
  else
	  print_solution(best_solution);
#else
  print_solution(best_solution);
#endif

  if (print_summary) {
    print_solution_summary(best_solution);
  }

#ifdef GRAPHVIZ_SUPPORT
  if (viz_prefix.size() != 0) {
    fstring aviz = viz_prefix + "_solution.viz";
    ofstream afile;
    afile.open(aviz.c_str());
    write_graphviz(afile,VG,solution_vertex_writer(),
		   solution_edge_writer(),graph_writer());
    afile.close();
  }
#endif
  
  if (violated != 0) {
      exit(EXIT_RETRYABLE);
  } else {
      exit(EXIT_SUCCESS);
  }
}

