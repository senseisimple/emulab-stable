/*
 * ASSUMPTIONS:
 *  1. Any switch can get to any other switch either directly
 *     or via at most one other switch (star formation).
 */

// Not sure we need all these LEDA includes.
#include <LEDA/graph_alg.h>
#include <LEDA/graphwin.h>
#include <LEDA/dictionary.h>
#include <LEDA/map.h>
#include <LEDA/graph_iterator.h>
#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#include "common.h"
#include "score.h"
#include "virtual.h"
#include "physical.h"

#include "assert.h"
#define ASSERT assert

float score;			// The score of the current mapping
int violated;			// How many times the restrictions
				// have been violated.
node pnodes[MAX_PNODES];	// int->node map
				// pnodes[0] == NULL

extern tb_vgraph G;		// virtual graph
extern tb_pgraph PG;		// physical grpaph

int find_interswitch_path(node src,node dst,int bandwidth,edge *f,edge *s);
edge *direct_link(node a,node b);

/*
 * score()
 * Returns the score.
 */
float get_score() {return score;}

/*
 * init_score()
 * This initialized the scoring system.  It also clears all
 * assignments.
 */
void init_score()
{
  score=0;
  violated=0;

  node n;
  edge e;
  forall_nodes(n,G) {
    tb_vnode &vn=G[n];
    vn.posistion=0;
    vn.no_connections=0;
    score += SCORE_UNASSIGNED;
    violated++;
  }
  forall_edges(e,G) {
    tb_vlink &ve=G[e];
    ve.type=tb_vlink::LINK_UNKNOWN;
  }
  forall_nodes(n,PG) {
    tb_pnode &pn=PG[n];
    pn.current_type=TYPE_UNKNOWN;
    pn.current_load=0;
    pn.pnodes_used=0;
  }
  forall_edges(e,PG) {
    tb_plink &pe=PG[e];
    pe.bw_used=0;
    pe.users=0;
  }

  ASSERT(pnodes[0] == NULL);
}

/*
 * void remove_node(node node)
 * This removes a virtual node from the assignments, adjusting
 * the score appropriately.
 */
void remove_node(node n)
{
  /* Find pnode assigned to */
  node pnode;

  tb_vnode &vnoder = G[n];
  pnode = pnodes[vnoder.posistion];
  tb_pnode &pnoder = PG[pnode];
  
  ASSERT(pnode != NULL);

  edge e;
  tb_vlink *vlink;
  node vdst;
  tb_vnode *vdstr;
  node pdst;
  tb_pnode *pdstr;

  // remove the scores associated with each edge
  forall_inout_edges(e,n) {
    vlink=&G[e];
    vdst=G.target(e);
    if (vdst == n)
      vdst = G.source(e);
    vdstr=&G[vdst];
    if (vlink->type == tb_vlink::LINK_DIRECT) {
      // DIRECT LINK
      PG[vlink->plink].users--;
      if (PG[vlink->plink].users == 0) {
	// link no longer used
	score -= SCORE_DIRECT_LINK;
      } else {
	// getting close to no violations
	score -= SCORE_DIRECT_LINK_PENALTY;
	violated--;
      }
    } else if (vlink->type == tb_vlink::LINK_INTERSWITCH) {
      // INTERSWITCH LINK
      pdst=pnodes[vdstr->posistion];
      pdstr=&PG[pdst];
      edge first,second;
      first = vlink->plink;
      second = vlink->plink_two;

      // remove bandwidth from edges      
      PG[first].bw_used -= vlink->bandwidth;
      if (second)
	PG[second].bw_used -= vlink->bandwidth;

      // XXX add check for over bandwidth

      // adjust score apropriately.
      score -= SCORE_INTERSWITCH_LINK;
      if (second) 
	score -= SCORE_INTERSWITCH_LINK;
    } else {
      // INTRASWITCH LINK
      ASSERT(vlink->type == tb_vlink::LINK_INTRASWITCH);
      score -= SCORE_INTRASWITCH_LINK;
    }
  }

  // remove scores associated with the node
  score -= SCORE_NO_CONNECTION * vnoder.no_connections;
  violated -= vnoder.no_connections;

  // adjust pnode scores
  pnoder.current_load--;
  if (pnoder.current_load == 0) {
    // release pnode
    score -= SCORE_PNODE;
    node the_switch =pnoder.the_switch;
    if ((PG[the_switch].pnodes_used--) == 0) {
      // release switch
      score -= SCORE_SWITCH;
    }
    // revert pnode type
    pnoder.current_type=TYPE_UNKNOWN;
  } else if (pnoder.current_load >= pnoder.max_load) {
    score -= SCORE_PNODE_PENALTY;
    violated--;
  }
  // add score for unassigned node
  score += SCORE_UNASSIGNED;
  violated++;
}

/*
 * int add_node(node node,int ploc)
 * Add a mapping of node to ploc and adjust score appropriately.
 * Returns 1 in the case of an incompatible mapping.  This should
 * never happen as the same checks should be in place in a higher
 * level.  (Optimization?)
 */
