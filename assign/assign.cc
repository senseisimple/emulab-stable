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

#include "testbed.h"

#include "phys.h"
topology *topo = NULL;
tbgraph PG(1,1);
/* How can we chop things up? */
#define PARTITION_BY_ANNEALING 0

#define MAX_DELAYS 64

int nparts = 3;     /* DEFAULTS */
int *intercap = NULL;
int *nodecap = NULL;
int better_heuristic = 0;
int accepts = 0;
int nnodes = 0;
int partition_mechanism;
int on_line = 0;
int cycles_to_best = 0;

float sensitivity = .1;

static const int initial_temperature = 100;
static const int temp_prob = 130;

int refreshed = 0;

tbgraph G(1, 1);
node_array<int> bestnodes, absnodes;
node_array<toponode*> physnodes,absphys;
float                       bestscore, absbest;

float *interlinks;
int *numnodes;

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
 * score performs two functions at the moment.  First, it actually
 * performs an assignment of the virtual nodes in a switch to
 * physical nodes in that switch.  Then it returns a numeric evaluation
 * of the current mapping.
 *
 * There are several problems it solves:
 *
 *  a)  Assigning generic vnodes to nodes.
 *
 *      This is performed by searching a list of the nodes and assigning
 *      the virtual node to a node with the minimum necessary number
 *      of interfaces.
 *
 *  b)  Assigning delay vnodes to delay nodes:
 *
 *      Since a delay node may support many delay vnodes, this is a
 *      unit weight knapsack problem.  I take the easy out here
 *      by simply assigning the delay vnodes to nodes as they
 *      come along in the list.  This works for finding a feasible
 *      solution, but does not optimize the use of physical nodes.
 *
 *  c)  Scoring
 *
 *      Scoring is handled by:
 *       i)  add 1 for each unassigned node (excess nodes or too many
 *           interfaces
 *      ii)  Add 1 for each link across switches in excess of the
 *           capacity
 *     iii)  Add .1 for each switch used to try to minimize the number
 *           of switches involved
 *      iv)  Add .1 for each unit of bandwidth between switches to try
 *           to minimize interswitch bandwidth consumption
 *
 *
 *  The score function is the big bottleneck of the program right
 *  now.  Unlike the earlier versions in which only changed nodes
 *  were updated, score recomputes the entire solution each time
 *  it's called.  This is very inefficient and should be fixed
 *  by having the scupdate function update only the parts of the
 *  topology which changed.  This needs to be looked at more,
 *  but should probably be delayed until the rest of the features
 *  have been added.
 */

float score()
{

	float sc = 0;

	for (int i = 0; i < nparts; i++) {
		/* XXX:  THIS MUST BE OPTIMIZED */
		
		/* Experimental:  Collapse delay nodes together
		 * and handle node fanout restrictions */

		/* Mark all nodes unused */
		for (int j = 0; j < topo->switches[i]->numnodes(); j++) {
			topo->switches[i]->nodes[j].used = 0;
		}
		
		int numdelays = 0;
		int assigned = 0;
		node n;
		node delays[MAX_DELAYS];
		forall_nodes(n, G) {
		    if (G[n].partition() == i) {
		        if (G[n].type() == testnode::TYPE_DELAY) {
				delays[numdelays++]=n;
			} else {
			    assigned = 0;
			    /* Assign to an available node */
			    for (int j = 0;
				 j < topo->switches[i]->numnodes();
				 j++) {
				    if (topo->switches[i]->nodes[j].used == 0 && (topo->switches[i]->nodes[j].ints >= G.degree(n))) {
					    topo->switches[i]->nodes[j].used = 1;
					    physnodes[n]=&(topo->switches[i]->nodes[j]);
					    assigned = 1;
					    break;
				    }
			    }
			    if (!assigned) {
				    sc += 1;
			    }
			}
		    }
		}
		int numn = numnodes[i];

		numn -= numdelays;
		/* Now turn normal nodes into delay nodes as needed */
		/* XXX:  THIS IS NOT OPTIMAL.  We should improve on this
		 * so it uses the right size nodes a bit... but oh well */
		int maxnodes = topo->switches[i]->numnodes();
		int j = 0;
		while (numdelays > 0 && j < maxnodes) {
			if (topo->switches[i]->nodes[j].used) { j++; continue; }
			for (int z=0;numdelays>0 && z<topo->switches[i]->nodes[j].ints/2;++z) {
				physnodes[delays[--numdelays]] = &(topo->switches[i]->nodes[j]);
			}
			topo->switches[i]->nodes[j].used = 1;
			j++;
		}

		/* Add in the unsatisfied delay nodes */
		if (numdelays > 0) {
			sc += numdelays;
		}

		/* Try to minimize the number of switches used */
		/* This is likely NOT an effective way to do it! */
		if (numnodes[i] > 0) {
			sc += .1;
		}
		/* Try to minimize the bandwidth used... also probably
		   not effective */
		if (interlinks[i] > 0) {
			sc += .001 * interlinks[i];
		}
		/* Have we violated bandwidth between switches? */
		
		if (interlinks[i] > intercap[i]) {
			sc += (interlinks[i]-intercap[i])/100;
		}
	}
	return sc;
}

