/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */


#include "port.h"

#include <iostream.h>
#include <float.h>

#include <hash_map>
#include <rope>
#include <queue>
#include <slist>
#include <hash_set>

#include <boost/config.hpp>
#include <boost/utility.hpp>
#include <boost/property_map.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>

using namespace boost;

#include "common.h"
#include "vclass.h"
#include "delay.h"
#include "physical.h"
#include "virtual.h"
#include "pclass.h"
#include "score.h"

#include "math.h"

extern switch_pred_map_map switch_preds;

extern bool disable_pclasses;

double score;			// The score of the current mapping
int violated;			// How many times the restrictions
				// have been violated.

violated_info vinfo;		// specific info on violations

extern tb_vgraph VG;		// virtual graph
extern tb_pgraph PG;		// physical grpaph
extern tb_sgraph SG;		// switch fabric

bool direct_link(pvertex a,pvertex b,tb_vlink *vlink,pedge &edge);
void score_link(pedge pe,vedge ve,tb_pnode *src_pnode,tb_pnode *dst_pnode);
void unscore_link(pedge pe,vedge ve,tb_pnode *src_pnode,tb_pnode *dst_pnode);
bool find_link_to_switch(pvertex pv,pvertex switch_pv,tb_vlink *vlink,
			 pedge &out_edge);
int find_interswitch_path(pvertex src_pv,pvertex dest_pv,
			  int bandwidth,pedge_path &out_path,
			  pvertex_list &out_switches);
double fd_score(tb_vnode *vnode,tb_pnode *pnoder,int &out_fd_violated);
void score_link_info(vedge ve, tb_pnode *src_pnode, tb_pnode *dst_pnode);
void unscore_link_info(vedge ve, tb_pnode *src_pnode, tb_pnode *dst_pnode);

#ifdef FIX_PLINK_ENDPOINTS
void score_link_endpoints(pedge pe);
#endif

#ifdef SCORE_DEBUG_MORE
#define SADD(amount) cerr << "SADD: " << #amount << "=" << amount << " from " << score;score+=amount;cerr << " to " << score << endl
#define SSUB(amount)  cerr << "SSUB: " << #amount << "=" << amount << " from " << score;score-=amount;cerr << " to " << score << endl
#else
#define SADD(amount) score += amount
#define SSUB(amount) score -= amount
#endif

#ifdef SCORE_DEBUG
#define SDEBUG(a) a
#else
#define SDEBUG(a)
#endif

/*
 * score()
 * Returns the score.
 */
double get_score() {return score;}

/*
 * init_score()
 * This initialized the scoring system.  It also clears all
 * assignments.
 */
void init_score()
{
  SDEBUG(cerr << "SCORE: Initializing" << endl);
  score=0;
  violated=0;

  vvertex_iterator vvertex_it,end_vvertex_it;
  tie(vvertex_it,end_vvertex_it) = vertices(VG);
  for (;vvertex_it!=end_vvertex_it;++vvertex_it) {
    tb_vnode *vnode=get(vvertex_pmap,*vvertex_it);
    vnode->assigned = false;
    SADD(SCORE_UNASSIGNED);
    vinfo.unassigned++;
    violated++;
  }
  vedge_iterator vedge_it,end_vedge_it;
  tie(vedge_it,end_vedge_it) = edges(VG);
  for (;vedge_it!=end_vedge_it;++vedge_it) {
    tb_vlink *vlink=get(vedge_pmap,*vedge_it);
    vlink->link_info.type=tb_link_info::LINK_UNKNOWN;
    vlink->no_connection=false;
  }
  pvertex_iterator pvertex_it,end_pvertex_it;
  tie(pvertex_it,end_pvertex_it) = vertices(PG);
  /*
  for (;pvertex_it!=end_pvertex_it;++pvertex_it) {
    tb_pnode *pn=get(pvertex_pmap,*pvertex_it);
    pn->typed=false;
    pn->current_load=0;
    pn->pnodes_used=0;
    pn->switch_used_links=0;
  }
  */
  pedge_iterator pedge_it,end_pedge_it;
  tie(pedge_it,end_pedge_it) = edges(PG);
  for (;pedge_it!=end_pedge_it;++pedge_it) {
    tb_plink *plink=get(pedge_pmap,*pedge_it);
    plink->bw_used=0;
    plink->emulated=0;
    plink->nonemulated=0;
  }

  SDEBUG(cerr << "  score=" << score << " violated=" << violated << endl);
}

/* unscore_link_info(vedge ve)
 * This routine is the highest level link scorer.  It handles all
 * scoring that depends on the link_info of vlink.
 */
void unscore_link_info(vedge ve,tb_pnode *src_pnode,tb_pnode *dst_pnode)
{
  tb_vlink *vlink = get(vedge_pmap,ve);
  if (vlink->link_info.type == tb_link_info::LINK_DIRECT) {
    // DIRECT LINK
    SDEBUG(cerr << "   direct link" << endl);
    unscore_link(vlink->link_info.plinks.front(),ve,src_pnode,dst_pnode);
    vlink->link_info.plinks.clear();
  } else if (vlink->link_info.type == tb_link_info::LINK_INTERSWITCH) {
    // INTERSWITCH LINK
    SDEBUG(cerr << "  interswitch link" << endl);
    
    pedge_path &path = vlink->link_info.plinks;
#ifndef INTERSWITCH_LENGTH
    SSUB(SCORE_INTERSWITCH_LINK);
#endif
    // XXX: Potentially bogus;
    int numinterlinks;
    numinterlinks = -2;
    for (pedge_path::iterator it=path.begin();
	 it != path.end();++it) {
      unscore_link(*it,ve,src_pnode,dst_pnode);
      numinterlinks++;
    }
#ifdef INTERSWITCH_LENGTH
    for (int i = 1; i <= numinterlinks; i++) {
      SSUB(SCORE_INTERSWITCH_LINK);
    }
#endif

    path.clear();
    for (pvertex_list::iterator it = vlink->link_info.switches.begin();
	 it != vlink->link_info.switches.end();++it) {
      tb_pnode *the_switch = get(pvertex_pmap,*it);
      if (--the_switch->switch_used_links == 0) {
	SDEBUG(cerr << "  releasing switch" << endl);
	SSUB(SCORE_SWITCH);
      }
    }
    vlink->link_info.switches.clear();
  } else if (vlink->link_info.type == tb_link_info::LINK_INTRASWITCH) {
    // INTRASWITCH LINK
    SDEBUG(cerr << "   intraswitch link" << endl);
    SSUB(SCORE_INTRASWITCH_LINK);
    
    unscore_link(vlink->link_info.plinks.front(),ve,src_pnode,dst_pnode);
    unscore_link(vlink->link_info.plinks.back(),ve,src_pnode,dst_pnode);
    vlink->link_info.plinks.clear();
    tb_pnode *the_switch = get(pvertex_pmap,
			       vlink->link_info.switches.front());
    if (--the_switch->switch_used_links == 0) {
      SDEBUG(cerr << "  releasing switch" << endl);
      SSUB(SCORE_SWITCH);
    }
    vlink->link_info.switches.clear();
  }

#ifdef TRIVIAL_LINK_BW
  else if (vlink->link_info.type == tb_link_info::LINK_TRIVIAL) {
      // Trivial link - we may get to remove violations
      if (src_pnode->trivial_bw &&
	      (src_pnode->trivial_bw_used > src_pnode->trivial_bw)) {
	  SSUB(SCORE_TRIVIAL_PENALTY);
	  vinfo.bandwidth--;
	  violated--;
      }
      src_pnode->trivial_bw_used -= vlink->delay_info.bandwidth;
      unscore_link(vlink->link_info.plinks.front(),ve,src_pnode,dst_pnode);
  }
#endif
}

