/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

static const char rcsid[] = "$Id: score.cc,v 1.69 2009-12-09 22:53:44 ricci Exp $";

#include "port.h"

#include <iostream>
#include <float.h>

/*
 * We have to do these includes differently depending on which version of gcc
 * we're compiling with
 */
#ifdef NEW_GCC
#include <ext/hash_map>
#include <ext/hash_set>
using namespace __gnu_cxx;
#else
#include <hash_map>
#include <hash_set>
#endif

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
#include "featuredesire.h"

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

void score_link(pedge pe,vedge ve,tb_pnode *src_pnode,tb_pnode *dst_pnode);
void unscore_link(pedge pe,vedge ve,tb_pnode *src_pnode,tb_pnode *dst_pnode);
bool find_best_link(pvertex pv,pvertex switch_pv,tb_vlink *vlink,
                    pedge &out_edge, bool check_src_iface,
                    bool check_dst_iface);
int find_interswitch_path(pvertex src_pv,pvertex dest_pv,
			  int bandwidth,pedge_path &out_path,
			  pvertex_list &out_switches);
score_and_violations score_fds(tb_vnode *vnode, tb_pnode *pnode, bool add);

void score_link_info(vedge ve, tb_pnode *src_pnode, tb_pnode *dst_pnode,
	tb_vnode *src_vnode, tb_vnode *dst_vnode);
void unscore_link_info(vedge ve, tb_pnode *src_pnode, tb_pnode *dst_pnode,
	tb_vnode *src_vnode, tb_vnode *dst_vnode);

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

// For convenience, so we can easily turn on or off one statement
#define SDEBADD(amount) cerr << "SADD: " << #amount << "=" << amount << " from " << score;score+=amount;cerr << " to " << score << endl
#define SDEBSUB(amount)  cerr << "SSUB: " << #amount << "=" << amount << " from " << score;score-=amount;cerr << " to " << score << endl

#ifdef SCORE_DEBUG_MAX
// Handy way to print only the first N debugging messages, since that's usually
// enough to get the idea
static unsigned long scoredebugcount = 0;
#endif

#ifdef SCORE_DEBUG
#ifdef SCORE_DEBUG_MAX
#define SDEBUG(a) if (scoredebugcount++ < SCORE_DEBUG_MAX) { a; }
#else
#define SDEBUG(a) a
#endif
#else
#define SDEBUG(a) 
#endif

#define MIN(a,b) (((a) < (b))? (a) : (b))
#define MAX(a,b) (((a) > (b))? (a) : (b))

/*
 * 'Constants' used in scoring. These can be changed, but it MUST be
 * done BEFORE any actual scoring is done.
 */
#ifdef PENALIZE_BANDWIDTH
float SCORE_DIRECT_LINK = 0.0;
float SCORE_INTRASWITCH_LINK = 0.0;
float SCORE_INTERSWITCH_LINK = 0.0;
#else
float SCORE_DIRECT_LINK = 0.01;
float SCORE_INTRASWITCH_LINK = 0.02;
float SCORE_INTERSWITCH_LINK = 0.2;
#endif
float SCORE_DIRECT_LINK_PENALTY = 0.5;
float SCORE_NO_CONNECTION = 0.5;
float SCORE_PNODE = 0.2;
float SCORE_PNODE_PENALTY = 0.5;
float SCORE_SWITCH = 0.5;
float SCORE_UNASSIGNED = 1.0;
float SCORE_MISSING_LOCAL_FEATURE = 1.0;
float SCORE_OVERUSED_LOCAL_FEATURE = 0.5;
#ifdef NO_PCLASS_PENALTY
float SCORE_PCLASS = 0.0;
#else
float SCORE_PCLASS = 0.5;
#endif
float SCORE_VCLASS = 1.0;
float SCORE_EMULATED_LINK = 0.01;
float SCORE_OUTSIDE_DELAY = 0.5;
#ifdef PENALIZE_UNUSED_INTERFACES
float SCORE_UNUSED_INTERFACE = 0.04;
#endif
float SCORE_TRIVIAL_PENALTY = 0.5;

float SCORE_TRIVIAL_MIX = 0.5;

float SCORE_SUBNODE = 0.5;
float SCORE_MAX_TYPES = 0.15;
float LINK_RESOLVE_TRIVIAL = 8.0;
float LINK_RESOLVE_DIRECT = 4.0;
float LINK_RESOLVE_INTRASWITCH = 2.0;
float LINK_RESOLVE_INTERSWITCH = 1.0;
float VIOLATION_SCORE = 1.0;

float opt_nodes_per_sw = 5.0;

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
    SADD(SCORE_NO_CONNECTION);
    vlink->no_connection=true;
    vinfo.no_connection++;
    vlink->link_info.type_used = tb_link_info::LINK_UNMAPPED;
    violated++;
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

/*
 * Find all possbile resolutions for a link. The possible resolutions are placed
 * into the resolutions parameter, and the total weight of all resolutions are
 * returned.
 */
