/*
 * parse ptop files.  These are basic topologies
 * that are used to represent the physical topology.
 *
 * format:
 * node <name> <type>
 *   <type> = pc | switch | dnard
 * link <src> <dst> <size> <number>
 */

#include <iostream.h>
#include <string.h>
#include <stdio.h>
#include <LEDA/graph_alg.h>
#include <LEDA/graphwin.h>
#include <LEDA/dictionary.h>
#include <LEDA/map.h>
#include <LEDA/graph_iterator.h>

#include "testbed.h"

int ptop_type(char *type)
{
  if (strcmp(type,"pc") == 0) return testnode::TYPE_PC;
  if (strcmp(type,"switch") == 0) return testnode::TYPE_SWITCH;
  if (strcmp(type,"dnard") == 0) return testnode::TYPE_DNARD;
  
  return -1;
}

void parse_ptop(tbgraph &G, istream& i)
{
	dictionary<string, node> nmap;
	node no1;
	edge ed1;
	string s1, s2;
	char inbuf[255];
	char n1[32], n2[32];
	char type[32];
	int itype;
	int size, num;
	
	while (!i.eof()) {
	    char *ret;
	    i.getline(inbuf, 254);
	    ret = strchr(inbuf, '\n');
	    if (ret) *ret = 0;
	    if (strlen(inbuf) == 0) { continue; }
	    
	    if (!strncmp(inbuf, "node", 4)) {
		if (sscanf(inbuf, "node %s %s", n1, type) != 2) {
		    fprintf(stderr, "bad node line: %s\n", inbuf);
		} else {
		    string s1(n1);
		    itype=ptop_type(type);
		    if (itype == -1) {
		      fprintf(stderr, "bad node type for node %s: %s\n",n1,type);
		    } else {
		      no1 = G.new_node();
		      G[no1].name(n1);
		      G[no1].type(itype);
		      nmap.insert(s1, no1);
		    }
		}
	    }
	    else if (!strncmp(inbuf, "link", 4)) {
		if (sscanf(inbuf, "link %s %s %d %d", n1, n2, &size, &num)
		    != 4) {
		    fprintf(stderr, "bad link line: %s\n", inbuf);
		} else {
		    string s1(n1);
		    string s2(n2);
		    node node1 = nmap.access(s1);
		    node node2 = nmap.access(s2);
		    ed1=G.new_edge(node1, node2);
		    G[ed1].capacity(size);
		    G[ed1].number(num);
		}
	    }
	    else {
		fprintf(stderr, "unknown directive: %s\n", inbuf);
	    }
	}
}

