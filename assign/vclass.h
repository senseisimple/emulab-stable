/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef __VCLASS_H
#define __VCLASS_H

#include "port.h"
#include "fstring.h"

/*
 * We have to do these includes differently depending on which version of gcc
 * we're compiling with
 */
#ifdef NEW_GCC
#include <ext/hash_map>
using namespace __gnu_cxx;
#else
#include <hash_map>
#endif

#include <iostream>
using namespace std;

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
  tb_vclass(fstring n,double w) : name(n), weight(w), score(0) {;}

  typedef hash_map<fstring,int> members_map;   
    
  void add_type(fstring type);	// Add a member of a certain type to the
				// vclass

  bool has_type(fstring type) const; // Does the vclass contain the given type?

  bool empty() const;                // True if no vnodes use this vlcass

  // The next two routines report the *change* in score.  The score
  // for the vclass as a whole is 0 if all nodes are of the dominant
  // type and the weight if they aren't.
  double assign_node(fstring type);
  double unassign_node(fstring type);

  // Semi randomly choose a type from a vclass.
  fstring choose_type() const;

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
  
  // Get the list of members
  const members_map &get_members() const {
    return(members);
  }
  
  // Just get the name
  fstring get_name() const {
    return(name);
  }
  
  // Get the name of the dominant type
  fstring get_dominant() const {
    return(dominant);
  }

private:
      
  fstring name;			// Name of the vclass

  members_map members;		// Maps type to number of members of that
				// type.
  
  double weight;		// Weight of class
  double score;			// the current score of the vclass
  
  fstring dominant;		// Current dominant type

};

typedef hash_map<fstring,tb_vclass*> name_vclass_map;
extern name_vclass_map vclass_map;

#endif
