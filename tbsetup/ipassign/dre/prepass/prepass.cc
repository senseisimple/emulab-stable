// prepass.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "prepass.h"
#include "coprocess.h"
#include "Partition.h"

using namespace std;

//-----------------------------------------------------------------------------

namespace g
{
    map<string, Host> allHosts;
    vector<Lan> allLans;
    map<int, Partition> allPartitions;
//    map<int, ComponentVertex> partitionGraph;
    map< pair<int, int>, int > partitionEdges;
    vector<char*> treeCommand;
    vector<char*> generalCommand;

    bool useNeato = false;
    bool printSize = false;
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
    static char * defaultCommand = "./trivial-assign";
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
        g::generalCommand.push_back(NULL);
        g::treeCommand.push_back(NULL);
    }
}

//-----------------------------------------------------------------------------

void parseSingleArg(char * arg)
{
    if (arg == string("--neato"))
    {
        g::useNeato = true;
        g::printSize = false;
    }
    else if (arg == string("--size"))
    {
        g::printSize = true;
        g::useNeato = false;
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

/*
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
            cerr << partitionCount;
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
            Host & currentHost = g::allHosts[*linkPos];
            bool touched = currentHost.touched;
            currentHost.touched = true;
//            currentLan.forwardEdge = currentHost.name;
            list<int>::iterator pos = currentHost.lans.begin();
            list<int>::iterator limit = currentHost.lans.end();
            for (; pos != limit; ++pos)
            {
                Lan & child = g::allLans[*pos];
                // Make sure we're not doubling back on ourselves
                // immediately.
//                if (child.number != lan)
//                if (parent == -1
//                    || currentHost.name != g::allLans[parent].forwardEdge)

                // If the host has been touched, only look at LANs
                // which have been visited before, but not sourced.
                // If the host hasn't been touched, anything but a
                // source will do.
//                cerr << "  " << touched << " "
//                  << (currentHost.sources[child.number].value == Pred::VISIT)
//                  << " "
//                  << (currentHost.sources[child.number].value != Pred::SOURCE)
//                  << endl;
                if (currentLan.number != child.number &&
                    ((touched &&
                     currentHost.sources[child.number].value == Pred::VISIT)
                    ||
                    (!touched &&
                     currentHost.sources[child.number].value != Pred::SOURCE)))
                {
                cerr << currentLan.number << " -" << currentHost.name
                     << "> " << child.number << endl;
                   currentHost.sources[currentLan.number].value = Pred::SOURCE;
                   currentHost.sources[child.number].value = Pred::VISIT;
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
*/

class BiconnectAlgorithm
{
private:
    static const int NO_LAN = -1;
    static const string NO_HOST;
public:
    void calculate(void)
    {
        now = 0;
        if (g::allHosts.size() > 0)
        {
            visit(g::allHosts.begin()->second, NO_LAN, NO_HOST);

            map<string, Host>::iterator pos = g::allHosts.begin();
            map<string, Host>::iterator limit = g::allHosts.end();
//            for (; pos != limit; ++pos)
//            {
//                cerr << pos->first << ": ";
//                enumerateEquivalenceClasses(pos->second);
//                cerr << endl;
//            }

            assignComponents();
        }
    }

    // Returns the minimum time encountered.
    int visit(Host & current, int parentLanNumber,
              string const & parentHostName)
    {
        // now is used to keep track of when vertices are first
        // visited. Those visited later have greater timestamps. These
        // timestamps are used to detect cycles. A cycle happens when
        // a vertex is visited that already has a time stamp less than
        // now.
        ++now;
        current.time = now;
        int minTime = current.time;

        initEquivalenceClasses(current);
        list<int>::iterator lanPos = current.lans.begin();
        list<int>::iterator lanLimit = current.lans.end();
        for (; lanPos != lanLimit; ++lanPos)
        {
            current.forwardEdge = *lanPos;
            Lan & lan = g::allLans[*lanPos];
            list<string>::iterator childPos = lan.hosts.begin();
            list<string>::iterator childLimit = lan.hosts.end();
            for (; childPos != childLimit; ++childPos)
            {
                Host & child = g::allHosts[*childPos];
                if (child.name != parentHostName
                    && child.name != current.name)
                {
                    if (child.time == 0)
                    {
                        // Initial visit for the child.
                        int childMin = visit(child, current.forwardEdge,
                                             current.name);
                        minTime = min(minTime, childMin);
                        if (childMin < current.time
                            && parentLanNumber != NO_LAN)
                        {
                            // I only have to worry about merging on
                            // the current host because if my min
                            // value is low enough, then my parent
                            // host will take care of merging itself
                            // as well.
                            mergeEquivalenceClasses(current, parentLanNumber,
                                                    current.forwardEdge);
                        }
                        // if childMin == current.time
                        // Then some ancestor of that child has encountered
                        // current at some time in the past. And this case
                        // has already been taken care of below.
                    }
                    else
                    {
                        // A later visit for the child.
                        // This is a back edge.
                        minTime = min(minTime, child.time);

                        // If child.time < current.time, then current
                        // is a descendent and child is an
                        // ancestor. The child is somewhere in the
                        // stack above me because if it weren't, then
                        // I would have already been visited. And
                        // running this function means that I am
                        // visiting this node for the first time.
                        if (child.time < current.time)
                        {
                            // The child's forwardEdge and the return
                            // edge are both part of the same
                            // component.
                            mergeEquivalenceClasses(child, child.forwardEdge,
                                                    current.forwardEdge);
                            // My parent and my forwardEdge are part
                            // of the same component. See ancestor
                            // argument above. Except when no parent
                            // exists. In theory, because the parent
                            // has the least visit time, no other time
                            // is going to be less than it. But best
                            // to be on the safe side.
                            if (parentLanNumber != NO_LAN)
                            {
                                mergeEquivalenceClasses(current,
                                                        parentLanNumber,
                                                        current.forwardEdge);
                            }
                        }
                        else
                        {
                            // Do nothing. This is because we have
                            // already traversed this edge from the
                            // descendant side and all the work has
                            // been done.
                        }
                    }
                }
            }
        }
        return minTime;
    }

    void initEquivalenceClasses(Host & current)
    {
        current.edgeLabels.clear();
        current.partitionLists.clear();
        list<int>::iterator lanPos = current.lans.begin();
        list<int>::iterator lanLimit = current.lans.end();
        for (; lanPos != lanLimit; ++lanPos)
        {
            // We don't care whether there is already a key for that
            // LAN or not. The only situation in which that is
            // possible is if a host were incident on a LAN multiple
            // times. And that case is ignorable.
            Host::EdgeIterator edge = current.edgeLabels
                .insert(make_pair(*lanPos, *lanPos)).first;

            list<Host::EdgeIterator> partitionList;
            partitionList.push_back(edge);

            current.partitionLists.insert(make_pair(*lanPos, partitionList));
        }
    }

    void mergeEquivalenceClasses(Host & current, int leftIndex, int rightIndex)
    {
        int leftLabel = current.edgeLabels[leftIndex];
        Host::PartitionIterator leftPos
                                     = current.partitionLists.find(leftLabel);
        int rightLabel = current.edgeLabels[rightIndex];
        Host::PartitionIterator rightPos
                                     = current.partitionLists.find(rightLabel);
        Host::PartitionIterator smallPos;
        Host::PartitionIterator bigPos;

        // Figure out which list has more elements.
        if (leftPos->second.size() < rightPos->second.size())
        {
            smallPos = leftPos;
            bigPos = rightPos;
        }
        else
        {
            smallPos = rightPos;
            bigPos = leftPos;
        }

        // Iterate through smallPos, changing the number to that of bigPos.
        list<Host::EdgeIterator>::iterator edgePos = smallPos->second.begin();
        list<Host::EdgeIterator>::iterator edgeLimit = smallPos->second.end();
        for (; edgePos != edgeLimit; ++edgePos)
        {
            // The label of this edge is set to bigPos.
            (*edgePos)->second = bigPos->first;
        }

        // splice smallPos to the end of bigPos.
        bigPos->second.splice(bigPos->second.end(), smallPos->second);

        // remove smallPos from the map.
        current.partitionLists.erase(smallPos);
    }

    void enumerateEquivalenceClasses(Host & current)
    {
        Host::PartitionIterator pos = current.partitionLists.begin();
        Host::PartitionIterator limit = current.partitionLists.end();
        for (; pos != limit; ++pos)
        {
            list<Host::EdgeIterator>::iterator edgePos = pos->second.begin();
            list<Host::EdgeIterator>::iterator edgeLimit = pos->second.end();
            for (; edgePos != edgeLimit; ++edgePos)
            {
                cerr << (*edgePos)->first << " ";
            }
            cerr << "-- ";
        }
    }

    void assignComponents(void)
    {
        int nextNumber = 0;
        vector<Lan>::iterator lanPos = g::allLans.begin();
        vector<Lan>::iterator lanLimit = g::allLans.end();
        for (; lanPos != lanLimit; ++lanPos)
        {
            if (lanPos->partition == -1)
            {
                Partition & component = g::allPartitions[nextNumber];
                component.setNumber(nextNumber);
//                ComponentVertex & node = g::partitionGraph[nextNumber];
//                node.number = nextNumber;
                if (isSingleton(*lanPos))
                {
                    component.setTree();
                    treeFill(*lanPos, component);
                }
                else
                {
                    floodFill(*lanPos, component);
                }
                ++nextNumber;
            }
        }
    }

    void treeFill(Lan & source, Partition & component)
    {
        component.addLan(source.number);
        list<string>::iterator hostPos = source.hosts.begin();
        list<string>::iterator hostLimit = source.hosts.end();
        for (; hostPos != hostLimit; ++hostPos)
        {
            Host & hop = g::allHosts[*hostPos];
            list<int>::iterator lanPos = hop.lans.begin();
            list<int>::iterator lanLimit = hop.lans.end();
            for (; lanPos != lanLimit; ++lanPos)
            {
                Lan & dest = g::allLans[*lanPos];
                if (dest.partition == -1 && isSingleton(dest))
                {
                    treeFill(dest, component);
                }
            }
        }
    }

    void floodFill(Lan & source, Partition & component)
    {
        component.addLan(source.number);
        list<string>::iterator hostPos = source.hosts.begin();
        list<string>::iterator hostLimit = source.hosts.end();
        for (; hostPos != hostLimit; ++hostPos)
        {
            Host & hop = g::allHosts[*hostPos];
            list<int>::iterator lanPos = hop.lans.begin();
            list<int>::iterator lanLimit = hop.lans.end();
            for (; lanPos != lanLimit; ++lanPos)
            {
                Lan & dest = g::allLans[*lanPos];
                // Now we have a source and a dest through a hop. What
                // we want is to figure out whether source and dest
                // are in the same equivalancy class on the hop. If
                // they are, then it means that they are part of the
                // same biconnected component.  We recurse if source
                // and dest are in the same biconnected component and
                // dest hasn't already been labeled.
                int sourceEquiv = hop.edgeLabels[source.number];
                int destEquiv = hop.edgeLabels[dest.number];
                if (sourceEquiv == destEquiv && dest.partition == -1)
                {
                    floodFill(dest, component);
                }
            }
        }
    }

    bool isSingleton(Lan & source)
    {
        bool result = true;
        list<string>::iterator hostPos = source.hosts.begin();
        list<string>::iterator hostLimit = source.hosts.end();
        for (; hostPos != hostLimit && result; ++hostPos)
        {
            Host & hop = g::allHosts[*hostPos];
            int label = hop.edgeLabels[source.number];
            result = result && hop.partitionLists[label].size() < 2;
        }
        return result;
    }

private:
    int now;
};

const string BiconnectAlgorithm::NO_HOST = "";

//-----------------------------------------------------------------------------

void breakGraph(void)
{
    BiconnectAlgorithm bi;
    bi.calculate();
    makePartitionGraph();
}

//-----------------------------------------------------------------------------

void makePartitionGraph(void)
{
    int edgeNumber = 0;
    vector<Lan>::iterator lanPos = g::allLans.begin();
    vector<Lan>::iterator lanLimit = g::allLans.end();
    for (; lanPos != lanLimit; ++lanPos)
    {
        Lan & source = *lanPos;
        list<string>::iterator hostPos = source.hosts.begin();
        list<string>::iterator hostLimit = source.hosts.end();
        for (; hostPos != hostLimit; ++hostPos)
        {
            Host & host = g::allHosts[*hostPos];
            list<int>::iterator destPos = host.lans.begin();
            list<int>::iterator destLimit = host.lans.end();
            for (; destPos != destLimit; ++destPos)
            {
                Lan & dest = g::allLans[*destPos];
                if (source.partition != dest.partition)
                {
                    insertPartitionEdge(source.partition, dest.partition,
                                        edgeNumber);
                    ++edgeNumber;
                }
            }
        }
    }
}

void insertPartitionEdge(int source, int dest, int edgeNumber)
{
    g::partitionEdges.insert(make_pair(make_pair(source, dest), edgeNumber));
    g::partitionEdges.insert(make_pair(make_pair(dest, source), edgeNumber));
//    g::partitionGraph[source].neighbors.insert(dest);
//    g::partitionGraph[dest].neighbors.insert(source);
}

//-----------------------------------------------------------------------------

void findNumbers(void)
{
    map<int, Partition>::iterator pos = g::allPartitions.begin();
    map<int, Partition>::iterator limit = g::allPartitions.end();
    for (; pos != limit; ++pos)
    {
        pos->second.dispatch();
    }
    if (g::allPartitions.size() > 1)
    {
        numberPartitions();
    }
}

void numberPartitions(void)
{
    auto_ptr<FileT> pipe;
    auto_assign(pipe, coprocess(g::treeCommand[0], &(g::treeCommand[0]),
                                NULL));
//    cerr << "---BEGIN_PARTITIONG---" << endl;
    printPartitionGraph(pipe->input());
    pipe->closeIn();
    getPartitionAddresses(pipe->output());
    int childReturn = 0;
    int error = wait3(&childReturn, 0, NULL);
    if (error == -1)
    {
        cerr << "Waiting for child in prepass failed: " << strerror(errno)
             << endl;
    }
    if (childReturn != 0)
    {
        cerr << "Error in supergraph: ";
        printError(pipe->error());
    }
//    cerr << "---END_PARTITIONG---" << endl;
}

void printError(std::istream & error)
{
    string line;
    getline(error, line);
    while (error)
    {
        cerr << line << endl;
        getline(error, line);
    }
}

void printPartitionGraph(std::ostream & output)
{
    // preamble
    output << "%%";
    // Print out all of the vertices.
    int last = -1;
    map< pair<int, int>, int >::iterator pos = g::partitionEdges.begin();
    map< pair<int, int>, int >::iterator limit = g::partitionEdges.end();
    for (; pos != limit; ++pos)
    {
        if (last != pos->first.first)
        {
            last = pos->first.first;
            output << endl << "8 1 ";
        }
        output << pos->second << " ";
    }
    output << endl;
}

void getPartitionAddresses(std::istream & input)
{
    string temp;
    input >> temp;
    input >> temp;
    input >> temp;
    input >> temp;
    map<int, Partition>::iterator pos = g::allPartitions.begin();
    map<int, Partition>::iterator limit = g::allPartitions.end();
    int address = 0;
    for (; pos != limit; ++pos)
    {
        input >> address;
        if (!input)
        {
            cerr << "Insufficient partition input" << endl;
//            throw;
        }
        else
        {
            pos->second.setAddress(address);
        }
    }
}

//-----------------------------------------------------------------------------

void ouputAddresses(void)
{
    if (g::useNeato)
    {
        neatoOutput();
    }
    else if (g::printSize)
    {
        sizeOutput();
    }
    else
    {
        normalOutput();
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
        for (; lanPos != lanLimit; ++lanPos)
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

void sizeOutput(void)
{
    cerr << "Total Lan Count: " << g::allLans.size() << endl;
    cerr << "Supergraph Size: " << g::allPartitions.size() << endl;
    // One entry for the supergraph. This goes at the top.
    cout << g::allPartitions.size() << endl;
    map<int,Partition>::iterator pos = g::allPartitions.begin();
    map<int,Partition>::iterator limit = g::allPartitions.end();
    for (; pos != limit; ++pos)
    {
        cout << pos->second.getLanCount() << endl;
    }
}

//-----------------------------------------------------------------------------

void auto_assign(std::auto_ptr<FileT> & left, std::auto_ptr<FileT> right)
{
    left = right;
}

//-----------------------------------------------------------------------------
