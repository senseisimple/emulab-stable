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

/* How can we chop things up? */
#define PARTITION_BY_ANNEALING 0

int nparts = 3;     /* DEFAULTS */
int nodecap   = 4;
int intercap = 2;
int better_heuristic = 0;
int accepts = 0;
int nnodes = 0;
int partition_mechanism;
int on_line = 0;

float sensitivity = .1;

static const int initial_temperature = 100;
static const int temp_prob = 130;

int refreshed = 0;

tbgraph G(1, 1);
node_array<int> bestnodes, absnodes;
float                       bestscore, absbest;

float *interlinks;
float *numnodes;

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

float score()
{

	float sc = 0;

	/* Then rank our score by them */

	for (int i = 0; i < nparts; i++) {
		if (interlinks[i] > intercap) {
			sc += (interlinks[i]-intercap);
		}
		if (numnodes[i] > nodecap) {
			sc += (numnodes[i] - nodecap);
		}
	}

	return sc;
}

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
			interlinks[G[v].partition()]++;
			interlinks[G[w].partition()]++;
		}
	}
}

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
			interlinks[prevpos]--;

			/* If they're together now, there's no interlink */
			if (G[n2].partition() == newpos) {
				interlinks[G[n2].partition()]--;
			} else { /* Otherwise, move the interlink */
				interlinks[newpos]++;
			}
		}
		else /* They were in the same bucket.  They aren't anymore,
		      * or we would have exited earlier */
		{
			interlinks[G[n2].partition()]++;
			interlinks[newpos]++;
		}
		++it;
	}

	G[n].partition(newpos);
}

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
					}
					absbest = newscore;
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
	for (int i = 0; i < nparts; i++) {
		if (numnodes[i] > nodecap) {
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
}

void loopassign()
{
	node_array<int> nodestorage;
	int optimal = 0;
	int orig_nparts;
	int orig_cap;
	float timestart = used_time();
	float totaltime;

	nodestorage.init(G, 0);
	bestnodes.init(G, 0);
	absnodes.init(G, 0);
    
	nnodes = G.number_of_nodes();
    
	int reqnodes = (int) ceilf((float)nnodes / (float)nodecap);
	cout << "Reqnodes:  " << reqnodes;

	orig_nparts = nparts;
	orig_cap    = intercap;
    
	if (nnodes < 1) return;
	/* XXX:  Should make this a binary search instead */
	/* Maybe.  It probably won't matter much */
	optimal = assign();
	if (!optimal) return;
	nparts--;
	while (optimal && nparts >= reqnodes) {
		node tmpnode;
		forall_nodes(tmpnode, G) {
			nodestorage[tmpnode] = G[tmpnode].partition();
			G[tmpnode].partition(0);
		}
		bestnodes.init(G, 0);
		absnodes.init(G, 0);
		optimal = assign();
		if (!optimal) { break; }
		if (optimal) {
			cout << "Found new optimal with " << nparts
			     << " switches\n";
		}
		nparts--;
	}
	nparts++;
	if (!optimal) {
		node tmpnode;
		forall_nodes(tmpnode, G) {
			G[tmpnode].partition(nodestorage[tmpnode]);
		}
	}

	optimal = 1;
	/* And now search for the minimum link capacity */
	intercap--;
	while (optimal && intercap > 0) {
		node tmpnode;
		forall_nodes(tmpnode, G) {
			nodestorage[tmpnode] = G[tmpnode].partition();
			G[tmpnode].partition(0);
		}
		bestnodes.init(G, 0);
		absnodes.init(G, 0);
		cout << "Searching with " << nparts << " switches and "
		     << intercap << " edge capacity" << endl;
		optimal = assign();
		if (optimal) {
			cout << "Found new optimal with "
			     << nparts << " switches\n";
		} else {
			break;
		}
		intercap--;
	}
	intercap++;
	if (!optimal) {
		node tmpnode;
		forall_nodes(tmpnode, G) {
			G[tmpnode].partition(nodestorage[tmpnode]);
		}
		cout << "Reverting to optimal solution, "
		     << nparts << " switches "
		     << intercap << " capacity" << endl;
		intercap++;
	}
	nparts = orig_nparts;
	intercap = orig_cap;
	totaltime = used_time(timestart);
	cout << "Total time to find solution "
	     << totaltime << " seconds" << endl;
}

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

void reassign(GraphWin& gw)
{
	node n;
	forall_nodes(n, G) {
		G[n].partition(0);
	}
	bestnodes.init(G, 0);
	absnodes.init(G, 0);
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
		"usage:  ann [-h] [-ao] [-s <switches>] [-n nodes/switch] [-c cap]\n"
		"           -h ...... brief help listing\n"
		"           -s #  ... number of switches in cluster\n"
		"           -n #  ... number of nodes per switch\n"
		"           -c #  ... inter-switch capacity (bw)\n"
		"                 (# of links which can go between switches\n"
		"           -a ...... Use simulated annealing (default)\n"
		"           -o ...... Update on-line (vs batch, default)\n"
		);
}

int main(int argc, char **argv)
{
	int h_menu;
	extern char *optarg;
	extern int optind;
    
	int ch;

	partition_mechanism = PARTITION_BY_ANNEALING;
    
	while ((ch = getopt(argc, argv, "oas:n:c:h")) != -1)
		switch(ch) {
		case 'h': usage(); exit(0);
		case 'n': nodecap = atoi(optarg); break;
		case 's': nparts = atoi(optarg); break;
		case 'c': intercap = atoi(optarg); break;
		case 'a': partition_mechanism = PARTITION_BY_ANNEALING; break;
		case 'o': on_line = 1; break;
		default: usage(); exit(-1);
		}

	argc -= optind;
	argv += optind;
    
	interlinks = new float[nparts];
	numnodes = new float[nparts];
	for (int i = 0; i < nparts; i++) {
		interlinks[i] = 0;
		numnodes[i] = 0;
	}
    
	srandom(time(NULL) + getpid());
    
	GraphWin gw(G, "Flux Testbed:  Simulated Annealing");
    
	gw.set_init_graph_handler(del_edge_handler);
	gw.set_new_edge_handler(new_edge_handler);
	gw.set_del_edge_handler(del_edge_handler);
	gw.set_new_node_handler(new_node_handler);
	gw.set_del_node_handler(del_node_handler);
    
	gw.set_node_width(24);
	gw.set_node_height(24);
    
	if (argc == 1) {
		ifstream infile;
		infile.open(argv[0]);
		/* XXX:  Check my return value, please */
		parse_top(G, infile);
		gw.update_graph();
		node n;
		forall_nodes(n, G) {
			gw.set_label(n, G[n].name());
			gw.set_position(n,
					point(random() % 200, random() % 200));
		}
	}
    
	gw.display();
    
	gw.set_directed(false);
    
	gw.set_node_shape(circle_node);
	gw.set_node_label_type(user_label);
    
	h_menu = gw.get_menu("Layout");
	gw_add_simple_call(gw, reassign, "Reassign", h_menu);
    
	gw.edit();
    
	return 0;
}
