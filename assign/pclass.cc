/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "port.h"

#include <stdlib.h>

#include <hash_map>
#include <rope>
#include <queue>
#include <slist>
#include <algorithm>

#include <boost/config.hpp>
#include <boost/utility.hpp>
#include <boost/property_map.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>

using namespace boost;

#include "common.h"
#include "delay.h"
#include "physical.h"
#include "virtual.h"
#include "pclass.h"


// The purpose of the code in s this file is to generate the physical
// equivalence classes and then maintain them during the simulated
// annealing process.  A function generate_pclasses is provided to
// fill out the pclass structure.  Then two routines pclass_set, and
// pclass_unset are used to maintaing the structure during annealing.


extern pnode_pvertex_map pnode2vertex;

// pclasses - A list of all pclasses.
extern pclass_list pclasses;

// type_table - Maps a type (crope) to a tt_entry.  A tt_entry
// consists of an array of pclasses that satisfy that type, and the
// length of the array.
extern pclass_types type_table;

typedef pair<pvertex,int> link_info; // dst, bw

struct hashlinkinfo {
  size_t operator()(link_info const &A) const {
    hashptr<void *> ptrhash;
    return ptrhash(A.first)/2+A.second;
  }
};

// returns 1 if a and b are equivalent.  They are equivalent if the
// type and features information match and if there is a one-to-one
// mapping between links that preserves bw, and destination.
int pclass_equiv(tb_pgraph &PG, tb_pnode *a,tb_pnode *b)
{
  typedef hash_multiset<link_info,hashlinkinfo> link_set;
  
  // check type information
  for (tb_pnode::types_map::iterator it=a->types.begin();
       it!=a->types.end();++it) {
    const crope &a_type = (*it).first;
    const int a_max_nodes = (*it).second;
    
    tb_pnode::types_map::iterator bit = b->types.find(a_type);
    if ((bit == b->types.end()) ||((*bit).second != a_max_nodes))
      return 0;
  }

  // check features
  for (tb_pnode::features_map::iterator it=a->features.begin();
       it != a->features.end();++it) {
    const crope &a_feature = (*it).first;
    const double a_weight = (*it).second;
    
    tb_pnode::features_map::iterator bit;
    bit = b->features.find(a_feature);
    if ((bit == b->features.end()) || ((*bit).second != a_weight)) 
      return 0;
  }

  // check links - to do this we first create sets of every link in b.
  // we then loop through every link in a, find a match in the set, and
  // remove it from the set.
  pvertex an = pnode2vertex[a];
  pvertex bn = pnode2vertex[b];

  link_set b_links;

  poedge_iterator eit,eendit;
  tie(eit,eendit) = out_edges(bn,PG);
  for (;eit != eendit;++eit) {
    pvertex dst = target(*eit,PG);
    if (dst == bn)
      dst = source(*eit,PG);
    b_links.insert(link_info(dst,get(pedge_pmap,*eit)->delay_info.bandwidth));
  }
  tie(eit,eendit) = out_edges(an,PG);
  for (;eit != eendit;++eit) {
    pvertex dst = target(*eit,PG);
    if (dst == an)
      dst = source(*eit,PG);
    int bw = get(pedge_pmap,*eit)->delay_info.bandwidth;
    link_info tomatch = link_info(dst,bw);
    link_set::iterator found = b_links.find(tomatch);
    if (found == b_links.end()) return 0;
    else b_links.erase(found);
  }
  if (b_links.size() != 0) return 0;
  return 1;
}

/* This function takes a physical graph and generates the set of
   equivalence classes of physical nodes.  The globals pclasses (a
   list of all equivalence classes) and type_table (and table of
   physical type to list of classes that can satisfy that type) are
   set by this routine. */
