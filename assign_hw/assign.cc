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
#include <assert.h>

#include "common.h"
#include "physical.h"
#include "virtual.h"
#include "score.h"

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

static const int initial_temperature = 100;
static const int temp_prob = 130;

int refreshed = 0;

node_array<int> bestnodes, absnodes;
float bestscore, absbest;

extern node pnodes[MAX_PNODES];
extern node_array<int> switch_index;
void parse_top(tb_vgraph &G, istream& i);
void parse_ptop(tb_pgraph &G, istream& i);

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
  float newscore, bestscore, absbest;
  node n;
  int iters = 0;

  float timestart = used_time();
  float timeend;
  float scorediff;

  nnodes = G.number_of_nodes();
 
  float cycles = 120.0*(float)(nnodes + G.number_of_edges());

  int mintrans = (int)cycles;
  int trans;
  int naccepts = 40*nnodes;
  int accepts = 0;
  int oldpos;

  float temp = initial_temperature;
  
  /* Set up the initial counts */
  init_score();

  bestscore = get_score();
  absbest = bestscore;
  node n3;
  forall_nodes(n3, G) {
    absnodes[n3] = G[n3].posistion;
  }

  if (bestscore < 0.11f) {
#ifdef VERBOSE
    cout << "Problem started optimal\n";
#endif
    goto DONE;
  }
  
  while (temp >= 2) {
#ifdef VERBOSE
    cout << "Temperature:  " << temp << endl;
#endif
    trans = 0;
    accepts = 0;
      
    while (trans < mintrans && accepts < naccepts) {
      int newpos;
      trans++;
      iters++;
      n = G.choose_node();
      oldpos = G[n].posistion;

      newpos = oldpos;
      /* XXX:  Room for improvement. :-) */
      while (newpos == oldpos)
	newpos = (random() % nparts)+1;
      if (oldpos != 0) {
	remove_node(n);
      }
      if (add_node(n,newpos) == 1) continue; // didn't go through
      newscore = get_score();
      if (newscore < 0.11f) {
	timeend = used_time(timestart);
	cout << "OPTIMAL (0.0) in "
	     << iters << " iters, "
	     << timeend << " seconds" << endl;
	goto DONE;
      }
      /* So it's negative if bad */
      scorediff = bestscore - newscore;

      if (newscore < bestscore || accept(scorediff, temp)) {
	bestnodes[n] = G[n].posistion;
	bestscore = newscore;
	accepts++;
	if (newscore < absbest) {
	  node n2;
	  forall_nodes(n2, G) {
	    absnodes[n2] = G[n2].posistion;
	  }
	  absbest = newscore;
	  cycles_to_best = iters;
	}
      } else { /* Reject this change */
	remove_node(n);
	if (oldpos != 0) {
	  int r = add_node(n,oldpos);
	  assert(r == 0);
	}
      }
    }
      
    temp *= .9;
  }

 DONE:
  forall_nodes(n, G) {
    bestnodes[n] = absnodes[n];
  }
  bestscore = absbest;

  forall_nodes(n, G) {
    if (G[n].posistion != 0)
      remove_node(n);
    add_node(n,absnodes[n]);
  }
  
  timeend = used_time(timestart);
  cout << "   BEST SCORE:  " << get_score() << " in "
       << iters << " iters and " << timeend << " seconds" << endl;
  cout << "With " << accepts << " accepts of increases\n";
  cout << "Iters to find best score:  " << cycles_to_best << endl;

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

/*
 * Something in the graph has changed!  Better redisplay.
 *
 * Performs the color assignment for whichever switch the
 * node belongs to and shows the inter-switch links as
 * dashed lines.
 */

