// prepass.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "prepass.h"
#include "Partition.h"

using namespace std;

//-----------------------------------------------------------------------------

namespace g
{
    map<string, Host> allHosts;
    vector<Lan> allLans;
    vector<Partition> allPartitions;
    vector<char*> treeCommand;
    vector<char*> generalCommand;

    bool useNeato = false;
}

//-----------------------------------------------------------------------------

int main(int argc, char * argv[])
{
    init();
    parseArgs(argc, argv);
    parseInput();
    breakGraph();
    findNumbers();
    ouputAddresses();
    return 0;
}

//-----------------------------------------------------------------------------

void init(void)
{
    // Disable SIGPIPE. We will write to our child process regardless
    // and recover later.
    signal(SIGPIPE, SIG_IGN);
}

//-----------------------------------------------------------------------------

// If there are no args, use trivial-assign as the assigner.
// Otherwise, take all the arguments up to the first dash as a command
// line for general graphs.  Take everything after the dash as a
// command line for trees. If there is no dash, then use the same
// command line for both trees and generally.
void parseArgs(int argc, char * argv[])
{
    static char * defaultCommand = "./trivial-ipassign";
    int dashCount = 0;
    int index = 1;
    while (argv[index] != NULL)
    {
        if (argv[index] == string("-"))
        {
            ++dashCount;
        }
        else if (dashCount == 0)
        {
            // Arguments meant for us
            parseSingleArg(argv[index]);
        }
        else if (dashCount == 1)
        {
            // Arguments meant for running the general command.
            g::generalCommand.push_back(argv[index]);
        }
        else
        {
            // Arguments meant for running the tree command.
            g::treeCommand.push_back(argv[index]);
        }
        ++index;
    }
    if (dashCount == 0)
    {
        g::generalCommand.push_back(defaultCommand);
        g::generalCommand.push_back(NULL);
        g::treeCommand = g::generalCommand;
    }
    else if (dashCount == 1)
    {
        g::generalCommand.push_back(NULL);
        g::treeCommand = g::generalCommand;
    }
    else
    {
        g::treeCommand.push_back(NULL);
    }
}

//-----------------------------------------------------------------------------

void parseSingleArg(char * arg)
{
    if (arg == string("--neato"))
    {
        g::useNeato = true;
    }
    else
    {
        cerr << "Invalid argument" << endl;
        throw;
    }
}

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

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
        return result;
    }

    void assignPartitions(void)
    {
        usedPartitionCount = 0;
        for (size_t i = 0; i < g::allLans.size(); ++i)
        {
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
private:
    int now;
    int usedPartitionCount;
};

//-----------------------------------------------------------------------------

void breakGraph(void)
{
    BiconnectAlgorithm bi;
    bi.calculate();
}

//-----------------------------------------------------------------------------

void findNumbers(void)
{
    for (size_t i = 0; i < g::allPartitions.size(); ++i)
    {
        g::allPartitions[i].dispatch();
    }
}

//-----------------------------------------------------------------------------

void ouputAddresses(void)
{
    if (!g::useNeato)
    {
        normalOutput();
    }
    else
    {
        neatoOutput();
    }
}

//-----------------------------------------------------------------------------

void normalOutput(void)
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

//-----------------------------------------------------------------------------

void neatoOutput(void)
{
    cout << "digraph foo {" << endl;
    cout << "\tnodesep=0.01" << endl;
    cout << "\tranksep=0.5" << endl;
    for (size_t i = 0; i < g::allLans.size(); ++i)
    {
        cout << "\tlan" << i << " [color=blue,label="
             << g::allPartitions[g::allLans[i].partition].getAddress()
             << ",style=filled,shape=box];" << endl;
    }

    map<string, Host>::iterator pos = g::allHosts.begin();
    map<string, Host>::iterator limit = g::allHosts.end();
    for (; pos != limit; ++pos)
    {
        cout << "\tnode" << pos->first << " [color=red,label="
             << pos->first << ",style=filled,shape=circle];" << endl;
    }

    pos = g::allHosts.begin();
    limit = g::allHosts.end();
    for (; pos != limit; ++pos)
    {
        Host & current = pos->second;
        list<int>::iterator lanPos = current.lans.begin();
        list<int>::iterator lanLimit = current.lans.end();
        for (; lanPos!= lanLimit; ++lanPos)
        {
            cout << "\tnode" << current.name << " -> "
                 << "lan" << *lanPos
                 << " [arrowhead=none,color=black,style=bold];"
                 << endl;
        }
    }
    cout << "}" << endl;
}

//-----------------------------------------------------------------------------