float find_link_resolutions(resolution_vector &resolutions, pvertex pv,
    pvertex dest_pv, tb_vlink *vlink, tb_pnode *pnode, tb_pnode *dest_pnode,
    bool flipped) {
  SDEBUG(cerr << "   finding link resolutions from " << pnode->name << " to " << dest_pnode->name << endl);
  /* We need to calculate all possible link resolutions, stick
   * them in a nice datastructure along with their weights, and
   * then select one randomly.
   */
  typedef vector<pvertex_list> switchlist_vector;

  float total_weight = 0;

  // Trivial link
  if (dest_pv == pv) {
    SDEBUG(cerr << "  trivial link" << endl);

    /*
     * Sometimes, we might not want to let a vlink be resolved as a
     * trivial link
     */
    if (allow_trivial_links && vlink->allow_trivial) {
      tb_link_info info(tb_link_info::LINK_TRIVIAL);
      resolutions.push_back(info);
      total_weight += LINK_RESOLVE_TRIVIAL;
    }

    /*
     * This is to preserve some old behaviour - if we see a trivial link, that's
     * the only one we consider using. Need to revisit this to see if it's always
     * desirable.
     */
    return total_weight;
  }

  pedge pe;
  // Direct link (have to check both interfaces if they are fixed)
  if (find_best_link(dest_pv,pv,vlink,pe,true,true)) {
    tb_link_info info(tb_link_info::LINK_DIRECT);
    info.plinks.push_back(pe);
    resolutions.push_back(info);
    total_weight += LINK_RESOLVE_DIRECT;
    SDEBUG(cerr << "    added a direct_link " << pe << endl);
  }

  /*
   * IMPORTANT NOTE ABOUT find_best_link -
   *
   * We have to tell it whether it is supposed to check the source of the
   * vlink, the dest of the vlink, or both, for fixed interfaces. Of course, we
   * only want to check the 'node' side, not the switch side, of interswitch
   * and intraswitch paths. Of course, which end we're looking at depends on
   * the order we're traversing the vlink - the flipped variable. This is why
   * you see 'flipped' and '!flipped' passed to find_best_link() below.
   */

  // Intraswitch link
  pedge first,second;
  for (pvertex_set::iterator switch_it = pnode->switches.begin();
      switch_it != pnode->switches.end();++switch_it) {
    if (dest_pnode->switches.find(*switch_it) !=
        dest_pnode->switches.end()) {
      SDEBUG(cerr << "    intraswitch: both are connected to " << *switch_it << endl);
      bool first_link, second_link;
      /*
       * Check to see if either, or both, pnodes are actually
       * the switch we are looking for
       */
      if (pv == *switch_it) {
        first_link = false;
      } else {
        first_link = true;
      }
      if (dest_pv == *switch_it) {
        second_link = false;
      } else {
        second_link = true;
      }

      /*
       * Intraswitch links to switches are not allowed - they
       * will be caught as direct links above
       */
      if (pnode->is_switch || dest_pnode->is_switch) {
        SDEBUG(cerr << "    intraswitch failed - switch" << endl;)
          continue;
      }

      if (first_link) {
        SDEBUG(cerr << "    intraswitch: finding first link" << endl;)
        // See note above
        if (!find_best_link(pv,*switch_it,vlink,first,!flipped,flipped)) {
          SDEBUG(cerr << "    intraswitch failed - no link first" <<
              endl;)
            // No link to this switch
            continue;
        }
      }

      if (second_link) {
        SDEBUG(cerr << "    intraswitch: finding second link (" <<  ")" << endl;)
        // See note above
        if (!find_best_link(dest_pv,*switch_it,vlink,second,flipped,!flipped)) {
          // No link to this switch
          SDEBUG(cerr << "    intraswitch failed - no link second" <<
              endl;)
            continue;
        }
      }

      tb_link_info info(tb_link_info::LINK_INTRASWITCH);
      /*
       * Order these need to go in depends on flipped bit
       */
      if (flipped) {
        if (second_link) {
          info.plinks.push_back(second);
        }
        if (first_link) {
          info.plinks.push_back(first);
        }
      } else {
        if (first_link) {
          info.plinks.push_back(first);
        }
        if (second_link) {
          info.plinks.push_back(second);
        }
      }
      info.switches.push_front(*switch_it);
      resolutions.push_back(info);
      
      total_weight += LINK_RESOLVE_INTRASWITCH;
      SDEBUG(cerr << "    intraswitch " << first << " and " << second
          <<                     endl);
    }
  }
  for (pvertex_set::iterator source_switch_it = pnode->switches.begin();
      source_switch_it != pnode->switches.end();
      ++source_switch_it) {
    for (pvertex_set::iterator dest_switch_it =
        dest_pnode->switches.begin();
        dest_switch_it != dest_pnode->switches.end();
        ++dest_switch_it) {
      if (*source_switch_it == *dest_switch_it) continue;
      tb_link_info info(tb_link_info::LINK_INTERSWITCH);

      /*
       * Check to see if either, or both, pnodes are actually the
       * switches we are looking for
       */
      bool first_link, second_link;
      if ((pv == *source_switch_it) || (pv ==
            *dest_switch_it)) {
        first_link = false;
        SDEBUG(cerr << "    interswitch: not first link in a path" << endl);
      } else {
        SDEBUG(cerr << "    interswitch: *is* first link in a path" << endl);
        first_link = true;
      }
      if ((dest_pv == *source_switch_it) ||
          (dest_pv == *dest_switch_it)) {
        second_link = false;
        SDEBUG(cerr << "    interswitch: not second link in a path" << endl);
      } else {
        SDEBUG(cerr << "    interswitch: *is* second link in a path" << endl);
        second_link = true;
      }

      // Get link objects
      if (first_link) {
        // See note above
        if (!find_best_link(pv,*source_switch_it,vlink,first,!flipped,flipped)) {
            // No link to this switch
            SDEBUG(cerr << "    interswitch failed - no first link"
                << endl;)
              continue;
          }
      }

      if (second_link) {
        // See note above
        if (!find_best_link(dest_pv,*dest_switch_it,vlink,second,flipped,!flipped)) {
          // No link to this switch
          SDEBUG(cerr << "    interswitch failed - no second link" <<                                          endl;)
            continue;
        }
      }

      // For regular links, we just grab the vlink's bandwidth; for links
      // where we're matching the link speed to the 'native' one for the
      // interface, we have to look up interface speeds on both ends.
      int bandwidth;
      if (vlink->delay_info.adjust_to_native_bandwidth) {
          // Grab the actual plink objects for both pedges - it's possible for
          // one or both to be missing if we're linking directly to a switch
          // (as with a LAN)
          tb_plink *first_plink = NULL;
          tb_plink *second_plink = NULL;
          if (first_link) {
              first_plink = get(pedge_pmap,first);
          }
          if (second_link) {
              second_plink = get(pedge_pmap,second);
          }

          if (first_plink != NULL && second_plink != NULL) {
              // If both endpoints are not switches, we use the minimum
              // bandwidth
              bandwidth = min(first_plink->delay_info.bandwidth,
                      second_plink->delay_info.bandwidth);
          } else if (first_plink == NULL) {
              // If one end is a switch, use the bandwidth from the other
              // end
              bandwidth = second_plink->delay_info.bandwidth;
          } else if (second_plink == NULL) {
              bandwidth = first_plink->delay_info.bandwidth;
          } else {
              // Both endpoints are switches! (eg. this might be a link between
              // two LANs): It is not at all clear what the right semantics
              // for this would be, and unfortunately, we can't catch this
              // earlier. So, exiting with an error is crappy, but it's
              // unlikely to happen in our regular use, and it's the best we
              // can do.
              cerr << "*** Using bandwidth adjustment on virutal links " <<
                      "between switches not allowed " << endl;
              exit(EXIT_FATAL);
          }
      } else {
          // If not auto-adjusting, just use the specified bandwidth
          bandwidth = vlink->delay_info.bandwidth;
      }

      // Find a path on the switch fabric between these two switches
      if (find_interswitch_path(*source_switch_it,*dest_switch_it,bandwidth,
            info.plinks, info.switches) != 0) {
        // Okay, we found a real resolution!
        if (flipped) { // Order these need to go in depends on flipped bit
          if (second_link) {
            info.plinks.push_front(second);
          }
          if (first_link) {
            info.plinks.push_back(first);
          }
        } else {
          if (first_link) {
           info.plinks.push_front(first);
          }
          if (second_link) {
            info.plinks.push_back(second);
          }
        }
        resolutions.push_back(info);
        total_weight += LINK_RESOLVE_INTERSWITCH;
        SDEBUG(cerr << "    interswitch " <<
            get(pvertex_pmap,*source_switch_it)->name
            << " and " <<
            get(pvertex_pmap,*dest_switch_it)->name <<
            endl);
      } else {
        SDEBUG(cerr << "    interswitch failed - no switch path";)
      }
    }
  }
      
  return total_weight;

}


/*
 * Return the cost of a resolution
 */
inline float resolution_cost(tb_link_info::linkType res_type) {
    switch (res_type) {
	case tb_link_info::LINK_DIRECT:
	    return LINK_RESOLVE_DIRECT; break;
	case tb_link_info::LINK_INTRASWITCH:
	    return LINK_RESOLVE_INTRASWITCH; break;
	case tb_link_info::LINK_INTERSWITCH:
	    return LINK_RESOLVE_INTERSWITCH; break;
	case tb_link_info::LINK_UNMAPPED:
	case tb_link_info::LINK_TRIVIAL:
	default:
	    // These shouldn't be passed in: fall through to below and die
	break;
    }
    
    cerr << "*** Internal error: Should not be here. (resolution_cost)" << endl;
    exit(EXIT_FATAL);
}

/*
 * Mark a vlink as unassigned
 */
void mark_vlink_unassigned(tb_vlink *vlink) {
    SDEBUG(cerr << "    marking " << vlink->name << " unassigned" << endl;)
    assert(!vlink->no_connection);
    SADD(SCORE_NO_CONNECTION);
    vlink->no_connection=true;
    vinfo.no_connection++;
    vlink->link_info.type_used = tb_link_info::LINK_UNMAPPED;
    violated++;
}

/*
 * Mark a vlink as assigned: fix up violations
 */
