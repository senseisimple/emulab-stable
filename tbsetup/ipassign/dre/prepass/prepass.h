// prepass.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef PREPASS_H_IPASSIGN_3
#define PREPASS_H_IPASSIGN_3

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <cstdlib>
#include <memory>

#include <signal.h>
#include <sys/wait.h>

struct Lan
{
    Lan() : number(0),
            partition(-1),
            assignment(0)/*,
            isNamingPoint(false),
            isTreeRoot(false),
            parentIndex(0),
            time (0)*/ {}
    // This is the index of the LAN in the great LAN array.
    int number;
    // The partition index number of the LAN.
    int partition;
    // A list of host keys used by the LAN.
    std::list<std::string> hosts;
    // The address assigned to this LAN. Used as part of the IP address.
    int assignment;
/*
    // Whether this LAN names a partition or not.
    bool isNamingPoint;
    // Whether this LAN is part of a tree or a cycle.
    bool isTreeRoot;
    // The index of the parent of this LAN in the DFS tree.
    int parentIndex;
    // The time that this LAN was visited in the DFS tree.
    int time;
    // This is the hostname that is the edge currently being explored
    // in the DFS tree. We must keep track of this to prevent the same
    // edge being used more than once in a cycle.
    //
    // After finishing the sub-tree rooted at a host, we can ignore
    // this because no other host can touch it to check the
    // forwardEdge. Likewise, we need only concern ourselves with the
    // current host we are exploring. Previously explored hosts will
    // never be touched again.
    std::string forwardEdge;
*/
};

struct Pred
{
    enum Visited
    {
        // This is the initial value. Not touched at all.
        INITIAL,
        // This interface has been visited, but is not a source.
        VISIT,
        // This interface has been the source of a traversal.
        SOURCE
    };

    Pred() : value(INITIAL) {}
    Visited value;
};

struct Host
{
    Host() : time(0), forwardEdge(0) {}
//    bool touched;
    std::string name;
    // The int is the index of a Lan.
    std::list<int> lans;
    int time;
    int forwardEdge;
    // A mapping from lan number to label number. Each label
    // represents an equivalence class of edges.
    std::map<int, int> edgeLabels;
    typedef std::map<int, int>::iterator EdgeIterator;
    typedef std::map<int,
        std::list<EdgeIterator> >::iterator PartitionIterator;
    // A mapping from equivalence class labels to lists of pointers
    // back to edgeLabels. If a is the size of the first set, and b is
    // the size of the second set, and n is the number of total edges
    // incident upon this host, then this data structure makes merging
    // sets O(min(a,b)*log(n)). Actually, that is only true if STL is
    // smart about the list splice operation and size. If it isn't
    // smart, then we can only achieve O((a + b)*log(n)).
    std::map<int, std::list< EdgeIterator > > partitionLists;
//    std::map<int, Pred> sources;
};

class StringException : public std::exception
{
public:
    explicit StringException(std::string const & error)
        : message(error)
    {
    }
    virtual char const * what() const throw()
    {
        return message.c_str();
    }
    virtual void addToMessage(char const * addend)
    {
        message += addend;
    }
    virtual void addToMessage(string const & addend)
    {
        addToMessage(addend.c_str());
    }
private:
    std::string message;
};

struct ComponentVertex
{
    ComponentVertex() : number(0) {}
    int number;
    std::set<int> neighbors;
};

class Partition;

namespace g
{
    extern std::map<std::string, Host> allHosts;
    extern std::vector<Lan> allLans;
    extern std::map<int, Partition> allPartitions;
//    extern std::map<int, ComponentVertex> partitionGraph;

    // pair<source, dest>, edgeNumber
    extern std::map< pair<int, int>, int > partitionEdges;
    extern std::vector<char*> treeCommand;
    extern std::vector<char*> generalCommand;

    extern bool useNeato;
    extern bool printSize;
}

void init(void);
void parseArgs(int argc, char * argv[]);
void parseSingleArg(char * arg);
void parseInput(void);
void addLan(int bits, int weights, vector<string> const & nodes);

void breakGraph(void);
void makePartitionGraph(void);
void insertPartitionEdge(int source, int dest, int edgeNumber);

void findNumbers(void);
void numberPartitions(void);
void printError(std::istream & error);
void printPartitionGraph(std::ostream & output);
void getPartitionAddresses(std::istream & input);

void ouputAddresses(void);
void normalOutput(void);
void neatoOutput(void);
void sizeOutput(void);

class FileT;

void auto_assign(std::auto_ptr<FileT> & left, std::auto_ptr<FileT> right);

#endif
