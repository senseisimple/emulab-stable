/*
 * Input a simple topology.
 */

#include <iostream.h>
#include <fstream.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * Simple topology description:
 *
 * topology   :: 1 or more lines
 * line       :: <comment> | <switch>
 * comment    :: # followed by anything
 * switch     :: <switchname> <nodecount> <nodelist>
 *
 * nodelist   :: <interfaces> <count> <nodelist>
 *
 * switchname :: [a-z0-9_]+
 * interfaces :: number
 * count      :: number
 *
 * Example:
 *
 * # A switch with 6 computers with 4 interfaces, and 2 computers with 8
 * # interfaces
 * switch1 2 4 6 8 2
 * # A switch with 8 computers with 4 interfaces
 * switch2 1 4 8
 *
 */

#define MAXSW 32

#include "phys.h"

topology *parse_phys(char *filename)
{
	char name[64];
	char linebuf[256];
	ifstream infile(filename);
	topology *topo;

	if (!infile.good()) { return NULL; }

	topo = new topology(MAXSW); // XXX
	
	topo->switchcount = 0;

	for (int i = 0; i < MAXSW; i++)
		topo->switches[i] = NULL;
	
	while (!infile.eof()) {
		infile >> name;
		if (infile.eof() || !infile.good()) { break; }
		if (name[0] == '#') {
			infile.getline(linebuf, 255);
		} else if (name[0] == 0) {
			continue;
		} else {
			int nodecount;
			infile >> nodecount;
			tbswitch *newsw;
			newsw = new tbswitch(nodecount, name);
			topo->switches[topo->switchcount++] = newsw;
#ifdef VERBOSE
			printf("Creating switch %d : %s\n",
			       topo->switchcount-1, name);
#endif
			if (!newsw) {
				perror("Could not allocate a tbswitch");
				exit(-1);
			}
			for (int i = 0; i < nodecount; i++) {
				infile >> newsw->nodes[i].ints;
				infile >> newsw->nodes[i].count;
				newsw->nodes[i].used = 0;
#ifdef VERBOSE
				printf("Adding node %d with %d %d\n",
				       i, newsw->nodes[i].ints,
				       newsw->nodes[i].count);
#endif
			}
		}
	}
	return topo;
}
