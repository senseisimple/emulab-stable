
// This converts a ptop graph to a phys topology description.
// There are several requirements on the ptop graph for this.
//  1. Nodes must be connected to switches, not to other nodes.
//  2. Switches must be connected a switch node called CENTER.

// XXX : Needs lots of error checking!

#include <iostream.h>
#include <fstream.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAXSW 32
#define MAXNODES 32

#include "testbed.h"
#include "phys.h"

typedef struct {
	node n;
	int interfaces;
	int bw;
} nodeelement;

int nodearray_cmp(const void *A,const void *B)
{
	int a,b;
	nodeelement *pA,*pB;

	pA=(nodeelement*)A;
	a=pA->interfaces;
	pB=(nodeelement*)B;
	b=pB->interfaces;

	if (a<b) return -1;
	if (a>b) return 1;
	return 0;
}

topology *ptop_to_phys(tbgraph &G)
{
	topology *topo;
	
	topo = new topology(MAXSW); // XXX
	
	topo->switchcount = 0;

	for (int i = 0; i < MAXSW; i++)
		topo->switches[i] = NULL;
	
	// Find CENTER
	node center=NULL;
	node n;
	forall_nodes(n,G) {
		if (strcmp(G[n].name(),"CENTER") == 0) {
			center=n;
			break;
		}
	}
	if (! center) {
#ifdef VERBOSE
		cerr << "Could not find CENTER node." << endl;
#endif
		return NULL;
	}
	if (G[center].type() != testnode::TYPE_SWITCH) {
#ifdef VERBOSE
		cerr << "CENTER should be a switch." << endl;
#endif
		return NULL;
	}
	
	// Foreach switch (neighbor)
	edge e;
	tbswitch *s;
	nodeelement nodearray[MAXNODES];
	int numnodes;
	int switch_bw;
	forall_inout_edges(e,center) {
		node sw=G.source(e);
		if (sw == center)
			sw=G.target(e);

		if (G[sw].type() != testnode::TYPE_SWITCH) {
#ifdef VERBOSE
			cerr << G[sw].name() << " should be a switch!" << endl;
#endif
			return NULL;
		}
		switch_bw=G[e].capacity();
		
		//   Foreach node
		numnodes=0;
		forall_inout_edges(e,sw) {
			// XXX: could be improved
			if (G.source(e) == center ||
			    G.target(e) == center)
				continue;
			node n=G.source(e);
			if (n == sw)
				n=G.target(e);

			// XXX: need node type checking here.
			
			//     Add to node list.
			nodearray[numnodes].n=n;
			nodearray[numnodes].interfaces=G[e].number();
			nodearray[numnodes].bw=G[e].capacity();
			numnodes++;
		}
	  
		//   Sort node list.
		qsort(nodearray,numnodes,sizeof(nodeelement),&nodearray_cmp);

		//   Create tbswitch with node list and count.
		tbswitch *newsw = new tbswitch(numnodes, G[sw].name());
		for (int i=0;i<numnodes;++i) {
			toponode *tnode = new toponode;
			tnode->ints=nodearray[i].interfaces;
			tnode->used=0;
			tnode->int_bw=nodearray[i].bw;
			tnode->n=nodearray[i].n;
			newsw->nodes[i]=*tnode;
		}
		newsw->bw=switch_bw;
	  
		//   Add to topology.
		topo->switches[topo->switchcount++] = newsw;
	}

	return topo;
}
