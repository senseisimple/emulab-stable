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

#include "testbed.h"

void parse_top(tbgraph &G, istream& i)
{
	dictionary<string, node> nmap;
	node no1;
	string s1, s2;
	char inbuf[255];
	char n1[32], n2[32];
	
	while (!i.eof()) {
	    char *ret;
	    i.getline(inbuf, 254);
	    ret = strchr(inbuf, '\n');
	    if (ret) *ret = 0;
	    if (strlen(inbuf) == 0) { continue; }
	    
	    if (!strncmp(inbuf, "node", 4)) {
		if (sscanf(inbuf, "node %s", n1) != 1) {
		    fprintf(stderr, "bad node line: %s\n", inbuf);
		} else {
		    string s1(n1);
		    no1 = G.new_node();
		    nmap.insert(s1, no1);
		}
	    }
	    else if (!strncmp(inbuf, "link", 4)) {
		if (sscanf(inbuf, "link %s %s", n1, n2) != 2) {
		    fprintf(stderr, "bad link line: %s\n", inbuf);
		} else {
		    string s1(n1);
		    string s2(n2);
		    node node1 = nmap.access(s1);
		    node node2 = nmap.access(s2);
		    G.new_edge(node1, node2);
		}
	    }
	    else {
		fprintf(stderr, "unknown directive: %s\n", inbuf);
	    }
	}
}