void mark_vlink_assigned(tb_vlink *vlink) {
    SDEBUG(cerr << "    marking " << vlink->name << " assigned" << endl;)
    assert(vlink->no_connection);
    SSUB(SCORE_NO_CONNECTION);
    vlink->no_connection=false;
    vinfo.no_connection--;
    violated--;
}

/*
 * Resolve an individual vlink
 */
void resolve_link(vvertex vv, pvertex pv, tb_vnode *vnode, tb_pnode *pnode,
    bool deterministic, vedge edge) {
  tb_vlink *vlink = get(vedge_pmap,edge);
  vvertex dest_vv = target(edge,VG);

  if (dest_vv == vv) {
    dest_vv = source(edge,VG);
    SDEBUG(cerr << "  dest_vv is backwards" << endl);
  }

  /*
   * Indicates that we've assigned the nodes in reverse order compared to
   * what's in the vlink, so we need to reverse the ordering in the
   * pedge_path
   */
  bool flipped = false;
  if (vlink->src != vv) {
    SDEBUG(cerr << "  vlink is flipped" << endl);
    flipped = true;
    assert(vlink->dst == vv);
  }

  /*
   * Find the link's destination
   */
  tb_vnode *dest_vnode = get(vvertex_pmap,dest_vv);
  SDEBUG(cerr << "  edge to " << dest_vnode->name << endl);

  /*
   * If the other end of the link is assigned, we can go ahead and try to
   * find a mapping for the link
   */
  if (dest_vnode->assigned) {
    pvertex dest_pv = dest_vnode->assignment;
    tb_pnode *dest_pnode = get(pvertex_pmap,dest_pv);

    SDEBUG(cerr << "   goes to " << dest_pnode->name << endl);

    if (dest_pv == pv) {
      SDEBUG(cerr << "  trivial link" << endl);
      
      if (allow_trivial_links && vlink->allow_trivial) {
        vlink->link_info.type_used = tb_link_info::LINK_TRIVIAL;
        /*
         * XXX - okay, this is really bad, but score_link_info
         * doesn't usually get called for trivial links, and
         * letting them fall through into the 'normal' link code
         * below is disatrous!
         * Note: We can't get here when doing adjust_to_native_bandwidth,
         * since it's illegal to allow trivial links when it's in use.
         */
        SDEBUG(cerr << "    allowed" << endl;)
        if (vlink->no_connection) {
          mark_vlink_assigned(vlink);
        }
        score_link_info(edge,pnode,dest_pnode,vnode,dest_vnode);
      } else {
        SDEBUG(cerr << "    not allowed" << endl;)
        if (!vlink->no_connection) {
          mark_vlink_unassigned(vlink);
        }
      }
    } else {
      //assert(resolution_index <= 1)
      resolution_vector resolutions;
      float total_weight = find_link_resolutions(resolutions, pv, dest_pv,
          vlink,pnode,dest_pnode,flipped);
      
      int n_resolutions = resolutions.size();
      //int resolution_index = n_resolutions - 1;
      int resolution_index = n_resolutions;
      
      /*
       * check for no link
       */
      if (resolution_index == 0) {
        SDEBUG(cerr << "  Could not find any resolutions" << endl;)
        if (!vlink->no_connection) {
            mark_vlink_unassigned(vlink);
        }
      } else {
        /*
         * Choose a link
         */
        int index;
        if (!deterministic && !greedy_link_assignment) {
          /*
           * Not deterministic - randomly pick between the possibilities
           * we've found.
           */
          float choice;
          choice = RANDOM()%(int)total_weight;
          for (index = 0;index < resolution_index;++index) {
            switch (resolutions[index].type_used) {
              case tb_link_info::LINK_DIRECT:
                choice -= LINK_RESOLVE_DIRECT; break;
              case tb_link_info::LINK_INTRASWITCH:
                choice -= LINK_RESOLVE_INTRASWITCH; break;
              case tb_link_info::LINK_INTERSWITCH:
                choice -= LINK_RESOLVE_INTERSWITCH; break;
              case tb_link_info::LINK_UNMAPPED:
              case tb_link_info::LINK_TRIVIAL:
	      case tb_link_info::LINK_DELAYED:
                cerr << "*** Internal error: Should not be here." <<
                  endl;
                exit(EXIT_FATAL);
                break;
            }
            if (choice < 0) break;
          }
        } else {
          /*
           * Deterministic - try all possibilities, and go with the best one
           * (the one with the best score)
           */
          cerr << "Doing deterministic link resolution" << endl;
          cerr << "total_weight: " << total_weight << ", resolution_index: "
            << resolution_index << endl;
          int bestindex;
          int bestviolated = 10000;
          double bestscore=10000.0;
          int i;
          for (i=0;i<resolution_index;++i) {
            vlink->link_info = resolutions[i];
            score_link_info(edge,pnode,dest_pnode,vnode,dest_vnode);
            if ((score <= bestscore) &&
                (violated <= bestviolated)) {
              bestscore = score;
              bestviolated = violated;
              bestindex = i;
            }
            unscore_link_info(edge,pnode,dest_pnode,vnode,dest_vnode);
          }
          index = bestindex;
        }
#ifdef PENALIZE_UNUSED_INTERFACES
        pnode->used_interfaces++;
#endif
        /*
         * Make it so
         */
        vlink->link_info = resolutions[index];
        SDEBUG(cerr << "  choice:" << vlink->link_info);
        score_link_info(edge,pnode,dest_pnode,vnode,dest_vnode);

        /*
         * Check to see if we are fixing a violation
         */
        if (vlink->no_connection) {
          SDEBUG(cerr << "  Fixing previous violations." << endl);
          mark_vlink_assigned(vlink);
        }
      }
    }
  }
}

/*
 * Assign any links on the vnode that can now be assigned (due to the other
 * end of the link also being assigned)
 */
void resolve_links(vvertex vv, pvertex pv, tb_vnode *vnode, tb_pnode *pnode,
    bool deterministic) {
  /*
   * Loop through all vlinks connected to this vnode
   */
  voedge_iterator vedge_it,end_vedge_it;
  tie(vedge_it,end_vedge_it) = out_edges(vv,VG);
  hash_set<const tb_vlink*, hashptr<const tb_vlink*> > seen_links;
  for (;vedge_it!=end_vedge_it;++vedge_it) {
    /*
     * We have to be careful, because we can see loopback links twice, since
     * both endpoints are on the same vnode and the same pnode. If we've seen
     * this link before, just go onto the next
     */
    // XXX: Seems silly to do this here, then again at the top of resolve_link
    tb_vlink *vlink = get(vedge_pmap,*vedge_it);
    if (seen_links.find(vlink) != seen_links.end()) {
      SDEBUG(cerr << "    seen before - skipping" << endl);
      continue;
    } else {
      seen_links.insert(vlink);      
      resolve_link(vv,pv,vnode,pnode,deterministic,*vedge_it);
    }
  }
}

/* unscore_link_info(vedge ve)
 * This routine is the highest level link scorer.  It handles all
 * scoring that depends on the link_info of vlink.
 */
