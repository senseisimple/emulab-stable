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
edge direct_link(node a,node b);

#ifdef SCORE_DEBUG_MORE
#define SADD(amount) fprintf(stderr,"SADD: %s = %.2f\n",#amount,amount);score+=amount
#define SSUB(amount) fprintf(stderr,"SSUB: %s = %.2f\n",#amount,amount);score-=amount
#else
#define SADD(amount) score += amount
#define SSUB(amount) score -= amount
#endif

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
#ifdef SCORE_DEBUG
  fprintf(stderr,"SCORE: Initializing\n");
#endif
  score=0;
  violated=0;

  node n;
  edge e;
  forall_nodes(n,G) {
    tb_vnode &vn=G[n];
    vn.posistion=0;
    vn.no_connections=0;
    SADD(SCORE_UNASSIGNED);
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
  fprintf(stderr,"  score = %.2f violated = %d\n",score,violated);
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

#ifdef SCORE_DEBUG
  fprintf(stderr,"SCORE: remove_node(%s)\n",vnoder.name);
  fprintf(stderr,"       no_connections = %d\n",vnoder.no_connections);
#endif

  
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
#ifdef SCORE_DEBUG
    fprintf(stderr,"  edge to %s\n",vdstr->name);
#endif
    if (vdstr->posistion == 0) continue;
    if (vlink->type == tb_vlink::LINK_DIRECT) {
      // DIRECT LINK
#ifdef SCORE_DEBUG
      fprintf(stderr,"   direct link\n");
#endif
      PG[vlink->plink].users--;
      if (PG[vlink->plink].users == 0) {
	// link no longer used
#ifdef SCORE_DEBUG
	fprintf(stderr,"   freeing link\n");
#endif
	SSUB(SCORE_DIRECT_LINK);
      } else {
	// getting close to no violations
#ifdef SCORE_DEBUG
	fprintf(stderr,"   reducing users\n");
#endif
	SSUB(SCORE_DIRECT_LINK_PENALTY);
	violated--;
      }
    } else if (vlink->type == tb_vlink::LINK_INTERSWITCH) {
      // INTERSWITCH LINK
      pdst=pnodes[vdstr->posistion];
      pdstr=&PG[pdst];
      edge first,second;
      first = vlink->plink;
      second = vlink->plink_two;
#ifdef SCORE_DEBUG
      node through;
      if (PG.source(first) == pnode)
	through = PG.target(first);
      else
	through = PG.source(first);
      fprintf(stderr,"   interswitch link (through %s)\n",
	      PG[through].name);
#endif
      

      // remove bandwidth from edges      
      PG[first].bw_used -= vlink->bandwidth;
      if (second)
	PG[second].bw_used -= vlink->bandwidth;

      // XXX add check for over bandwidth

      // adjust score apropriately.
      SSUB(SCORE_INTERSWITCH_LINK);
      if (second)
	SSUB(SCORE_INTERSWITCH_LINK);
    } else if (vlink->type == tb_vlink::LINK_INTRASWITCH) {
      // INTRASWITCH LINK
#ifdef SCORE_DEBUG
      fprintf(stderr,"   intraswitch link\n");
#endif
      SSUB(SCORE_INTRASWITCH_LINK);
    } else {
      // No link
      assert(0);
    }
  }

  // remove scores associated with the node
  SSUB(SCORE_NO_CONNECTION*vnoder.no_connections);
  violated -= vnoder.no_connections;

  // adjust pnode scores
  pnoder.current_load--;
  vnoder.posistion = 0;
  if (pnoder.current_load == 0) {
    // release pnode
#ifdef SCORE_DEBUG
    fprintf(stderr,"  releasing pnode\n");
#endif
    SSUB(SCORE_PNODE);
    node the_switch=pnoder.the_switch;
    if (the_switch) {
      if ((--PG[the_switch].pnodes_used) == 0) {
#ifdef SCORE_DEBUG
	fprintf(stderr,"  releasing switch %s\n",PG[the_switch].name);
#endif
	// release switch
	SSUB(SCORE_SWITCH);
      }
    }
    // revert pnode type
    pnoder.current_type=TYPE_UNKNOWN;
  } else if (pnoder.current_load >= pnoder.max_load) {
#ifdef SCORE_DEBUG
    fprintf(stderr,"  reducing penalty, new load = %d (>= %d)\n",pnoder.current_load,pnoder.max_load);
#endif
    SSUB(SCORE_PNODE_PENALTY);
    violated--;
  }
  // add score for unassigned node
  SADD(SCORE_UNASSIGNED);
  violated++;
#ifdef SCORE_DEBUG
  fprintf(stderr,"  new score = %.2f  new violated = %d\n",score,violated);
#endif
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

#ifdef SCORE_DEBUG
  fprintf(stderr,"SCORE: add_node(%s,%s[%d])\n",
	  vnoder.name,pnoder.name,ploc);
  fprintf(stderr,"  vnode type = %d pnode switch = %s\n",vnoder.type,
	  (pnoder.the_switch ? PG[pnoder.the_switch].name : "No switch"));
  
