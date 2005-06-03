// prepass.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <map>

#include <cstdlib>

#include "coprocess.h"

using namespace std;

struct Lan
{
    Lan() : number(0),
            partition(-1),
            isNamingPoint(false),
            isTreeRoot(false),
            parentIndex(0),
            assignment(0),
            time (0) {}
    // This is the index of the LAN in the great LAN array.
    int number;
    // The partition index number of the LAN.
    int partition;
    // A list of host keys used by the LAN.
    list<string> hosts;
    // Whether this LAN names a partition or not.
    bool isNamingPoint;
    // Whether this LAN is part of a tree or a cycle.
    bool isTreeRoot;
    // The index of the parent of this LAN in the DFS tree.
    int parentIndex;
    // The address assigned to this LAN. Used as part of the IP address.
    int assignment;
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
    string forwardEdge;
};

struct Host
{
    Host() {}
    string name;
    list<int> lans;
};

// The partition is a collection of all the information needed to
// output a sub-graph to a pipe and interpret the results.
class Partition
{
public:
    Partition(int newNumber = 0);
    void setNumber(int newNumber);
    int getNumber(void) const;
    void setAddress(int newAddress);
    int getAddress(void) const;
    // Add a lan to the information. This increases the count and adds
    // the lan number to the mappings.
    void addLan(int lanNumber);
    void dispatch(void);
private:
    struct OrderCount
    {
        OrderCount() : order(0), count(0) {}
        int order;
        int count;
    };
private:
    void mapHosts(void);
    void printGraph(ostream & output);
    void getNumbering(istream & input);
private:
    int number;
    int address;
    bool isTree;
    int lanCount;
    int hostCount;
    // This provides a mappings back and forth from sub-program
    // concepts to user concepts. We process our pipe output and then
    // input based on these mappings.
    map<int, int> lanToOrder;
    map<int, int> orderToLan;
    // The hostToOrder thing is a little different from the other
    // mappings. In order to determine whether we want to count a host
    // at all, we have to check to make sure that it touches two or
    // more hosts. That is what the count attached to this is.
    map<string, OrderCount> hostToOrder;
    map<int, string> orderToHost;
};

//-----------------------------------------------------------------------------

namespace g
{
    map<string, Host> allHosts;
    vector<Lan> allLans;
    vector<Partition> allPartitions;
    vector<char*> treeCommand;
    vector<char*> generalCommand;
}

//-----------------------------------------------------------------------------

Partition::Partition(int newNumber)
    : address(0)
    , isTree(true)
    , lanCount(0)
    , hostCount(0)
{
    setNumber(newNumber);
}

void Partition::setNumber(int newNumber)
{
    // TODO: Assign a real address. Not just the LAN number.
    address = newNumber;
    number = newNumber;
}

int Partition::getNumber(void) const
{
    return number;
}

void Partition::setAddress(int newAddress)
{
    address = newAddress;
}

int Partition::getAddress(void) const
{
    return address;
}

void Partition::addLan(int lanNumber)
{
    // Have we already added this LAN in the past?
    map<int, int>::iterator lanSearch = lanToOrder.find(lanNumber);
    if (lanSearch == lanToOrder.end())
    {
        // If not, add a mapping for the LAN.
        lanToOrder[lanNumber] = lanCount;
        orderToLan[lanCount] = lanNumber;
        ++lanCount;
        isTree = isTree && g::allLans[lanNumber].isTreeRoot;
        g::allLans[lanNumber].partition = number;
        // Add one to the count of every host we touch. If two or more
        // LANs touch a host, then that host is sent to the children
        // as a hyper-edge. We just keep track of the count for
        // now. Later we'll worry about filtering them.
        list<string>::iterator pos = g::allLans[lanNumber].hosts.begin();
        list<string>::iterator limit = g::allLans[lanNumber].hosts.end();
        for (; pos != limit; ++pos)
        {
            ++(hostToOrder[*pos].count);
        }
    }
}

template <class T>
void auto_assign(auto_ptr<T> & left, auto_ptr<T> right)
{
    left = right;
}