void unscore_link_info(vedge ve,tb_pnode *src_pnode,tb_pnode *dst_pnode, tb_vnode *src_vnode,
	tb_vnode *dst_vnode)
{
  tb_vlink *vlink = get(vedge_pmap,ve);

  // Handle vnodes that are not allowed to have a mix of trivial and
  // non-trivial links
  if (vlink->link_info.type_used == tb_link_info::LINK_TRIVIAL) {
      src_vnode->trivial_links--;
      dst_vnode->trivial_links--;
      if (src_vnode->disallow_trivial_mix &&
	      (src_vnode->trivial_links == 0) &&
	      (src_vnode->nontrivial_links != 0)) {
	  // We just removed the last trivial link
	  SSUB(SCORE_TRIVIAL_MIX);
	  violated--;
	  vinfo.trivial_mix--;
      }
      if (dst_vnode->disallow_trivial_mix &&
	      (dst_vnode->trivial_links == 0) &&
	      (dst_vnode->nontrivial_links != 0)) {
	  // We just removed the last trivial link
	  SSUB(SCORE_TRIVIAL_MIX);
	  violated--;
	  vinfo.trivial_mix--;
      }
  } else if (vlink->link_info.type_used != tb_link_info::LINK_UNMAPPED) {
      src_vnode->nontrivial_links--;
      dst_vnode->nontrivial_links--;
      if (src_vnode->disallow_trivial_mix &&
	      (src_vnode->nontrivial_links == 0) &&
	      (src_vnode->trivial_links != 0)) {
	  // We just removed the last nontrivial link
	  SSUB(SCORE_TRIVIAL_MIX);
	  violated--;
	  vinfo.trivial_mix--;
      }
      if (dst_vnode->disallow_trivial_mix &&
	      (dst_vnode->nontrivial_links == 0) &&
	      (dst_vnode->trivial_links != 0)) {
	  // We just removed the last nontrivial link
	  SSUB(SCORE_TRIVIAL_MIX);
	  violated--;
	  vinfo.trivial_mix--;
      }
  }

  // Unscore the link itself
  if (vlink->link_info.type_used == tb_link_info::LINK_DIRECT) {
    // DIRECT LINK
    SDEBUG(cerr << "   direct link" << endl);
    src_pnode->nontrivial_bw_used -= vlink->delay_info.bandwidth;
    dst_pnode->nontrivial_bw_used -= vlink->delay_info.bandwidth;

    SSUB(SCORE_DIRECT_LINK);
    unscore_link(vlink->link_info.plinks.front(),ve,src_pnode,dst_pnode);
    vlink->link_info.plinks.clear();
  } else if (vlink->link_info.type_used == tb_link_info::LINK_INTERSWITCH) {
    // INTERSWITCH LINK
    SDEBUG(cerr << "  interswitch link" << endl);
    src_pnode->nontrivial_bw_used -= vlink->delay_info.bandwidth;
    dst_pnode->nontrivial_bw_used -= vlink->delay_info.bandwidth;
    
#ifndef INTERSWITCH_LENGTH
    SSUB(SCORE_INTERSWITCH_LINK);
#endif

    pedge_path &path = vlink->link_info.plinks;
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
      // We count the bandwidth into and out of the switch
      the_switch->nontrivial_bw_used -= vlink->delay_info.bandwidth * 2;
    }
    vlink->link_info.switches.clear();
  } else if (vlink->link_info.type_used == tb_link_info::LINK_INTRASWITCH) {
    // INTRASWITCH LINK
    SDEBUG(cerr << "   intraswitch link" << endl);
    src_pnode->nontrivial_bw_used -= vlink->delay_info.bandwidth;
    dst_pnode->nontrivial_bw_used -= vlink->delay_info.bandwidth;

    SSUB(SCORE_INTRASWITCH_LINK);
    
    unscore_link(vlink->link_info.plinks.front(),ve,src_pnode,dst_pnode);
    unscore_link(vlink->link_info.plinks.back(),ve,src_pnode,dst_pnode);
    vlink->link_info.plinks.clear();
    tb_pnode *the_switch = get(pvertex_pmap,
			       vlink->link_info.switches.front());
    // We count the bandwidth into and out of the switch
    the_switch->nontrivial_bw_used -= vlink->delay_info.bandwidth * 2;
    if (--the_switch->switch_used_links == 0) {
      SDEBUG(cerr << "  releasing switch" << endl);
      SSUB(SCORE_SWITCH);
    }
    vlink->link_info.switches.clear();
  }

#ifdef TRIVIAL_LINK_BW
  else if (vlink->link_info.type_used == tb_link_info::LINK_TRIVIAL) {
      // Trivial link - we may get to remove violations
      SDEBUG(cerr << "  trivial bandwidth used " <<
	      src_pnode->trivial_bw_used << " max is " <<
	      src_pnode->trivial_bw << " subtracting " <<
	      vlink->delay_info.bandwidth << endl);
      if (src_pnode->trivial_bw) {
	int old_over_bw;
	int old_bw = src_pnode->trivial_bw_used;
	if (src_pnode->trivial_bw_used > src_pnode->trivial_bw) {
	  old_over_bw = src_pnode->trivial_bw_used - src_pnode->trivial_bw;
	} else {
	  old_over_bw = 0;
	}
        SDEBUG(cerr << "  old trivial bandwidth over by " << old_over_bw << endl);

	src_pnode->trivial_bw_used -= vlink->delay_info.bandwidth;

	int new_over_bw;
	if (src_pnode->trivial_bw_used > src_pnode->trivial_bw) {
	  new_over_bw = src_pnode->trivial_bw_used - src_pnode->trivial_bw;
	} else {
	  new_over_bw = 0;
	}
        SDEBUG(cerr << "  new trivial bandwidth over by " << new_over_bw << endl);

	if (old_over_bw) {
	  // Count how many multiples of the maximum bandwidth we're at
          int new_multiple = src_pnode->trivial_bw_used / src_pnode->trivial_bw;
          int old_multiple = old_bw / src_pnode->trivial_bw;
	  int num_violations = old_multiple - new_multiple;
          SDEBUG(cerr << "  removing " << num_violations <<
                 " violations for trivial bandwidth" << endl);
	  violated -= num_violations;
	  vinfo.bandwidth -= num_violations;
	  double removed_bandwidth_percent = (old_over_bw - new_over_bw) * 1.0 /
	    src_pnode->trivial_bw;
	  SSUB(SCORE_TRIVIAL_PENALTY * removed_bandwidth_percent);
	}
      }

      unscore_link(vlink->link_info.plinks.front(),ve,src_pnode,dst_pnode);
  }
