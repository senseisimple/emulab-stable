/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef __VCLASS_H
#define __VCLASS_H

class tb_vclass {
public:
  tb_vclass(string n,double w) : name(n), weight(w), score(0) {;}
  
  string name;

  dictionary<string,int> members;
  string dominant;
  double weight;
  double score;			// the current score of the vclass
  
  void add_type(string type);

  // The next two routines report the *change* in score.  The score
  // for the vclass as a whole is 0 if all nodes are of the dominant
  // type and the weight if they aren't.
  double assign_node(string type);
  double unassign_node(string type);
  
  string choose_type();

  void dump_data();
};
  
#endif