int add_node(node n,int ploc)
{
  tb_vnode &vnoder=G[n];
  node pnode = pnodes[ploc];
  tb_pnode &pnoder=PG[pnode];

  // set up pnode
  // figure out type
  if (pnoder.current_type != TYPE_UNKNOWN) {
    return 1;
  }
  
  // !!!: This code could be improved
  // Hard code types to ints for efficiency?
  // Remove check assuming at higher level?
  // Remove higher level checks?
  // XXX fix this.
#if 0
  pnoder.max_load=0;
  for (int i=0;i<MAX_TYPES;++i) {
    if (pnoder.types[i] &&
	strcmp(pnoder.types[i].name,n->type.name) == 0) {
      pnoder.current_type=i;
      pnoder.max_load=pnoder.types[i].max;
      break;
    }
  }
#endif
  if (pnoder.max_load == 0) {
    // didn't find a type
    return 1;
  }

  // set up links
  vnoder.no_connections=0;
  edge e;
  node dst;
  tb_vlink *er;
  tb_plink *pl;
  edge *pedge;
  forall_inout_edges(e,n) {
    dst=G.source(e);
    er=&G[e];
    if (dst == n) {
      dst=G.target(e);
    }
    tb_vnode &dstr=G[dst];

    if (dstr.posistion != 0) {
      // dstr is assigned
      node dpnode=pnodes[dstr.posistion];
      tb_pnode &dpnoder=PG[pnode];

      // XXX - choose method of getting there and set up links
      // XXX - this is all wrong, need to distirnguish between
      // plinks and vlinks.  AAAA!!!
      // XXX - 5/00 - Really need to write thise code.
      if ((pedge=direct_link(dpnode,pnode)) != NULL) {
	pl = &PG[*pedge];
	// direct
	er->type = tb_vlink::LINK_DIRECT;
	er->plink = *pedge;
	pl->users++;
	if (pl->users == 1) {
	  score += SCORE_DIRECT_LINK;
	} else {
	  score += SCORE_DIRECT_LINK_PENALTY;
	  violated++;
	}
      } else if (pnoder.the_switch == dpnoder.the_switch) {
	// intraswitch
	er->type = tb_vlink::LINK_INTRASWITCH;
	score += SCORE_INTRASWITCH_LINK;
      } else {
	// try to find interswitch
	edge first,second;
	first=second=NULL;
	if (find_interswitch_path(pnoder.the_switch,dpnoder.the_switch,
				  er->bandwidth,
				  &first,&second) == 0) {
	  // couldn't fidn path.
	  vnoder.no_connections++;
	  score += SCORE_NO_CONNECTION;
	  violated++;
 	} else {
	  er->type=tb_vlink::LINK_INTERSWITCH;
	  score += SCORE_INTERSWITCH_LINK;
	  if (second)
	    score += SCORE_INTERSWITCH_LINK;

	  // add bandwidth usage
	  PG[first].bw_used += er->bandwidth;
	  if (second)
	    PG[second].bw_used += er->bandwidth;

	  er->plink = first;
	  er->plink_two = second;
	  
	  // XXX add checking for over bandwidth
	}
      }
    }
  }
    
  // finish setting up pnode
  pnoder.current_load++;
  if (pnoder.current_load > pnoder.max_load) {
    score += SCORE_PNODE_PENALTY;
    violated++;
  } else {
    score += SCORE_PNODE;
    if ((PG[pnoder.the_switch].pnodes_used++) == 1) {
      score += SCORE_SWITCH;
    }
  }

  // node no longer unassigned
  score -= SCORE_UNASSIGNED;
  violated--;

  return 0;
}

// returns "best" direct link between a and b.
// best = less users
//        break ties with minimum bw_used
edge *direct_link(node a,node b)
{
  node dst;
  edge e;
  edge *best = NULL;
  tb_plink *pl;
  tb_plink *bestpl = NULL;
  forall_inout_edges(e,a) {
    dst=PG.target(e);
    if (dst == a)
      dst=PG.source(e);
    if (dst == b) {
      pl = &PG[e];
      if (pl->users < bestpl->users ||
	  (pl->users == bestpl->users &&
	   pl->bw_used < bestpl->bw_used)) {
	best = &e;
	bestpl = pl;
      }
    }
  }
  return best;
}

// this needs to find a path between switches src and dst (in PG).
// needs to return the best of all possible
// best =
//   shorter is better
//   lowest % of bw used after link added is better.
int find_interswitch_path(node src,node dst,int bandwidth,edge *f,edge *s)
{
  edge *best_first,*best_second;
  float best_bw;
  float bw;
  
  best_bw=100.0;
  best_first = best_second = NULL;
  
  // try to find a path to the destination
  node ldst,ldstb;
  edge first,second;
  forall_inout_edges(first,src) {
    ldst = PG.target(first);
    if (ldst == src)
      ldst = PG.source(first);
    if (ldst == dst) {
      // we've found a path, it's just firstedge
      if (first)
	bw = (PG[first].bw_used+bandwidth)/PG[first].bandwidth;
      if (! best_first || bw < best_bw) {
	best_first = &first;
	best_bw = bw;
	best_second = NULL;
      }
    }
    forall_inout_edges(second,ldst) {
      ldstb = PG.target(second);
      if (ldstb == ldst)
	ldstb = PG.source(second);
      if (ldstb == dst) {
	if (! best_second) continue;
	bw = (PG[first].bw_used+bandwidth)/PG[first].bandwidth;
	if (second)
	  // NOTE: One thing to try differently.
	  bw *= (PG[second].bw_used+bandwidth)/PG[second].bandwidth;
	if (bw < best_bw) {
	  best_first = &first;
	  best_second = &second;
	  best_bw = bw;
	}
      }
    }
  }
  // if we get here we didn't find a path
  if (best_first) {
    *f = *best_first;
    if (best_second)
      *s = *best_second;
    return 1;
  } else {
    return 0;
  }
}