void Partition::dispatch(void)
{
    mapHosts();
    auto_ptr<FileT> pipe;
    if (isTree)
    {
        auto_assign(pipe, coprocess(g::treeCommand[0], &(g::treeCommand[0]),
                                    NULL));
    }
    else
    {
        auto_assign(pipe, coprocess(g::generalCommand[0],
                                    &(g::generalCommand[0]), NULL));
    }
    printGraph(pipe->input());
    pipe->closeIn();
    getNumbering(pipe->output());
}

void Partition::printGraph(ostream & output)
{
    output << "vertex-count " << lanCount << endl;
    output << "edge-count " << hostCount << endl;
    output << "total-bits " << 32 << endl;
    output << "%%" << endl;
    map<int, int>::iterator lanPos = orderToLan.begin();
    map<int, int>::iterator lanLimit = orderToLan.end();
    for (; lanPos != lanLimit; ++lanPos)
    {
        output << "8 1 ";
        list<string>::iterator hostPos =
            g::allLans[lanPos->second].hosts.begin();
        list<string>::iterator hostLimit =
            g::allLans[lanPos->second].hosts.end();
        for (; hostPos != hostLimit; ++hostPos)
        {
            OrderCount oc = hostToOrder[*hostPos];
            if (oc.count > 1)
            {
                output << oc.order << " ";
            }
        }
        output << endl;
    }
}

void Partition::getNumbering(istream & input)
{
    int assignedNumber = 0;
    int order = 0;
    input >> assignedNumber;
    while (input)
    {
        g::allLans[orderToLan[order]].assignment = assignedNumber;
        input >> assignedNumber;
        ++order;
    }
}

void Partition::mapHosts(void)
{
    // For every host we've touched. If it has been touched by more
    // than two LANs, then set up mapping information.
    map<string, OrderCount>::iterator pos = hostToOrder.begin();
    map<string, OrderCount>::iterator limit = hostToOrder.end();
    for ( ; pos != limit; ++pos)
    {
        if (pos->second.count > 1)
        {
            pos->second.order = hostCount;
            orderToHost[hostCount] = pos->first;
            ++hostCount;
        }
    }
}

//-----------------------------------------------------------------------------

void parseArgs(int argc, char * argv[]);
void parseInput(void);
void addLan(int bits, int weights, vector<string> const & nodes);

void breakGraph(void);

void findNumbers(void);
void ouputAddresses(void);

int main(int argc, char * argv[])
{
    parseArgs(argc, argv);
    parseInput();
    breakGraph();
    findNumbers();
    ouputAddresses();
    return 0;
}

// If there are no args, use trivial-assign as the assigner.
// Otherwise, take all the arguments up to the first dash as a command
// line for general graphs.  Take everything after the dash as a
// command line for trees. If there is no dash, then use the same
// command line for both trees and generally.
void parseArgs(int argc, char * argv[])
{
    static char * defaultCommand = "./trivial-ipassign";
    if (argc <= 1)
    {
        g::generalCommand.push_back(defaultCommand);
        g::generalCommand.push_back(NULL);
        g::treeCommand = g::generalCommand;
    }
    else
    {
        bool foundDash;
        int index = 1;
        while (argv[index] != NULL)
        {
            if (!foundDash && argv[index] == string("-"))
            {
                foundDash = true;
            }
            else if (foundDash)
            {
                g::treeCommand.push_back(argv[index]);
            }
            else
            {
                g::generalCommand.push_back(argv[index]);
            }
        }
        g::generalCommand.push_back(NULL);
        if (foundDash)
        {
            g::treeCommand.push_back(NULL);
        }
        else
        {
            g::treeCommand = g::generalCommand;
        }
    }
}

void parseInput(void)
{
    string bufferString;
    getline(cin, bufferString);
    while (cin)
    {
        istringstream buffer(bufferString);
        int bits = 0;
        int weight = 0;
        string temp;
        vector<string> nodes;
        buffer >> bits >> weight;
        buffer >> temp;
        while (buffer)
        {
            nodes.push_back(temp);
            buffer >> temp;
        }
        addLan(bits, weight, nodes);
        getline(cin, bufferString);
    }
}

