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
  
  dictionary<string,int> types;	// contains max nodes for each type
  sortseq<string,double> features; // contains cost of each feature
  string current_type;
  bool typed;			// has it been typed
  int max_load;			// maxmium load for current type
  int current_load;		// how many vnodes are assigned to this pnode
  string name;
  node the_switch;		// the switch the node is attached to
				// this is in the physical graph.
  int pnodes_used;		// for switch nodes
  node sgraph_switch;		// only for switches, the corresponding
				// sgraph switch.
};

class tb_switch {
public:
  tb_switch() {;}
   friend ostream &operator<<(ostream &o, const tb_switch& node)
    {
      o << "unimplemented osteam << for tb_switch" << endl;
      return o;
    }
  
  friend istream &operator>>(istream &i, const tb_switch& node)
    {
      return i;
    }

  node mate;			// match in PG
};

class tb_plink {
public:
  tb_plink() {;}
  friend ostream &operator<<(ostream &o, const tb_plink& edge)
    {
      o << "unimplemeted ostream << for tb_plink " << endl;
      return o;
    }
  friend istream & operator>>(istream &i, const tb_plink& edge)
    {
      return i;
    }
  int bandwidth;		// maximum bandwidth of this link
  int bw_used;			// how much is used
  int users;			// number of users in direct links
  string srcmac,dstmac;		// source and destination MAC addresses.
  string name;			// The name
};

class tb_slink {
public:
  tb_slink() {;}
  friend ostream &operator<<(ostream &o, const tb_slink& edge)
    {
      o << "unimplemeted ostream << for tb_psink " << endl;
      return o;
    }
  friend istream & operator>>(istream &i, const tb_slink& edge)
    {
      return i;
    }
  edge mate;			// match in PG
};

typedef GRAPH<tb_pnode,tb_plink> tb_pgraph;
typedef UGRAPH<tb_switch,tb_slink> tb_sgraph;

#endif