/*
 * This removes a virtual node from the assignments, adjusting
 * the score appropriately.
 */
void remove_node(vvertex vv)
{
  // Find the vnode associated with the vvertex, and the pnode it's assigned to
  tb_vnode *vnode = get(vvertex_pmap,vv);
  assert(vnode->assigned);
  pvertex pv = vnode->assignment;
  tb_pnode *pnode = get(pvertex_pmap,pv);

  SDEBUG(cerr <<  "SCORE: remove_node(" << vnode->name << ")" << endl);
  SDEBUG(cerr <<  "  assignment=" << pnode->name << endl);
#ifdef SCORE_DEBUG_LOTS
  cerr << *vnode;
  cerr << *pnode;
#endif

  assert(pnode != NULL);

  /*
   * Clean up the pnode's state
   */
  if (pnode->my_class) {
    pclass_unset(pnode);
  }

#ifdef SMART_UNMAP
  pnode->assigned_nodes.erase(vnode);
#endif

  // pclass
  if ((!disable_pclasses) && pnode->my_class && (pnode->my_class->used == 0)) {
    SDEBUG(cerr << "  freeing pclass" << endl);
    SSUB(SCORE_PCLASS);
  }

  // vclass
  if (vnode->vclass != NULL) {
    double score_delta = vnode->vclass->unassign_node(vnode->type);
    SDEBUG(cerr << "  vclass unassign " << score_delta << endl);
    
    if (score_delta <= -1) {
      violated--;
      vinfo.vclass--;
    }
    SSUB(-score_delta*SCORE_VCLASS);
  }
  
  /*
   * Now, take care of the virtual links that are attached to the vnode
   */
  voedge_iterator vedge_it,end_vedge_it;
  tie(vedge_it,end_vedge_it) = out_edges(vv,VG);
  for (;vedge_it!=end_vedge_it;++vedge_it) {
    tb_vlink *vlink = get(vedge_pmap,*vedge_it);

    // Find the other end of the vlink - we might be either its source or
    // destination (target)
    vvertex dest_vv = target(*vedge_it,VG);
    if (dest_vv == vv)
      dest_vv = source(*vedge_it,VG);
    tb_vnode *dest_vnode = get(vvertex_pmap,dest_vv);
    SDEBUG(cerr << "  edge to " << dest_vnode->name << endl);

    // A 'not-connected' vlink only counts as a violation if both of its
    // endpoints are assigned
    if (vlink->no_connection) {
      SDEBUG(cerr << "  link no longer in violation.\n";)
      SSUB(SCORE_NO_CONNECTION);
      vlink->no_connection=false;
      vinfo.no_connection--;
      violated--;
    }
    
    // Only unscore the link if the vnode on the other end is assigned - this
    // way, only the first end to be unmapped causes unscoring
    if (! dest_vnode->assigned) continue;
    
    // Find the pnode on the ther end of the link, and unscore it!
    pvertex dest_pv = dest_vnode->assignment;
    tb_pnode *dest_pnode = get(pvertex_pmap,dest_pv);
    unscore_link_info(*vedge_it,pnode,dest_pnode);
  }
 
#ifdef PENALIZE_UNUSED_INTERFACES
  // Keep track of the number of interfaces that the pnode is using
  SSUB((pnode->total_interfaces - pnode->used_interfaces) * SCORE_UNUSED_INTERFACE);
  pnode->used_interfaces = 0;
#endif
 
  /*
   * Adjust scores for the pnode
   */
  pnode->current_load--;
#ifdef LOAD_BALANCE
  // Use this tricky formula to score based on how 'full' the pnode is, so that
  // we prefer to equally fill the minimum number of pnodes
  SSUB(SCORE_PNODE * (powf(1+ ((pnode->current_load+1) * 1.0)/pnode->max_load,2)));
  SADD(SCORE_PNODE * (powf(1+ pnode->current_load * 1.0/pnode->max_load,2)));
#endif

  if (pnode->current_load == 0) {
    // If the pnode is now free, we need to do some cleanup
    SDEBUG(cerr << "  releasing pnode" << endl);
    SSUB(SCORE_PNODE);
    pnode->typed=false;
  } else if (pnode->current_load >= pnode->max_load) {
    // If the pnode is still over its max load, reduce the penalty to adjust
    // for the new load.
    // XXX - This shouldn't ever happen, right? I don't think we can ever
    // over-assign to a pnode.
    SDEBUG(cerr << "  reducing penalty, new load=" << pnode->current_load <<
	   " (>= " << pnode->max_load << ")" << endl);
    SSUB(SCORE_PNODE_PENALTY);
    vinfo.pnode_load--;
    violated--;
  }

  /*
   * Score the fact that we now have one more unassigned vnode
   */
  vnode->assigned = false;
  SADD(SCORE_UNASSIGNED);
  vinfo.unassigned++;
  violated++;

  /*
   * Scoring for features and desires
   */
  int fd_violated;
  double fds=fd_score(vnode,pnode,fd_violated);
  SSUB(fds);
  violated -= fd_violated;
  vinfo.desires -= fd_violated;

  /*
   * If we just unassigned a LAN node, we can delete the pnode entirely
   */
  if (vnode->type.compare("lan") == 0) {
    SDEBUG(cerr << "Deleting lan node." << endl);
    delete_lan_node(pv);
  }

  SDEBUG(cerr << "  new score = " << score << " new violated = " << violated << endl);
}