#endif

  // If auto-adjusting the vlink bandwidth, we set it to a sentinel value
  // so that we can detect any problems next time we do a score_link_info()
  if (vlink->delay_info.adjust_to_native_bandwidth) {
      vlink->delay_info.bandwidth = -2;
  }

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
   * Find the type on the pnode that this vnode is associated with
   */
  tb_pnode::types_map::iterator mit = pnode->types.find(vnode->type);
  tb_pnode::type_record *tr;
  if (mit == pnode->types.end()) {
      // This is kind of a hack - if we don't find the type, then the vnode
      // must have a vtype. So, we assume it must have been assigned to this
      // node's dynamic type at some point.
      // A consequence of this hack is that vtypes can't map to static types,
      // for now.
      RDEBUG(cerr << "Failed to find type " << vnode->type << " (for vnode " <<
	  vnode->name << ") on pnode " << pnode->name << endl;)
      tr = pnode->current_type_record;
  } else {
      tr = mit->second;
  }
 

  /*
   * Clean up the pnode's state
   */
  if (!tr->is_static()) {
    if (pnode->my_class) {
      pclass_unset(pnode);
    }
  }

  // pclass
  if ((!disable_pclasses) && !(tr->is_static()) && pnode->my_class
	  && (pnode->my_class->used_members == 0)) {
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
   * Handle subnodes
   */
  if (vnode->subnode_of) {
      // First handle our parent
      if (vnode->subnode_of->assigned) {
	  tb_pnode *assignment = get(pvertex_pmap,
		  vnode->subnode_of->assignment);
	  if ((!pnode->subnode_of) ||
		 (pnode->subnode_of != assignment)) {
	      SSUB(SCORE_SUBNODE);
	      violated--;
	      vinfo.subnodes--;
	  }
      }
  }
  if (!vnode->subnodes.empty()) {
      // Then any children we might have
      tb_vnode::subnode_list::iterator sit;
      for (sit = vnode->subnodes.begin(); sit != vnode->subnodes.end(); sit++) {
	  if ((*sit)->assigned) {
	      tb_pnode *assignment = get(pvertex_pmap,(*sit)->assignment);
	  if ((!assignment->subnode_of) ||
		 (assignment->subnode_of != pnode)) {
		  SSUB(SCORE_SUBNODE);
		  violated--;
		  vinfo.subnodes--;
	      }
	  }
      }
  }

  /*
   * Now, take care of the virtual links that are attached to the vnode
   */
  voedge_iterator vedge_it,end_vedge_it;
  tie(vedge_it,end_vedge_it) = out_edges(vv,VG);
  hash_set<const tb_vlink*, hashptr<const tb_vlink*> > seen_loopback_links;
  for (;vedge_it!=end_vedge_it;++vedge_it) {
    tb_vlink *vlink = get(vedge_pmap,*vedge_it);

    // Find the other end of the vlink - we might be either its source or
    // destination (target)
    vvertex dest_vv = target(*vedge_it,VG);
    if (dest_vv == vv)
      dest_vv = source(*vedge_it,VG);
    tb_vnode *dest_vnode = get(vvertex_pmap,dest_vv);
    SDEBUG(cerr << "  edge to " << dest_vnode->name << endl);

    if (dest_vv == vv) {
      // We have to be careful with 'loopback' links, because we can see them
      // twice - bail out if this is the second time
      if (seen_loopback_links.find(vlink) != seen_loopback_links.end()) {
	SDEBUG(cerr << "    seen before - skipping" << endl);
	continue;
      } else {
	seen_loopback_links.insert(vlink);
      }
    }


    // Only unscore the link if the vnode on the other end is assigned - this
    // way, only the first end to be unmapped causes unscoring
    if (! dest_vnode->assigned) {
      continue;
    }
    
    // Find the pnode on the ther end of the link, and unscore it!
    pvertex dest_pv = dest_vnode->assignment;
    tb_pnode *dest_pnode = get(pvertex_pmap,dest_pv);
    unscore_link_info(*vedge_it,pnode,dest_pnode,vnode,dest_vnode);
    
    // If the other end was connected before, it's not now
    if (!vlink->no_connection) {
      SDEBUG(cerr << "      link now in violation.\n";)
      mark_vlink_unassigned(vlink);
    }
    
  }
 
#ifdef PENALIZE_UNUSED_INTERFACES
  // Keep track of the number of interfaces that the pnode is using
  SSUB((pnode->total_interfaces - pnode->used_interfaces) * SCORE_UNUSED_INTERFACE);
  pnode->used_interfaces = 0;
#endif
 
  /*
   * Adjust scores for the pnode
   */
  int old_load = tr->get_current_load();
  tr->remove_load(vnode->typecount);
  pnode->total_load -= vnode->typecount;
#ifdef LOAD_BALANCE
  // Use this tricky formula to score based on how 'full' the pnode is, so that
  // we prefer to equally fill the minimum number of pnodes
  SSUB(SCORE_PNODE * (powf(1+ ((pnode->current_load+1) * 1.0)/pnode->max_load,2)));
  SADD(SCORE_PNODE * (powf(1+ pnode->current_load * 1.0/pnode->max_load,2)));
#endif
  if (pnode->total_load == 0) {
    // If the pnode is now free, we need to do some cleanup
    SDEBUG(cerr << "  releasing pnode" << endl);
    SSUB(SCORE_PNODE);
    pnode->remove_current_type();
    // ptypes
    tb_pnode::types_list::iterator lit = pnode->type_list.begin();
    while (lit != pnode->type_list.end()) {
	int removed_violations =
	    (*lit)->get_ptype()->remove_users((*lit)->get_max_load());
	if (removed_violations) {
	    SSUB(SCORE_MAX_TYPES * removed_violations);
	    violated -= removed_violations;
	    vinfo.max_types -= removed_violations;
	}
	lit++;
    }
  } else if (old_load > tr->get_max_load()) {
    // If the pnode was over its load, remove the penalties for the nodes we
    // just removed, down to the max_load.
    SDEBUG(cerr << "  reducing penalty, old load was " << old_load <<
	    ", new load = " << tr->get_current_load() << ", max load = " <<
	    tr->get_max_load() << endl);
    for (int i = old_load;
	 i > MAX(tr->get_current_load(),tr->get_max_load());
	 i--) {
      SSUB(SCORE_PNODE_PENALTY);
      vinfo.pnode_load--;
      violated--;
    }
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
  score_and_violations sv = score_fds(vnode,pnode,false);
  SSUB(sv.first);
  violated -= sv.second;
  vinfo.desires -= sv.second;

  SDEBUG(cerr << "  new score = " << score << " new violated = " << violated << endl);
}

/* score_link_info(vedge ve)
 * This routine is the highest level link scorer.  It handles all
 * scoring that depends on the link_info of vlink.
 */
void score_link_info(vedge ve, tb_pnode *src_pnode, tb_pnode *dst_pnode, tb_vnode *src_vnode,
	tb_vnode *dst_vnode)
{
  tb_vlink *vlink = get(vedge_pmap,ve);
  tb_pnode *the_switch;
  
  // If this link is to be adjusted to the native speed of the interface, go
  // ahead and do that now - we use the minimum of the two endpoint interfaces
  // Note! Not currently supported on trivial links! (it's illegal for
  // adjust_to_native_bandwidth and trivial_ok to both be true)
  if (vlink->delay_info.adjust_to_native_bandwidth &&
      vlink->link_info.type_used != tb_link_info::LINK_TRIVIAL) {
    // Check for special sentinel value to make sure we remembered to re-set
    // the value before
    assert(vlink->delay_info.bandwidth == -2);
    tb_plink *front_plink = get(pedge_pmap, vlink->link_info.plinks.front());
    tb_plink *back_plink = get(pedge_pmap, vlink->link_info.plinks.back());
    vlink->delay_info.bandwidth =
      min(front_plink->delay_info.bandwidth, back_plink->delay_info.bandwidth);
  }
  
  switch (vlink->link_info.type_used) {
  case tb_link_info::LINK_DIRECT:
    SADD(SCORE_DIRECT_LINK);
    src_pnode->nontrivial_bw_used += vlink->delay_info.bandwidth;
    dst_pnode->nontrivial_bw_used += vlink->delay_info.bandwidth;
    score_link(vlink->link_info.plinks.front(),ve,src_pnode,dst_pnode);
    break;
  case tb_link_info::LINK_INTRASWITCH:
    SADD(SCORE_INTRASWITCH_LINK);
    src_pnode->nontrivial_bw_used += vlink->delay_info.bandwidth;
    dst_pnode->nontrivial_bw_used += vlink->delay_info.bandwidth;
    score_link(vlink->link_info.plinks.front(),ve,src_pnode,dst_pnode);
    score_link(vlink->link_info.plinks.back(),ve,src_pnode,dst_pnode);
    the_switch = get(pvertex_pmap,
		     vlink->link_info.switches.front());
    if (++the_switch->switch_used_links == 1) {
      SDEBUG(cerr << "  new switch" << endl);
      SADD(SCORE_SWITCH);
    }
    // We count the bandwidth into and out of the switch
    the_switch->nontrivial_bw_used += vlink->delay_info.bandwidth * 2;
    break;
  case tb_link_info::LINK_INTERSWITCH:
    src_pnode->nontrivial_bw_used += vlink->delay_info.bandwidth;
    dst_pnode->nontrivial_bw_used += vlink->delay_info.bandwidth;
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
      // We count the bandwidth into and out of the switch
      the_switch->nontrivial_bw_used += vlink->delay_info.bandwidth * 2;
    }
    break;
  case tb_link_info::LINK_TRIVIAL:
  #ifdef TRIVIAL_LINK_BW
    SDEBUG(cerr << "  trivial bandwidth used " <<
	    src_pnode->trivial_bw_used << " max is " <<
	    src_pnode->trivial_bw << " adding " << vlink->delay_info.bandwidth
	    << endl);
    if (src_pnode->trivial_bw) {
      int old_over_bw;
      int old_bw = src_pnode->trivial_bw_used;
      if (src_pnode->trivial_bw_used > src_pnode->trivial_bw) {
	old_over_bw = src_pnode->trivial_bw_used - src_pnode->trivial_bw;
      } else {
	old_over_bw = 0;
      }
      SDEBUG(cerr << "  old trivial bandwidth over by " << old_over_bw << endl);

      dst_pnode->trivial_bw_used += vlink->delay_info.bandwidth;

      int new_over_bw;
      if (src_pnode->trivial_bw_used > src_pnode->trivial_bw) {
	new_over_bw = src_pnode->trivial_bw_used - src_pnode->trivial_bw;
      } else {
	new_over_bw = 0;
      }
      SDEBUG(cerr << "  new trivial bandwidth over by " << new_over_bw << endl);
	
      if (new_over_bw) {
	// Count how many multiples of the maximum bandwidth we're at
        int new_multiple = src_pnode->trivial_bw_used / src_pnode->trivial_bw;
        int old_multiple = old_bw / src_pnode->trivial_bw;
	int num_violations = new_multiple - old_multiple;
        SDEBUG(cerr << "  adding " << num_violations <<
               " violations for trivial bandwidth" << endl);
	violated += num_violations;
	vinfo.bandwidth += num_violations;

	double added_bandwidth_percent = (new_over_bw - old_over_bw) * 1.0 /
	  src_pnode->trivial_bw;
	SADD(SCORE_TRIVIAL_PENALTY * added_bandwidth_percent);
      }
    }
    break;
#endif
  case tb_link_info::LINK_UNMAPPED:
  case tb_link_info::LINK_DELAYED:
    cout << "*** Internal error: Should not be here either." << endl;
    exit(EXIT_FATAL);
    break;
  }

  // Handle vnodes that are not allowed to have a mix of trivial and
  // non-trivial links
  if (vlink->link_info.type_used == tb_link_info::LINK_TRIVIAL) {
      src_vnode->trivial_links++;
      dst_vnode->trivial_links++;
      if (src_vnode->disallow_trivial_mix &&
	      (src_vnode->trivial_links == 1) &&
	      (src_vnode->nontrivial_links != 0)) {
	  // We just added the first trivial link
	  SADD(SCORE_TRIVIAL_MIX);
	  violated++;
	  vinfo.trivial_mix++;
      }
      if (dst_vnode->disallow_trivial_mix &&
	      (dst_vnode->trivial_links == 1) &&
	      (dst_vnode->nontrivial_links != 0)) {
	  // We just added the first trivial link
	  SADD(SCORE_TRIVIAL_MIX);
	  violated++;
	  vinfo.trivial_mix++;
      }
  } else {
      src_vnode->nontrivial_links++;
      dst_vnode->nontrivial_links++;
      if (src_vnode->disallow_trivial_mix &&
	      (src_vnode->nontrivial_links == 1) &&
	      (src_vnode->trivial_links != 0)) {
	  // We just added the first trivial link
	  SADD(SCORE_TRIVIAL_MIX);
	  violated++;
	  vinfo.trivial_mix++;
      }
      if (dst_vnode->disallow_trivial_mix &&
	      (dst_vnode->nontrivial_links == 1) &&
	      (dst_vnode->trivial_links != 0)) {
	  // We just added the first trivial link
	  SADD(SCORE_TRIVIAL_MIX);
	  violated++;
	  vinfo.trivial_mix++;
      }
  }

}

