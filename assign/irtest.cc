#include <iostream.h>
#include <fstream.h>
#include <LEDA/graph_alg.h>
#include <LEDA/graphwin.h>
#include <LEDA/dictionary.h>
#include <LEDA/map.h>
#include <LEDA/graph_iterator.h>


#include "testbed.h"

int main(int argc, char **argv)
{
	tbgraph G(1,1);
	ifstream infile;
	
	if (argc < 2) {
		cerr << "Need to supply a filename, buckeroo" << endl;
		exit(-1);
	}

	infile.open(argv[1]);
	
	parse_ir(G, infile);
	cout << "Done\n";
	exit(0);
}