/* score_link_info(vedge ve)
 * This routine is the highest level link scorer.  It handles all
 * scoring that depends on the link_info of vlink.
 */
void score_link_info(vedge ve, tb_pnode *src_pnode, tb_pnode *dst_pnode)
{
  tb_vlink *vlink = get(vedge_pmap,ve);
  tb_pnode *the_switch;
  switch (vlink->link_info.type) {
  case tb_link_info::LINK_DIRECT:
    SADD(SCORE_DIRECT_LINK);
    score_link(vlink->link_info.plinks.front(),ve,src_pnode,dst_pnode);
    break;
  case tb_link_info::LINK_INTRASWITCH:
    SADD(SCORE_INTRASWITCH_LINK);
    score_link(vlink->link_info.plinks.front(),ve,src_pnode,dst_pnode);
    score_link(vlink->link_info.plinks.back(),ve,src_pnode,dst_pnode);
    the_switch = get(pvertex_pmap,
		     vlink->link_info.switches.front());
    if (++the_switch->switch_used_links == 1) {
      SDEBUG(cerr << "  new switch" << endl);
      SADD(SCORE_SWITCH);
    }
    break;
  case tb_link_info::LINK_INTERSWITCH:
#ifndef INTERSWITCH_LENGTH
    SADD(SCORE_INTERSWITCH_LINK);
#endif
    // XXX: Potentially bogus!
    int numinterlinks;
    numinterlinks = -2;
    for (pedge_path::iterator plink_It = vlink->link_info.plinks.begin();
	 plink_It != vlink->link_info.plinks.end();
	 ++plink_It) {
	score_link(*plink_It,ve,src_pnode,dst_pnode);
	numinterlinks++;
    }
#ifdef INTERSWITCH_LENGTH
    for (int i = 1; i <= numinterlinks; i++) {
	SADD(SCORE_INTERSWITCH_LINK);
    }
#endif

    for (pvertex_list::iterator switch_it = vlink->link_info.switches.begin();
	 switch_it != vlink->link_info.switches.end();++switch_it) {
      the_switch = get(pvertex_pmap,*switch_it);
      if (++the_switch->switch_used_links == 1) {
	SDEBUG(cerr << "  new switch" << endl);
	SADD(SCORE_SWITCH);
      }
    }
    break;
  case tb_link_info::LINK_TRIVIAL:
  #ifdef TRIVIAL_LINK_BW
    if (dst_pnode->trivial_bw) {
      dst_pnode->trivial_bw_used += vlink->delay_info.bandwidth;
      if (dst_pnode->trivial_bw_used > dst_pnode->trivial_bw) {
	SADD(SCORE_TRIVIAL_PENALTY);
	vinfo.bandwidth++;
	violated++;
      }
    }
    break;
#endif
  case tb_link_info::LINK_UNKNOWN:
    cerr << "Internal error: Should not be here either." << endl;
    exit(1);
    break;
  }
}

/*
 * int add_node(vvertex vv,pvertex pv,bool deterministic)
 * Add a mapping of vv to pv and adjust score appropriately.
 * Returns 1 in the case of an incompatible mapping.  If determinisitic
 * is true then it deterministically solves the link problem for best
 * score.  Note: deterministic takes considerably longer.
 */