/*
 * int add_node(vvertex vv,pvertex pv,bool deterministic)
 * Add a mapping of vv to pv and adjust score appropriately.
 * Returns 1 in the case of an incompatible mapping.  If determinisitic
 * is true then it deterministically solves the link problem for best
 * score.  Note: deterministic takes considerably longer.
 */
int add_node(vvertex vv,pvertex pv, bool deterministic, bool is_fixed, bool skip_links)
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
   * Handle types - first, check to see if the node is capable of taking on the
   * vnode's type. If it can with a static type, just do the bookkeeping for
   * that static type. Otherwise convert the node to the correct dynamic type,
   * failing if we can't for some reason (ie. it already has another type.
   */
  tb_pnode::type_record *tr;
  tb_pnode::types_map::iterator mit = pnode->types.find(vnode->type);

  if (mit == pnode->types.end()) {
    // This pnode can't take on the vnode's type - we normally shouldn't get
    // here, due to the way we pick pnodes
    return 1;
  }

  /*
   * Handle types
   */
  tr = mit->second;
  if (tr->is_static()) {
    // XXX: Scoring???
    if (tr->get_current_load() < tr->get_max_load()) {
    } else {
      return 1;
    }
  } else {
    // Figure out the pnode's type
    if (!pnode->typed) {
      // If this pnode has no type yet, give it one
      SDEBUG(cerr << "  virgin pnode" << endl);
      SDEBUG(cerr << "    vtype = " << vnode->type << endl);

      // Remove check assuming at higher level?
      // Remove higher level checks?
      if (!pnode->set_current_type(vnode->type)) {
	// didn't find a type
	SDEBUG(cerr << "  no matching type" << endl);
	//cerr << "Failed due to bad type!" << endl;
	return 1;
      }

      SDEBUG(cerr << "  matching type found (" << pnode->current_type <<
	  ", max = " << pnode->current_type_record->get_max_load() << ")" <<
          endl);
      } else {
	// The pnode already has a type, let's just make sure it's compatible
	SDEBUG(cerr << "  pnode already has type" << endl);
	if (pnode->current_type != vnode->type) {
	  SDEBUG(cerr << "  incompatible types" << endl);
	  return 1;
	} else {
	  SDEBUG(cerr << "  compatible types" << endl);
	}
      }
    }

  /*
   * Handle subnodes
   */
  if (vnode->subnode_of) {
      // First handle our parent
      if (vnode->subnode_of->assigned) {
	  tb_pnode *assignment = get(pvertex_pmap,
		  vnode->subnode_of->assignment);
	  if ((!pnode->subnode_of) ||
		 (pnode->subnode_of != assignment)) {
	      SADD(SCORE_SUBNODE);
	      violated++;
	      vinfo.subnodes++;
	  }
      }
  }
  if (!vnode->subnodes.empty()) {
      // Then any children we might have
      tb_vnode::subnode_list::iterator sit;
      for (sit = vnode->subnodes.begin(); sit != vnode->subnodes.end(); sit++) {
	  if ((*sit)->assigned) {
	      tb_pnode *assignment = get(pvertex_pmap,(*sit)->assignment);
	  if ((!assignment->subnode_of) ||
		 (assignment->subnode_of != pnode)) {
		  SADD(SCORE_SUBNODE);
		  violated++;
		  vinfo.subnodes++;
	      }
	  }
      }
  }

#ifdef PENALIZE_UNUSED_INTERFACES
  pnode->used_interfaces = 0;
#endif
 
  /*
   * Record the node's assignment. Need to do this now so that 'loopback' links
   * work below.
   */
  vnode->assignment = pv;
  vnode->assigned = true;

  /* Links */
  if (!skip_links) {
      resolve_links(vv,pv,vnode,pnode,deterministic);
  }
  
  int old_load = tr->get_current_load();
  int old_total_load = pnode->total_load;

  // finish setting up pnode
  tr->add_load(vnode->typecount);
  pnode->total_load += vnode->typecount;

