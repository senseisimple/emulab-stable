class tb_type {
public:
  char *name;
  int max;
};
const int MAX_PNODES = 1024;	// maximum # of physical nodes
typedef enum {TYPE_UNKNOWN, TYPE_PC, TYPE_DNARD, TYPE_DELAY, TYPE_DELAY_PC,
	      TYPE_SWITCH} nodeType;
enum {MAX_TYPES = 5};
  