int generate_pclasses(tb_pgraph &PG, bool pclass_for_each_pnode) {
  typedef hash_map<tb_pclass*,tb_pnode*,hashptr<tb_pclass*> > pclass_pnode_map;
  typedef hash_map<crope,pclass_list*> name_pclass_list_map;

  pvertex cur;
  pclass_pnode_map canonical_members;

  pvertex_iterator vit,vendit;
  tie(vit,vendit) = vertices(PG);
  for (;vit != vendit;++vit) {
    cur = *vit;
    bool found_class = 0;
    tb_pnode *curP = get(pvertex_pmap,cur);
    tb_pclass *curclass;

    // If we're putting each pnode in its own pclass, don't bother to find
    // existing pclasses that it matches
    if (!pclass_for_each_pnode) {
      pclass_pnode_map::iterator dit;
      for (dit=canonical_members.begin();dit!=canonical_members.end();
	  ++dit) {
	curclass=(*dit).first;
	if (pclass_equiv(PG,curP,(*dit).second)) {
	  // found the right class
	  found_class=1;
	  curclass->add_member(curP);
	  break;
	} 
      }
    }

    if (found_class == 0) {
      // new class
      tb_pclass *n = new tb_pclass;
      pclasses.push_back(n);
      canonical_members[n]=curP;
      n->name = curP->name;
      n->add_member(curP);
    }
  }

  name_pclass_list_map pre_type_table;

  pclass_list::iterator it;
  for (it=pclasses.begin();it!=pclasses.end();++it) {
    tb_pclass *cur = *it;
    tb_pclass::pclass_members_map::iterator dit;
    for (dit=cur->members.begin();dit!=cur->members.end();
	 ++dit) {
      if (pre_type_table.find((*dit).first) == pre_type_table.end()) {
	pre_type_table[(*dit).first]=new pclass_list;
      }
      pre_type_table[(*dit).first]->push_back(cur);
    }
  }

  // now we convert the lists in pre_type_table into arrays for
  // faster access.
  name_pclass_list_map::iterator dit;
  for (dit=pre_type_table.begin();dit!=pre_type_table.end();
       ++dit) {
    pclass_list *L = (*dit).second;
    pclass_vector *A = new pclass_vector(L->size());
    int i=0;
    
    type_table[(*dit).first]=tt_entry(L->size(),A);

    pclass_list::iterator it;
    for (it=L->begin();it!=L->end();++it) {
      (*A)[i++] = *it;
    }
    delete L;
  }
  return 0;
}

int tb_pclass::add_member(tb_pnode *p)
{
  tb_pnode::types_map::iterator it;
  for (it=p->types.begin();it!=p->types.end();++it) {
    crope type = (*it).first;
    if (members.find(type) == members.end()) {
      members[type]=new tb_pnodelist;
    }
    members[type]->push_back(p);
  }
  size++;
  p->my_class=this;
  return 0;
}

// should be called after add_node
int pclass_set(tb_vnode *v,tb_pnode *p)
{
  tb_pclass *c = p->my_class;
  
  // remove p node from correct lists in equivalence class.
  tb_pclass::pclass_members_map::iterator dit;
  for (dit=c->members.begin();dit!=c->members.end();dit++) {
    if ((*dit).first == p->current_type) {
      // same class - only remove if node is full
      if (p->current_load == p->max_load) {
	(*dit).second->remove(p);
//#ifdef SMART_UNMAP
//	c->used_members[(*dit).first]->push_back(p);
//#endif
      }
    } else {
      // If it's not in the list then this fails quietly.
      (*dit).second->remove(p);
    }
#ifdef SMART_UNMAP
    if (c->used_members.find((*dit).first) == c->used_members.end()) {
	c->used_members[(*dit).first] = new tb_pclass::tb_pnodeset;
    }

    // XXX: bogus? Maybe we're only supposed to insert if it was in the
    // other list?
    //if (find((*dit).second->L.begin(),(*dit).second->L.end(),p) != (*dit).second->L.end()) {
    c->used_members[(*dit).first]->insert(p);
    //}
#endif
  }

  
  c->used += 1.0/(p->max_load);
  
  return 0;
}

int pclass_unset(tb_pnode *p)
{
  // add pnode to all lists in equivalence class.
  tb_pclass *c = p->my_class;

  //cout << "Unassigning " << p->name << ": ";

  tb_pclass::pclass_members_map::iterator dit;
  for (dit=c->members.begin();dit!=c->members.end();++dit) {
    //cout << " Type " << dit->first << ": ";
    if ((*dit).first == p->current_type) {
      // If it's not in the list then we need to add it to the back if it's
      // empty and the front if it's not.  Since unset is called before
      // remove_node empty means only one user.
      if (! (*dit).second->exists(p)) {
	assert(p->current_load > 0);
#ifdef PNODE_ALWAYS_FRONT
	(*dit).second->push_front(p);
#else
#ifdef PNODE_SWITCH_LOAD
	if (p->current_load == 0) {
#else
	if (p->current_load == 1) {
#endif
	  //cout << "Pushing back: " << p->current_load << " ";
	  (*dit).second->push_back(p);
	} else {
	  //cout << "Pushing front: " << p->current_load << " ";
	  (*dit).second->push_front(p);
	}
#endif
      }
    } else {
      //cout << "Pushing back (2) ";
      (*dit).second->push_back(p);
    }

#ifdef SMART_UNMAP
      c->used_members[(*dit).first]->erase(p);
#endif
  }

  //cout << endl;

  c->used -= 1.0/(p->max_load);
  
  return 0;
}

void pclass_debug()
{
  cout << "PClasses:\n";
  pclass_list::iterator it;
  for (it=pclasses.begin();it != pclasses.end();++it) {
    cout << *(*it);
  }

  cout << "\n";
  cout << "Type Table:\n";
  pclass_types::iterator dit;
  for (dit=type_table.begin();dit!=type_table.end();++dit) {
    cout << (*dit).first << ":";
    int n = (*dit).second.first;
    pclass_vector &A = *((*dit).second.second);
    for (int i = 0; i < n ; ++i) {
      cout << " " << A[i]->name;
    }
    cout << "\n";
  }
}