#ifdef PENALIZE_UNUSED_INTERFACES
  // XXX
  assert(pnode->used_interfaces <= pnode->total_interfaces);
  SADD((pnode->total_interfaces - pnode->used_interfaces) * SCORE_UNUSED_INTERFACE);
#endif

  if (tr->get_current_load() > tr->get_max_load()) {
    SDEBUG(cerr << "  load too high - penalty (" <<
	pnode->current_type_record->get_current_load() << ")" << endl);
    for (int i = MAX(old_load,tr->get_max_load());
	 i < tr->get_current_load();
	 i++) {
      SADD(SCORE_PNODE_PENALTY);
      vinfo.pnode_load++;
      violated++;
    }
  } else {
    SDEBUG(cerr << "  load is fine" << endl);
  }
  if (old_total_load == 0) {
    SDEBUG(cerr << "  new pnode" << endl);
    SADD(SCORE_PNODE);
    // ptypes
    tb_pnode::types_list::iterator lit = pnode->type_list.begin();
    while (lit != pnode->type_list.end()) {
	int new_violations = 
	    (*lit)->get_ptype()->add_users((*lit)->get_max_load());
	if (new_violations) {
	    SADD(SCORE_MAX_TYPES * new_violations);
	    violated += new_violations;
	    vinfo.max_types += new_violations;
	}
	lit++;
    }
  }
#ifdef LOAD_BALANCE
  SSUB(SCORE_PNODE * (powf(1 + ((pnode->current_load-1) * 1.0)/pnode->max_load,2)));
  SADD(SCORE_PNODE * (powf(1 + ((pnode->current_load) * 1.0)/pnode->max_load,2)));
#endif

  // node no longer unassigned
  SSUB(SCORE_UNASSIGNED);
  vinfo.unassigned--;
  violated--;

  // features/desires
  score_and_violations sv = score_fds(vnode,pnode,true);
  SADD(sv.first);
  if (!is_fixed) {
      violated += sv.second;
      vinfo.desires += sv.second;
  }

  // pclass
  if ((!disable_pclasses) && (!tr->is_static()) && pnode->my_class &&
	  (pnode->my_class->used_members == 0)) {
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

  SDEBUG(cerr << "  assignment=" << pnode->name << endl);
  SDEBUG(cerr << "  new score=" << score << " new violated=" << violated << endl);

  if (!tr->is_static()) {
    if (pnode->my_class) {
      pclass_set(vnode,pnode);
    }
  }
  
  return 0;
}

/*
 * Find the best link between two physical nodes, making sure it matches the
 * requirements of the given vlink. Returns results in the out_edge parameter
 * NOTE: assumes the caller has done enough bookkeeping to know whether the
 * source and dest interfaces need to be checked for fixed interfaces - that
 * is: check_src_iface should be set if:
 *    this is a direct link OR
 *    we're mapping the end of an inter or intra switch link that connects to
 *       the 'source' vnode in the vlink structure
 * Similar rules apply to check_dst_iface
 */
