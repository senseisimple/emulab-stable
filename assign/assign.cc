#include <LEDA/graph_alg.h>
#include <LEDA/graphwin.h>
#include <LEDA/ugraph.h>
#include <LEDA/dictionary.h>
#include <LEDA/map.h>
#include <LEDA/graph_iterator.h>
#include <LEDA/node_pq.h>
#include <LEDA/sortseq.h>
#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "physical.h"
#include "virtual.h"
#include "score.h"
#include "pclass.h"

void parse_options(char **argv, struct config_param options[], int nopt);
int config_parse(char **args, struct config_param cparams[], int nparams);
void dump_options(const char *str, struct config_param cparams[], int nparams);

// Purely heuristic

#ifdef USE_OPTIMAL
#define OPTIMAL_SCORE(edges,nodes) (nodes*SCORE_PNODE + \
                                    nodes/opt_nodes_per_sw*SCORE_SWITCH + \
                                    edges*((SCORE_INTRASWITCH_LINK+ \
                                    SCORE_DIRECT_LINK*2)*4+\
                                    SCORE_INTERSWITCH_LINK)/opt_nodes_per_sw)
#else
#define OPTIMAL_SCORE(edges,nodes) 0
#endif

tb_sgraph SG;
edge_array<int> edge_costs;
typedef node_array<int> switch_distance_array;
typedef node_array<edge> switch_pred_array;

node_array<switch_distance_array> switch_distances;
node_array<switch_pred_array> switch_preds;
tb_pgraph PG;
tb_vgraph G;
dictionary<tb_pnode*,node> pnode2node;
dictionary<tb_pnode*,int> pnode2posistion;
pclass_list pclasses;
dictionary<string,pclass_list*> type_table;

/* How can we chop things up? */
#define PARTITION_BY_ANNEALING 0

#define MAX_DELAYS 64

int nparts = 0;     /* DEFAULTS */
int accepts = 0;
int nnodes = 0;
int partition_mechanism;
int on_line = 0;
int cycles_to_best = 0;
int batch_mode = 0;

float sensitivity = .1;

int refreshed = 0;

node_array<int> bestnodes, absnodes;
float bestscore, absbest;

extern node pnodes[MAX_PNODES];
extern node_array<int> switch_index;
node_pq<int> unassigned_nodes(G);

int parse_top(tb_vgraph &G, istream& i);
int parse_ptop(tb_pgraph &PG, tb_sgraph &SG, istream& i);

/* The following two sets hold all the virtual and physical types.  These
 * are compared to make sure that every member of vtypes is in ptypes.
 * Both are filled by the parse_* routines.  I'd love to use LEDA sets
 * to implement this but LEDA, an otherwise profession work, did a lousy
 * job when it came to sets.  They clash with graph iterators!  Since
 * we only use these once we'll use a much less efficient linked list
 * <shudder>
 */
list<string> vtypes;
list<string> ptypes;

// Makes LEDA happy

int compare(tb_pnode *const &a, tb_pnode *const &b)
{
  if (a==b) return 0;
  if (a < b) return -1;
  return 1;
}

/*
 * Basic simulated annealing parameters:
 *
 * Make changes proportional to T
 * Accept worse solution with p = e^(change/Temperature*sensitivity)
 *
 */

inline int accept(float change, float temperature)
{
  float p;
  int r;

  if (change == 0) {
    p = 1000 * temperature / temp_prob;
  } else {
    p = expf(change/(temperature*sensitivity)) * 1000;
  }
  r = random() % 1000;
  if (r < p) {
    accepts++;
    return 1;
  }
  return 0;
}

// This routine chooses randomly chooses a pclass based on the weights
// and *removes that pclass from the weights*.  Total is the total of
// all weights and is adjusted when a pclass is removed.
tb_pnode *choose_pnode(dictionary<tb_pclass*,double> &weights,double &total,
		       string vtype)
{
  tb_pnode *pnode;
  dic_item dit=nil;

  double r = random()/(double)RAND_MAX*total;
  forall_items(dit,weights) {
    r -= weights.inf(dit);
    if (r <= 0) break;
  }
  if (dit == nil) return NULL;

  tb_pclass *chosen_class = weights.key(dit);

  // Take the first node of the correct type from the class.
  pnode = chosen_class->members.access(vtype)->front();
  
  total -= weights.inf(dit);
  weights.del_item(dit);
  
#ifdef PCLASS_DEBUG_MORE
  cout << "choose_pnode = [" << chosen_class->name << "] = "
       << ((pnode == NULL) ? string("NULL"):pnode->name) << endl;
#endif
  
  return pnode;
}
/*
 * The workhorse of our program.
 *
 * Assign performs an assignment of the virtual nodes (vnodes) to
 * nodes in the physical topology.
 *
 * The input virtual topology is the graph G (global)
 * the input physical topology is the topology topo (global).
 *
 * The simulated annealing logic is contained herein,
 * except for the "accept a bad change" computation,
 * which is performed in accept().
 */

