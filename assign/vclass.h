/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef __VCLASS_H
#define __VCLASS_H

// tb_vclass represents a virtual equivalence class.  The main purpose
// of the code here is to monitor which members of the class are being
// used, what types that are being used as, and calculating the
// appropriate score changes.  Each vclass gives some contribution to
// the score and as nodes are assigned and unassigned this score changes.

// The membership of nodes is not stored here.  Rather each vnode
// stores which vclass it belongs to.  This class is more abstract and
// represents the scoring model.


class tb_vclass {
public:
  tb_vclass(crope n,double w) : name(n), weight(w), score(0) {;}

  crope name;			// Name of the vclass

  typedef hash_map<crope,int> members_map; 
  members_map members;		// Maps type to number of members of that
				// type.
  
  double weight;		// Weight of class
  double score;			// the current score of the vclass
  
  void add_type(crope type);	// Add a member of a certain type to the
				// vclass

  crope dominant;		// Current dominant type

  // The next two routines report the *change* in score.  The score
  // for the vclass as a whole is 0 if all nodes are of the dominant
  // type and the weight if they aren't.
  double assign_node(crope type);
  double unassign_node(crope type);

  // Semi randomly choose a type from a vclass.
  crope choose_type();

  friend ostream &operator<<(ostream &o, const tb_vclass& c)
  {
    o << "vclass: " << c.name << " dominant=" << c.dominant <<
      " weight=" << c.weight << " score=" << c.score << endl;
    o << "  ";
    for (members_map::const_iterator dit=c.members.begin();
	 dit!=c.members.end();++dit) {
      o << (*dit).first << ":" << (*dit).second << " ";
    }
    o << endl;
    return o;
  }
};

typedef hash_map<crope,tb_vclass*> name_vclass_map;

#endif