int add_node(vvertex vv,pvertex pv, bool deterministic)
{
  // Get the vnode and pnode associated with the passed vertices
  tb_vnode *vnode = get(vvertex_pmap,vv);
  tb_pnode *pnode = get(pvertex_pmap,pv);

  assert(!vnode->assigned);

  SDEBUG(cerr << "SCORE: add_node(" << vnode->name << "," <<
	 pnode->name << ")" << endl);
#ifdef SCORE_DEBUG_LOTS
  cerr << *vnode;
  cerr << *pnode;
#endif
  SDEBUG(cerr << "  vnode type = " << vnode->type << endl);
  
  /*
   * Set up pnode
   */

  // Figure out the pnode's type
  if (!pnode->typed) {
    // If this pnode has no type yet, give it one
    SDEBUG(cerr << "  virgin pnode" << endl);
    SDEBUG(cerr << "    vtype = " << vnode->type << endl);

    // Remove check assuming at higher level?
    // Remove higher level checks?
    pnode->max_load=0;
    if (pnode->types.find(vnode->type) != pnode->types.end()) {
      pnode->max_load = pnode->types[vnode->type];
    }
    if (pnode->max_load == 0) {
      // didn't find a type
      SDEBUG(cerr << "  no matching type" << endl);
      return 1;
    }
    
    pnode->current_type=vnode->type;
    pnode->typed=true;

    SDEBUG(cerr << "  matching type found (" <<pnode->current_type <<
	   ", max = " << pnode->max_load << ")" << endl);
  } else {
    // The pnode already has a type, let's just make sure it's compatible
    SDEBUG(cerr << "  pnode already has type" << endl);
    if (pnode->current_type != vnode->type) {
      SDEBUG(cerr << "  incompatible types" << endl);
      return 1;
    } else {
      SDEBUG(cerr << "  compatible types" << endl);
      if (pnode->current_load == pnode->max_load) {
	/* XXX - We could ignore this check and let the code
	   at the end of the routine penalize for going over
	   load.  Failing here seems to work better though. */

	// XXX is this a bug?  do we need to revert the pnode/vnode to
	// it's initial state.
	SDEBUG(cerr << "  node is full" << endl);
	cerr << "  node is full" << endl;
	return 1;
      }
    }
  }

#ifdef PENALIZE_UNUSED_INTERFACES
  pnode->used_interfaces = 0;
#endif
 
#ifdef SMART_UNMAP
  pnode->assigned_nodes.insert(vnode);
#endif
 
  /*
   * Assign any links on the vnode that can now be assigned (due to the other
   * end of the link also being assigned)
   */
  voedge_iterator vedge_it,end_vedge_it;
  tie(vedge_it,end_vedge_it) = out_edges(vv,VG);	    
  for (;vedge_it!=end_vedge_it;++vedge_it) {
    tb_vlink *vlink = get(vedge_pmap,*vedge_it);
    vvertex dest_vv = target(*vedge_it,VG);

    if (dest_vv == vv) {
      dest_vv = source(*vedge_it,VG);
    }

    bool flipped = false; // Indicates that we've assigned the nodes in reverse
    			  // order compared to what's in the vlink, so we need
			  // to reverse the ordering in the pedge_path
    
    if (vlink->src != vv) {
	flipped = true;
    }

    // Find the link's destination
    tb_vnode *dest_vnode = get(vvertex_pmap,dest_vv);
    SDEBUG(cerr << "  edge to " << dest_vnode->name << endl);

    // If the other end of the link is assigned, we can go ahead and try to
    // find a home for the link itself
    if (dest_vnode->assigned) {
      pvertex dest_pv = dest_vnode->assignment;
      tb_pnode *dest_pnode = get(pvertex_pmap,dest_pv);

      SDEBUG(cerr << "   goes to " << dest_pnode->name << endl);

      if (dest_pv == pv) {
	SDEBUG(cerr << "  trivial link" << endl);
	if (allow_trivial_links && vlink->allow_trivial) {
	    vlink->link_info.type = tb_link_info::LINK_TRIVIAL;
	    // XXX - okay, this is really bad, but score_link_info doesn't
	    // usually get called for trivial links, and letting them fall
	    // through into the 'normal' link code below is disatrous!
	    score_link_info(*vedge_it,pnode,dest_pnode);
	} else {
	    SADD(SCORE_NO_CONNECTION);
	    vlink->no_connection=true;
	    vinfo.no_connection++;
	    violated++;
	}
      } else {
	SDEBUG(cerr << "   finding link resolutions" << endl);
	// We need to calculate all possible link resolutions, stick them
	// in a nice datastructure along with their weights, and then
	// select one randomly.
	typedef vector<tb_link_info> resolution_vector;
	typedef vector<pvertex_list> switchlist_vector;

	resolution_vector resolutions(10);
	int resolution_index = 0;
	float total_weight = 0;

	pedge pe;
	// Direct link
	if (direct_link(dest_pv,pv,vlink,pe)) {
	  resolutions[resolution_index].type = tb_link_info::LINK_DIRECT;
	  resolutions[resolution_index].plinks.push_back(pe);
	  resolution_index++;
	  total_weight += LINK_RESOLVE_DIRECT;
	  SDEBUG(cerr << "    direct_link " << pe << endl);
	}
	// Intraswitch link
	pedge first,second;
	for (pvertex_set::iterator switch_it = pnode->switches.begin();
	     switch_it != pnode->switches.end();++switch_it) {
	  if (dest_pnode->switches.find(*switch_it) != dest_pnode->switches.end()) {
#ifdef FIX_SHARED_INTERFACES
	      if ((!find_link_to_switch(pv,*switch_it,vlink,first)) || 
		      (!find_link_to_switch(dest_pv,*switch_it,vlink,second))) {
		  //cerr << "No acceptable links" << endl;
		  continue;
	      }
#else
	    find_link_to_switch(pv,*switch_it,vlink,first);
	    find_link_to_switch(dest_pv,*switch_it,vlink,second);
#endif
	    resolutions[resolution_index].type = tb_link_info::LINK_INTRASWITCH;
	    if (flipped) { // Order these need to go in depends on flipped bit
	      resolutions[resolution_index].plinks.push_back(second);
	      resolutions[resolution_index].plinks.push_back(first);
	    } else { 
	      resolutions[resolution_index].plinks.push_back(first);
	      resolutions[resolution_index].plinks.push_back(second);
	    }
	    resolutions[resolution_index].switches.push_front(*switch_it);
	    resolution_index++;
	    total_weight += LINK_RESOLVE_INTRASWITCH;
	    SDEBUG(cerr << "    intraswitch " << first << " and " << second << endl);
	  }
	}
	// Interswitch paths
	//cout << "Source switches list has " << pnode->switches.size() <<
	 //   " entries" << endl;
	for (pvertex_set::iterator source_switch_it = pnode->switches.begin();
	     source_switch_it != pnode->switches.end();
	     ++source_switch_it) {
	  //cout << "Source switch: " << get(pvertex_pmap,*source_switch_it)->name << endl;
	  //cout << "Dest switches list has " << dest_pnode->switches.size() <<
	   //   " entries" << endl;
	  //cout << "Dest pnode is " << dest_pnode->name << endl;
	  int tmp = 0;
	  for (pvertex_set::iterator dest_switch_it = dest_pnode->switches.begin();
	       dest_switch_it != dest_pnode->switches.end();
	       ++dest_switch_it) {
	    //cout << "Dest switch number " << ++tmp << endl;
	    //cout << "Dest switch: " << get(pvertex_pmap,*dest_switch_it)->name << endl;
	    if (*source_switch_it == *dest_switch_it) continue;
	    if (find_interswitch_path(*source_switch_it,*dest_switch_it,vlink->delay_info.bandwidth,
				      resolutions[resolution_index].plinks,
				      resolutions[resolution_index].switches) != 0) {
#ifdef FIX_SHARED_INTERFACES
	      if ((!find_link_to_switch(pv,*source_switch_it,vlink,first)) || 
		      (!find_link_to_switch(dest_pv,*dest_switch_it,vlink,second))) {
		  //cerr << "No acceptable links" << endl;
		  continue;
	      }
#else
	      find_link_to_switch(pv,*source_switch_it,vlink,first);
	      find_link_to_switch(dest_pv,*dest_switch_it,vlink,second);
#endif

	      resolutions[resolution_index].type = tb_link_info::LINK_INTERSWITCH;
	      if (flipped) { // Order these need to go in depends on flipped bit
	        resolutions[resolution_index].plinks.push_front(second);
	        resolutions[resolution_index].plinks.push_back(first);
	      } else {
	        resolutions[resolution_index].plinks.push_front(first);
	        resolutions[resolution_index].plinks.push_back(second);
	      }
	      resolution_index++;
	      total_weight += LINK_RESOLVE_INTERSWITCH;
	      SDEBUG(cerr << "    interswitch " <<
		     get(pvertex_pmap,*source_switch_it)->name << " and " <<
		     get(pvertex_pmap,*dest_switch_it)->name << endl);
	    }
	  }
	}

	// check for no link
	if (resolution_index == 0) {
	  //cerr << "No resolutions at all" << endl;
	  SDEBUG(cerr << "  Could not find any resolutions. Trying delay." <<
		 endl);

	  SADD(SCORE_NO_CONNECTION);
	  vlink->no_connection=true;
	  vinfo.no_connection++;
	  vlink->link_info.type = tb_link_info::LINK_UNKNOWN;
	  violated++;
	} else {
	  //cerr << "Some resolutions" << endl;
	  // Check to see if we are fixing a violation
	  if (vlink->no_connection) {
	    SDEBUG(cerr << "  Fixing previous violations." << endl);
	    SSUB(SCORE_NO_CONNECTION);
	    vlink->no_connection=false;
	    vinfo.no_connection--;
	    violated--;
	  }
	  
	  // Choose a link
	  int index;
	  if (!deterministic) {
	    float choice = std::random()%(int)total_weight;
	    for (index = 0;index < resolution_index;++index) {
	      switch (resolutions[index].type) {
	      case tb_link_info::LINK_DIRECT:
		choice -= LINK_RESOLVE_DIRECT; break;
	      case tb_link_info::LINK_INTRASWITCH:
		choice -= LINK_RESOLVE_INTRASWITCH; break;
	      case tb_link_info::LINK_INTERSWITCH:
		choice -= LINK_RESOLVE_INTERSWITCH; break;
	      case tb_link_info::LINK_UNKNOWN:
	      case tb_link_info::LINK_TRIVIAL:
		cerr << "Internal error: Should not be here." << endl;
		exit(1);
		break;
	      }
	      if (choice < 0) break;
	    }
	  } else {
	    // Deterministic
	    int bestindex;
	    int bestviolated = 10000;
	    double bestscore=10000.0;
	    int i;
	    for (i=0;i<resolution_index;++i) {
	      vlink->link_info = resolutions[i];
	      score_link_info(*vedge_it,pnode,dest_pnode);
	      if ((score <= bestscore) &&
		  (violated <= bestviolated)) {
		bestscore = score;
		bestviolated = violated;
		bestindex = i;
	      }
	      unscore_link_info(*vedge_it,pnode,dest_pnode);
	    }
	    index = bestindex;
	  }
#ifdef PENALIZE_UNUSED_INTERFACES
	  pnode->used_interfaces++;
#endif
	  vlink->link_info = resolutions[index];
	  SDEBUG(cerr << "  choice:" << vlink->link_info);
	  score_link_info(*vedge_it,pnode,dest_pnode);
	}
      }
#ifdef AUTO_MIGRATE
      if (dest_vnode->type.compare("lan") == 0) {
	  //cout << "Auto-migrating LAN " << dest_vnode->name << " (because of "
	   //   << vnode->name << ")" << endl;
	  remove_node(dest_vv);
	  pvertex lanpv = make_lan_node(dest_vv);
	  add_node(dest_vv,lanpv,false);
      }
#endif
    }
  }
  
  // finish setting up pnode
  pnode->current_load++;

#ifdef PENALIZE_UNUSED_INTERFACES
  assert(pnode->used_interfaces <= pnode->total_interfaces);
  SADD((pnode->total_interfaces - pnode->used_interfaces) * SCORE_UNUSED_INTERFACE);
#endif

  vnode->assignment = pv;
  vnode->assigned = true;
  if (pnode->current_load > pnode->max_load) {
    SDEBUG(cerr << "  load to high - penalty (" << pnode->current_load <<
	   ")" << endl);
    SADD(SCORE_PNODE_PENALTY);
    vinfo.pnode_load++;
    violated++;
  } else {
    SDEBUG(cerr << "  load is fine" << endl);
  }
  if (pnode->current_load == 1) {
    SDEBUG(cerr << "  new pnode" << endl);
    SADD(SCORE_PNODE);
#ifdef LOAD_BALANCE
    //SADD(SCORE_PNODE); // Yep, twice
#endif
  }
#ifdef LOAD_BALANCE
  //SSUB(SCORE_PNODE * (1.0 / pnode->max_load));
  //SSUB(SCORE_PNODE * (1 + powf(((pnode->current_load-1) * 1.0)/pnode->max_load,2)));
  //SADD(SCORE_PNODE * (1 + powf(((pnode->current_load) * 1.0)/pnode->max_load,2)));
  SSUB(SCORE_PNODE * (powf(1 + ((pnode->current_load-1) * 1.0)/pnode->max_load,2)));
  SADD(SCORE_PNODE * (powf(1 + ((pnode->current_load) * 1.0)/pnode->max_load,2)));
#endif

  // node no longer unassigned
  SSUB(SCORE_UNASSIGNED);
  vinfo.unassigned--;
  violated--;

  // features/desires
  int fd_violated;
  double fds = fd_score(vnode,pnode,fd_violated);
  SADD(fds);
  violated += fd_violated;
  vinfo.desires += fd_violated;

  // pclass
  if ((!disable_pclasses) && pnode->my_class && (pnode->my_class->used == 0)) {
    SDEBUG(cerr << "  new pclass" << endl);
    SADD(SCORE_PCLASS);
  }

  // vclass
  if (vnode->vclass != NULL) {
    double score_delta = vnode->vclass->assign_node(vnode->type);
    SDEBUG(cerr << "  vclass assign " << score_delta << endl);
    SADD(score_delta*SCORE_VCLASS);
    if (score_delta >= 1) {
      violated++;
      vinfo.vclass++;
    }
  }

  SDEBUG(cerr << "  assignment=" << vnode->assignment << endl);
  SDEBUG(cerr << "  new score=" << score << " new violated=" << violated << endl);

  if (pnode->my_class) {
    pclass_set(vnode,pnode);
  }
  
  return 0;
}

