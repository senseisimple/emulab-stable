#include <LEDA/graph_alg.h>
#include <LEDA/graphwin.h>
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

tb_pgraph PG(1,1);
tb_vgraph G(1,1);

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
int parse_ptop(tb_pgraph &G, istream& i);

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
  int naccepts = 40*nnodes;
  int accepts = 0;
  int oldpos;

  int bestviolated;
  int absbestv;
  
  float temp = init_temp;
  
  /* Set up the initial counts */
  init_score();

  bestscore = get_score();
  bestviolated = violated;
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
    cout << "Temperature:  " << temp << endl;
#endif
    trans = 0;
    accepts = 0;

    bool unassigned = true;
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
      newpos = ((oldpos+(random()%(nparts-1)+1))%nparts)+1;

      if (oldpos != 0) {
	remove_node(n);
	unassigned = false;
      }

      int first_newpos = newpos;
      while (add_node(n,newpos) == 1) {
	newpos = (newpos % nparts)+1;
	if (newpos == first_newpos) {
	  // no place to put a node.
	  // instead re randomly choose a node and remove it and
	  // then continue the 'continue'
	  // XXX - could improve this
	  node ntor = G.choose_node();
	  while (G[ntor].posistion == 0)
	    ntor = G.choose_node();
	  remove_node(ntor);
	  unassigned_nodes.insert(ntor,random());
	  continue;
	}
      }
      if (unassigned) unassigned_nodes.del(n);

      newscore = get_score();

      /* So it's negative if bad */
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
	//      if (newscore < 0.11f) {
	if (newscore < optimal) {
	  timeend = used_time(timestart);
	  cout << "OPTIMAL ( " << optimal << ") in "
	       << iters << " iters, "
	       << timeend << " seconds" << endl;
	  goto DONE;
	}
      } else { /* Reject this change */
	remove_node(n);
	if (oldpos != 0) {
	  int r = add_node(n,oldpos);
	  assert(r == 0);
	}
      }
    }
      
    temp *= temp_rate;
  }

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
      cout << G[n].name << " " << (pnoder.the_switch ?
				   PG[pnoder.the_switch].name :
				   "NO_SWITCH") <<
	" " << pnoder.name << endl;

    }
  }
  cout << "End Nodes" << endl;
  cout << "Edges:" << endl;
  edge e;
  forall_edges(e,G) {
    tb_vlink &v = G[e];
    tb_plink *p,*p2,*p3,*p4;
    cout << G[e].name;
    if (v.type == tb_vlink::LINK_DIRECT) {
      p = &PG[v.plink];
      cout << " direct " << PG[v.plink].name << " (" <<
	p->srcmac << "," << p->dstmac << ")" << endl;
    } else if (v.type == tb_vlink::LINK_INTRASWITCH) {
      p = &PG[v.plink];
      p2 = &PG[v.plink_two];
      cout << " intraswitch " << PG[v.plink].name << " (" <<
	p->srcmac << "," << p->dstmac << ") " <<
	PG[v.plink_two].name << " (" << p2->srcmac << "," << p2->dstmac <<
	")" << endl;
    } else if (v.type == tb_vlink::LINK_INTERSWITCH) {
      p = &PG[v.plink];
      p3 = &PG[v.plink_local_one];
      p4 = &PG[v.plink_local_two];
      cout << " interswitch " << PG[v.plink].name << " (" <<
	p->srcmac << "," << p->dstmac << ")";
      if (v.plink_two) {
	p2 = &PG[v.plink_two];
	cout << " " << PG[v.plink_two].name << " (" << p2->srcmac << "," <<
	  p2->dstmac <<	")";
      }
      cout << " " << PG[v.plink_local_one].name << " (" << p3->srcmac << "," <<
	p3->dstmac << ") " << PG[v.plink_local_two].name << " (" <<
	p4->srcmac << "," << p4->dstmac << ")" << endl;
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
  dump_options("Send options", options, noptions);
#endif

  int seed = time(NULL)+getpid();
  if (getenv("ASSIGN_SEED") != NULL) {
    sscanf(getenv("ASSIGN_SEED"),"%d",&seed);
  }
  printf("seed = %d\n",seed);
  srandom(seed);

  /*
   * Allow the user to specify a topology in ".top" format.
   */

  if (argc == 1) {
    ifstream infile;
    infile.open(argv[0]);
    if (!infile || !infile.good()) {
      cerr << "Error opening file: " << argv[0] << endl;
      exit(-11);
    }
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
    nparts = parse_ptop(PG,ptopfile);
    cout << "Nparts: " << nparts << endl;
  }

  batch();

  print_solution();
    
  return 0;
}
