/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "anneal.h"

#include "virtual.h"
#include "maps.h"
#include "common.h"
#include "score.h"
#include "solution.h"
#include "vclass.h"
#include "neighborhood.h"

/*
 * Internal variables
 */
// These variables store the best solution.
//node_map absassignment;		// assignment field of vnode
//assigned_map absassigned;	// assigned field of vnode
//type_map abstypes;		// type field of vnode
solution best_solution;

// Map of virtual node name to its vertex descriptor.
name_vvertex_map vname2vertex;

// This is a vector of all the nodes in the top file.  It's used
// to randomly choose nodes.
vvertex_vector virtual_nodes;

// Map of physical node name to its vertex descriptor.
name_pvertex_map pname2vertex;
  
// Map of virtual node name to the physical node name it's fixed to.
// The domain is the set of all fixed virtual nodes and the range is
// the set of all fixed physical nodes.
name_name_map fixed_nodes;

// Map of virtual node name to the physical node name that we should
// start the virtual node on. However, unlike fixed nodes, assign is
// allowed to move these.
name_name_map node_hints;

// From assign.cc
#ifdef GNUPLOT_OUTPUT
extern FILE *scoresout, *tempout, *deltaout;
#endif

// Determines whether to accept a change of score difference 'change' at
// temperature 'temperature'.
inline bool accept(double change, double temperature) {
  double p;
  int r;

  if (change == 0) {
    p = 1000 * temperature / temp_prob;
  } else {
    p = expf(change/temperature) * 1000;
  }
  r = RANDOM() % 1000;
  if (r < p) {
    return 1;
  }
  return 0;
}


#ifdef SMART_UNMAP
/*
 * XXX - I pulled this code out of the anneal loop, and it needs to be fixed
 * up (and get some arguments and a return type) before it will compile
 */
void smart_unmap() {
	// XXX: Should probably randomize this
	// XXX: Add support for not using PER_VNODE_TT
	// XXX: Not very robust

	freednode = true;

	tt_entry tt = vnode_type_table[vn->name];
	int size = tt.first;
	pclass_vector *acceptable_types = tt.second;
	// Find a node to kick out
	bool foundnode = false;
	int offi = RANDOM();
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
	  tb_pclass::tb_pnodeset::iterator it =
	    (*acceptable_types)[index]->used_members[vn->type]->begin();
	  int j = RANDOM() %
	    (*acceptable_types)[index]->used_members[vn->type]->size();
	  while (j > 0) {
	    it++;
	    j--;
	  }
	  tb_vnode_set::iterator it2 = (*it)->assigned_nodes.begin();
	  int k = RANDOM() % (*it)->assigned_nodes.size();
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
		RANDOM()));
	} else {
	  cerr << "Failed to find a replacement!" << endl;
	}
#endif /* SMART_UNMAP */

#ifdef SMART_UNMAP
/*
 * Part II of the smart_unmap code - again, needs to be fixed before it
 * will compile.
 */
void smart_unmap_part2() {
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
	}
#endif	
	
// We put the temperature outside the function so that external stuff, like
// status_report in assign.cc, can see it.
double temp;