// returns "best" direct link between a and b.
// best = less users
//        break ties with minimum bw_used
bool direct_link(pvertex a,pvertex b,tb_vlink *vlink,pedge &edge)
{
  pvertex dest_pv;
  pedge best_pedge;
  tb_plink *plink;
  tb_plink *best_plink = NULL;
  poedge_iterator pedge_it,end_pedge_it;
  int best_users;
  double best_distance;
  tie(pedge_it,end_pedge_it) = out_edges(a,PG);
  for (;pedge_it!=end_pedge_it;++pedge_it) {
    dest_pv = target(*pedge_it,PG);
    if (dest_pv == a)
      dest_pv = source(*pedge_it,PG);
    if (dest_pv == b) {
      plink = get(pedge_pmap,*pedge_it);
      int users = plink->nonemulated;
      if (! vlink->emulated) {
	users += plink->emulated;
      }
      tb_delay_info physical_delay;
      physical_delay.bandwidth = plink->delay_info.bandwidth - plink->bw_used;
      physical_delay.delay = plink->delay_info.delay;
      physical_delay.loss = plink->delay_info.loss;
      double distance = vlink->delay_info.distance(physical_delay);
      if (distance == -1) {distance = DBL_MAX;}
      
      if ((! best_plink) ||
	  (users < best_users) ||
	  ((users == best_users) && (distance < best_distance))) {
	best_users = users;
	best_distance = distance;
	best_pedge = *pedge_it;
	best_plink = plink;
      }
    }
  }
  if (best_plink == NULL) {
    return false;
  } else {
    edge = best_pedge;
    return true;
  }
}