#endif
  
  // set up pnode
  // figure out type
  if (pnoder.current_type == TYPE_UNKNOWN) {
#ifdef SCORE_DEBUG
    fprintf(stderr,"  virgin pnode\n");
#endif
    
    // Remove check assuming at higher level?
    // Remove higher level checks?
    pnoder.max_load=0;
    pnoder.current_type=vnoder.type;
    pnoder.max_load = pnoder.types[vnoder.type].max;
    
    if (pnoder.max_load == 0) {
      // didn't find a type
#ifdef SCORE_DEBUG
      fprintf(stderr,"  no matching type\n");
#endif
      return 1;
    }
#ifdef SCORE_DEBUG
    fprintf(stderr,"  matching type found (%d, max = %d)\n",
	    pnoder.current_type,pnoder.max_load);
#endif
  } else {
#ifdef SCORE_DEBUG
    fprintf(stderr,"  pnode already has type\n");
#endif
    if (pnoder.current_type != vnoder.type) {
#ifdef SCORE_DEBUG      
      fprintf(stderr,"  incompatible types\n");
#endif
      return 1;
    } else {
#ifdef SCORE_DEBUG
      fprintf(stderr,"  comaptible types\n");
#endif
      ; 
      
    }
  }

  // set up links
  vnoder.no_connections=0;
  edge e;
  node dst;
  tb_vlink *er;
  tb_plink *pl;
  edge pedge;
  forall_inout_edges(e,n) {
    dst=G.source(e);
    er=&G[e];
    if (dst == n) {
      dst=G.target(e);
    }
    tb_vnode &dstr=G[dst];
#ifdef SCORE_DEBUG
    fprintf(stderr,"  edge to %s\n",dstr.name);
#endif

    if (dstr.posistion != 0) {
      // dstr is assigned
      node dpnode=pnodes[dstr.posistion];
      tb_pnode &dpnoder=PG[pnode];

#ifdef SCORE_DEBUG
      fprintf(stderr,"   goes to %s\n",dpnoder.name);
#endif

      if ((pedge=direct_link(dpnode,pnode)) != NULL) {
#ifdef SCORE_DEBUG
	fprintf(stderr,"   found direct link = %p\n",pedge);
#endif
	pl = &PG[pedge];
	// direct
	er->type = tb_vlink::LINK_DIRECT;
	er->plink = pedge;
	pl->users++;
	if (pl->users == 1) {
#ifdef SCORE_DEBUG
	  fprintf(stderr,"    first user\n");
#endif
	  SADD(SCORE_DIRECT_LINK);
	} else {
#ifdef SCORE_DEBUG
	  fprintf(stderr,"    not first user - penalty\n");
#endif
	  SADD(SCORE_DIRECT_LINK_PENALTY);
	  violated++;
	}
      } else if (pnoder.the_switch &&
		 pnoder.the_switch == dpnoder.the_switch) {
	// intraswitch
#ifdef SCORE_DEBUG
	fprintf(stderr,"   found intraswitch link\n");
#endif
	er->type = tb_vlink::LINK_INTRASWITCH;
	SADD(SCORE_INTRASWITCH_LINK);
      } else {
	// try to find interswitch
#ifdef SCORE_DEBUG
	fprintf(stderr,"   looking for interswitch link\n");
#endif
	edge first,second;
	first=second=NULL;
	if (find_interswitch_path(pnoder.the_switch,dpnoder.the_switch,
				  er->bandwidth,
				  &first,&second) == 0) {
#ifdef SCORE_DEBUG
	  fprintf(stderr,"   could not find path - no connection\n");
#endif
	  // couldn't fidn path.
	  vnoder.no_connections++;
	  SADD(SCORE_NO_CONNECTION);
	  violated++;
 	} else {
#ifdef SCORE_DEBUG
	  fprintf(stderr,"   found interswitch link (%p, %p)\n",first,second);
#endif
	  er->type=tb_vlink::LINK_INTERSWITCH;
	  SADD(SCORE_INTERSWITCH_LINK);
	  if (second)
	    SADD(SCORE_INTERSWITCH_LINK);

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
  vnoder.posistion = ploc;
  if (pnoder.current_load > pnoder.max_load) {
#ifdef SCORE_DEBUG
    fprintf(stderr,"  load to high - penalty (%d)\n",pnoder.current_load);
#endif
    SADD(SCORE_PNODE_PENALTY);
    violated++;
  } else {
#ifdef SCORE_DEBUG
    fprintf(stderr,"  load is fine\n");
#endif
  }
  if (pnoder.current_load == 1) {
#ifdef SCORE_DEBUG
    fprintf(stderr,"  new pnode\n");
#endif
    SADD(SCORE_PNODE);
    if (pnoder.the_switch &&
	(++PG[pnoder.the_switch].pnodes_used) == 1) {
#ifdef SCORE_DEBUG
      fprintf(stderr,"  new switch\n");
#endif
      SADD(SCORE_SWITCH);
    }
  }

  // node no longer unassigned
  SSUB(SCORE_UNASSIGNED);
  violated--;

#ifdef SCORE_DEBUG
  fprintf(stderr,"  posistion = %d\n",vnoder.posistion);
  fprintf(stderr,"  new score = %.2f  new violated = %d\n",score,violated);
#endif
  return 0;
}

// returns "best" direct link between a and b.
// best = less users
//        break ties with minimum bw_used
edge direct_link(node a,node b)
{
  node dst;
  edge e;
  edge best = NULL;
  tb_plink *pl;
  tb_plink *bestpl = NULL;
  forall_inout_edges(e,a) {
    dst=PG.target(e);
    if (dst == a)
      dst=PG.source(e);
    if (dst == b) {
      pl = &PG[e];
      if (! bestpl ||
	  (pl->users < bestpl->users ||
	   (pl->users == bestpl->users &&
	    pl->bw_used < bestpl->bw_used))) {
	best = e;
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

  if (!src || ! dst) return 0;
  
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
