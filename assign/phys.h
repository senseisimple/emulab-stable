class toponode {
public:
	toponode();
	int ints;
	int used;
};

class tbswitch {
public:
	tbswitch();
	virtual ~tbswitch();
	tbswitch(int ncount, char *newname);	// Specify number of toponodes
	void setsize(int ncount);

	inline int numnodes();
	
	int nodecount;
	toponode *nodes;
	char *name;
};

class topology {
public:
	topology(int nswitches);
	void print_topo();
	virtual ~topology();
	int switchcount;
	tbswitch **switches;
};

topology *parse_phys(char *filename);

