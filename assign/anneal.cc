/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "anneal.h"

/*
 * Internal variables
 */
// These variables store the best solution.
node_map absassignment;		// assignment field of vnode
assigned_map absassigned;	// assigned field of vnode
type_map abstypes;		// type field of vnode

// Map of virtual node name to its vertex descriptor.
name_vvertex_map vname2vertex;

// This is a vector of all the nodes in the top file.  It's used
// to randomly choose nodes.
vvertex_vector virtual_nodes;

// Map of physical node name to its vertex descriptor.
name_pvertex_map pname2vertex;
  
// Map of virtual node name to the physical node name it's fixed too.
// The domain is the set of all fixed virtual nodes and the range is
// the set of all fixed physical nodes.
name_name_map fixed_nodes;

// Determines whether to accept a change of score difference 'change' at
// temperature 'temperature'.
inline int accept(double change, double temperature)
{
  double p;
  int r;

  if (change == 0) {
    p = 1000 * temperature / temp_prob;
  } else {
    p = expf(change/temperature) * 1000;
  }
  r = std::random() % 1000;
  if (r < p) {
    return 1;
  }
  return 0;
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

  /*
  int enabled_pclasses = 0;
  for (int i = 0; i < num_types; i++) {
    if ((*acceptable_types)[i]->disabled) {
	continue;
    }
    enabled_pclasses++;
  }
  cout << "Looking for a pnode for " << vn->name << " - there are " <<
      enabled_pclasses << " to choose from (" << num_types << " total)" << endl;
  assert(num_types == enabled_pclasses);
  */
  
  int i = std::random()%num_types;
  int first = i;
  for (;;) {
    i = (i+1)%num_types;
#ifdef PCLASS_SIZE_BALANCE
    int acceptchance = 1000 * (*acceptable_types)[i]->size * 1.0 /
	npnodes;
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
    if ((*acceptable_types)[i]->disabled) {
	i = std::rand()%num_types;
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
		    ((*it)->current_type_record->current_load >=
		     (*it)->current_type_record->max_load))) {
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


/* When this is finished the state will reflect the best solution found. */
void anneal(bool scoring_selftest)
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
  double temp = init_temp;
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

  /* We'll check against this later to make sure that whe we've unmapped
   * everything, the score is the same */
  double initial_score = get_score();

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
	    if (scoring_selftest) {
	      // Run a little test here - see if the score we get by adding
	      // this node, then removing it, is the same one we would have
	      // gotten otherwise
	      double oldscore = get_score();
	      if (!add_node(vv,newpos,false)) {
		remove_node(vv);
	      }
	      assert(oldscore == get_score());
	    }
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
		  cout << vinfo;)
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
    if (revert) {
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

      // Check to make sure that our 'clean' solution scores the same as
      // the initial score - if not, that indicates a bug
      if (get_score() != initial_score) {
	  cerr << "*** WARNING: 'Clean' score does not match initial score" <<
	      endl << "     This indicates a bug - contact the operators" <<
	      endl << "     (initial score: " << initial_score <<
	      ", current score: " << get_score() << ")" << endl;
	  // One source of this can be pclasses that are still used - check for
	  // those
	  pclass_list::iterator pit = pclasses.begin();
	  for (;pit != pclasses.end();pit++) {
	      if ((*pit)->used_members != 0) {
		  cerr << (*pit)->name << " is " << (*pit)->used_members
		      << "% used" << endl;
	      }
	  }
      }
      tie(vvertex_it,end_vvertex_it) = vertices(VG);
      for (;vvertex_it!=end_vvertex_it;++vvertex_it) {
	tb_vnode *vnode = get(vvertex_pmap,*vvertex_it);
	if (vnode->fixed) continue;
	if (absassigned[*vvertex_it]) {
	  if (vnode->vclass != NULL) {
	    vnode->type = abstypes[*vvertex_it];
	  }
	  assert(!add_node(*vvertex_it,absassignment[*vvertex_it],true));
	}
      }
    }

    tsteps++;

    if (finished) {
      goto DONE;
    }
  }
 DONE:
  cout << "Done" << endl;
}
