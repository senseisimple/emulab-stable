// Graph.h

#ifndef GRAPH_H_IP_ASSIGN_1
#define GRAPH_H_IP_ASSIGN_1

enum { totalBits = 32 };

extern const int prefixBits;
extern const int postfixBits;
extern const std::bitset<totalBits> prefix;
extern const std::bitset<totalBits> prefixMask;
extern const std::bitset<totalBits> postfixMask;

class Graph
{
private:
    struct Lan
    {
        Lan()
            : weight(0), partition(0), number(0)
        {
        }
        int number;
        int weight;
        int partition;
        std::list<int> nodes;
    };
public:
    //////////////////////////////////////////////////////
    // construction
    //////////////////////////////////////////////////////

    // populate graph from stream. See reset()
    Graph(std::istream & input = cin);
    ~Graph();

    //////////////////////////////////////////////////////
    // copy
    //////////////////////////////////////////////////////

    // copy disabled for now. It looks like it will be quite complicated
    // and inefficient and we have no specific need for it.
private:
    Graph(Graph const & right) {}
    Graph & operator=(Graph const & right) { return *this; }
public:

    //////////////////////////////////////////////////////
    // statistics
    //////////////////////////////////////////////////////

    // return the size of the largest lan
    int getLargestLan(void) const;
    // return the number of lans
    int getLanCount(void) const;

    //////////////////////////////////////////////////////
    // information
    //////////////////////////////////////////////////////

    // populate arguments with the METIS graph format
    void convert(std::vector<int> & indexes, std::vector<int> & neighbors,
                 std::vector<int> & weights) const;

    //////////////////////////////////////////////////////
    // change
    //////////////////////////////////////////////////////

    // repopulate graph from input. Uses the input format specified
    // in ipassign.cc
    // This function may throw an InvalidCharacterException, a
    // NotEnoughNodesException, or a bad_alloc
    void reset(std::istream & input = cin);

    // set the partion value of every lan based on input array
    void partition(std::vector<int> & partitions);

    //////////////////////////////////////////////////////
    // output
    //////////////////////////////////////////////////////

    // print IP addresses to specified stream. Uses the output
    // format specified in ipassign.cc
    void assign(int networkSize, int lanSize, int hostSize,
                int numPartitions) const;
    void assign(std::ostream & output, int networkSize, int lanSize,
                int hostSize, int numPartitions) const;
private:
    //////////////////////////////////////////////////////
    // utilities
    //////////////////////////////////////////////////////

    // if a line has something other than [0-9], or has less than three
    // seperate numbers, it is invalid
    // This function may throw an InvalidCharacterException, an
    // EmptyLineException, a NotEnoughNodesException, or a bad_alloc
    int parseLine(std::string const & line, std::list<int> & nodes);

    // Given a lan, add the appropriate adjacency and weight information
    // to arrays.
    void convertAddLan(Lan const & lanToAdd, std::vector<int> & neighbors,
                       std::vector<int> & weights) const;

    // Given the number of a node, calculate the adjacencies and weights
    // and put them into arrays.
    void convertAddNode(Lan const & info, int currentNode,
                        std::vector<int> & neighbors,
                        std::vector<int> & weights) const;

    // if IP_ASSIGN_DEBUG is set, then this will run, outputting the
    // raw numbers for network, lan, host rather than an IP address.
    void debugAssign(std::ostream & output, int networkSize, int lanSize,
                     int hostSize, int numPartitions) const;

    // if IP_ASSIGN_DEBUG is not set, then an IP address is generated.
    void releaseAssign(std::ostream & output, int networkSize, int lanSize,
                       int hostSize, int numPartitions) const;

    // convert a portion of a bitset into a value. The first bit
    // to be used is the one at location start, and the last is the one
    // just before location finish.
    // if finish is out of range, it becomes totalBits.
    // if start is out of range or greater than finish, 0 is returned.
    int bits2int(bitset<totalBits> & bits, size_t start, size_t finish) const;
private:
    // the greatest number of nodes that we've found in a lan.
    int m_largestLan;

    // This holds all of the lans in the graph.
    std::list<Lan> m_lanList;

    // A map connecting each node (key) with its lans (value)
    std::multimap<int, std::list<Lan>::iterator> m_nodes;
};

#endif
