/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

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
#include <LEDA/ugraph.h>
#include <LEDA/graphwin.h>
#include <LEDA/dictionary.h>
#include <LEDA/map.h>
#include <LEDA/graph_iterator.h>
#include <LEDA/sortseq.h>
#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#include "common.h"
#include "vclass.h"
#include "virtual.h"
#include "physical.h"
#include "pclass.h"
#include "score.h"


#include "assert.h"

typedef node_array<edge> switch_pred_array;
extern node_array<switch_pred_array> switch_preds;

float score;			// The score of the current mapping
int violated;			// How many times the restrictions
				// have been violated.
node pnodes[MAX_PNODES];	// int->node map
				// pnodes[0] == NULL

violated_info vinfo;		// specific info on violations

extern tb_vgraph G;		// virtual graph
extern tb_pgraph PG;		// physical grpaph
extern tb_sgraph SG;		// switch fabric

edge direct_link(node a,node b);
void score_link(edge e,edge v,bool interswitch);
void unscore_link(edge e,edge v,bool interswitch);
edge find_link_to_switch(node n);
int find_intraswitch_path(node src,node dst,edge *first,edge *second);
int find_interswitch_path(node src,node dst,int bandwidth,list<edge> &L);

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
    ve.plink=NULL;
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
    pe.emulated=0;
    pe.nonemulated=0;
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
  cerr <<  "SCORE: remove_node(" << vnoder.name << ")\n";
  fprintf(stderr,"       no_connections = %d\n",vnoder.no_connections);
#endif

  assert(pnode != NULL);

  pclass_unset(pnoder);

  // pclass
  if (pnoder.my_class->used == 0) {
#ifdef SCORE_DEBUG
    cerr << "  freeing pclass\n";
#endif
    SSUB(SCORE_PCLASS);
  }

  // vclass
  if (vnoder.vclass != NULL) {
    double score_delta = vnoder.vclass->unassign_node(vnoder.type);
#ifdef SCORE_DEBUG
    cerr << "  vclass unassign " << score_delta << endl;
#endif
    
    if (score_delta <= -1) {
      violated--;
      vinfo.vclass--;
    }
    SSUB(-score_delta*SCORE_VCLASS);
  }
  
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
    cerr << "  edge to " << vdstr->name << endl;
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
#ifdef SCORE_DEBUG
      cerr << "  interswitch link\n";
