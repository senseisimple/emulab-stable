
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
#include "physical.h"
#include "virtual.h"
#include "pclass.h"

extern node pnodes[MAX_PNODES];
extern dictionary<tb_pnode*,node> pnode2node;
extern pclass_list pclasses;
extern dictionary<string,pclass_list*> type_table;

typedef two_tuple<node,int> link_info; // dst, bw

// returns 1 if a and b are equivalent.  They are equivalent if the
// type and features information match and if there is a one-to-one
// mapping between links that preserves bw, and destination.
int pclass_equiv(tb_pgraph &PG, tb_pnode *a,tb_pnode *b)
{
  // check type information
  dic_item it;
  forall_items(it,a->types) {
    dic_item bit;
    bit = b->types.lookup(a->types.key(it));
    if (bit == nil)
      return 0;
    if (b->types[bit] != a->types.inf(it))
      return 0;
  }

  // check features
  seq_item sit;
  forall_items(sit,a->features) {
    seq_item bit;
    bit = b->features.lookup(a->features.key(sit));
    if (bit == nil)
      return 0;
    if (b->features[bit] != a->features.inf(sit))
      return 0;
  }

  // check links - to do this we first create sets of every link in b.
  // we then loop through every link in a, find a match in the set, and
  // remove it from the set.
  node an = pnode2node.access(a);
  node bn = pnode2node.access(b);

  // yet another place where we'd like to use sets <sigh>
  list<link_info> b_links;
  
  edge e;
  forall_inout_edges(e,bn) {
    node dst=PG.target(e);
    if (dst == bn)
      dst = PG.source(e);
    b_links.push_front(link_info(dst,PG[e].bandwidth));
  }
  forall_inout_edges(e,an) {
    link_info link;
    node dst=PG.target(e);
    if (dst == an)
      dst = PG.source(e);
    int bw = PG[e].bandwidth;
    bool found = 0;
    list_item lit;
    forall_items(lit,b_links) {
      link=b_links.inf(lit);
      if ((dst == link.first()) && (bw == link.second())) {
	found = 1;
	b_links.del(lit);
	break;
      }
    }
    if (found == 0) return 0;
  }
  return 1;
}

/* This function takes a physical graph and generates an set of
   equivalence classes of physical nodes.  The globals pclasses (a
   list of all equivalence classes) and type_table (and table of
   physical type to list of classes that can satisfy that type) are
   set by this routine. */
int generate_pclasses(tb_pgraph &PG) {
  node cur;
  dictionary<tb_pclass*,tb_pnode*> canonical_members;
  
  forall_nodes(cur, PG) {
    tb_pclass *curclass;
    bool found_class = 0;
    tb_pnode *curP = &PG[cur];
    dic_item dit;
    forall_items(dit,canonical_members) {
      curclass=canonical_members.key(dit);
      if (pclass_equiv(PG,curP,canonical_members[dit])) {
	// found the right class
	found_class=1;
	curclass->add_member(curP);
	break;
      }
    }
    if (found_class == 0) {
      // new class
      tb_pclass *n = new tb_pclass;
      pclasses.push_back(n);
      canonical_members.insert(n,curP);
      n->name = curP->name;
      n->add_member(curP);
    }
  }

  list_item it;
  forall_items(it,pclasses) {
    tb_pclass *cur = pclasses[it];
    dic_item dit;
    forall_items(dit,cur->members) {
      if (type_table.lookup(cur->members.key(dit)) == nil) {
	type_table.insert(cur->members.key(dit),new pclass_list);
      }
      type_table.access(cur->members.key(dit))->push_back(cur);
    }
  }
  return 0;
}

int tb_pclass::add_member(tb_pnode *p)
{
  dic_item it;
  forall_items(it,p->types) {
    string type = p->types.key(it);
    if (members.lookup(type) == nil) {
      members.insert(type,new tb_pnodelist);
    }
    members.access(type)->push_back(p);
  }
  size++;
  p->my_class=this;
  return 0;
}

// should be called after add_node
int pclass_set(tb_vnode &v,tb_pnode &p)
{
  tb_pclass *c = p.my_class;
  
  // remove p node from correct lists in equivalence class.
  dic_item dit;
  forall_items(dit,c->members) {
    if (c->members.key(dit) == p.current_type) {
      // same class - only remove if node is full
      if (p.current_load == p.max_load) {
	(c->members.inf(dit))->remove(&p);
      }
    } else {
      // If it's not in the list then this fails quietly.
      (c->members.inf(dit))->remove(&p);
    }
  }
  
  c->used += 1.0/(p.max_load);
  
  return 0;
}

int pclass_unset(tb_pnode &p)
{
  // add pnode to all lists in equivalence class.
  tb_pclass *c = p.my_class;
  dic_item dit;

  forall_items(dit,c->members) {
    if (c->members.key(dit) == p.current_type) {
      // If it's not in the list then we need to add it to the back if it's
      // empty and the front if it's not.  Since unset is called before
      // remove_node empty means only one user.
      if (! (c->members.inf(dit))->exists(&p)) {
	assert(p.current_load > 0);
	if (p.current_load == 1) {
	  (c->members.inf(dit))->push_back(&p);
	} else {
	  (c->members.inf(dit))->push_front(&p);
	}
      }
    } else {
      (c->members.inf(dit))->push_back(&p);
    }
  }

  c->used -= 1.0/(p.max_load);
  
  return 0;
}

void pclass_debug()
{
  cout << "PClasses:\n";
  list_item it;
  forall_items(it,pclasses) {
    tb_pclass *p = pclasses[it];
    cout << p->name << ": size = " << p->size << "\n";
    dic_item dit;
    forall_items(dit,p->members) {
      cout << "  " << p->members.key(dit) << ":\n";
      list_item lit;
      const list<tb_pnode*> L = p->members.inf(dit)->L;
      forall_items(lit,L) {
	cout << "    " << L.inf(lit)->name << "\n";
      }
    }
    cout << "\n";
  }

  cout << "\n";
  cout << "Type Table:\n";
  dic_item dit;
  forall_items(dit,type_table) {
    cout << type_table.key(dit) << ":";
    list_item lit;
    pclass_list *L = type_table.inf(dit);
    forall_items(lit,*L) {
      cout << " " << (L->inf(lit))->name;
    }
    cout << "\n";
  }
}

int compare(tb_pclass *const &a, tb_pclass *const &b)
{
  if (a==b) return 0;
  if (a < b) return -1;
  return 1;
}