bool find_link_to_switch(pvertex pv,pvertex switch_pv,tb_vlink *vlink,
			 pedge &out_edge)
{
  pvertex dest_pv;
  double best_distance = 1000.0;
  int best_users = 1000;
  pedge best_pedge;
  bool found_best=false;
  poedge_iterator pedge_it,end_pedge_it;
  tie(pedge_it,end_pedge_it) = out_edges(pv,PG);
  for (;pedge_it!=end_pedge_it;++pedge_it) {
    dest_pv = target(*pedge_it,PG);
    if (dest_pv == pv)
      dest_pv = source(*pedge_it,PG);
    if (dest_pv == switch_pv) {
      tb_plink *plink = get(pedge_pmap,*pedge_it);
      tb_delay_info physical_delay;
      physical_delay.bandwidth = plink->delay_info.bandwidth - plink->bw_used;
      physical_delay.delay = plink->delay_info.delay;
      physical_delay.loss = plink->delay_info.loss;
      double distance = vlink->delay_info.distance(physical_delay);
      int users;

      // For sticking emulated links in emulated links we care about the
      // distance, and whether or not we've gone over bandwidth
      users = plink->nonemulated;
      if (! vlink->emulated) {
	users += plink->emulated;
      } else {
	  if (vlink->delay_info.bandwidth >
		  (plink->delay_info.bandwidth - plink->bw_used)) {
	      // Silly hack to make sure that this link doesn't get chosen
	      users = best_users + 1;
	  }
      }

      if (distance == -1) {
	// -1 == infinity
	distance = DBL_MAX;
      }
      if ((users < best_users) ||
	  ((users  == best_users) && (distance < best_distance))) {
	best_pedge = *pedge_it;
	best_distance = distance;
	found_best = true;
	best_users = plink->emulated+plink->nonemulated;
      }
    }
  }

#ifdef FIX_SHARED_INTERFACES
  if ((!vlink->emulated) && found_best && (best_users > 0)) {
      return false;
  }
#endif
  if (found_best) {
    out_edge = best_pedge;
    return true;
  } else {
    return false;
  }
}

// this uses the shortest paths calculated over the switch graph to
// find a path between src and dst.  It passes out list<edge>, a list
// of the edges used. (assumed to be empty to begin with).
// Returns 0 if no path exists and 1 otherwise.
int find_interswitch_path(pvertex src_pv,pvertex dest_pv,
			  int bandwidth,pedge_path &out_path,
			  pvertex_list &out_switches)
{
  // We know the shortest path from src to node already.  It's stored
  // in switch_preds[src] and is a node_array<edge>.  Let P be this
  // array.  We can trace our shortest path by starting at the end and
  // following the pred edges back until we reach src.  We need to be
  // careful though because the switch_preds deals with elements of SG
  // and we have elements of PG.

  svertex src_sv = get(pvertex_pmap,src_pv)->sgraph_switch;
  svertex dest_sv = get(pvertex_pmap,dest_pv)->sgraph_switch;

  sedge current_se;
  svertex current_sv = dest_sv;
  switch_pred_map &preds = *switch_preds[src_sv];
  
  if (preds[dest_sv] == dest_sv) {
    // unreachable
    return 0;
  }
  while (current_sv != src_sv) {
    out_switches.push_front(get(svertex_pmap,current_sv)->mate);
    current_se = edge(current_sv,preds[current_sv],SG).first;
    out_path.push_back(get(sedge_pmap,current_se)->mate);
    current_sv = preds[current_sv];
  }
  out_switches.push_front(get(svertex_pmap,current_sv)->mate);
  return 1;
}