bool find_best_link(pvertex pv,pvertex switch_pv,tb_vlink *vlink,
			 pedge &out_edge, bool check_src_iface,
                         bool check_dst_iface)
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

      // Skip any links whose type is wrong (ie. doesn't match the vlink)
      if (plink->types.find(vlink->type) == plink->types.end()) {
	  continue;
      }

      SDEBUG(cerr << "         find_best_link: fix_src_iface = " <<
              vlink->fix_src_iface << " check_src_iface = " << check_src_iface
              << " fix_dst_iface = " << vlink->fix_dst_iface
              << " check_dst_iface = " << check_dst_iface << endl;)

      // Whether we check the 'source' or 'destination' on the vlink against
      // the phyisical link's source interface depends on whether the
      // interface order in the in the pedge matches the interface order in 
      // the vlink
      bool plink_order_reversed;
      tb_vnode *src_vnode = get(vvertex_pmap,vlink->src);
      tb_pnode *src_pnode = get(pvertex_pmap,src_vnode->assignment);
      if (src_pnode->name != plink->srcnode) {
          SDEBUG(cerr << "          find_best_link: plink and vlink in " <<
                  "different order (" << src_pnode->name << " != " <<
                  plink->srcnode << ")" << endl;)
          plink_order_reversed = true;
      } else {
          SDEBUG(cerr << "          find_best_link: plink and vlink in " <<
                  "same order (" << src_pnode->name << " == " <<
                  plink->srcnode << ")" << endl;)
          plink_order_reversed = false;
      }


      // If the vlink has a fixed source interface, and it doesn't match
      // this plink, skip it
      if (vlink->fix_src_iface && check_src_iface) {
          // The interface name we compare to on the plink depends on wether it
          // goes in the same 'direction' as the vlink
	  fstring compare_iface = (plink_order_reversed? plink->dstiface : plink->srciface);
          if (vlink->src_iface != compare_iface) {
              SDEBUG(cerr << "          find_best_link (" << vlink->name <<
                      "): Fix source: " << vlink->src_iface << " != " <<
                      compare_iface << endl);
              continue;
          } else {
              SDEBUG(cerr << "          find_best_link (" << vlink->name <<
                      "): Fix source: " << vlink->src_iface << " == " <<
                      compare_iface << endl);
          }
      }

      // Same for destination
      if (vlink->fix_dst_iface && check_dst_iface) {
          // The interface name we compare to on the plink depends on wether it
          // goes in the same 'direction' as the vlink
	  fstring compare_iface = (plink_order_reversed? plink->srciface : plink->dstiface);
          if (vlink->dst_iface != compare_iface) {
              SDEBUG(cerr << "          find_best_link (" << vlink->name <<
                      "): Fix dst: " << vlink->dst_iface << " != " <<
                      compare_iface << endl);
              continue;
          } else {
              SDEBUG(cerr << "          find_best_link (" << vlink->name <<
                      "): Fix dst: " << vlink->dst_iface << " == " <<
                      compare_iface << endl);
          }
      }


      // Get delay characteristics - NOTE: Currently does not actually do
      // anything
      tb_delay_info physical_delay;
      physical_delay.bandwidth = plink->delay_info.bandwidth - plink->bw_used;
      physical_delay.delay = plink->delay_info.delay;
      physical_delay.loss = plink->delay_info.loss;
      double distance = vlink->delay_info.distance(physical_delay);

      double available_bandwidth =  plink->delay_info.bandwidth - plink->bw_used;

      // For sticking emulated links in emulated links we care about the
      // distance, and whether or not we've gone over bandwidth
      int users;
      users = plink->nonemulated;
      if (! vlink->emulated) {
	users += plink->emulated;
      }

      if (distance == -1) {
	// -1 == infinity
	distance = DBL_MAX;
      }
      if (vlink->emulated) {
	// For emulated links, we need to do bin packing. Right now, we use the
	// first-fit approximation; there may be a better one
	
	// Skip any links that already have non-emulated links assigned to them
	if (plink->nonemulated > 0) {
	    continue;
	}
	if (available_bandwidth >= vlink->delay_info.bandwidth) {
	  best_pedge = *pedge_it;
	  found_best = true;
	  break;
	}
      } else {
	// For non-emulated links, we're just looking for links with few (0,
	// actually) users, and enough bandwidth (if we're adjusting the bw
        // on the vlink to match what's on the interfaces selected, we don't
        // even need to check bandwidth)
	if ((users < best_users) &&
                (vlink->delay_info.adjust_to_native_bandwidth ||
		 (plink->delay_info.bandwidth >= vlink->delay_info.bandwidth)
                 )) {
	  best_pedge = *pedge_it;
	  best_distance = distance;
	  found_best = true;
	  best_users = plink->emulated+plink->nonemulated;
          SDEBUG(cerr << "          find_best_link: picked " << plink->name <<
                  " with " << best_users << " users" << endl;)
	}
      }
    }
  }

  if ((!vlink->emulated) && found_best && (best_users > 0)) {
      SDEBUG(cerr << "      find_best_link failing (first case) (" <<
              vlink->emulated << "," << found_best << "," << best_users <<
              ")" << endl;)
      return false;
  }
  if (found_best) {
    out_edge = best_pedge;
    SDEBUG(cerr << "      find_best_link succeeding" << endl;)
    return true;
  } else {
    SDEBUG(cerr << "      find_best_link failing (second case)" << endl;)
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
  
  if (plink->is_type == tb_plink::PLINK_NORMAL) {
    // need to account for three things here, the possiblity of a new plink
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
      if ((! vlink->emulated) && (plink->nonemulated > 1)) {
	  SADD(SCORE_DIRECT_LINK_PENALTY);
	  vinfo.link_users++;
	  violated++;
      }
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

  if (plink->is_type != tb_plink::PLINK_LAN) {

    // Handle being over bandwidth
    // NOTE - this will have to change substantially when we can handle
    // asymmetric bandwidths on links.
    int old_over_bw;
    int old_bw = plink->bw_used;
    if (plink->bw_used > plink->delay_info.bandwidth) {
      old_over_bw = plink->bw_used - plink->delay_info.bandwidth;
    } else {
      old_over_bw = 0;
    }

    plink->bw_used += vlink->delay_info.bandwidth;

    int new_over_bw;
    if (plink->bw_used > plink->delay_info.bandwidth) {
      new_over_bw = plink->bw_used - plink->delay_info.bandwidth;
    } else {
      new_over_bw = 0;
    }
    
    if (new_over_bw) {
      // Count how many multiples of the maximum bandwidth we're at
      int num_violations =
	(int)(floor((double)((plink->bw_used -1) / plink->delay_info.bandwidth))
	      - floor((double)((old_bw -1)/plink->delay_info.bandwidth)));
      violated += num_violations;
      vinfo.bandwidth += num_violations;

      double added_bandwidth_percent = (new_over_bw - old_over_bw) * 1.0 /
	plink->delay_info.bandwidth;
      SADD(SCORE_OUTSIDE_DELAY * added_bandwidth_percent);
    }

#ifdef PENALIZE_BANDWIDTH
    SADD(plink->penalty * (vlink->delay_info.bandwidth * 1.0) / (plink->delay_info.bandwidth));
#endif

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
  if (vlink->link_info.type_used == tb_link_info::LINK_TRIVIAL) {
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

  if (plink->is_type == tb_plink::PLINK_NORMAL) {
    if (vlink->emulated) {
      plink->emulated--;
      SSUB(SCORE_EMULATED_LINK);
    } else {
      plink->nonemulated--;
      if (plink->nonemulated >= 1) {
	  //cerr << "Freeing overused link" << endl;
	  SSUB(SCORE_DIRECT_LINK_PENALTY);
	  vinfo.link_users--;
	  violated--;
      }
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
  if (plink->is_type != tb_plink::PLINK_LAN) {
#ifdef PENALIZE_BANDWIDTH
    SSUB(plink->penalty * (vlink->delay_info.bandwidth * 1.0) / (plink->delay_info.bandwidth));
#endif

    // Handle being over bandwidth
    int old_over_bw;
    int old_bw = plink->bw_used;
    if (plink->bw_used > plink->delay_info.bandwidth) {
      old_over_bw = plink->bw_used - plink->delay_info.bandwidth;
    } else {
      old_over_bw = 0;
    }

    plink->bw_used -= vlink->delay_info.bandwidth;

    int new_over_bw;
    if (plink->bw_used > plink->delay_info.bandwidth) {
      new_over_bw = plink->bw_used - plink->delay_info.bandwidth;
    } else {
      new_over_bw = 0;
    }

    if (old_over_bw) {
      // Count how many multiples of the maximum bandwidth we're at
      int num_violations = (int)
	(floor((double)((old_bw -1)/plink->delay_info.bandwidth))
	  - floor((double)((plink->bw_used -1) / plink->delay_info.bandwidth)));
      violated -= num_violations;
      vinfo.bandwidth -= num_violations;
      double removed_bandwidth_percent = (old_over_bw - new_over_bw) * 1.0 /
	plink->delay_info.bandwidth;
      SSUB(SCORE_OUTSIDE_DELAY * removed_bandwidth_percent);
    }
  }

UNSCORE_TRIVIAL:
  vlink->link_info.type_used = tb_link_info::LINK_UNMAPPED;
}

score_and_violations score_fds(tb_vnode *vnode, tb_pnode *pnode, bool add) {
    // Iterate through the set of features and desires on these nodes
    tb_featuredesire_set_iterator fdit(vnode->desires.begin(),
	    vnode->desires.end(), pnode->features.begin(),
	    pnode->features.end());

    // Keep track of the score and violations changes we rack up as we go along
    double score_delta = 0.0f;
    int violations_delta = 0;

    for (;!fdit.done();fdit++) {
	node_fd_set::iterator fit, dit;
	dit = fdit.first_iterator();
	fit = fdit.second_iterator();
	// What we do here depends on whether it's a feature of the vnode, a desire
	// of the pnode, or both
	switch (fdit.membership()) {
	    case tb_featuredesire_set_iterator::BOTH:
		/*
		 * On both 
		 */
		// note: Right now, global features cannot be
		// desires, so there is no code path for them here
		if (fdit->is_local()) {
		    if (fdit->is_l_additive()) {
			score_and_violations delta;
			if (add) {
			    delta = fit->add_local(dit->cost());
			} else {
			    delta = fit->subtract_local(dit->cost());
			}
			score_delta += delta.first;
			violations_delta += delta.second;
		    }
		} else {
		    // Just a normal feature or desire - since both have it,
		    // no need to do any scoring
		}
		break;
	    case tb_featuredesire_set_iterator::FIRST_ONLY:
		/*
		 * On the vnode, but not the pnode
		 */
		// May be a violation to not have the matching feature
		if (fdit->is_violateable()) {
		    violations_delta++;
		}
		if (fdit->is_local()) {
		    // We handle local features specially
		    score_delta += SCORE_MISSING_LOCAL_FEATURE;
		} else {
		    score_delta += fdit->cost();
		}
		break;
	    case tb_featuredesire_set_iterator::SECOND_ONLY:
		/*
		 * On the pnode, but not the vnode
		 */
		// What we do here depends on what kind o feature it is
		if (fdit->is_local()) { 
		    // Do nothing for local features that are not matched -
		    // they are free to waste
		} else if (fdit->is_global()) {
		    // Global feature - we have to look at what others are in
		    // use
		    score_and_violations delta;
		    if (add) {
			delta = fdit->add_global_user();
		    } else {
			delta = fdit->remove_global_user();
		    }
		    score_delta += delta.first;
		    violations_delta += delta.second;
		} else {
		    // Regular feature - score the sucker
		    score_delta += fdit->cost();
		    if (fdit->is_violateable()) {
			violations_delta++;
		    }
		}
		break;
	    default:
		cerr << "*** Internal error - bad set membership" << endl;
		exit(EXIT_FATAL);
	}
    }

    // Okay, return the score and violations
    //cerr << "Returning (" << score_delta << "," << violations_delta << ")" <<
    //   endl;
    return score_and_violations(score_delta,violations_delta);
}