void addLan(int bits, int weights, vector<string> const & nodes)
{
    int lanNumber = static_cast<int>(g::allLans.size());
    g::allLans.push_back(Lan());
    g::allLans.back().number = lanNumber;
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        g::allLans.back().hosts.push_back(nodes[i]);
        g::allHosts[nodes[i]].name = nodes[i];
        g::allHosts[nodes[i]].lans.push_back(lanNumber);
    }
}

class BiconnectAlgorithm
{
    static const int NO_INDEX = -1;

    struct VisitResult
    {
        VisitResult(int newCount = 0, int newTime = 0)
            : partitionCount(newCount)
            , minTime(newTime) {}
        int partitionCount;
        int minTime;
    };
public:
    void calculate(void)
    {
        now = 0;
        if (g::allLans.size() > 0)
        {
            VisitResult data = visit(0, NO_INDEX);
            int partitionCount = data.partitionCount;
            g::allPartitions.resize(partitionCount);
            for (int i = 0; i < partitionCount; ++i)
            {
                g::allPartitions[i].setNumber(i);
            }
            assignPartitions();
        }
    }

    // See IpAssignHypergraphBiconnectivity for a definition of
    // biconnectivity in the presence of hypergraphs.
    //
    // This algorithm has to do three things at once.  First, it must
    // keep track of the time when each vertex is visited for the
    // first time. This is the core of basic biconnectivity
    // detection. But merely doing this results only in a tagging of
    // articulation points which is insufficient for a
    // hypergraph. Therefore, second, at the same time, we must create
    // a labelling predecessor graph. Each vertex is either the top of
    // a biconnected component and must be responsible for giving that
    // component a name, or it is in the middle of some component and
    // depneds on its parent for a name. The idea of consolidating
    // trees adds a third player to our little dance. A vertex is the
    // potential root of a tree when a) it has no immediate back edges
    // and b) it is not part of any cycle of its parents. Note that
    // this makes constructions like:
    //
    //   V
    //   |
    //   h
    //  / \
    // V   V
    //
    // trees. Likewise instances where parents and children are
    // connected by a LAN rather than individual links are considered
    // trees. For instance:
    //
    //       V
    //       |
    //       h
    //      / \
    //     /   \
    //    V     V
    //    |     |
    //    h     h
    //   / \   / \
    //  V   V V   V
    //
    // is a tree.
    //
    // The last, and relatively minor task, is to determine as we go
    // along how many partitions we will actually have.
    VisitResult visit(int lan, int parent)
    {
        // The result returns two things: The minimum time encountered
        // by this recursion, and the number of partitions which will
        // result.
        VisitResult result;

        // This is used to help determine the number of partitions. If
        // we are not a tree, then each of these children is a
        // partition. If we are a tree, then we can absorb all of
        // these children into us.
        int treeChildrenCount = 0;

        // now is used to keep track of when vertices are first
        // visited. Those visited later have greater timestamps. These
        // timestamps are used to detect cycles. A cycle happens when
        // a vertex is visited that already has a time stamp less than
        // now.
        ++now;
        Lan & currentLan = g::allLans[lan];

        // result.minTime is used to keep track of how high up the DFS
        // tree a cycle is detected. It is always set to the lowest
        // number encountered.
        result.minTime = now;
        currentLan.time = now;

        // A namingPoint is a place where a new name is generated
        // rather than sought from the parent. A naming point should
        // be part of no cycles with any parent. We assume this is
        // true and set it to be false if we find it to be not so.
        currentLan.isNamingPoint = true;

        // A treeRoot is a vertex that is part of a tree. A tree is
        // part of no cycle. If isNamingPoint is false, then
        // isTreeRoot is also false because that means that the vertex
        // is part of a loop with a parent. If this vertex has any
        // back edges, then it is part of some loop and is not part of
        // a tree. We set this to tree and falsify as necessary.
        currentLan.isTreeRoot = true;

        // Every vertex keeps track of its parent. If it is neither a
        // naming point nor a tree root, then the name of its
        // partition is based upon the name of its parent's
        // partition. If it is a naming point but not a tree root,
        // then the parent is ignored and a new partition name is
        // generated. If it is a treeRoot, then if the parent is also
        // a treeRoot, then it uses the parents name. Otherwise it
        // generates a new one.

        // The whole point of this is that namingPoints are cuts in
        // the tree that define biconnected components. Therefore they
        // determine what names to use. Likewise, treeRoots are all
        // individually biconnected components. But we want to group
        // them together if they are adjascent. This is done by
        // delegating upwards until a non-treeRoot is found.
        currentLan.parentIndex = parent;
        list<string>::iterator linkPos = currentLan.hosts.begin();
        list<string>::iterator linkLimit = currentLan.hosts.end();
        for (; linkPos != linkLimit; ++linkPos)
        {
            // The currentHost is our hyperedge. If it is currently
            // used, then we don't want to traverse it because this
            // would violate our edge-unique definition of
            // biconnectivity. Therefore, we ignore it if used, and
            // bracket the scope of the traversal with used and unused
            // notifications.
            Host & currentHost = g::allHosts[*linkPos];
            currentLan.forwardEdge = currentHost.name;
            list<int>::iterator pos = currentHost.lans.begin();
            list<int>::iterator limit = currentHost.lans.end();
            for (; pos != limit; ++pos)
            {
                Lan & child = g::allLans[*pos];
                // Make sure we're not doubling back on ourselves
                // immediately.
                if (child.number != lan)
                {
                    // child.time is zero if we've not visited that
                    // vertex before.
                    if (child.time == 0)
                    {
                        VisitResult childResult = visit(child.number, lan);
                        // Update to reflect possible loops.
                        result.minTime = min(result.minTime,
                                             childResult.minTime);
                        result.partitionCount +=
                            childResult.partitionCount;

                        // A separate count is kept of tree children. We
                        // don't know yet whether we will absorb them into
                        // our partition.
                        if (child.isTreeRoot)
                        {
                            ++treeChildrenCount;
                        }
                        else if (child.isNamingPoint)
                        {
                            ++(result.partitionCount);
                        }

                        // If the minTime of the child is less than
                        // the time we were created, that means that
                        // the child has encountered some vertex out
                        // of our sub-tree which means that this
                        // vertex is part of a loop and therefore not
                        // a naming point.
                        if (childResult.minTime < currentLan.time)
                        {
                            currentLan.isNamingPoint = false;
                            currentLan.isTreeRoot = false;
                        }
                    }
                    else
                    {
                        // We have encountered a back edge.

                        // We need to make sure we are not doubling
                        // back and double-dipping from the same LAN.
                        if (currentHost.name != child.forwardEdge)
                        {
                            // We still update our min time to reflect the
                            // childs knowledge.
                            result.minTime = min(result.minTime,
                                                 child.time);
                            // A back edge means that we are definitely
                            // not part of a tree.
                            currentLan.isTreeRoot = false;
                            // If the time of the child is less than our
                            // time, this means that we've looped back on
                            // some parent. We are part of that parents
                            // cycle and therefore are not a naming point.
                            if (child.time < currentLan.time)
                            {
                                currentLan.isNamingPoint = false;
                            }
                        }
                    }
                }
            }
        }
        // If we are a not a tree, then they are all partitions
        // themselves.
        if (!(currentLan.isTreeRoot))
        {
            result.partitionCount += treeChildrenCount;
        }
        // If we are truly the root, then we are a naming point too.
        if (parent == NO_INDEX)
        {
            ++(result.partitionCount);
            currentLan.isNamingPoint = true;
        }
        cerr << lan << " partitionCount: " << result.partitionCount
             << " treeChildren: " << treeChildrenCount << endl;
        return result;
    }