// this does scoring for over users and over bandwidth on edges.
void score_link(pedge pe,vedge ve,tb_pnode *src_pnode, tb_pnode *dst_pnode)
{
  tb_plink *plink = get(pedge_pmap,pe);
  tb_vlink *vlink = get(vedge_pmap,ve);

  SDEBUG(cerr << "  score_link(" << pe << ") - " << plink->name << " / " <<
	 vlink->name << endl);

#ifdef SCORE_DEBUG_LOTS
  cerr << *plink;
  cerr << *vlink;
#endif
  
  if (plink->type == tb_plink::PLINK_NORMAL) {
    // need too account for three things here, the possiblity of a new plink
    // the user of a new emulated link, and a possible violation.
    if (vlink->emulated) {
      plink->emulated++;
      SADD(SCORE_EMULATED_LINK);
    }
    else plink->nonemulated++;
    if (plink->nonemulated+plink->emulated == 1) {
      // new link
      SDEBUG(cerr << "    first user" << endl);
      SADD(SCORE_DIRECT_LINK);
    } else {
      // check for violation, basically if this is the first of it's
      // type to be added.
      if (((! vlink->emulated) && (plink->nonemulated == 1)) ||
	  ((vlink->emulated) && (plink->emulated == 1))) {
	SDEBUG(cerr << "    link user - penalty" << endl);
	SADD(SCORE_DIRECT_LINK_PENALTY);
	vinfo.link_users++;
	violated++;
      }
#ifdef FIX_SHARED_INTERFACES
      if ((! vlink->emulated) && (plink->nonemulated > 1)) {
	  //cerr << "Penalizing overused link" << endl;
	  assert(false);
	  SADD(SCORE_DIRECT_LINK_PENALTY);
	  vinfo.link_users++;
	  violated++;
      }
#endif
    }
  }

#ifdef FIX_PLINK_ENDPOINTS
  if (plink->fixends) {
      // Add this to the list of endpoints used by this plink
      nodepair p;
      if (src_pnode->name < dst_pnode->name) {
	  p.first = src_pnode->name;
	  p.second = dst_pnode->name;
      } else {
	  p.first = dst_pnode->name;
	  p.second = src_pnode->name;
      }
      if (plink->vedge_counts.find(p) == plink->vedge_counts.end()) {
	  plink->vedge_counts[p] = 1;
      } else {
	  plink->vedge_counts[p]++;
      }
      // Figure out if we need to add a violation
      if (p == plink->current_endpoints) {
	  plink->current_count++;
      } else {
	  // Nope, we just passed the old leader
	  if (plink->vedge_counts[p] > plink->current_count) {
	      plink->current_endpoints = p;
	      plink->current_count++;
	  } else {
	      // Yup, it's a new violation
	      SADD(5 * SCORE_DIRECT_LINK_PENALTY);
	      vinfo.incorrect_endpoints++;
	      violated++;
	  }
      }
  }
#endif

  if (plink->type != tb_plink::PLINK_LAN) {
    tb_delay_info physical_delay;
    physical_delay.bandwidth = plink->delay_info.bandwidth - plink->bw_used;
    physical_delay.delay = plink->delay_info.delay;
    physical_delay.loss = plink->delay_info.loss;
    
    double distance = vlink->delay_info.distance(physical_delay);

    plink->bw_used += vlink->delay_info.bandwidth;
#ifdef PENALIZE_BANDWIDTH
    SADD(plink->penalty * (vlink->delay_info.bandwidth * 1.0) / (plink->delay_info.bandwidth));
#endif

    if (distance == -1) {
      // violation
      SDEBUG(cerr << "    outside delay requirements." << endl);
      violated++;
      vinfo.delay++;
      SADD(SCORE_OUTSIDE_DELAY);
    } else {
      SADD(distance * SCORE_DELAY);
    }
  }
}

/*
 * Remove the score incurred by a link. This should be the inverse of
 * score_link()
 */
void unscore_link(pedge pe,vedge ve, tb_pnode *src_pnode, tb_pnode *dst_pnode)
{
  // Get the vlink from the passed in edges
  tb_vlink *vlink = get(vedge_pmap,ve);

  // This is not in the slightest bit graceful! This function was not designed
  // for use with trivial links (which have no plink,) but I would like to call
  // it for symmetry
  if (vlink->link_info.type == tb_link_info::LINK_TRIVIAL) {
    goto UNSCORE_TRIVIAL;
  }

  tb_plink *plink;
  plink = get(pedge_pmap,pe);

  SDEBUG(cerr << "  unscore_link(" << pe << ") - " << plink->name << " / " <<
	 vlink->name << endl);

#ifdef SCORE_DEBUG_LOTS
  cerr << *plink;
  cerr << *vlink;
#endif

  if (plink->type == tb_plink::PLINK_NORMAL) {
    if (vlink->emulated) {
      plink->emulated--;
      SSUB(SCORE_EMULATED_LINK);
    } else {
      plink->nonemulated--;
#ifdef FIX_SHARED_INTERFACES
      if (plink->nonemulated >= 1) {
	  //cerr << "Freeing overused link" << endl;
	  SSUB(SCORE_DIRECT_LINK_PENALTY);
	  vinfo.link_users--;
	  violated--;
      }
#endif
    }
    if (plink->nonemulated+plink->emulated == 0) {
      // link no longer used
      SDEBUG(cerr << "   freeing link" << endl);
      SSUB(SCORE_DIRECT_LINK);
    } else {
      // check to see if re freed up a violation, basically did
      // we remove the last of it's link type.
      if ((vlink->emulated && (plink->emulated == 0)) ||
	  ((! vlink->emulated) && plink->nonemulated == 0)) {
	// all good
	SDEBUG(cerr << "   users ok" << endl);
	SSUB(SCORE_DIRECT_LINK_PENALTY);
	vinfo.link_users--;
	violated--;
      }
    }
  }
#ifdef FIX_PLINK_ENDPOINTS
  if (plink->fixends) {
      // Subtract this from the list of endpoints for this plink
      nodepair p;
      if (src_pnode->name < dst_pnode->name) {
	  p.first = src_pnode->name;
	  p.second = dst_pnode->name;
      } else {
	  p.first = dst_pnode->name;
	  p.second = src_pnode->name;
      }
      int newcount;
      assert(plink->vedge_counts[p] > 0);
      newcount = --plink->vedge_counts[p];
      if (newcount == 0) {
	  plink->vedge_counts.erase(p);
      }

      // Ok, let's see if this removes any violations
      if (p == plink->current_endpoints) {
	  // Need to re-find the heaviest endpoint count
	  nodepair_count_map::iterator it = plink->vedge_counts.begin();
	  int highestcount = 0;
	  nodepair highestp;
	  while (it != plink->vedge_counts.end()) {
	      if (it->second > highestcount) {
		  highestcount = it->second;
		  highestp = it->first;
	      }
	      it++;
	  }
	  plink->current_endpoints = highestp;
	  plink->current_count = highestcount;
	  if (newcount < highestcount) {
	      // Yep, we just got rid of a violation
	      SSUB(5*SCORE_DIRECT_LINK_PENALTY);
	      vinfo.incorrect_endpoints--;
	      violated--;
	  }
      } else {
	  // Yep, we just got rid of a violation
	  SSUB(5*SCORE_DIRECT_LINK_PENALTY);
	  vinfo.incorrect_endpoints--;
	  violated--;
      }
  }
#endif
  
  // bandwidth check
  if (plink->type != tb_plink::PLINK_LAN) {
    plink->bw_used -= vlink->delay_info.bandwidth;
#ifdef PENALIZE_BANDWIDTH
    SSUB(plink->penalty * (vlink->delay_info.bandwidth * 1.0) / (plink->delay_info.bandwidth));
#endif

    tb_delay_info physical_delay;
    physical_delay.bandwidth = plink->delay_info.bandwidth - plink->bw_used;
    physical_delay.delay = plink->delay_info.delay;
    physical_delay.loss = plink->delay_info.loss;
    double distance = vlink->delay_info.distance(physical_delay);

    if (distance == -1) {
      // violation
      SDEBUG(cerr << "    removing delay violation." << endl);
      violated--;
      vinfo.delay--;
      SSUB(SCORE_OUTSIDE_DELAY);
    } else {
      SSUB(distance * SCORE_DELAY);
    }
  }

UNSCORE_TRIVIAL:
  vlink->link_info.type = tb_link_info::LINK_UNKNOWN;
}