void display_scc(GraphWin& gw)
{
  edge e;
  node n;
	
  if (!refreshed) {
    if (on_line)
      chopgraph();
  }
	
  refreshed = 0;
	
  /* Now color them according to their partition */
  // XXX - Need to color them according to switch now!
  // XXX - Need to add labels based on physical node matching
  forall_nodes(n, G) {
    int switchi;
    if (G[n].posistion) {
      gw.set_label(n,PG[pnodes[G[n].posistion]].name);
      switchi = switch_index[PG[pnodes[G[n].posistion]].the_switch];
      switch (switchi) {
      case 0:
	gw.set_color(n, black);
	break;
      case 1:
	gw.set_color(n, blue);
	break;
      case 2:
	gw.set_color(n, green);
	break;
      case 3:
	gw.set_color(n, red);
	break;
      case 4:
	gw.set_color(n, yellow);
	break;
      case 5:
	gw.set_color(n, violet);
	break;
      case 6:
	gw.set_color(n, cyan);
	break;
      case 7:
	gw.set_color(n, brown);
	break;
      case 8:
	gw.set_color(n, pink);
	break;
      case 9:
	gw.set_color(n, orange);
	break;
      case 10:
	gw.set_color(n, grey1);
	break;
      case 11:
	gw.set_color(n, grey3);
	break;
      }
    }
  }
	
  forall_edges(e, G) {
    node v = G.source(e);
    node w = G.target(e);
    node sa,sb;
    if (G[v].posistion && G[w].posistion) {
      sa = PG[pnodes[G[v].posistion]].the_switch;
      sb = PG[pnodes[G[w].posistion]].the_switch;
      if (sa == sb) {
	gw.set_style(e, solid_edge);
      } else {
	gw.set_style(e, dashed_edge);
      }
    }
  }
  gw.redraw();
}

/*
 * Someone clicked on the "reassign" button.
 * Reset and redisplay.
 */

void reassign(GraphWin& gw)
{
  bestnodes.init(G, 0);
  absnodes.init(G, 0);
  chopgraph();
  refreshed = 1;
  display_scc(gw);
}

// XXX : another code by copy
void batch()
{
  bestnodes.init(G, 0);
  absnodes.init(G, 0);
  chopgraph();
}

void new_edge_handler(GraphWin& gw, edge)  { display_scc(gw); }
void del_edge_handler(GraphWin& gw)        { display_scc(gw); }
void new_node_handler(GraphWin& gw, node)  { display_scc(gw); }
void del_node_handler(GraphWin& gw)        { display_scc(gw); }

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
  cout << "End solution" << endl;
}

int main(int argc, char **argv)
{
  int h_menu;
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

  //  srandom(time(NULL) + getpid());
  srandom(42);

  /*
   * Set up the LEDA graph window environment.  Whenever
   * the user does anything to the graph, call the
   * proper handler.
   */
  // XXX: alas, we need this because all the GW stuff is not in
  // the same spot.
  GraphWin gw(G, "Flux Testbed:  Simulated Annealing");
  if (! batch_mode) {
    gw.set_init_graph_handler(del_edge_handler);
    gw.set_new_edge_handler(new_edge_handler);
    gw.set_del_edge_handler(del_edge_handler);
    gw.set_new_node_handler(new_node_handler);
    gw.set_del_node_handler(del_node_handler);
    
    gw.set_node_width(24);
    gw.set_node_height(24);
  }
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
    if (! batch_mode) {
      gw.update_graph();
      node n;
      forall_nodes(n, G) {
	if (G[n].name == NULL) {
	  G[n].name = "";
	}
	gw.set_label(n, G[n].name);
	gw.set_position(n,
			point(random() % 200, random() % 200));
      }
    }
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
    parse_ptop(PG,ptopfile);
    nparts = PG.number_of_nodes();
    cout << "Nparts: " << nparts << endl;
  }

  if (! batch_mode) {
    gw.display();
		
    gw.set_directed(false);
		
    gw.set_node_shape(circle_node);
    gw.set_node_label_type(user_label);
		
    h_menu = gw.get_menu("Layout");
    gw_add_simple_call(gw, reassign, "Reassign", h_menu);
		
    /* Run until the user quits.  Everything is handled by callbacks
     * from LEDA's event loop from here on.                           */
		
    gw.edit();
  } else {
    batch();
  }

  print_solution();
    
  return 0;
}