/*
 * This is a completely bogus function.  It's a straight copy of the
 * score() function, but instead of incrementing a score counter,
 * it prints out a list of the constraints which are violated.
 *
 * In the future, this function should return something allowing us
 * to determine if a critical resource (nodes, interfaces, etc.) is
 * violated, or if a non-critical resource (interswitch bandwidth)
 * has been violated, so we can allow the user to proceed with a
 * potentially bad configuration if they so desire.
 *
 * ... and the "coding by copy" junk should be eliminated.  Jeez. :)
 */

void violated()
{
	for (int i = 0; i < nparts; i++) {
		/* Have we violated bandwidth between switches? */
		if (interlinks[i] > intercap[i]) {
			cout << "violated:  switch " << i << " bandwidth"
			     << endl;
		}
		for (int j = 0; j < topo->switches[i]->numnodes(); j++) {
			topo->switches[i]->nodes[j].used = 0;
		}
		
		int numdelays = 0;
		int assigned = 0;
		int unassigned = 0;
		node n;
		node delays[MAX_DELAYS];
		forall_nodes(n, G) {
		    if (G[n].partition() == i) {
		        if (G[n].type() == testnode::TYPE_DELAY) {
				delays[numdelays++]=n;
			} else {
			    /* Assign to an available node */
			    assigned = 0;
			    for (int j = 0;
				 j < topo->switches[i]->numnodes();
				 j++) {
				    if ((topo->switches[i]->nodes[j].used == 0) && (topo->switches[i]->nodes[j].ints >= G.degree(n))) {
					    topo->switches[i]->nodes[j].used = 1;
					    assigned = 1;
	  				    physnodes[n]=&(topo->switches[i]->nodes[j]);
					    break;
				    }
			    }
			    if (!assigned) {
				    unassigned++;
			    }
			}
		    }
		}
		if (unassigned > 0) {
			cout << "violated:  switch " << i << " had "
			     << unassigned << " unassigned nodes" << endl;
		}
		int numn = numnodes[i];

		numn -= numdelays;
		/* Now turn normal nodes into delay nodes as needed */
		/* XXX:  THIS IS NOT OPTIMAL.  We should improve on this
		 * so it uses the right size nodes a bit... but oh well */
		int maxnodes = topo->switches[i]->numnodes();
		int j = 0;
		while (numdelays > 0 && j < maxnodes) {
			if (topo->switches[i]->nodes[j].used) { j++; continue; }
			for (int z=0;numdelays>0 && z<topo->switches[i]->nodes[j].ints/2;++z) {
				// XXX: ugh
				physnodes[delays[--numdelays]] = &(topo->switches[i]->nodes[j]);
			}
			topo->switches[i]->nodes[j].used = 1;
			j++;
		}

		/* Add in the unsatisfied delay nodes */
		if (numdelays > 0) {
			cout << "violated:  switch " << i << " had "
			     << numdelays << " unassigned delay nodes"
			     << endl;
		}
	}
}


/*
 * Reset the interlinks and numnodes arrays to an accurate
 * value.  Requires an inspection of all nodes and edges.
 */

void screset() {
	edge e;
	node n;
	for (int i = 0; i < nparts; i++) {
		interlinks[i] = numnodes[i] = 0;
	}
    
	forall_nodes(n, G) {
		numnodes[G[n].partition()]++;
	}
	forall_edges(e, G) {
		node v = G.source(e);
		node w = G.target(e);
	
		if (G[v].partition() != G[w].partition()) {
			interlinks[G[v].partition()] += G[e].capacity();
			interlinks[G[w].partition()] += G[e].capacity();
		}
	}
}

