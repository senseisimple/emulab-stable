/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef __PCLASS_H
#define __PCLASS_H

// tb pnode list is a data structure that acts like list but has
// O(1) removal.  It is a list of tb_pnode*.
class tb_pnodelist {
public:
  list<tb_pnode*> L;
  dictionary<tb_pnode*,list_item> D;

  list_item push_front(tb_pnode *p) {
    list_item it = L.push_front(p);
    D.insert(p,it);
    return it;
  };
  list_item push_back(tb_pnode *p) {
    list_item it = L.push_back(p);
    D.insert(p,it);
    return it;
  };
  int remove(tb_pnode *p) {
    if (exists(p)) {
      list_item it = D.access(p);
      L.del_item(it);
      D.del(p);
      return 0;
    } else {
      return 1;
    }
  };
  int exists(tb_pnode *p) {
    return (D.lookup(p) != nil);
  }
  tb_pnode *front() {
    list_item it = L.first();
    return ((it == nil) ? NULL : L[it]);
  };
};

class tb_pclass {
public:
  tb_pclass() {
    size=0;
    used=0;
  }
  
  int add_member(tb_pnode *p);

  string name;			// purely for debugging
  int size;
  double used;
  dictionary<string,tb_pnodelist*> members;
};

typedef list<tb_pclass*> pclass_list;
typedef array<tb_pclass*> pclass_array;
typedef two_tuple<int,pclass_array*> tt_entry;
typedef dictionary<string,tt_entry> pclass_types;

/* Constants */
#define PCLASS_BASE_WEIGHT 1
#define PCLASS_NEIGHBOR_WEIGHT 1
#define PCLASS_UTIL_WEIGHT 1
#define PCLASS_FDS_WEIGHT 2

/* routines defined in pclass.cc */
int generate_pclasses(tb_pgraph &PG);// sets pclasses and type_table globals

/* The following two routines sets and remove mappings in pclass
   datastructures */
int pclass_set(tb_vnode &v,tb_pnode &p);
int pclass_unset(tb_pnode &p);

void pclass_debug();		// dumps debug info
int compare(tb_pclass *const &, tb_pclass *const &);

#endif