#endif

      edge cur;      
      forall(cur,G[e].path) {
	SSUB(SCORE_INTERSWITCH_LINK);
	unscore_link(cur,e,true);
      }
      G[e].path.clear();
      
      unscore_link(vlink->plink_local_one,e,false);
      unscore_link(vlink->plink_local_two,e,false);
    } else if (vlink->type == tb_vlink::LINK_INTRASWITCH) {
      // INTRASWITCH LINK
#ifdef SCORE_DEBUG
      fprintf(stderr,"   intraswitch link\n");
#endif
      SSUB(SCORE_INTRASWITCH_LINK);

      unscore_link(vlink->plink,e,false);
      unscore_link(vlink->plink_two,e,false); 
    } else if (vlink->type != tb_vlink::LINK_TRIVIAL) {
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
	cerr << "  releasing switch " << PG[the_switch].name << endl;
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

  // features/desires
  int fd_violated;
  double fds=fd_score(vnoder,pnoder,&fd_violated);
  SSUB(fds);
  violated -= fd_violated;
  vinfo.desires -= fd_violated;
  
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
  cerr << "SCORE: add_node(" << vnoder.name << "," << pnoder.name << "[" << ploc << "])\n";
  fprintf(stderr,"  vnode type = ");
  cerr << vnoder.type << " pnode switch = ";
  if (pnoder.the_switch) {
    cerr << PG[pnoder.the_switch].name;
  } else {
    cerr << "No switch";
  }
  cerr << endl;
#endif
  
  // set up pnode
  // figure out type
  if (!pnoder.typed) {
#ifdef SCORE_DEBUG
    fprintf(stderr,"  virgin pnode\n");
    cerr << "    vtype = " << vnoder.type << "\n";
#endif
    
    // Remove check assuming at higher level?
    // Remove higher level checks?
    pnoder.max_load=0;
    if (pnoder.types.lookup(vnoder.type) != nil)
      pnoder.max_load = pnoder.types.access(vnoder.type);
    
    if (pnoder.max_load == 0) {
      // didn't find a type
#ifdef SCORE_DEBUG
      fprintf(stderr,"  no matching type\n");
#endif
      return 1;
    }

    pnoder.current_type=vnoder.type;
    pnoder.typed=true;

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
	/* XXX - We could ignore this check and let the code
	   at the end of the routine penalize for going over
	   load.  Failing here seems to work better though. */
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
    cerr << "  edge to " << dstr.name << endl;
#endif

    if (dstr.posistion != 0) {
      // dstr is assigned
      node dpnode=pnodes[dstr.posistion];
      tb_pnode &dpnoder=PG[dpnode];

#ifdef SCORE_DEBUG
      cerr << "   goes to " << dpnoder.name << endl;
#endif

      if (dpnode == pnode) {
#ifdef SCORE_DEBUG
	fprintf(stderr,"  trivial link\n");
#endif SCORE_DEBUG
	if (allow_trivial_links) {
	  er->type = tb_vlink::LINK_TRIVIAL;
	} else {
	  goto CLEANUP;
	}
      } else if ((pedge=direct_link(dpnode,pnode)) != NULL) {
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
#ifdef SCORE_DEBUG
	cerr << "   looking for interswitch link " <<
	  (pnoder.the_switch != nil?PG[pnoder.the_switch].name:string("No Switch")) << " " <<
	  (dpnoder.the_switch != nil?PG[dpnoder.the_switch].name:string("No Switch")) << endl;
#endif
	if (find_interswitch_path(pnoder.the_switch,dpnoder.the_switch,
				  er->bandwidth,er->path) == 0) {
#ifdef SCORE_DEBUG
	  fprintf(stderr,"   could not find path - no connection\n");
#endif

CLEANUP:
	  // Need to free up all links already made and abort
	  forall_inout_edges(e,n) {
	    tb_vlink &vlink = G[e];
	    if (vlink.type == tb_vlink::LINK_DIRECT) {
	      unscore_link(vlink.plink,e,false);
	    } else if (vlink.type == tb_vlink::LINK_INTRASWITCH) {
	      SSUB(SCORE_INTRASWITCH_LINK);
	      unscore_link(vlink.plink,e,false);
	      unscore_link(vlink.plink_two,e,false);
	    } else if (vlink.type == tb_vlink::LINK_INTERSWITCH) {
	      edge cur;
	      forall(cur,G[e].path) {
		SSUB(SCORE_INTERSWITCH_LINK);
		unscore_link(cur,e,true);
	      }
	      G[e].path.clear();
	      unscore_link(vlink.plink_local_one,e,false);
	      unscore_link(vlink.plink_local_two,e,false);
	    } // else LINK_UNKNOWN i.e. unassigned.
	  }

	  // Reset to be retyped next time and abort.  This is a
	  // fatal error.
	  pnoder.typed = false;
	  return 1;
 	} else {
#ifdef SCORE_DEBUG
	  fprintf(stderr,"   found interswitch link\n");
#endif
	  er->type=tb_vlink::LINK_INTERSWITCH;

	  edge cur;
	  forall(cur,er->path) {
	    SADD(SCORE_INTERSWITCH_LINK);
#ifdef SCORE_DEBUG
	    cerr << "     " << PG[cur].name << endl;
#endif
	    score_link(cur,e,true);
	  }

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

  // features/desires
  int fd_violated;
  double fds = fd_score(vnoder,pnoder,&fd_violated);
  SADD(fds);
  violated += fd_violated;
  vinfo.desires += fd_violated;

  // pclass
  if (pnoder.my_class->used == 0) {
#ifdef SCORE_DEBUG
    cerr << "  new pclass\n";
#endif
    SADD(SCORE_PCLASS);
  }

  // vclass
  if (vnoder.vclass != NULL) {
    double score_delta = vnoder.vclass->assign_node(vnoder.type);
#ifdef SCORE_DEBUG
    cerr << "  vclass assign " << score_delta << endl;
#endif
    SADD(score_delta*SCORE_VCLASS);
    if (score_delta >= 1) {
      violated++;
      vinfo.vclass++;
    }
  }

#ifdef SCORE_DEBUG
  fprintf(stderr,"  posistion = %d\n",vnoder.posistion);
  fprintf(stderr,"  new score = %.2f  new violated = %d\n",score,violated);
#endif

  pclass_set(vnoder,pnoder);
  
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
	  ((pl->emulated+pl->nonemulated <
	    bestpl->emulated+bestpl->nonemulated) ||
	   (pl->emulated+pl->nonemulated ==
	    bestpl->emulated+bestpl->nonemulated) &&
	   (pl->bw_used < bestpl->bw_used))) {
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
      if ((er.emulated+er.nonemulated < best_users) ||
	  ((er.emulated+er.nonemulated == best_users) && (bw < best_bw))) {
	best = e;
	best_bw = bw;
	best_users = er.emulated+er.nonemulated;
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

// this uses the shortest paths calculated over the switch graph to
// find a path between src and dst.  It passes out list<edge>, a list
// of the edges used. (assumed to be empty to begin with).
// Returns 0 if no path exists and 1 otherwise.
int find_interswitch_path(node src,node dst,int bandwidth,list<edge> &L)
{
  // We know the shortest path from src to node already.  It's stored
  // in switch_preds[src] and is a node_array<edge>.  Let P be this
  // array.  We can trace our shortest path by starting at the end and
  // following the pred edges back until we reach src.  We need to be
  // careful though because the switch_preds deals with elements of SG
  // and we have elements of PG.
  if ((src == nil) || (dst == nil)) {return 0;}
  
  node sg_src = PG[src].sgraph_switch;
  node sg_dst = PG[dst].sgraph_switch;
  edge sg_ed;
  node sg_cur=sg_dst;
  switch_pred_array &preds = switch_preds[sg_src];
  if (preds[sg_dst] == nil) {
    // unreachable
    return 0;
  }
  while (sg_cur != sg_src) {
    sg_ed = preds[sg_cur];
    L.push(SG[sg_ed].mate);
    if (SG.source(sg_ed) == sg_cur) {
      sg_cur = SG.target(sg_ed);
    } else {
      sg_cur = SG.source(sg_ed);
    }
  }
  return 1;
}

// this does scoring for over users and over bandwidth on edges.
void score_link(edge e,edge v,bool interswitch)
{
  tb_plink &pl = PG[e];
  tb_vlink &er = G[v];

#ifdef SCORE_DEBUG
  cerr << "  score_link(" << e << ") - " << pl.name << " / " << er.name << endl;
#endif

  if (! interswitch) {
    // need too account for three things here, the possiblity of a new plink
    // the user of a new emulated link, and a possible violation.
    if (er.emulated) {
      pl.emulated++;
      SADD(SCORE_EMULATED_LINK);
    }
    else pl.nonemulated++;
    if (pl.nonemulated+pl.emulated == 1) {
      // new link
#ifdef SCORE_DEBUG
      fprintf(stderr,"    first user\n");
#endif
      SADD(SCORE_DIRECT_LINK);
    } else {
      // check for violation, basically if this is the first of it's
      // type to be added.
      if (((! er.emulated) && (pl.nonemulated == 1)) ||
	  ((er.emulated) && (pl.emulated == 1))) {
#ifdef SCORE_DEBUG
	  fprintf(stderr,"    link user - penalty\n");
#endif
	  SADD(SCORE_DIRECT_LINK_PENALTY);
	  vinfo.link_users++;
	  violated++;
      }
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
    if (er.emulated) {
      pl.emulated--;
      SSUB(SCORE_EMULATED_LINK);
    } else {
      pl.nonemulated--;
    }
    if (pl.nonemulated+pl.emulated == 0) {
      // link no longer used
#ifdef SCORE_DEBUG
      fprintf(stderr,"   freeing link\n");
#endif
      SSUB(SCORE_DIRECT_LINK);
    } else {
      // check to see if re freed up a violation, basically did
      // we remove the last of it's link type.
      if ((er.emulated && (pl.emulated == 0)) ||
	  ((! er.emulated) && pl.nonemulated == 0)) {
	// all good
#ifdef SCORE_DEBUG
	fprintf(stderr,"   users ok\n");
#endif
	SSUB(SCORE_DIRECT_LINK_PENALTY);
	vinfo.link_users--;
	violated--;
      }
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

  er.type = tb_vlink::LINK_UNKNOWN;
}

double fd_score(tb_vnode &vnoder,tb_pnode &pnoder,int *fd_violated)
{
  double fd_score=0;
  (*fd_violated)=0;
  
  seq_item desire;
  seq_item feature;
  double value;
  for (desire = vnoder.desires.min_item();desire;
       desire = vnoder.desires.succ(desire)) {
    feature = pnoder.features.lookup(vnoder.desires.key(desire));
#ifdef SCORE_DEBUG
    cerr << "  desire = " << vnoder.desires.key(desire) \
	 << " " << vnoder.desires.inf(desire) << "\n";
#endif
    if (!feature) {
      // Unmatched desire.  Add cost.
#ifdef SCORE_DEBUG
      cerr << "    unmatched\n";
#endif
      value = vnoder.desires.inf(desire);
      fd_score += SCORE_DESIRE*value;
      if (value >= 1) {
	(*fd_violated)++;
      }
    }
  }
  for (feature = pnoder.features.min_item();feature;
       feature = pnoder.features.succ(feature)) {
    desire = vnoder.desires.lookup(pnoder.features.key(feature));
#ifdef SCORE_DEBUG
    cerr << "  feature = " << pnoder.features.key(feature) \
	 << " " << pnoder.features.inf(feature) << "\n";
#endif
    if (! desire) {
      // Unused feature.  Add weight
#ifdef SCORE_DEBUG
      cerr << "    unused\n";
#endif
      value = pnoder.features.inf(feature);
      fd_score+=SCORE_FEATURE*value;
    }
  }

  return fd_score;
}
