
#include <LEDA/graph_alg.h>
#include <LEDA/graphwin.h>
#include <LEDA/ugraph.h>
#include <LEDA/dictionary.h>
#include <LEDA/map.h>
#include <LEDA/graph_iterator.h>
#include <LEDA/node_pq.h>
#include <LEDA/sortseq.h>
#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "vclass.h"
#include "virtual.h"

void tb_vclass::add_type(string type)
{
  members.insert(type,0);
  if (members.lookup(dominant) == 0) {
    dominant = type;
  }
}

double tb_vclass::assign_node(string type)
{
  double new_score = score;
  dic_item dit = members.lookup(type);
  int newnum = members.inf(dit) + 1;
  members.change_inf(dit,newnum);

  if (type != dominant) {
    if (members.access(dominant) != 0) {
      new_score = weight;
    }
    if (newnum > members.access(dominant)) {
      dominant = type;
    }
  }

  double delta = new_score-score;
  score=new_score;
  return delta;
}

double tb_vclass::unassign_node(string type)
{
  double new_score = 0;
  dic_item dit = members.lookup(type);
  int newnum = members.inf(dit) - 1;
  members.change_inf(dit,newnum);

  int curmax = 0;
  forall_items(dit,members) {
    int n = members.inf(dit);
    if (n > curmax) {
      if (curmax != 0) {
	new_score = weight;
      }
      curmax = n;
      dominant = members.key(dit);
    }
  }

  
  double delta = new_score-score;
  score=new_score;
  return delta;
}

string tb_vclass::choose_type()
{
  // This may take some tweaking
  if (random()%2 == 0) {
    return dominant;
  }
  int r = random()%members.size();
  dic_item dit;
  forall_items(dit,members) {
    if (r == 0) break;
    r--;
  }
  return members.key(dit);
}

void tb_vclass::dump_data()
{
  cerr << "vclass: " << name;
  cerr << " dominant: " << dominant;
  cerr << " weight: " << weight;
  cerr << " score: " << score << endl;
  dic_item dit;
  cerr << "  ";
  forall_items(dit,members) {
    cerr << members.key(dit) << ":" << members.inf(dit) << " ";
  }
  cerr << endl;
}
