#include <iostream.h>
#include <stdlib.h>
#include <string.h>
#include "phys.h"

toponode::toponode() {
	ints = used = 0;
}

tbswitch::tbswitch() {
	nodes = NULL;
	nodecount = 0;
}

tbswitch::tbswitch(int ncount, char *newname) {
	nodecount = ncount;
	nodes = new toponode[ncount];
	name = strdup(newname);
}

void tbswitch::setsize(int ncount) {
	if (nodes) { delete(nodes); }
	nodecount = ncount;
	nodes = new toponode[ncount];
}

tbswitch::~tbswitch() {
	if (nodes) { delete(nodes); }
	nodecount = 0;
}

inline int tbswitch::numnodes() {
	return nodecount;
}

topology::topology(int nswitches) {
	switchcount = nswitches;
	switches = new tbswitch *[switchcount];
}

topology::~topology() {
	if (switches) delete(switches);
}

void topology::print_topo() {
	cout << "Topology has " << switchcount << " switches" << endl;
	cout << "--------" << endl;
	for (int i = 0; i < switchcount; i++) {
		cout <<  " sw " << i << " has " << switches[i]->numnodes() << " nodes"
		     << endl;
		for (int j = 0; j < switches[i]->nodecount; j++) {
			cout << switches[i]->nodes[j].ints << " ";
		}
		cout << endl;
	}

}