int assign()
{
  float newscore, bestscore;
  node n;
  int iters = 0;

  float timestart = used_time();
  float timeend;
  float scorediff;

  nnodes = G.number_of_nodes();
 
  float cycles = CYCLES*(float)(nnodes + G.number_of_edges());

  float optimal = OPTIMAL_SCORE(G.number_of_edges(),nnodes);
#ifdef STATS
  cout << "STATS_OPTIMAL = " << optimal << endl;
#endif
  
  int mintrans = (int)cycles;
  int trans;
  int naccepts = 20*nnodes;
  int accepts = 0;
  int oldpos;

  int bestviolated;
  int absbestv;
  
  float temp = init_temp;

#ifdef VERBOSE
  cout << "Initialized to cycles="<<cycles<<" optimal="<<optimal<<" mintrans="
       << mintrans<<" naccepts="<<naccepts<<" nnodes="<<nnodes<<"\n";
#endif
  
  
  /* Set up the initial counts */
  init_score();

  bestscore = get_score();
  bestviolated = violated;
#ifdef VERBOSE
  cout << "Problem started with score "<<bestscore<<" and "<< violated
       << " violations.\n";
#endif
  absbest = bestscore;
  absbestv = bestviolated;
  node n3;
  forall_nodes(n3, G) {
    absnodes[n3] = G[n3].posistion;
  }

  //  if (bestscore < 0.11f) {
  if (bestscore < optimal) {
#ifdef VERBOSE
    cout << "Problem started optimal\n";
#endif
    goto DONE;
  }
  
  while (temp >= temp_stop) {
#ifdef VERBOSE
    cout << "Temperature:  " << temp << " AbsBest: " << absbest << " (" << absbestv << ")" << endl;
#endif
    trans = 0;
    accepts = 0;

    while (trans < mintrans && accepts < naccepts) {
#ifdef STATS
      cout << "STATS temp:" << temp << " score:" << get_score() <<
	" violated:" << violated << " trans:" << trans <<
	" accepts:" << accepts << endl;
#endif STATS
      int newpos;
      trans++;
      iters++;

      n = unassigned_nodes.find_min();
      if (n == nil)
	n = G.choose_node();

      // Note: we have a lot of +1's here because of the first
      // node loc in pnodes is 1 not 0.
      oldpos = G[n].posistion;

      if (oldpos != 0) {
	remove_node(n);
	unassigned_nodes.insert(n,random());
      }

      //////////////////////////////////////////////////////////////////////
      // Lots of code to calculate the relative weights of the pclasses.
      // The weights are stored in the weights dictionary which is indexed
      // by pclass* and contains the weight.
      //////////////////////////////////////////////////////////////////////
      dictionary<tb_pclass*,double> weights;
      list_item lit;
      dic_item dit;
      
      // find acceptable pclasses
      tb_vnode &vn=G[n];
      pclass_list *L = type_table.access(vn.type);
      forall_items(lit,*L) {
	tb_pclass *c = L->inf(lit);
	if (c->used != c->size) {
	  if (c->members.access(vn.type)->front() != NULL) {
	    weights.insert(c,PCLASS_BASE_WEIGHT);
	  }
	}
      }
      
      // adjust weight by neighbors
      edge e;
      forall_inout_edges(e,n) {
	node dst = G.target(e);
	if (dst == n) dst = G.source(e);
	tb_vnode &vdst = G[dst];
	if (vdst.posistion != 0) {
	  tb_pnode &pdst = PG[pnodes[vdst.posistion]];
	  tb_pclass *c = pdst.my_class;
	  dit = weights.lookup(c);
	  if (dit != nil) {
	    weights.change_inf(dit,weights.inf(dit)+PCLASS_NEIGHBOR_WEIGHT);
	  }
	}
      }
      
      // Adjust classes by utilizaiton
      forall_items(lit,pclasses) {
	tb_pclass *c = pclasses.inf(lit);
	dit = weights.lookup(c);
	if (dit != nil) {
	  if (c->used > 0) {
	    weights.change_inf(dit,weights.inf(dit)+
			       PCLASS_UTIL_WEIGHT);
	  }
	}
      }

      // Adjust classes by features/desires
      // We calculate the fd score for all possible classes and normalize
      // to 1 and then multiply by PCLASS_FD_WEIGHT
      dictionary<tb_pclass*,double> fdscores;
      double max_afds=0;
      forall_items(dit,weights) {
	tb_pclass *c = weights.key(dit);
	tb_pnode *p = c->members.access(vn.type)->front();
	int v;
	double score = fd_score(vn,*p,&v);
	fdscores.insert(c,score);
	if (fabs(score) > max_afds) max_afds=fabs(score);
      }
      if (max_afds == 0) max_afds=1;
      
      forall_items(dit,weights) {
	tb_pclass *c = weights.key(dit);
	double fds = fdscores.access(c);
	weights.change_inf(dit,
			   weights.inf(dit)+PCLASS_FDS_WEIGHT*fds/max_afds);
      }
      
      // Calculate total weight.
      double total=0;
      forall_items(dit,weights) {
	total += weights.inf(dit);
      }
      
#ifdef PCLASS_DEBUG_MORE
      cerr << "Finding pclass for " << vn.name << endl;
      {
	dic_item pit;
	forall_items(pit,weights) {
	  tb_pclass *p = weights.key(pit);
	  cout << "  " << p->name << " " << weights.inf(pit) << endl;
	}
      }
#endif
      
      ////
      // We've now calculated the weights and the total weight.  We
      // will loop through all the classes until we find an acceptable
      // one.
      ////
      // Loop will break eventually.
      tb_pnode *newpnode;
      do {
	newpnode = choose_pnode(weights,total,vn.type);
	if (newpnode == NULL) {
	  // no available nodes
	  // need to free up a node and recalculate weights.
	  int pos = 0;
	  node ntor;
	  while (pos == 0) {
	    ntor = G.choose_node();
	    pos = G[ntor].posistion;
	  }
	  remove_node(ntor);
	  unassigned_nodes.insert(ntor,random());
	  break;
	}
	newpos = pnode2posistion.access(newpnode);
      } while (add_node(n,newpos) == 1);

      // This occurs when no pclass could be found.
      if (newpnode == NULL) continue;

      unassigned_nodes.del(n);
      
      newscore = get_score();
      
      // Negative means bad
      scorediff = bestscore - newscore;
      
      // tinkering aournd witht his.
      if ((newscore < optimal) || (violated < bestviolated) ||
	  ((violated == bestviolated) && (newscore < bestscore)) ||
	  accept(scorediff*((bestviolated - violated)/2), temp)) {
	bestnodes[n] = G[n].posistion;
	bestscore = newscore;
	bestviolated = violated;
	accepts++;
	if ((violated < absbestv) ||
	    ((violated == absbestv) &&
	     (newscore < absbest))) {
	  node n2;
	  forall_nodes(n2, G) {
	    absnodes[n2] = G[n2].posistion;
	  }
	  absbest = newscore;
	  absbestv = violated;
	  cycles_to_best = iters;
	}
	if (newscore < optimal) {
	  timeend = used_time(timestart);
	  cout << "OPTIMAL ( " << optimal << ") in "
	       << iters << " iters, "
	       << timeend << " seconds" << endl;
	  goto DONE;
	}
	// Accept change
      } else {
	// Reject change
	remove_node(n);
	if (oldpos != 0) {
	  add_node(n,oldpos);
	}
      }
    }
    temp *= temp_rate;
  }
  cout << "Done.\n";
  
 DONE:
  forall_nodes(n, G) {
    bestnodes[n] = absnodes[n];
  }
  bestscore = absbest;

  forall_nodes(n, G) {
    if (G[n].posistion != 0)
      remove_node(n);
  }
	
  forall_nodes(n, G) {
    if (absnodes[n] != 0) {
      if (add_node(n,absnodes[n]) != 0) {
	cerr << "Invalid assumption.  Tell calfeld that reseting to best configuration doesn't work" << endl;
      }
    } else {
      cout << "Unassigned node: " << G[n].name << endl;
    }
  }
  
  timeend = used_time(timestart);
  printf("   BEST SCORE:  %.2f",get_score());
  cout << " in " << iters << " iters and " << timeend << " seconds" << endl;
  cout << "With " << violated << " violations" << endl;
  cout << "With " << accepts << " accepts of increases\n";
  cout << "Iters to find best score:  " << cycles_to_best << endl;
  cout << "Violations: " << violated << endl;
  cout << "  unassigned: " << vinfo.unassigned << endl;
  cout << "  pnode_load: " << vinfo.pnode_load << endl;
  cout << "  no_connect: " << vinfo.no_connection << endl;
  cout << "  link_users: " << vinfo.link_users << endl;
  cout << "  bandwidth:  " << vinfo.bandwidth << endl;
  cout << "  desires:    " << vinfo.desires << endl;

  return 0;
}