    void assignPartitions(void)
    {
        usedPartitionCount = 0;
        for (size_t i = 0; i < g::allLans.size(); ++i)
        {
            if (g::allLans[i].isTreeRoot)
            {
                cerr << i << " = Tree" << endl;
            }
            else if (g::allLans[i].isNamingPoint)
            {
                cerr << i << " = Naming" << endl;
            }
            else
            {
                cerr << i << " = Zip" << endl;
            }
            findRoot(static_cast<int>(i));
        }
        int maxPartitionCount = static_cast<int>(g::allPartitions.size());
        if (usedPartitionCount != maxPartitionCount)
        {
            cerr << "Error: partition count error: used="
                 << usedPartitionCount << " max=" << maxPartitionCount
                 << endl;
            throw;
        }
    }

    int findRoot(int lan)
    {
        Lan & currentLan = g::allLans[lan];
        int result = 0;
        if (currentLan.partition != -1)
        {
            result = currentLan.partition;
        }
        else
        {
            // if we don't have a parent and are therefore the root
            // or we are a tree root and our parent is not a tree root
            // or we are a naming point and not a tree root.
            if (currentLan.parentIndex == NO_INDEX
                || (currentLan.isTreeRoot
                    && !(g::allLans[currentLan.parentIndex].isTreeRoot))
                || (!(currentLan.isTreeRoot) && currentLan.isNamingPoint))
            {
                result = newPartition();
            }
            else
            {
                // Either we are a tree root without a parent tree
                // root, or we are not a naming point. We therefore
                // delegate our partition decision to our parent.
                result = findRoot(currentLan.parentIndex);
            }
            g::allPartitions[result].addLan(lan);
        }
        return result;
    }