/*
 * Move a node 'n' from its current switch to a new one, indicated by
 * newpos.
 *
 * Right now, this function performs the update logic to change the
 * node counts and the interswitch bandwidth, but does not cope
 * with the rest of the score.  In the future, it should perform
 * the incremental score update.  See the comments for the score()
 * function for more details.
 */

void scupdate(node n, int newpos)
{
	int prevpos;

	AdjIt it(G, n);
	prevpos = G[n].partition();
	if (newpos == prevpos) return;

	numnodes[prevpos]--;
	numnodes[newpos]++;

	while (it.eol() == false) {
		edge e = it.get_edge();
		node n1 = G.source(e);
		node n2 = G.target(e);
		/* Ensure that n2 points to the stationary node */
		if (n2 == n) n2 = n1;

		/* They were not in the same bucket to start with */
		/* So both contributed to their interlinks */
		if (G[n2].partition() != prevpos) {
			interlinks[prevpos] -= G[e].capacity();

			/* If they're together now, there's no interlink */
			if (G[n2].partition() == newpos) {
				interlinks[G[n2].partition()] -= G[e].capacity();
			} else { /* Otherwise, move the interlink */
				interlinks[newpos] += G[e].capacity();
			}
		}
		else /* They were in the same bucket.  They aren't anymore,
		      * or we would have exited earlier */
		{
			interlinks[G[n2].partition()] += G[e].capacity();
			interlinks[newpos] += G[e].capacity();
		}
		++it;
	}

	G[n].partition(newpos);
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
	screset();

	bestscore = score();
	absbest = bestscore;

	if (bestscore == 0) {
#ifdef VERBOSE
		cout << "Problem started optimal\n";
#endif
		return 1;
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
			oldpos = G[n].partition();

			newpos = oldpos;
			/* XXX:  Room for improvement. :-) */
			while (newpos == oldpos)
				newpos = random() % nparts;
			scupdate(n, newpos);
			newscore = score();
			if (newscore < 0.1f) {
				timeend = used_time(timestart);
				cout << "OPTIMAL (0.0) in "
				     << iters << " iters, "
				     << timeend << " seconds" << endl;
				return 1;
			}
			/* So it's negative if bad */
			scorediff = bestscore - newscore;

			if (newscore < bestscore || accept(scorediff, temp)) {
				bestnodes[n] = G[n].partition();
				bestscore = newscore;
				accepts++;
				if (newscore < absbest) {
					node n2;
					forall_nodes(n2, G) {
						absnodes[n2] = G[n2].partition();
						absphys[n2] = physnodes[n2];
					}
					absbest = newscore;
					cycles_to_best = iters;
				}
			} else { /* Reject this change */
				scupdate(n, oldpos);
			}
		}
      
		temp *= .9;
	}
	forall_nodes(n, G) {
		bestnodes[n] = absnodes[n];
	}
	bestscore = absbest;

	forall_nodes(n, G) {
		G[n].partition(absnodes[n]);
	}
	timeend = used_time(timestart);
	cout << "   BEST SCORE:  " << score() << " in "
	     << iters << " iters and " << timeend << " seconds" << endl;
	cout << "With " << accepts << " accepts of increases\n";
	cout << "Iters to find best score:  " << cycles_to_best << endl;
#if 0
	for (int i = 0; i < nparts; i++) {
		if (numnodes[i] > nodecap[i]) {
			cout << "node " << i << " has "
			     << numnodes[i] << " nodes" << endl;
		}
		if (interlinks[i] > intercap) {
			cout << "node " << i << " has "
			     << interlinks[i] << " links" << endl;
		}
	}
	if (score() < 0.0001) {
		return 1; /* Optimal enough */
	} else {
		return 0;
	}
#endif
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
	physnodes.init(G, 0);
	absphys.init(G, 0);
    
	nnodes = G.number_of_nodes();
	optimal = assign();
	totaltime = used_time(timestart);
	violated();
	cout << "Total time to find solution "
	     << totaltime << " seconds" << endl;
}

/*
 * If we have more ways of partitioning the graph other than just
 * simulated annealing, throw them in here.
 */

