#include <LEDA/graph_alg.h>
#include <LEDA/graphwin.h>
#include <LEDA/dictionary.h>
#include <LEDA/map.h>
#include <LEDA/graph_iterator.h>
#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#include "testbed.h"

int main(int argc, char **argv)
{
  tbgraph G(1,1);
  
  GraphWin gw(G, "Flux Testbed: Physical Topology");
  
  ifstream infile;
  infile.open(argv[1]);
  parse_ptop(G,infile);
  gw.update_graph();
  node n;
  forall_nodes(n,G) {
    gw.set_label(n,G[n].name());
    gw.set_position(n,point(random()%200,random()%200));
  }

  gw.display();
  gw.set_directed(false);
  gw.set_node_shape(circle_node);
  gw.set_node_label_type(user_label);
  gw.edit();
}