/*
 * A legacy function from a less general version of the program.
 *
 * Now simply resets the node assignment, performs a new assignment,
 * and prints out the results.
 *
 */
void loopassign()
{
  node_array<int> nodestorage;
  int optimal = 0;
  float timestart = used_time();
  float totaltime;

  nodestorage.init(G, 0);
  bestnodes.init(G, 0);
  absnodes.init(G, 0);
    
  nnodes = G.number_of_nodes();
  optimal = assign();
  totaltime = used_time(timestart);

  if (violated > 0) {
    cout << "violations: " << violated << endl;
  }
  cout << "Total time to find solution "
       << totaltime << " seconds" << endl;
}

/*
 * If we have more ways of partitioning the graph other than just
 * simulated annealing, throw them in here.
 */

void chopgraph() {
  switch(partition_mechanism) {
  case PARTITION_BY_ANNEALING:
    loopassign();
    break;
  default:
    cerr << "Unknown partition mechanism.  eeeek." << endl;
    exit(-1);
  }
}

void batch()
{
  bestnodes.init(G, 0);
  absnodes.init(G, 0);
  chopgraph();
}


void usage() {
  fprintf(stderr,
	  "usage:  assign [-h] [-bao] [-s <switches>] [-n nodes/switch] [-c cap] [file]\n"
	  "           -h ...... brief help listing\n"
	  //	  "           -s #  ... number of switches in cluster\n"
	  //	  "           -n #  ... number of nodes per switch\n"
	  "           -a ...... Use simulated annealing (default)\n"
	  "           -o ...... Update on-line (vs batch, default)\n"
	  "           -t <file> Input topology desc. from <file>\n"
	  "           -b ...... batch mode (no gui)\n"
	  );
}

