/* All of the data we want associated with nodes and edges in the testbed
 */

#include <iostream.h>
#include <LEDA/graph_alg.h>


class testnode {
public:

	/* Node types */
	static const int TYPE_UNSPECIFIED = 0;
	static const int TYPE_PC = 1;
	static const int TYPE_SWITCH = 2;
	static const int TYPE_DNARD = 3;
	
	
	testnode();
	virtual ~testnode();

	inline char *name() { return name_; }
	void name(char *newname);
	inline int interfaces() { return interfaces_; }
	inline void interfaces(int newint) { interfaces_ = newint; }
	inline int type() { return type_; }
	inline void type(int newtype) { type_ = newtype; }
	inline int partition() { return partition_; }
	inline void partition(int newpart) { partition_ = newpart; }
	

	friend ostream &operator<<(ostream &o, const testnode& node)
		{
			o << node.name_ << endl;
			return o;
		}

	friend istream &operator>>(istream &i, const testnode& node)
		{
			return i;
		}

private:
	
	char *name_;
	int interfaces_;
	int type_;
	int partition_;     /* To which partition does this node belong? */
};

class testedge {
public:	
	testedge();
	virtual ~testedge();

	inline int capacity() { return capacity_; }
	inline void capacity(int newcap) { capacity_ = newcap; }

	/* Stupid stuff LEDA requires... */
	
	friend ostream &operator<<(ostream &o, const testedge& edge)
		{
			o << "unimplemeted ostream << for edge " << endl;
			return o;
		}
	friend istream & operator>>(istream &i, const testedge& edge)
		{
			return i;
		}

private:
	int capacity_;

};

/*
 * The graphtype we use
 */

typedef GRAPH<testnode, testedge> tbgraph;

/*
 * Input format parsing functions
 */

void parse_top(tbgraph &G, istream& i);
void parse_ir(tbgraph &G, istream& i);


