/*
 * ASSUMPTIONS:
 *  1. Any switch can get to any other switch either directly
 *     or via at most one other switch (star formation).
 */

// Note on variable names: LEDA has generic 'edge' and 'node'.  When
// these are translated to 'tb_*' structures the variables end in
// r.  I.e. dst -> dstr.  dst is a node, and dstr is a tb_pnode or similar.

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

float score;			// The score of the current mapping
int violated;			// How many times the restrictions
				// have been violated.
node pnodes[MAX_PNODES];	// int->node map
				// pnodes[0] == NULL

violated_info vinfo;		// specific info on violations

extern tb_vgraph G;		// virtual graph
extern tb_pgraph PG;		// physical grpaph

int find_interswitch_path(node src,node dst,int bandwidth,edge *f,edge *s);
edge direct_link(node a,node b);
void score_link(edge e,edge v,bool interswitch);
void unscore_link(edge e,edge v,bool interswitch);
edge find_link_to_switch(node n);
int find_intraswitch_path(node src,node dst,edge *first,edge *second);

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
  vinfo.unassigned = vinfo.pnode_load = 0;
  vinfo.no_connection = vinfo.link_users = vinfo.bandwidth = 0;

  node n;
  edge e;
  forall_nodes(n,G) {
    tb_vnode &vn=G[n];
    vn.posistion=0;
    vn.no_connections=0;
    SADD(SCORE_UNASSIGNED);
    vinfo.unassigned++;
    violated++;
  }
  forall_edges(e,G) {
    tb_vlink &ve=G[e];
    ve.type=tb_vlink::LINK_UNKNOWN;
  }
  forall_nodes(n,PG) {
    tb_pnode &pn=PG[n];
    pn.typed=false;
    pn.current_load=0;
    pn.pnodes_used=0;
  }
  forall_edges(e,PG) {
    tb_plink &pe=PG[e];
    pe.bw_used=0;
    pe.users=0;
  }

  assert(pnodes[0] == NULL);
#ifdef SCORE_DEBUG
  fprintf(stderr,"  score = %.2f violated = %d\n",score,violated);
#endif
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

  
  assert(pnode != NULL);

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
      unscore_link(vlink->plink,e,false);
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

      unscore_link(first,e,true);
      if (second) unscore_link(second,e,true);
      unscore_link(vlink->plink_local_one,e,false);
      unscore_link(vlink->plink_local_two,e,false);

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

      unscore_link(vlink->plink,e,false);
      unscore_link(vlink->plink_two,e,false); 
    } else {
      // No link
      fprintf(stderr,"Internal error - no link\n");
      abort();
    }
  }

  // remove scores associated with the node
  SSUB(SCORE_NO_CONNECTION*vnoder.no_connections);
  violated -= vnoder.no_connections;
  vinfo.no_connection -= vnoder.no_connections;

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
    pnoder.typed=false;
  } else if (pnoder.current_load >= pnoder.max_load) {
#ifdef SCORE_DEBUG
    fprintf(stderr,"  reducing penalty, new load = %d (>= %d)\n",pnoder.current_load,pnoder.max_load);
#endif
    SSUB(SCORE_PNODE_PENALTY);
    vinfo.pnode_load--;
    violated--;
  }
  // add score for unassigned node
  SADD(SCORE_UNASSIGNED);
  vinfo.unassigned++;
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
  fprintf(stderr,"  vnode type = ");
  cerr << vnoder.type << " pnode switch = ";
  cerr << (pnoder.the_switch ? PG[pnoder.the_switch].name : "No switch");
  cerr << endl;