/* When this is finished the state will reflect the best solution found. */
void anneal(bool scoring_selftest, double scale_neighborhood,
	double *initial_temperature, double use_connected_pnode_find)
{
  cout << "Annealing." << endl;

  double newscore = 0;
  double bestscore = 0;
 
  // The number of iterations that took place.
  iters = 0;
  iters_to_best = 0;
  int accepts = 0;
  
  double scorediff;

  int nnodes = num_vertices(VG);
  int npnodes = num_vertices(PG);
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
  temp = init_temp;
  double deltatemp, deltaavg;

  // Priority queue of unassigned virtual nodes.  Basically a fancy way
  // of randomly choosing a unassigned virtual node.  When nodes become
  // unassigned they are placed in the queue with a random priority.
  vvertex_int_priority_queue unassigned_nodes;

#ifdef VERBOSE
  cout << "Initialized to cycles="<<cycles<<" optimal="<<optimal<<" mintrans="
       << mintrans<<" naccepts="<<naccepts<< endl;
#endif

  /* Set up the initial counts */
  init_score();

  /* Set up fixed nodes */
  /* Count of nodes which could not be fixed - we wait until we've tried to fix
   * all nodes before bailing, so that the user gets to see all of the
   * messages.
   */
  int fix_failed = 0;
  for (name_name_map::iterator fixed_it=fixed_nodes.begin();
       fixed_it!=fixed_nodes.end();++fixed_it) {
    if (vname2vertex.find((*fixed_it).first) == vname2vertex.end()) {
      cout << "*** Fixed virtual node: " << (*fixed_it).first <<
	" does not exist." << endl;
      fix_failed++;
      continue;
    }
    vvertex vv = vname2vertex[(*fixed_it).first];
    if (pname2vertex.find((*fixed_it).second) == pname2vertex.end()) {
      cout << "*** Fixed physical node: " << (*fixed_it).second <<
	" not available." << endl;
      fix_failed++;
      continue;
    }
    pvertex pv = pname2vertex[(*fixed_it).second];
    tb_vnode *vn = get(vvertex_pmap,vv);
    tb_pnode *pn = get(pvertex_pmap,pv);
    if (vn->vclass != NULL) {
      // Find a type on this physical node that can satisfy something in the
      // virtual class
      if (pn->typed) {
        if (vn->vclass->has_type(pn->current_type)) {
          vn->type = pn->current_type;
        }
      } else {
        for (tb_pnode::types_list::iterator i = pn->type_list.begin();
            i != pn->type_list.end(); i++) {
          // For now, if we find more than one match, we pick the first. It's
          // possible that picking some other type would give us a better
          // score, but let's noty worry about that
          if (vn->vclass->has_type((*i)->ptype->name())) {
            vn->type = (*i)->ptype->name();
            break;
          }
        }
      }
      if (vn->type.empty()) {
        // This is an internal error, so it's okay to handle it in a different
        // way from the others
        cout << "*** Unable to find a type for fixed, vtyped, node " << vn->name
          << endl;
        exit(EXIT_FATAL);
      } else {
        cout << "Setting type of vclass node " << vn->name << " to "
          << vn->type << "\n";
      }
    }
    if (add_node(vv,pv,false,true,false) == 1) {
      cout << "*** Fixed node: Could not map " << vn->name <<
	" to " << pn->name << endl;
      fix_failed++;
      continue;
    }
    vn->fixed = true;
    /*
    if (vn->vclass != NULL) {
      vn->type = vn->vclass->choose_type();
      cout << "Picked type " << vn->type << " for " << vn->name << endl;
    }
    */
    num_fixed++;
  }

  if (fix_failed){
    cout << "*** Some fixed nodes failed to map" << endl;
    exit(EXIT_UNRETRYABLE);
  }

  // Subtract the number of fixed nodes from nnodes, since they don't really
  // count
  if (num_fixed) {
      cout << "Adjusting dificulty estimate for fixed nodes, " <<
	  (nnodes - num_fixed) << " remain.\n";
  }

  /* We'll check against this later to make sure that whe we've unmapped
   * everything, the score is the same */
  double initial_score = get_score();

  /*
   * Handle node hints - we do this _after_ we've figured out the initial
   * score, since, unlike fixed nodes, hints get unmapped before we do the
   * final mapping. Also, we ignore any hints for vnodes which have already
   * been assigned - they must have been fixed, and that over-rides the hint.
   */
  for (name_name_map::iterator hint_it=node_hints.begin();
       hint_it!=node_hints.end();++hint_it) {
    if (vname2vertex.find((*hint_it).first) == vname2vertex.end()) {
      cout << "Warning: Hinted node: " << (*hint_it).first <<
	"does not exist." << endl;
      continue;
    }
    vvertex vv = vname2vertex[(*hint_it).first];
    if (pname2vertex.find((*hint_it).second) == pname2vertex.end()) {
      cout << "Warning: Hinted node: " << (*hint_it).second <<
	" not available." << endl;
      continue;
    }
    pvertex pv = pname2vertex[(*hint_it).second];
    tb_vnode *vn = get(vvertex_pmap,vv);
    tb_pnode *pn = get(pvertex_pmap,pv);
    if (vn->assigned) {
      cout << "Warning: Skipping hint for node " << vn->name << ", which is "
	<< "fixed in place" << endl;
      continue;
    }
    if (add_node(vv,pv,false,false,false) == 1) {
      cout << "Warning: Hinted node: Could not map " << vn->name <<
	" to " << pn->name << endl;
      continue;
    }
  }
       
  /*
   * Find out the starting temperature
   */
  bestscore = get_score();
  bestviolated = violated;

#ifdef VERBOSE
  cout << "Problem started with score "<<bestscore<<" and "<< violated
       << " violations." << endl;
#endif

  absbest = bestscore;
  absbestviolated = bestviolated;

  /*
   * Make a list of all nodes that are still unassigned
   */
  vvertex_iterator vit,veit;
  tie(vit,veit) = vertices(VG);
  for (;vit!=veit;++vit) {
    tb_vnode *vn = get(vvertex_pmap,*vit);
    if (vn->assigned) {
	// XXX
//      absassignment[*vit] = vn->assignment;
//      abstypes[*vit] = vn->type;
	best_solution.set_assignment(*vit,vn->assignment);
	best_solution.set_vtype_assignment(*vit,vn->type);
    } else {
	best_solution.clear_assignment(*vit);
	unassigned_nodes.push(vvertex_int_pair(*vit,RANDOM()));
    }
  }
  
  /*
   * Set any links that have been assigned
   */
  vedge_iterator eit, eeit;
  tie(eit, eeit) = edges(VG);
  for (;eit!=eeit;++eit) {
      tb_vlink *vlink = get(vedge_pmap, *eit);
      if (vlink->link_info.type_used != tb_link_info::LINK_UNMAPPED) {
	  best_solution.set_link_assignment(*eit,vlink->link_info);
      }
  }

  /*
   * The neighborhood size is the number of solutions we can reach with one
   * transition operation - it's roughly the number of virtual nodes times the
   * number of pclasses. This is how long we usually stick with a given 
   * temperature.
   */
  int neighborsize;
  neighborsize = (nnodes - num_fixed) * npclasses;
  if (neighborsize < min_neighborhood_size) {
    neighborsize = min_neighborhood_size;
  }

  // Allow scaling of the neighborhood size, so we can make assign try harder
  // (or less hard)
  neighborsize = (int)(neighborsize * scale_neighborhood);

#ifdef CHILL
  double scores[neighborsize];
#endif

  if (num_fixed >= nnodes) {
    cout << "All nodes are fixed.  No annealing." << endl;
    goto DONE;
  }
  
  vvertex vv;
  tb_vnode *vn;

  // Crap added by ricci
#ifdef MELT
  bool melting;
  double meltstart;
#endif
  int nincreases, ndecreases;
  double avgincrease;
  double avgscore;
  double initialavg;
  double stddev;
  bool finished; 
  bool forcerevert; 
  // Lame, we have to do this on a seperate line, or the compiler gets mad about
  // the goto above crossing initialization. Well, okay, okay, I know the goto
  // itself is lame....
  finished = forcerevert = false;
  int tsteps;
  int mintsteps;

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

  /*
   * Initial temperature calcuation/melting
   */
#ifdef MELT
  if (initial_temperature == NULL) {
      melting = true;
  } else {
      melting = false;
      temp = *initial_temperature;
      cout << "Starting with initial temperature " << temp << endl;
  }
#ifdef TIME_TARGET
  meltstart = used_time();
#endif
#else
  melting = false;
#endif

  /*
   * Calculate the number of iterations to stay at this temperature step
   */
  melt_trans = neighborsize;
  
  /*
   * The main annealing loop!
   * Each iteration is a temperature step - how we get out of the loop depends
   * on what the termination condition is. Normally, we have a target temperature
   * at which we stop, but with EPSILON_TERMINATE, we watch the derivative of the
   * average temperature, and break out of the loop when it gets small enough.
   */
#ifdef EPSILON_TERMINATE
  while(1) {
#else
  while (temp >= temp_stop) {
#endif
      
#ifdef VERBOSE
    cout << "Temperature:  " << temp << " AbsBest: " << absbest <<
      " (" << absbestviolated << ")" << endl;
#endif
    
    /*
     * Initialize this temperature step
     */
    trans = 0;
    accepts = 0;
    nincreases = ndecreases = 0;
    avgincrease = 0.0;
    avgscore = bestscore;
#ifdef CHILL
    scores[0] = bestscore;
#endif

    // Adjust the number of transitions we're going to do based on the number
    // of pclasses that are actually 'in play'
    int transitions = (int)(neighborsize *
      (count_enabled_pclasses() *1.0 / pclasses.size()));
    assert(transitions <= neighborsize);

    if (melting) {
      cout << "Doing melting run" << endl;
    }

    /*
     * The inner loop - 
     * Each iteration of this inner loop corresponds to one attempt to try a new
     * solution. When we're melting, we have a special number of transitions
     * we're shooting for.
     */
    while ((melting && (trans < melt_trans))
#ifdef NEIGHBOR_LENGTH
	    || (trans < transitions)) {
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

      /*
       * Find a virtual node to map -
       * If there are any virtual nodes that are not yet mapped, start with
       *   those
       * If not, find some other random vnode, which we'll unmap then remap
       */
      if (! unassigned_nodes.empty()) {
	vv = unassigned_nodes.top().first;
	assert(!get(vvertex_pmap,vv)->assigned);
	unassigned_nodes.pop();
      } else {
	int start = RANDOM()%nnodes;
	int choice = start;
	while (get(vvertex_pmap,virtual_nodes[choice])->fixed) {
	  choice = (choice +1) % nnodes;
	  if (choice == start) {
	      choice = -1;
	      break;
	  }
	}
	if (choice >= 0) {
	    vv = virtual_nodes[choice];
	} else {
	    cout << "**** Error, unable to find any non-fixed nodes" << endl;
	    goto DONE;
	}
      }      
      vn = get(vvertex_pmap,vv);
      RDEBUG(cout << "Reassigning " << vn->name << endl;)
	  
      /*
       * Keep track of the old assignment for this node
       */
      oldassigned = vn->assigned;
      oldpos = vn->assignment;
      
      /*
       * Problem: If we free the chosen vnode now, we might just try remapping
       * it to the same pnode. If FREE_IMMEDIATELY is not set, we do the 
       * later, after we've chosen a pnode unmapping
       */
#ifdef FREE_IMMEDIATELY
      if (oldassigned) {
	remove_node(vv);
	RDEBUG(cout << "Freeing up " << vn->name << endl;)
      }
#endif
      
      /*
       * We have to handle vnodes with vtypes (vclasses) specially - we have
       * to make the vtype pick a type to masquerade as for now.
       */
      if (vn->vclass != NULL) {
	vn->type = vn->vclass->choose_type();
#ifdef SCORE_DEBUG
	cerr << "vclass " << vn->vclass->name  << ": choose type for " <<
	    vn->name << " = " << vn->type << " dominant = " <<
	    vn->vclass->dominant << endl;
#endif
      }
      
      // Did we free a node?
      bool freednode = false;
      
      /* 
       * Find a pnode to map this vnode to
       */
      tb_pnode *newpnode = NULL;
      if ((use_connected_pnode_find != 0)
	  && ((RANDOM() % 1000) < (use_connected_pnode_find * 1000))) {
	newpnode = find_pnode_connected(vv,vn);
      }
      
      /* 
       * If not using the connected find, or it failed to find a node, then
       * fall back on the regular algorithm to find a pnode
       */
      if (newpnode == NULL) {
	newpnode = find_pnode(vn);
      }
      
      /*
       * If we didn't free the vnode up above, do it now
       */
#ifndef FREE_IMMEDIATELY
      if (oldassigned) {
	RDEBUG(cout << "removing: !lan, oldassigned" << endl;)
	remove_node(vv);
      }
#endif
      
      /*
       * If we didn't find a node to map this vnode to, free up some other
       * vnode so that we can make progress - otherwise, we could get stuck
       */
      if (newpnode == NULL) {
#ifndef SMART_UNMAP
	// Push this node back onto the unassigned map
	unassigned_nodes.push(vvertex_int_pair(vv,RANDOM()));
	int start = RANDOM()%nnodes;
	int toremove = start;
	while (get(vvertex_pmap,virtual_nodes[toremove])->fixed ||
	       (! get(vvertex_pmap,virtual_nodes[toremove])->assigned)) {
	    toremove = (toremove +1) % nnodes;
	  if (toremove == start) {
	    toremove = -1;
	    break;
	  }	
      }	
      if (toremove >= 0) {
	RDEBUG(cout << "removing: freeing up nodes" << endl;)
	remove_node(virtual_nodes[toremove]);
	unassigned_nodes.push(vvertex_int_pair(virtual_nodes[toremove], RANDOM()));
      }	
      
      /*
       * Start again with another vnode - which will probably be the same one,
       * since we just marked it as unmapped. But now, there will be at least one
       * free pnode
       */
      continue;	
#else /* SMART_UNMAP */
      // XXX: This code is broken for now, which is okay, because we weren't
      // using it
      smart_unmap();
      smart_unmap_part2();
#endif
      }
    
      /*
       * Okay, we've got pnode to map this vnode to - let's do it
       */
      if (newpnode != NULL) {	
        newpos = pnode2vertex[newpnode];
        if (scoring_selftest) {
	  // Run a little test here - see if the score we get by adding	
	  // this node, then removing it, is the same one we had before
	  double oldscore = get_score();
	  int oldviolated = violated;
	  double tempscore;
	  int tempviolated;
	  if (!add_node(vv,newpos,false,false,false)) {
	    tempscore = get_score();
	    tempviolated = violated;
	    remove_node(vv);
	  }	
	  if ((oldscore != get_score()) || (oldviolated != violated)) {
	    cerr << "Scoring problem adding a mapping - oldscore was " <<
		oldscore <<  " newscore is " << newscore << " tempscore was "
		<< tempscore << endl;
	    cerr << "oldviolated was " << oldviolated << " newviolated is "
		<< violated << " tempviolated was " << tempviolated << endl;
	    cerr << "I was tring to map " << vn->name << " to " <<
		newpnode->name << endl;
	    print_solution(best_solution);
	    cerr << vinfo;
	    abort();
          }
        }
      
        /*
         * Actually try the new mapping - if it fails, the node is still
         * unassigned, and we go back and try with another
         */
        if (add_node(vv,newpos,false,false,false) != 0) {
	  unassigned_nodes.push(vvertex_int_pair(vv,RANDOM()));
	  continue;
        }
      } else { // pnode != NULL
#ifdef SMART_UNMAP
        unassigned_nodes.push(vvertex_int_pair(vv,RANDOM()));
#endif
        if (freednode) {
	  continue;
        }	
      }

      /*
       * Okay, now that we've mapped some new node, let's check the scoring
       */
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
   
      /*
       * Here are all the various conditions for deciding if we're going to accept
       * this transition
       */
      bool accepttrans = false;
      if (newscore < optimal) {
        // If this score is smaller than the one we think is optimal, of course we
        // take it!
        accepttrans = true;
	RDEBUG(cout << "accept: optimal (" << newscore << "," << optimal
	       << ")" << endl;)
      } else if (melting) {
        // When melting, we take everything!
	accepttrans = true;
	RDEBUG(cout << "accept: melting" << endl;)
      } else {
#ifdef NO_VIOLATIONS
        // Here, we don't consider violations at all, just whether the regular
        // simulated annealing accept conditions
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
#ifdef SPECIAL_VIOLATION_TREATMENT
        /*
         * In this ifdef, we always accept new solutions that have fewer
         * violations than the old solution, and when we're trying to
         * determine whether or not to accept a new solution with a higher
         * score, we don't take violations into the account.
         *
         * The problem with this shows up at low temperatures. What can often
         * happen is that we accept a solution with worse violations but a
         * better (or similar) score. Then, if we were to try, say the first
         * solution (or a score-equivalent one) again, we'd accept it again.
         *
         * What this leads to is 'thrashing', where we have a whole lot of
         * variation of scores over time, but are not making any real
         * progress. This prevents the cooling schedule from converging for
         * much, much longer than it should really take.
         */
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
	    cout << vinfo;)
        } else if (accept(scorediff,temp)) {
	  accepttrans = true;
	  RDEBUG(cout << "accept: metropolis (" << newscore << ","
		 << bestscore << "," << expf(scorediff/(temp*sensitivity))
		 << ")" << endl;)
        }
#else // no SPECIAL_VIOLATION_TREATMENT
        /*
         * In this branch of the ifdef, we give violations no special
         * treatment when it comes to accepting new solution - we just add
         * them into the score. This makes assign behave in a more 'classic'
         * simulated annealing manner.
         *
         * One consequence, though, is that we have to be more careful with
         * scores. We do not want to be able to get into a situation where
         * adding a violation results in a _lower_ score than a solution with
         * fewer violations.
         */
        double adjusted_new_score = newscore + violated * VIOLATION_SCORE;
        double adjusted_old_score = bestscore + bestviolated * VIOLATION_SCORE;

        if (adjusted_new_score < adjusted_old_score) {
          accepttrans = true;
        } else if (accept(adjusted_old_score - adjusted_new_score,temp)) {
	  accepttrans = true;
        }

#endif // SPECIAL_VIOLATION_TREATMENT

      }
#endif // NO_VIOLATIONS

      /* 
       * Okay, we've decided to accep this transition - do some bookkeeping
       */
      if (accepttrans) {
	bestscore = newscore;
	bestviolated = violated;

#ifdef GNUPLOT_OUTPUT
	fprintf(tempout,"%f\n",temp);
	fprintf(scoresout,"%f\n",newscore);
	fprintf(deltaout,"%f\n",-scorediff);
#endif // GNUPLOT_OUTPUT

	avgscore += newscore;
	accepts++;

#ifdef CHILL
	 if (!melting) {
	     scores[accepts] = newscore;
	 }
#endif // CHILL

        /*
         * Okay, if this is the best score we've gotten so far, let's do some
	 * further bookkeeping - copy it into the structures for our best solution
	 */
#ifdef NO_VIOLATIONS
	if (newscore < absbest) {
#else // NO_VIOLATIONS
	if ((violated < absbestviolated) ||
	    ((violated == absbestviolated) &&
	     (newscore < absbest))) {
#endif // NO_VIOLATIONS
	    
#ifdef SCORE_DEBUG
	  cerr << "New best solution." << endl;
#endif // SCORE_DEBUG
	  tie(vit,veit) = vertices(VG);
	  for (;vit!=veit;++vit) {
	      tb_vnode *vn = get(vvertex_pmap,*vit);
	      if (vn->assigned) {
		  best_solution.set_assignment(*vit,vn->assignment);
		  best_solution.set_vtype_assignment(*vit,vn->type);
	      } else {
		  best_solution.clear_assignment(*vit);
	      }
	    //absassignment[*vit] = get(vvertex_pmap,*vit)->assignment;
	    //absassigned[*vit] = get(vvertex_pmap,*vit)->assigned;
	    //abstypes[*vit] = get(vvertex_pmap,*vit)->type;
	  }
	  
	  vedge_iterator eit, eeit;
	  tie(eit, eeit) = edges(VG);
	  for (;eit!=eeit;++eit) {
	      tb_vlink *vlink = get(vedge_pmap, *eit);
	      if (vlink->link_info.type_used != tb_link_info::LINK_UNMAPPED) {
		  best_solution.set_link_assignment(*eit,vlink->link_info);
	      } else {
		  best_solution.clear_link_assignment(*eit);
	      }
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
      } else { // !acceptrans
	// Reject change, go back to the state we were in before
	RDEBUG(cout << "removing: rejected change" << endl;)
	remove_node(vv);
	if (oldassigned) {
	  add_node(vv,oldpos,false,false,false);
	}
      }

      /*
       * If we're melting, we do a little extra bookkeeping to do, becuase the
       * goal of melting is to come up with an initial temperature such that
       * almost every transition will be accepted
       */
      if (melting) {
	temp = avgincrease /
	  log(nincreases/ (nincreases * X0 - ndecreases * (1 - X0)));
	if (!(temp > 0.0)) {
	    temp = 0.0;
	}
      }
      
      /*
       * With TIME_TERMINATE, we just give up after our time limit
       */
#ifdef TIME_TERMINATE
      if (timelimit && ((used_time() - timestart) > timelimit)) {
	printf("Reached end of run time, finishing\n");
	forcerevert = true;
	finished = true;
	goto NOTQUITEDONE;
      }
#endif

    } /* End of inner annealing loop */
     

NOTQUITEDONE:
    RDEBUG(printf("avgscore: %f = %f / %i\n",avgscore / (accepts +1),avgscore,accepts+1);)
	
    /*
     * Most of the code past this point concerns itself with the cooling
     * schedule (what the next temperature step should be
     */
	
    // Keep an average of the score over this temperature step	
    avgscore = avgscore / (accepts +1);

    /*
     * If we were melting, then we we need to pick an initial temperature
     */
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
      /*
       * With TIME_TARGET, we look at how long melting took, then use that to
       * estimate how many temperature steps it will take to hit our time
       * target. We adjust our cooling schedule accordingly.
       */
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
      /*
       * The CHILL cooling schedule is the standard one from the Simulated
       * Annealing literature - it lower the temperature based on the standard
       * deviation of the scores of accepted configurations
       */
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
      /* 
       * This is assign's original cooling schedule - more predictable, but not
       * at all reactive to the problem at hand
       */
      temp *= temp_rate;
#endif
    }


    /*
     * Debugging
     */
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
    
    RDEBUG(
    printf("temp_end: temp: %f ratio: %f stddev: %f\n",temp,temp * avgscore / initialavg,stddev);
    );

    /*
     * The next section of code deals with termination conditions - how do we
     * decide that we're done?
     */
    
    /*
     * Keep a history of the average scores over the last MAX_AVG_HIST
     * temperature steps. We treat the avghist array like a ring buffer.
     * Add this temperature step to the history, and computer a smoothed
     * average.
     */
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

    /*
     * Are we computing the derivative of the average temperatures over the
     * whole history, or just the most recent one?
     */
#ifdef LOCAL_DERIVATIVE
    deltaavg = lastsmoothed - smoothedavg;
    deltatemp = lasttemp - temp;
#else
    deltaavg = initialavg - smoothedavg;
    deltatemp = meltedtemp - temp;
#endif

    lastsmoothed = smoothedavg;
    lasttemp = temp;

    /*
     * EPSILON_TERMINATE means that we define some small number, epsilon, and
     * the derivative of the average change in temperature gets below that
     * epsilon (ie. we have stopped getting improvements in score), we're done
     */
#ifdef EPSILON_TERMINATE
    RDEBUG(
       printf("avgs: real: %f, smoothed %f, initial: %f\n",avgscore,smoothedavg,initialavg);
       printf("epsilon: (%f) %f / %f * %f / %f < %f (%f)\n", fabs(deltaavg), temp, initialavg,
	   deltaavg, deltatemp, epsilon,(temp / initialavg) * (deltaavg/ deltatemp));
    );
    if ((tsteps >= mintsteps) &&
    /*
     * ALLOW_NEGATIVE_DELTA controls whether we're willing to stop if the
     * derivative gets small and negative, not just small and positive.
     */
#ifdef ALLOW_NEGATIVE_DELTA
	((temp < 0) || isnan(temp) ||
//	 || (fabs((temp / initialavg) * (deltaavg/ deltatemp)) < epsilon))) {
	 ((temp / initialavg) * (deltaavg/ deltatemp)) < epsilon)) {
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
    
    /*
     * RANDOM_ASSIGNMENT is not really very random, but we stop after the first
     * valid solution we get
     */
#ifdef RANDOM_ASSIGNMENT
    if (violated == 0) {
       finished = true;
    }
#endif

    /*
     * REALLY_RANDOM_ASSIGNMENT stops after we've assigned all nodes, whether or
     * not our solution is valid
     */
#ifdef REALLY_RANDOM_ASSIGNMENT
    if (unassigned_nodes.size() == 0) {
      finished = true;
    }
#endif

    /*
     * The following section deals with reverting. This is not standard
     * Simulated Annealing at all. In assign, a revert means that we go back
     * to some previous solution (usually a better one). There are lots of
     * things that could trigger this, so we use a bool to check if any of
     * them happened.
     */
    bool revert = false;
    
    /*
     * Some of the termination condidtions force a revert when they decide
     * they're finished. This is fine - of course, we want to return the best
     * solution we ever found, which might not be the one we're sitting at right
     * now.
     */
    if (forcerevert) {
      cout << "Reverting: forced" << endl;
      revert = true;
    }
    if (REVERT_LAST && (temp < temp_stop)) {
       cout << "Reverting: REVERT_LAST" << endl;
       revert = true;
    }

    
    /*
     * Okay, NO_REVERT is not the best possible name for this ifdef. 
     * Historically, assign used to revert to the best solution at the end of
     * every temperature step. This is definitely NOT kosher. In my mind, it
     * assign too susceptible to falling into local minima. Anyhow, the idea is
     * that we go back to the best soltion if the current solution is worse than
     * it either in violations or in score.
     */
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

    /*
     * This is the code to do the actual revert.
     * IMPORTANT: At this time, a revert does not take you back to _exactly_ the
     * same state as before, because there are some things, like link
     * assignments, that we don't save. Since the way these get mapped is
     * dependant on the order they happen in, and this order is almost certainly
     * different than the order they got mapped during annealing, there can be
     * discrepancies (ie. now we have violations, when before we had none.)
     */
    vvertex_iterator vvertex_it,end_vvertex_it;
    vedge_iterator vedge_it,end_vedge_it;
    if (revert) {
      cout << "Reverting to best solution\n";
      /*
       * We start out by unmapping every vnode that's currently allocated
       */
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

      // Check to make sure that our 'clean' solution scores the same as
      // the initial score - if not, that indicates a bug
      if (!compare_scores(get_score(),initial_score)) {
	  cout << "*** WARNING: 'Clean' score does not match initial score" <<
	      endl << "     This indicates a bug - contact the operators" <<
	      endl << "     (initial score: " << initial_score <<
	      ", current score: " << get_score() << ")" << endl;
	  // One source of this can be pclasses that are still used - check for
	  // those
	  pclass_list::iterator pit = pclasses.begin();
	  for (;pit != pclasses.end();pit++) {
	      if ((*pit)->used_members != 0) {
		  cout << (*pit)->name << " is " << (*pit)->used_members
		      << "% used" << endl;
	      }
	  }
      }
      
      /* 
       * Now, go through the previous best solution, and add all of the node
       * mappings back in.
       */
      tie(vvertex_it,end_vvertex_it) = vertices(VG);
      for (;vvertex_it!=end_vvertex_it;++vvertex_it) {
	tb_vnode *vnode = get(vvertex_pmap,*vvertex_it);
	if (vnode->fixed) continue;
	if (best_solution.is_assigned(*vvertex_it)) {
	  if (vnode->vclass != NULL) {
	    vnode->type = best_solution.get_vtype_assignment(*vvertex_it);
	  }
	  assert(!add_node(*vvertex_it,best_solution.get_assignment(*vvertex_it),true,false,true));
	}
      }
      
      /*
       * Add back in the old link resolutions
       */
      tie(vedge_it,end_vedge_it) = edges(VG);
      for (;vedge_it != end_vedge_it; ++vedge_it) {
	  tb_vlink *vlink = get(vedge_pmap,*vedge_it);
          tb_vnode *src_vnode = get(vvertex_pmap,vlink->src);
          tb_vnode *dst_vnode = get(vvertex_pmap,vlink->dst);
	  if (best_solution.link_is_assigned(*vedge_it)) {
	      // XXX: It's crappy that I have to do all this work here - something
	      // needs re-organzing
	      /*
	       * This line does the actual link mapping revert
		*/
	      vlink->link_info = best_solution.get_link_assignment(*vedge_it);
	      
	      if (!dst_vnode->assigned || !src_vnode->assigned) {
		  // This shouldn't happen, but don't try to score links which
		  // don't have both endpoints assigned.
		  continue;
	      }
              if (dst_vnode->fixed && src_vnode->fixed) {
                  // If both endpoints were fixed, this link never got
                  // unmapped, so don't map it again
                  continue;
              }
	      tb_pnode *src_pnode = get(pvertex_pmap,src_vnode->assignment);
	      tb_pnode *dst_pnode = get(pvertex_pmap,dst_vnode->assignment);
	      
	      /*
	       * Okay, now that we've jumped through enough hoops, we can actually
	       * do the scoring
	       */
	      score_link_info(*vedge_it, src_pnode, dst_pnode, src_vnode, dst_vnode);
	  } else {
              /*
               * If one endpoint or the other was unmapped, we just note that
               * the link wasn't mapped - however, if both endpoints were
               * mapped, then we have to make sure the score reflects that.
               */
	      if (!dst_vnode->assigned || !src_vnode->assigned) {
                  vlink->link_info.type_used = tb_link_info::LINK_UNMAPPED;
              } else {
                  mark_vlink_unassigned(vlink);
              }
	  }
      }
    } // End of reverting code

    /*
     * Whew, that's it!
     */
    tsteps++;

    if (finished) {
      goto DONE;
    }
  } /* End of outer annealing loop */
DONE:
  cout << "Done" << endl;
} // End of anneal()
	    
