class tb_vnode {
public:
  int posistion;		// index into pnode array
  int no_connections;		// how many unfulfilled connections from this node
  nodeType type;		// type of the node
  char *name;			// string name of the node
};

enum {LINK_DIRECT,LINK_INTRASWITCH,LINK_INTERSWITCH} tb_link_type;

class tb_plink;

class tb_vlink {
public:
  typedef enum {LINK_UNKNOWN, LINK_DIRECT,
	LINK_INTRASWITCH, LINK_INTERSWITCH} linkType;
  
  int bandwidth;		// how much bandwidth this uses
  linkType type;		// link type
  edge plink;			// plink this belongs to
  edge plink_two;		// second plink for INTRA and INTER links
};

typedef GRAPH<tb_vnode,tb_vlink> tb_vgraph;