#endif
  
  // set up pnode
  // figure out type
  if (!pnoder.typed) {
#ifdef SCORE_DEBUG
    fprintf(stderr,"  virgin pnode\n");
#endif
    
    // Remove check assuming at higher level?
    // Remove higher level checks?
    pnoder.max_load=0;
    pnoder.current_type=vnoder.type;
    pnoder.typed=true;
    pnoder.max_load = pnoder.types.access(vnoder.type);
    
    if (pnoder.max_load == 0) {
      // didn't find a type
#ifdef SCORE_DEBUG
      fprintf(stderr,"  no matching type\n");
#endif
      return 1;
    }
#ifdef SCORE_DEBUG
    fprintf(stderr,"  matching type found (");
    cerr << pnoder.current_type << ", max = " << pnoder.max_load;
    cerr << ")" << endl;
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
      if (pnoder.current_load == pnoder.max_load) {
#ifdef SCORE_DEBUG
	fprintf(stderr,"  node is full\n");
#endif
	return 1;
      }
      
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
    assert(dst != n);
    assert(&dstr != &vnoder);
#ifdef SCORE_DEBUG
    fprintf(stderr,"  edge to %s\n",dstr.name);
#endif

    if (dstr.posistion != 0) {
      // dstr is assigned
      node dpnode=pnodes[dstr.posistion];
      tb_pnode &dpnoder=PG[dpnode];

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

	score_link(pedge,e,false);
      } else if (pnoder.the_switch &&
		 (pnoder.the_switch == dpnoder.the_switch)) {
	// intraswitch
	assert(pnoder.the_switch == dpnoder.the_switch);
	edge first,second;
	if (find_intraswitch_path(pnode,dpnode,&first,&second) == 1) {
	  fprintf(stderr,"Internal error: Could not find intraswitch link!\n");
	  abort();
	}

	assert(first != NULL);
	assert(second != NULL);
	
#ifdef SCORE_DEBUG
	fprintf(stderr,"   found intraswitch link (%p,%p)\n",first,second);
#endif
	er->type = tb_vlink::LINK_INTRASWITCH;
	er->plink = first;
	er->plink_two = second;
	SADD(SCORE_INTRASWITCH_LINK);

	// check users and bandwidth
	score_link(first,e,false);
	score_link(second,e,false);
      } else {
	// try to find interswitch
	// XXX - need to do user and bandwidth checking for links to
	// and from node as well!
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
	  // XXX - this is a hack for the moment
	  pnoder.typed = false;
	  return 1;
	  
	  // couldn't find path.
	  vnoder.no_connections++;
	  SADD(SCORE_NO_CONNECTION);
	  vinfo.no_connection++;
	  violated++;
 	} else {
#ifdef SCORE_DEBUG
	  fprintf(stderr,"   found interswitch link (%p, %p)\n",first,second);
#endif
	  er->type=tb_vlink::LINK_INTERSWITCH;
	  SADD(SCORE_INTERSWITCH_LINK);
	  if (second)
	    SADD(SCORE_INTERSWITCH_LINK);

	  score_link(first,e,true);
	  if (second) score_link(second,e,true);

	  er->plink = first;
	  er->plink_two = second;

	  er->plink_local_one = find_link_to_switch(pnode);
	  assert(er->plink_local_one != NULL);
	  er->plink_local_two = find_link_to_switch(dpnode);
	  assert(er->plink_local_two != NULL);

	  score_link(er->plink_local_one,e,false);
	  score_link(er->plink_local_two,e,false);
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
    vinfo.pnode_load++;
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
  vinfo.unassigned--;
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

edge find_link_to_switch(node n)
{
  tb_pnode &nr = PG[n];

  edge e;
  node edst;
  float best_bw=1000.0;
  int best_users = 1000;
  float bw;
  edge best=NULL;
  forall_inout_edges(e,n) {
    edst = PG.target(e);
    if (edst == n)
      edst = PG.source(e);
    if (edst == nr.the_switch) {
      tb_plink &er = PG[e];
      bw = er.bw_used / er.bandwidth;
      if ((er.users < best_users) ||
	  ((er.users == best_users) && (bw < best_bw))) {
	best = e;
	best_bw = bw;
	best_users = er.users;
      }
    }
  }
  
  return best;
}

// this looks for the best links between the src and the switch
// and the dst and the switch.
int find_intraswitch_path(node src,node dst,edge *first,edge *second)
{
  tb_pnode &srcr=PG[src];
  tb_pnode &dstr=PG[dst];

  assert(srcr.the_switch == dstr.the_switch);
  assert(srcr.the_switch != NULL);
  assert(src != dst);

  *first = find_link_to_switch(src);
  *second = find_link_to_switch(dst);

  if ((*first != NULL) &&
      (*second != NULL)) return 0;
  return 1;
}

// this needs to find a path between switches src and dst (in PG).
// needs to return the best of all possible
// best =
//   shorter is better
//   lowest % of bw used after link added is better.
int find_interswitch_path(node src,node dst,int bandwidth,edge *f,edge *s)
{
  edge best_first,best_second;
  float best_bw;
  float bw;
  int best_length = 1000;
  
  best_bw=100.0;
  best_first = best_second = NULL;


  if (!src || ! dst) return 0;
  
  // try to find a path to the destination
  node ldst,ldstb;
  edge first,second;
#ifdef SCORE_DEBUG
  tb_pnode *ldstr,*ldstbr;
  tb_plink *firstr,*secondr;
#endif
  forall_inout_edges(first,src) {
    ldst = PG.target(first);
    if (ldst == src)
      ldst = PG.source(first);
#ifdef SCORE_DEBUG
    ldstr = &PG[ldst];
    firstr = &PG[first];
#endif
    if (ldst == dst) {
      // we've found a path, it's just firstedge
      if (first)
	bw = (PG[first].bw_used+bandwidth)/PG[first].bandwidth;
      if (! best_first || bw < best_bw) {
	best_first = first;
	best_bw = bw;
	best_second = NULL;
	best_length = 1;
      }
    }
    forall_inout_edges(second,ldst) {
      ldstb = PG.target(second);
      if (ldstb == ldst)
	ldstb = PG.source(second);
#ifdef SCORE_DEBUG
      ldstbr = &PG[ldstb];
      secondr = &PG[second];
#endif
      if (ldstb == dst) {
	if (best_length == 1) continue;
	bw = (PG[first].bw_used+bandwidth)/PG[first].bandwidth;
	if (second)
	  // NOTE: One thing to try differently.
	  bw *= (PG[second].bw_used+bandwidth)/PG[second].bandwidth;
	if (bw < best_bw) {
	  best_first = first;
	  best_second = second;
	  best_bw = bw;
	  best_length = 2;
	}
      }
    }
  }
  // if we get here we didn't find a path
  if (best_first) {
    *f = best_first;
    if (best_second)
      *s = best_second;
    return 1;
  } else {
    return 0;
  }
}

// this does scoring for over users and over bandwidth on edges.
void score_link(edge e,edge v,bool interswitch)
{
  tb_plink &pl = PG[e];
  tb_vlink &er = G[v];

#ifdef SCORE_DEBUG
  fprintf(stderr,"  score_link(%p)\n",e);
#endif

  if (! interswitch) {
    pl.users++;
    if (pl.users == 1) {
#ifdef SCORE_DEBUG
      fprintf(stderr,"    first user\n");
#endif
      SADD(SCORE_DIRECT_LINK);
    } else {
#ifdef SCORE_DEBUG
      fprintf(stderr,"    not first user - penalty\n");
#endif
      SADD(SCORE_DIRECT_LINK_PENALTY);
      vinfo.link_users++;
      violated++;
    }
  }
  
  // bandwidth
  int prev_bw = pl.bw_used;
  pl.bw_used += er.bandwidth;
  if ((pl.bw_used > pl.bandwidth) &&
      (prev_bw <= pl.bandwidth)) {
#ifdef SCORE_DEBUG
    fprintf(stderr,"    went over bandwidth (%d > %d)\n",
	    pl.bw_used,pl.bandwidth);
#endif
    violated++;
    vinfo.bandwidth++;
    SADD(SCORE_OVER_BANDWIDTH);
  }
}

void unscore_link(edge e,edge v,bool interswitch)
{
  tb_plink &pl = PG[e];
  tb_vlink &er = G[v];

#ifdef SCORE_DEBUG
  fprintf(stderr,"  unscore_link(%p)\n",e);
#endif

  if (!interswitch) {
    pl.users--;
    if (pl.users == 0) {
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
      vinfo.link_users--;
      violated--;
    }
  }
  
  // bandwidth check
  int prev_bw = pl.bw_used;
  pl.bw_used -= er.bandwidth;
  if ((pl.bw_used <= pl.bandwidth) &&
      (prev_bw > pl.bandwidth)) {
#ifdef SCORE_DEBUG
    fprintf(stderr,"   went under bandwidth (%d <= %d)\n",
	    pl.bw_used,pl.bandwidth);
#endif
    violated--;
    vinfo.bandwidth--;
    SSUB(SCORE_OVER_BANDWIDTH);
  }
}
