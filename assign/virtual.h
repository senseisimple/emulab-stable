#ifndef __VIRTUAL_H
#define __VIRTUAL_H

class tb_vnode {
public:
  tb_vnode() {;}
  friend ostream &operator<<(ostream &o, const tb_vnode& node)
    {
      o << "TBVNode: " << node.name << endl;
      return o;
    }
  
  friend istream &operator>>(istream &i, const tb_vnode& node)
    {
      return i;
    }

  sortseq<string,double> desires; // contains weight of each desire
  int posistion;		// index into pnode array
  int no_connections;		// how many unfulfilled connections from this node
  string type;
  string name;			// string name of the node
  bool fixed;			// is this node fixed
};


class tb_plink;

class tb_vlink {
public:
  tb_vlink() {;}
  friend ostream &operator<<(ostream &o, const tb_vlink& edge)
    {
      o << "unimplemeted ostream << for tb_vlink " << endl;
      return o;
    }
  friend istream & operator>>(istream &i, const tb_vlink& edge)
    {
      return i;
    } 
  typedef enum {LINK_UNKNOWN, LINK_DIRECT,
	LINK_INTRASWITCH, LINK_INTERSWITCH} linkType;
  
  int bandwidth;		// how much bandwidth this uses
  linkType type;		// link type
  edge plink;			// plink this belongs to
  edge plink_two;		// second plink for INTRA links
  edge plink_local_one;		// only used in inter - link to local switch
  edge plink_local_two;		// only used in inter - link to local switch
  list<edge> path;		// path for interswitch links
  string name;			// name
};

typedef GRAPH<tb_vnode,tb_vlink> tb_vgraph;

#endif