double fd_score(tb_vnode *vnode,tb_pnode *pnode,int &fd_violated)
{
  double fd_score=0;
  fd_violated=0;

  double value;
  tb_vnode::desires_map::iterator desire_it;
  tb_pnode::features_map::iterator feature_it;
  for (desire_it = vnode->desires.begin();
       desire_it != vnode->desires.end();
       desire_it++) {
    feature_it = pnode->features.find((*desire_it).first);
    SDEBUG(cerr << "  desire = " << (*desire_it).first << " " <<
	   (*desire_it).second << endl);

    if (feature_it == pnode->features.end()) {
      // Unmatched desire.  Add cost.
      SDEBUG(cerr << "    unmatched" << endl);
      value = (*desire_it).second;
      fd_score += SCORE_DESIRE*value;
      if (value >= FD_VIOLATION_WEIGHT) {
	fd_violated++;
      }
    }
  }
  for (feature_it = pnode->features.begin();
       feature_it != pnode->features.end();++feature_it) {
    desire_it = vnode->desires.find((*feature_it).first);
    SDEBUG(cerr << "  feature = " << (*feature_it).first
	   << " " << (*feature_it).second << endl);

    if (desire_it == vnode->desires.end()) {
      // Unused feature.  Add weight
      SDEBUG(cerr << "    unused" << endl);
      value = (*feature_it).second;
      fd_score+=SCORE_FEATURE*value;
      if (value >= FD_VIOLATION_WEIGHT) {
	fd_violated++;
      }
    }
  }

  SDEBUG(cerr << "  Total feature score: " << fd_score << endl;)
  return fd_score;
}

/* make_lan_node(vvertex vv)
 * This routines create a physical lan node and connects it to a switch
 * with a LAN plink.  Most of the code is in determining which switch to
 * connect the LAN node to.  Specifically, it connects it to the switch
 * which will maximize the number of intra (rather than inter) links for
 * assigned adjancent nodes of vv.
 */
pvertex make_lan_node(vvertex vv)
{
  typedef hash_map<pvertex,int,hashptr<void *> > switch_int_map;
  switch_int_map switch_counts;

  tb_vnode *vnode = get(vvertex_pmap,vv);

  SDEBUG(cerr << "make_lan_node(" << vnode->name << ")" << endl);
  //cerr << "make_lan_node(" << vnode->name << ")" << endl;
  
  // Choose switch
  pvertex largest_switch;
  int largest_switch_count=0;
  voedge_iterator vedge_it,end_vedge_it;
  tie(vedge_it,end_vedge_it) = out_edges(vv,VG);
  for (;vedge_it!=end_vedge_it;++vedge_it) {
    vvertex dest_vv = target(*vedge_it,VG);
    if (dest_vv == vv)
      dest_vv = source(*vedge_it,VG);
    tb_vnode *dest_vnode = get(vvertex_pmap,dest_vv);
    if (dest_vnode->assigned) {
      pvertex dest_pv = dest_vnode->assignment;
      tb_pnode *dest_pnode = get(pvertex_pmap,dest_pv);
      for (pvertex_set::iterator switch_it = dest_pnode->switches.begin();
	   switch_it != dest_pnode->switches.end();switch_it++) {
	if (switch_counts.find(*switch_it) != switch_counts.end()) {
	  switch_counts[*switch_it]++;
	} else {
	  switch_counts[*switch_it]=1;
	}
	if (switch_counts[*switch_it] > largest_switch_count) {
	  largest_switch = *switch_it;
	  largest_switch_count = switch_counts[*switch_it];
	}
      }
    }
  }

  SDEBUG(cerr << "  largest_switch=" << largest_switch <<
	 " largest_switch_count=" << largest_switch_count << endl);
  
  pvertex pv = add_vertex(PG);
  tb_pnode *p = new tb_pnode(vnode->name);
  put(pvertex_pmap,pv,p);
  p->typed = true;
  p->current_type = "lan";
  p->max_load = 1;
  p->types["lan"] = 1;
  
  // If the below is false then we have an orphaned lan node which will
  // quickly be destroyed when add_node fails.
  if (largest_switch_count != 0) {
    pedge pe = (add_edge(pv,largest_switch,PG)).first;

    p->name = "lan-";
    p->name += get(pvertex_pmap,largest_switch)->name;
    p->name += "-";
    p->name += vnode->name;

    // Build a link name that looks like the ones we used to supply in the ptop
    // file
    crope link_name = "link-";
    link_name += p->name;
    tb_plink *pl = new tb_plink(link_name, tb_plink::PLINK_LAN,
	    p->name, "(null)");

    p->switches.insert(largest_switch);
    put(pedge_pmap,pe,pl);

#ifdef FIX_PLINK_ENDPOINTS
    pl->fixends = false;
#endif
  } else {
    p->name += "orphan";
  }

  return pv;
}

/* delete_lan_node(pvertex pv)
 * Removes the physical lan node and the physical lan link.  Assumes that
 * nothing is assigned to it.
 */
void delete_lan_node(pvertex pv)
{
  tb_pnode *pnode = get(pvertex_pmap,pv);

  SDEBUG(cerr << "delete_lan_node(" << pnode->name << ")" << endl);

  // delete LAN link
  typedef list<pedge> pedge_list;
  pedge_list to_free;
  
  poedge_iterator pedge_it,end_pedge_it;
  tie(pedge_it,end_pedge_it) = out_edges(pv,PG);
  // We need to copy because removing edges invalidates out iterators.
  for (;pedge_it != end_pedge_it;++pedge_it) {
    to_free.push_front(*pedge_it);
  }
  for (pedge_list::iterator free_it = to_free.begin();
       free_it != to_free.end();++free_it) {
    delete(get(pedge_pmap,*free_it));
    remove_edge(*free_it,PG);
  }

  remove_vertex(pv,PG);
  delete pnode;
}

