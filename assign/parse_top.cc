/*
 * Parse chris' ".top" file format into a LEDA graph
 */

#include <iostream.h>
#include <string.h>
#include <stdio.h>
#include <LEDA/graph_alg.h>
#include <LEDA/graphwin.h>
#include <LEDA/ugraph.h>
#include <LEDA/dictionary.h>
#include <LEDA/map.h>
#include <LEDA/graph_iterator.h>
#include <LEDA/node_pq.h>
#include <LEDA/sortseq.h>

#include "common.h"
#include "vclass.h"
#include "virtual.h"

extern dictionary<string,node> vname2node;
extern dictionary<string,string> fixed_nodes;
extern list<string> vtypes;
extern tb_vgraph G;

extern node_pq<int> unassigned_nodes;

int parse_top(tb_vgraph &G, istream& i)
{
  node no1;
  string s1, s2;
  char inbuf[255];
  char n1[1024], n2[1024],emustring[1024];
  char lname[1024];
  int num_nodes = 0;
  int bw;
  int r;
  dictionary<string,tb_vclass*> vclass_map;
  bool emulated=false;
  
  while (!i.eof()) {
    char *ret;
    i.getline(inbuf, 254);
    ret = strchr(inbuf, '\n');
    if (ret) *ret = 0;
    if (strlen(inbuf) == 0) { continue; }
	    
    if (!strncmp(inbuf, "node", 4)) {
      char *snext = inbuf;
      char *scur = strsep(&snext," ");
      if (strcmp("node",scur) != 0) {
	fprintf(stderr, "bad node line: %s\n", inbuf);
      } else {
	scur=strsep(&snext," ");
	num_nodes++;
	string s1(scur);
	no1 = G.new_node();
	unassigned_nodes.insert(no1,random());
#ifdef GRAPH_DEBUG
	cout << "Found virt. node '"<<scur<<"'\n";
#endif
	G[no1].name=string(scur);
	G[no1].posistion = 0;
	G[no1].no_connections=0;
	vname2node.insert(s1, no1);
	scur=strsep(&snext," ");
	dic_item dit = vclass_map.lookup(string(scur));
	if (dit != nil) {
	  G[no1].type = string("");
	  G[no1].vclass = vclass_map.inf(dit);
	} else {
	  G[no1].type=string(scur);
	  G[no1].vclass=NULL;
	  vtypes.push(G[no1].type);
	}
	G[no1].fixed = false;	// this may get set to true later
	
	/* Read in desires */
	while ((scur=strsep(&snext," ")) != NULL) {
	  char *desire = scur;
	  char *t;
	  double iweight;
	  t = strsep(&desire,":");
	  string sdesire(t);
	  if ((! desire) || sscanf(desire,"%lg",&iweight) != 1) {
	    fprintf(stderr,"Bad desire specifier for %s\n",t);
	    iweight = 0.01;
	  }
	  G[no1].desires.insert(sdesire,iweight);
	}
      }
    } else if (!strncmp(inbuf, "link", 4)) {
      r=sscanf(inbuf, "link %s %s %s %d %s", lname, n1, n2,&bw,emustring);
      if ((r <= 3) || ((r == 5) && strcmp(emustring,"emulated"))) {
	fprintf(stderr, "bad link line: %s\n", inbuf);
      } else {
	if (r == 4) {
	  emulated = false;
	} else {
	  emulated = true;
	}
	string s1(n1);
	string s2(n2);
	edge e;
	node node1 = vname2node.access(s1);
	node node2 = vname2node.access(s2);
	e = G.new_edge(node1, node2);
	G[e].bandwidth = bw;
	G[e].type = tb_vlink::LINK_UNKNOWN;
	G[e].plink = NULL;
	G[e].plink_two = NULL;
	G[e].name = string(lname);
	G[e].emulated = emulated;
      }
    } else if (! strncmp(inbuf, "fix-node",8)) {
      r=sscanf(inbuf,"fix-node %s %s",n1,n2);
      if (r != 2) {
	fprintf(stderr, "bad fix-node line: %s\n",inbuf);
      } else {
	string s1(n1);
	string s2(n2);
	fixed_nodes.insert(s1,s2);
      }
    } else if (! strncmp(inbuf, "make-vclass",11)) {
      char *snext = inbuf;
      char *scur = strsep(&snext," ");
      if (strcmp("make-vclass",scur) != 0) {
	fprintf(stderr,"bad vclass line: %s\n",inbuf);
      } else {
	scur=strsep(&snext," ");
	string s(scur);
	scur=strsep(&snext," ");
	double weight;
	if (sscanf(scur,"%lg",&weight) != 1) {
	  fprintf(stderr,"bad vclass weight: %s\n",inbuf);
	  weight=0.5;
	}
	tb_vclass *v = new tb_vclass(s,weight);
	vclass_map.insert(s,v);
	while ((scur = strsep(&snext, " ")) != NULL) {
	  string s2(scur);
	  v->add_type(s2);
	  vtypes.push(s2);
	}
      }
    } else {
      fprintf(stderr, "unknown directive: %s\n", inbuf);
    }
  }
  return num_nodes;
}