int mst_comp(const edge &A,const edge &B)
{
  edge pA,pB;
  pA = SG[A].mate;
  pB = SG[B].mate;
  // Highbandwidth = low score
  if (PG[pA].bandwidth > PG[pB].bandwidth) return -1;
  if (PG[pA].bandwidth < PG[pB].bandwidth) return 1;
  return 0;
}

void print_solution()
{
  node n;
  cout << "Best solution: " << absbest << endl;
  cout << "Nodes:" << endl;
  forall_nodes(n,G) {
    if (!G[n].posistion) {
      cout << "unassigned: " << G[n].name << endl;
    } else {
      node pnode = pnodes[G[n].posistion];
      tb_pnode &pnoder = PG[pnode];
      cout << G[n].name << " ";
      if (pnoder.the_switch) {
	cout << PG[pnoder.the_switch].name;
      } else {
	cout << "NO_SWITCH";
      }
      cout << " " << pnoder.name << endl;
    }
  }
  cout << "End Nodes" << endl;
  cout << "Edges:" << endl;
  edge e;
  forall_edges(e,G) {
    tb_vlink &v = G[e];
    cout << G[e].name;
    if (v.type == tb_vlink::LINK_DIRECT) {
      tb_plink &p = PG[v.plink];
      cout << " direct " << p.name << " (" <<
	p.srcmac << "," << p.dstmac << ")" << endl;
    } else if (v.type == tb_vlink::LINK_INTRASWITCH) {
      tb_plink &p = PG[v.plink];
      tb_plink &p2 = PG[v.plink_two];
      cout << " intraswitch " << p.name << " (" <<
	p.srcmac << "," << p.dstmac << ") " <<
	p2.name << " (" << p2.srcmac << "," << p2.dstmac <<
	")" << endl;
    } else if (v.type == tb_vlink::LINK_INTERSWITCH) {
      cout << " interswitch ";
      edge e;
      tb_plink &lp = PG[v.plink_local_one];
      tb_plink &lp2 = PG[v.plink_local_two];
      cout << lp.name << " (" << lp.srcmac << "," << lp.dstmac << ")";
      forall(e,v.path) {
	tb_plink &p = PG[e];
	cout << " " << p.name << " (" << p.srcmac << "," << p.dstmac << ")";
      }
      cout << " " << lp2.name << " (" << lp2.srcmac << "," <<
	lp2.dstmac << ")" << endl;
    } else {
      cout << "Unknown link type" << endl;
    }
  }
  cout << "End Edges" << endl;
  cout << "End solution" << endl;
}

