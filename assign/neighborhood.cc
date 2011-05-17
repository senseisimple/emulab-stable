/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005-2006,2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * A set of functions useful for exploring the neighborhood of a particular
 * solution.
 */

static const char rcsid[] = "$Id: neighborhood.cc,v 1.4 2009-05-20 18:06:08 tarunp Exp $";

#include "neighborhood.h"

// From asssign.cc
extern bool allow_overload;
#ifdef PER_VNODE_TT
extern pclass_types vnode_type_table;
#endif

/*
 * This overly-verbose function returns true if it's okay to map vn to pn,
 * false otherwise
 */
inline bool pnode_is_match(tb_vnode *vn, tb_pnode *pn) {
  // Find the type record for this type
  tb_pnode::types_map::iterator mit = pn->types.find(vn->type);
  if (mit == pn->types.end()) {
    // The node doesn't even have this type, we can exit early
    return false;
  }

  bool matched = false;
  tb_pnode::type_record *tr = mit->second;
  if (tr->is_static()) {
    if ((tr->get_current_load() + vn->typecount) > tr->get_max_load()) {
      // This would put us over its max load
      if (allow_overload && (tr->get_max_load() > 1)) {
	// That's okay, we're allowing overload
	matched = true;
      } else {
	// Nope, it's full
	matched = false;
      }
    } else {
      // Plenty of room for us
      matched = true;
    }
  } else { // the type is not static
    if (pn->typed) {
      if (pn->current_type != vn->type) {
	// Failure - the pnode has a type, and it isn't ours
	matched = false;
      } else {
	if ((pn->current_type_record->get_current_load() + vn->typecount) >
	    pn->current_type_record->get_max_load()) {
	  // This would put us over its max load
	  //if (allow_overload && (tr->max_load > 1) &&
	  //    ((pn->current_type_record->current_load + vn->typecount) <
	  //    (pn->current_type_record->max_load + 2))) {
	  if (allow_overload && (tr->get_max_load() > 1)) {
	    // That's okay, we're allowing overload
	    matched = true;
	  } else {
	    // Failure - the type is right, but the pnode is full
	    matched = false;
	  }
	} else {
	  // It's under its max load, we can fit in
	  matched = true;
	}
      }
    } else {
      // pnode doesn't have a type
      matched = true;
    }
  }

  // Commented out for now because it's too slow!
#if 0
  // Check for 'local' desires - the reason we take the time to do this here is
  // that they are actually, in many ways, like types with vn->typecount > 1.
  if (matched && !vn->desires.empty()) {
    tb_vnode::desires_map::iterator desire_it;
    for (desire_it = vn->desires.begin();
	desire_it != vn->desires.end();
	desire_it++) {
      if (desire_it->is_l_additive()) {
	  tb_pnode::features_map::iterator feature_it =
	      pn->features.find(desire_it->first);
	  if (feature_it == pn->features.end()) {
	      matched = false;
	      break;
	  }
	  // If we are allowing overloading, do so only to a limited degree
	  if (allow_overload) {
	      if ((feature_it->second < desire_it->second)
		  && (feature_it->second - desire_it->second)
		  < (desire_it->second * 2)) {
		      matched = false;
		      break;
		  }
	  } else {
	      // No overloading, and this would put us over the limit
	      if (feature_it->second < desire_it->second) {
		  matched = false;
		  break;
	      }
	  }
      }
  }
  }
#endif

  return matched;
}

/*
 * Finds a pnode which:
 * 1) One of the vnode's neighbors is mapped to
 * 2) Satisifies the usual pnode mapping constraints
 * 3) The vnode is not already mapped to
 */
