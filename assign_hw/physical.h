#ifndef __PHYSICAL_H
#define __PHYSICAL_H

class tb_pnode {
public:
  tb_pnode() {;}
  friend ostream &operator<<(ostream &o, const tb_pnode& node)
    {
      o << "TBNode: " << node.name << endl;
      return o;
    }
  
  friend istream &operator>>(istream &i, const tb_pnode& node)
    {
      return i;
    }
  
  tb_type types[MAX_TYPES];	// array of types, with counts of max
  nodeType current_type;	// the current type of the node
  int max_load;			// maxmium load for current type
  int current_load;		// how many vnodes are assigned to this pnode
  char *name;
  node the_switch;		// the switch the node is attached to
  int pnodes_used;		// for switch nodes
};

class tb_plink {
public:
  tb_plink() {;}
  friend ostream &operator<<(ostream &o, const tb_plink& edge)
    {
      o << "unimplemeted ostream << for tb_plinke " << endl;
      return o;
    }
  friend istream & operator>>(istream &i, const tb_plink& edge)
    {
      return i;
    }  
  int bandwidth;		// maximum bandwidth of this link
  int bw_used;			// how much is used
  int users;			// number of users in direct links
  char *srcmac,*dstmac;		// source and destination MAC addresses.
  char *name;			// The name
};

class tb_route {
public:
private:
  int length;
  list<tb_plink> links;
};

typedef GRAPH<tb_pnode,tb_plink> tb_pgraph;

#endif
