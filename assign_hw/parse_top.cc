/*
 * Parse chris' ".top" file format into a LEDA graph
 */

#include <iostream.h>
#include <string.h>
#include <stdio.h>
#include <LEDA/graph_alg.h>
#include <LEDA/graphwin.h>
#include <LEDA/dictionary.h>
#include <LEDA/map.h>
#include <LEDA/graph_iterator.h>
#include <LEDA/node_pq.h>

#include "common.h"
#include "virtual.h"

extern tb_vgraph G;

extern node_pq<int> unassigned_nodes;

int parse_top(tb_vgraph &G, istream& i)
{
  dictionary<string, node> nmap;
  node no1;
  string s1, s2;
  char inbuf[255];
  char n1[32], n2[32], type[32];
  int num_nodes = 0;
  
  while (!i.eof()) {
    char *ret;
    i.getline(inbuf, 254);
    ret = strchr(inbuf, '\n');
    if (ret) *ret = 0;
    if (strlen(inbuf) == 0) { continue; }
	    
    if (!strncmp(inbuf, "node", 4)) {
      if (sscanf(inbuf, "node %s %s", n1, type) < 1) {
	fprintf(stderr, "bad node line: %s\n", inbuf);
      } else {
	num_nodes++;
	string s1(n1);
	no1 = G.new_node();
	unassigned_nodes.insert(no1,random());
	G[no1].name=strdup(n1);
	G[no1].posistion = 0;
	G[no1].no_connections=0;
	nmap.insert(s1, no1);
	if (!strcmp(type, "delay")) {
	  G[no1].type = TYPE_DELAY;
	}
	else if (!strcmp(type, "pc")) {
	  G[no1].type = TYPE_PC;
	}
	else if (!strcmp(type, "switch")) {
	  fprintf(stderr,"Can not have switch's in top file\n");
	  G[no1].type = TYPE_UNKNOWN;
	}
	else if (!strcmp(type, "dnard")) {
	  G[no1].type = TYPE_DNARD;
	}
	else {
	  fprintf(stderr,"Unknown type in top file: %s\n",type);
	  G[no1].type = TYPE_UNKNOWN;
	}
      }
    }
    else if (!strncmp(inbuf, "link", 4)) {
      if (sscanf(inbuf, "link %s %s", n1, n2) != 2) {
	fprintf(stderr, "bad link line: %s\n", inbuf);
      } else {
	string s1(n1);
	string s2(n2);
	edge e;
	node node1 = nmap.access(s1);
	node node2 = nmap.access(s2);
	e = G.new_edge(node1, node2);
	// XXX - add more bandwidth code
	G[e].bandwidth = 10;
	G[e].type = tb_vlink::LINK_UNKNOWN;
	G[e].plink = NULL;
	G[e].plink_two = NULL;
      }
    }
    else {
      fprintf(stderr, "unknown directive: %s\n", inbuf);
    }
  }
  return num_nodes;
}