tb_pnode *find_pnode_connected(vvertex vv, tb_vnode *vn) {

  //cerr << "find_pnode_connected(" << vn->name << ") called" << endl;

  // We make a list of all neighboring vnodes so that we can go through
  // them in random order
  vector<vedge> visit_order(out_degree(vv,VG));
  voedge_iterator vedge_it,end_vedge_it;
  tie(vedge_it,end_vedge_it) = out_edges(vv,VG);	    
  for (int i = 0; vedge_it != end_vedge_it; vedge_it++, i++) {
    visit_order[i] = *vedge_it;
  }
  for (size_t i = 0; i < visit_order.size(); i++) {
	int i1 = RANDOM() % visit_order.size();
	int i2 = RANDOM() % visit_order.size();
	vedge tmp = visit_order[i1];
	visit_order[i1] = visit_order[i2];
	visit_order[i2] = tmp;
  }
  for (size_t i = 0; i < visit_order.size(); i++) {
    vvertex neighbor_vv = target(visit_order[i],VG);
    tb_vnode *neighbor_vn = get(vvertex_pmap,neighbor_vv);
    //cerr << "    trying " << neighbor_vn->name << endl;
    // Skip any that aren't assigned
    if (!neighbor_vn->assigned) {
      //cerr << "        not assigned" << endl;
      continue;
    }

    // Skip any that are assigned to the same pnode we are
    if (neighbor_vn->assignment == vn->assignment) {
      //cerr << "        same assignment" << endl;
      continue;
    }

    // Check to make sure that our vn can map to the neibor's assigment
    tb_pnode *neighbor_pnode = get(pvertex_pmap,neighbor_vn->assignment);
    //cerr << "        neighbor on " << neighbor_pnode->name << endl;
    if (pnode_is_match(vn,neighbor_pnode)) {
      //cerr << "        good" << endl;
      //cerr << "    worked" << endl;
      return neighbor_pnode;
    }
    //cerr << "        doesn't match" << endl;
  }

  //cerr << "    failed" << endl;
  return NULL;
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
  
  tb_pnode *newpnode = NULL;
  
  //cerr << "Node is " << vn->name << " First = " << first << endl;

  // Randomize the order in which we go through the list of acceptable pclasses
  // We do this by making a randomly-ordered list of indicies into the
  // acceptable_types vector
  vector<int> traversal_order(num_types);
  for (int i = 0; i < num_types; i++) {
	traversal_order[i] = i;
  }
  for (int i = 0; i < num_types; i++) {
	int i1 = RANDOM() % num_types;
	int i2 = RANDOM() % num_types;
	int tmp = traversal_order[i1];
	traversal_order[i1] = traversal_order[i2];
	traversal_order[i2] = tmp;
  }

  for (int i = 0; i < num_types; i++) {

    int index = traversal_order[i];
    tb_pclass *pclass = (*acceptable_types)[index];

    // Skip pclasses that have been disabled
    if (pclass->disabled) {
	  continue;
    }

#ifndef FIND_PNODE_SEARCH
    // If not searching for the pnode, just grab the front one
    newpnode = pclass->members[vn->type]->front();
#else
#ifdef PER_VNODE_TT
    // If using PER_VNODE_TT and vclasses, it's possible that there are
    // some pclasses in this node's type table that can't be used right now,
    // becuase they contain entires that don't contain the vnodes _current_
    // type
    if (pclass->members.find(vn->type) == pclass->members.end()) {
	continue;
    }
#endif

    RDEBUG(cout << "find_pnode: Members list has " <<
            pclass->members[vn->type]->L.size() << ", type is " << vn->type <<
            ", used is " << pclass->used_members << endl;)
    list<tb_pnode*>::iterator it = pclass->members[vn->type]->L.begin();
    while (it != pclass->members[vn->type]->L.end()) {
	if (pnode_is_match(vn,*it)) {
	    break; 
	} else {
	    it++;
	}
    }
    if (it == pclass->members[vn->type]->L.end()) {
	newpnode = NULL;
    } else {
	newpnode = *it;
    }
#endif // FIND_PNODE_SEARCH
#ifdef PCLASS_DEBUG
    cerr << "Found pclass: " <<
      pclass->name << " and node " <<
      (newpnode == NULL ? "NULL" : newpnode->name) << "\n";
#endif
    if (newpnode != NULL) {
      RDEBUG(cout << " to " << newpnode->name << endl;)
      return newpnode;
    }
  }

  // Nope, didn't find one
  return NULL;
}
