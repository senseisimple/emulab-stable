/*
 * Parse the IR format into a graph type structure.
 *
 * For ease of use, I'm going to assume the following
 * capabilities with the graph:
 *
 * Graph G;
 * node n;
 * edge e;
 *
 * n = G.new_node();
 * n.set_name(char *name);
 */

#include <iostream.h>
#include <string.h>
#include <LEDA/graph_alg.h>
#include <LEDA/graphwin.h>
#include <LEDA/dictionary.h>
#include <LEDA/map.h>
#include <LEDA/graph_iterator.h>

#include "testbed.h"

void parse_ir(tbgraph& G, istream& i)
{
	char buf[256];
	char *hash;

	while (!i.eof())
	{
		i.getline(buf, sizeof(buf));
		hash = strchr(buf, '#');
		if (hash) *hash = 0;
		if (strlen(buf) < 1) { continue; }
		printf("Got line:  %s\n", buf);
	}
}