    int newPartition(void)
    {
        int maxPartitionCount = static_cast<int>(g::allPartitions.size());
        if (usedPartitionCount >= maxPartitionCount)
        {
            cerr << "Error: More partitions than we calculated" << endl;
            throw;
        }
        int result = usedPartitionCount;
        ++usedPartitionCount;
        return result;
    }

/*
    void calculate(void)
    {
        now = 0;
        val.resize(hostCount);
        fill(val.begin(), val.end(), 0);
        rootChildren = 0;
        visit(0);
        if (rootChildren > 1)
        {
            g::allHosts[0].isBorder = true;
        }
    }

    int visit (int k)
    {
        list<int>::iterator linkPos;
        list<int>::iterator linkLimit;
        int m;
        int min;
        now = now + 1;
        val[k] = now;
        min = now;
        linkPos = g::allHosts[k].lans.begin();
        linkLimit = g::allHosts[k].lans.end();
        for ( ; linkPos != linkLimit; ++linkPos)
        {
            list<int>::iterator t = g::allLans[*linkPos].hosts.begin();
            list<int>::iterator z = g::allLans[*linkPos].hosts.end();
            for ( ; t != z; ++t)
            {
                if (val[*t] == 0)
                {
                    if (k == 0) // root
                    {
                        ++rootChildren;
                    }
                    m = visit(*t);
                    if (m < min)
                    {
                        min = m;
                    }
                    if (m >= val[k])
                    {
                        g::allHosts[k].isBorder = true;
                    }
                }
                else if (val[*t] < min)
                {
                    min = val[*t];
                }
            }
        }
        return min;
    }
*/
private:
//    vector<int> val;
    int now;
    int usedPartitionCount;
//    int rootChildren;
};

void breakGraph(void)
{
    BiconnectAlgorithm bi;
    bi.calculate();
}

void findNumbers(void)
{
    for (size_t i = 0; i < g::allPartitions.size(); ++i)
    {
        g::allPartitions[i].dispatch();
    }
}

void ouputAddresses(void)
{
    for (size_t i = 0; i < g::allLans.size(); ++i)
    {
        list<string>::iterator pos = g::allLans[i].hosts.begin();
        list<string>::iterator limit = g::allLans[i].hosts.end();
        int hostNumber = 1;
        for (; pos != limit; ++pos)
        {
            cout << i << " " << *pos << " " << "10."
                 << g::allPartitions[g::allLans[i].partition].getAddress()
                 << "."
                 << g::allLans[i].assignment << "." << hostNumber << endl;
            ++hostNumber;
        }
    }
}