int main(int argc, char **argv)
{
  extern char *optarg;
  extern int optind;
  char *topofile = NULL;
    
  int ch;

  partition_mechanism = PARTITION_BY_ANNEALING;

  while ((ch = getopt(argc, argv, "boas:n:t:h")) != -1)
    switch(ch) {
    case 'h': usage(); exit(0);
      //		case 's': nparts = atoi(optarg); break;
    case 'a': partition_mechanism = PARTITION_BY_ANNEALING; break;
    case 'o': on_line = 1; break;
    case 't': topofile = optarg; break;
    case 'b': batch_mode = 1; break;
    default: usage(); exit(-1);
    }

  argc -= optind;
  argv += optind;

  /* newbold@cs
     These relate to the globals defined in common.h
     It reads in all the parameters for the program that were formerly
     all hardcoded constants.
  */
  parse_options(argv, options, noptions);
#ifdef SCORE_DEBUG
  dump_options("Configuration options:", options, noptions);
#endif

  int seed;
  if (getenv("ASSIGN_SEED") != NULL) {
    sscanf(getenv("ASSIGN_SEED"),"%d",&seed);
  } else {
    seed = time(NULL)+getpid();
  }
  printf("seed = %d\n",seed);
  srandom(seed);

  /*
   * Allow the user to specify a topology in ".top" format.
   */

  if (argc >= 1) {
    ifstream infile;
    infile.open(argv[0]);
    if (!infile || !infile.good()) {
      cerr << "Error opening file: " << argv[0] << endl;
      exit(-11);
    }
    cout << "Parsing top\n";
    parse_top(G, infile);
  }

  /*
   * Allow the user to specify a physical topology
   * in .phys format.  Fills in the "topo" global variable.
   * Make no mistake:  This is actually mandatory now.
   */
  if (topofile != NULL) {
    cout << "Parsing ptop\n";
    ifstream ptopfile;
    ptopfile.open(topofile);
    if (!ptopfile || !ptopfile.good()) {
      cerr << "Error opening file: " << topofile << endl;
      exit(-1);
    }
    nparts = parse_ptop(PG,SG,ptopfile);
    cout << "Nparts: " << nparts << endl;

    cout << "Type Precheck" << endl;
    string curtype;
    int ok=1;
    forall (curtype,vtypes) {
      if (ptypes.search(curtype) == nil) {
	cout << "  No physical nodes of type " << curtype << endl;
	ok=0;
      }
    }
    if (! ok) exit(-1);
    
    cout << "Initializing data structures." << endl;
    edge_costs.init(SG);
    switch_distances.init(SG);
    switch_preds.init(SG);
    cout << "Calculating shortest paths on switch fabric." << endl;
    edge ed;
    forall_edges(ed,SG) {
      edge_costs[ed] = 100000000-PG[SG[ed].mate].bandwidth;
#ifdef SCORE_DEBUG
      cerr << "  " << PG[SG[ed].mate].name << " " << edge_costs[ed] << endl;
#endif

    }
    node sw;
    forall_nodes(sw,SG) {
      switch_distances[sw].init(SG);
      switch_preds[sw].init(SG);
      DIJKSTRA_T(SG,sw,edge_costs,
		 switch_distances[sw],switch_preds[sw]);
#ifdef SCORE_DEBUG
      cerr << "Source " << PG[SG[sw].mate].name << endl;
      node dsw;
      forall_nodes(dsw,SG) {
	cerr << "  " << PG[SG[dsw].mate].name;
	int dist = switch_distances[sw][dsw];
	cerr << "  dist " << dist;
	edge de = switch_preds[sw][dsw];
	if (de == nil) {
	  cerr << "  pred nil" << endl;
	} else {
	  cerr << "  pred " << PG[SG[de].mate].name << endl;
	}
      }
#endif
    }
  }

  node pn;
  forall_nodes(pn,PG) {
    pnode2node.insert(&PG[pn],pn);
  }
  for (int i=0;i<MAX_PNODES;++i) {
    if (pnodes[i] != nil) {
      pnode2posistion.insert(&PG[pnodes[i]],i);
    }
  }

  cout << "Generating physical classes\n";
  generate_pclasses(PG);
  cout << "Nclasses: " << pclasses.length() << endl;

#ifdef PCLASS_DEBUG
  pclass_debug();
#endif
  
  cout << "Annealing!" << endl;
  batch();

  print_solution();
    
  return 0;
}
