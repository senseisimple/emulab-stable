/*
 * A simple representation of the physical topology.
 *
 * Nodes are simply represented by a number of interfaces.
 *
 * The assumption is that all of those interfaces are connected
 * to the same switch.
 *
 * Note that nodes MUST be stored in their list in order of increasing
 * number of interfaces.  A good 10-minute project would be to
 * create an "addnode" method which does a sorted insert in the
 * list, or a sort function.
 *
 * This sorting is required by the assignment routine.
 */

#include <LEDA/graph_alg.h>

class toponode {
public:
	toponode();
	int ints;		/* Number of ethernet interfaces */
	int int_bw;		/* Bandwidth used by each interface */
	int used;		/* Used by assign.  Have we assigned here? */
	node n;
};

class tbswitch {
public:
	tbswitch();
	virtual ~tbswitch();
	tbswitch(int ncount, char *newname);	// Specify number of toponodes
	void setsize(int ncount);

	inline int numnodes();
	
	int nodecount;          /* Total number of nodes in the switch */
	toponode *nodes;	/* Sorted list of the nodes */
	char *name;		/* Anything you want */
	int bw;			/* Bandwidth between switch and center */
};

class topology {
public:
	topology(int nswitches);
	void print_topo();
	virtual ~topology();
	int switchcount;	// Number of switches in the topology
	tbswitch **switches;	// Unordered list of the switches
};

/*
 * Parse a simple physical topology description into a topology structure.
 *
 * Eventually, this will be replaced by parsing Chris' ptop format
 * into the topology structure
 */

topology *parse_phys(char *filename);
topology *ptop_to_phys(tbgraph &G);