void chopgraph(GraphWin& gw) {
	node n;
	forall_nodes(n, G) {
		G[n].partition(0);
	}
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
		forall_nodes(n, G) {
			G[n].partition(0);
		}
		if (on_line)
			chopgraph(gw);
	}
	
	refreshed = 0;
	
	/* Now color them according to their partition */
	forall_nodes(n, G) {
		switch(G[n].partition()) {
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
	
	forall_edges(e, G) {
		node v = G.source(e);
		node w = G.target(e);
		if (G[v].partition() == G[w].partition()) {
			gw.set_style(e, solid_edge);
		} else {
			gw.set_style(e, dashed_edge);
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
	node n;
	forall_nodes(n, G) {
		G[n].partition(0);
	}
	bestnodes.init(G, 0);
	absnodes.init(G, 0);
	physnodes.init(G, 0);
	absphys.init(G, 0);
	chopgraph(gw);
	refreshed = 1;
	display_scc(gw);
}


void new_edge_handler(GraphWin& gw, edge)  { display_scc(gw); }
void del_edge_handler(GraphWin& gw)        { display_scc(gw); }
void new_node_handler(GraphWin& gw, node)  { display_scc(gw); }
void del_node_handler(GraphWin& gw)        { display_scc(gw); }

void usage() {
	fprintf(stderr,
		"usage:  assign [-h] [-ao] [-s <switches>] [-n nodes/switch] [-c cap] [file]\n"
		"           -h ...... brief help listing\n"
		"           -s #  ... number of switches in cluster\n"
		"           -n #  ... number of nodes per switch\n"
		"           -a ...... Use simulated annealing (default)\n"
		"           -o ...... Update on-line (vs batch, default)\n"
		"           -t <file> Input topology desc. from <file>\n"
		);
}

void print_solution()
{
	node n;
	cout << "Best solution: " << absbest << endl;
	forall_nodes(n,G) {
		if (!absphys[n]) {
			cout << "unassigned: " << G[n].name() << endl;
		} else {
			cout << G[n].name() << " " << absnodes[n] << " " << PG[absphys[n]->n].name() << endl;
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
    
	while ((ch = getopt(argc, argv, "oas:n:t:h")) != -1)
		switch(ch) {
		case 'h': usage(); exit(0);
		case 's': nparts = atoi(optarg); break;
		case 'a': partition_mechanism = PARTITION_BY_ANNEALING; break;
		case 'o': on_line = 1; break;
		case 't': topofile = optarg; break;
		default: usage(); exit(-1);
		}

	argc -= optind;
	argv += optind;
    
	interlinks = new float[nparts];
	numnodes = new int[nparts];
	for (int i = 0; i < nparts; i++) {
		interlinks[i] = 0;
		numnodes[i] = 0;
	}
    
	srandom(time(NULL) + getpid());

	/*
	 * Set up the LEDA graph window environment.  Whenever
	 * the user does anything to the graph, call the
	 * proper handler.
	 */
	
	GraphWin gw(G, "Flux Testbed:  Simulated Annealing");
    
	gw.set_init_graph_handler(del_edge_handler);
	gw.set_new_edge_handler(new_edge_handler);
	gw.set_del_edge_handler(del_edge_handler);
	gw.set_new_node_handler(new_node_handler);
	gw.set_del_node_handler(del_node_handler);
    
	gw.set_node_width(24);
	gw.set_node_height(24);
    
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
		gw.update_graph();
		node n;
		forall_nodes(n, G) {
			if (G[n].name() == NULL) {
				G[n].name("");
			}
			gw.set_label(n, G[n].name());
			gw.set_position(n,
					point(random() % 200, random() % 200));
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
		topo=ptop_to_phys(PG);
		if (!topo) {
			cerr << "Could not convert ptop to phys: "
			     << topofile << endl;
			exit(-1);
		}
		nparts = topo->switchcount;
		cout << "Nparts: " << nparts << endl;
		nodecap = new int[nparts];
		intercap = new int[nparts];
		for (int i = 0; i < nparts; i++) {
			nodecap[i] = topo->switches[i]->numnodes();
		}
		for (int i = 0; i < nparts; i++) {
			intercap[i] = topo->switches[i]->bw;
		}
		topo->print_topo();
	}

	gw.display();
    
	gw.set_directed(false);
    
	gw.set_node_shape(circle_node);
	gw.set_node_label_type(user_label);
    
	h_menu = gw.get_menu("Layout");
	gw_add_simple_call(gw, reassign, "Reassign", h_menu);

	/* Run until the user quits.  Everything is handled by callbacks
	 * from LEDA's event loop from here on.                           */
	
	gw.edit();

	print_solution();
    
	return 0;
}
