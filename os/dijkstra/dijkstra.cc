
/*      dijkstra.c

        Compute shortest paths in weighted graphs using Dijkstra's algorithm

        by: Steven Skiena
        date: March 6, 2002
*/

#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace std;

#include "Exception.h"
#include "dijkstra.h"
#include "SingleSource.h"
#include "bitmath.h"
#include "IpTree.h"

// The command line options given to us.
namespace arg
{
    string source;
    bool runAll = true;
    bool compress = false;
    bool strong = true;
}

// Figure out what the users command line wishes are and set arg::*
// appropriately.
void processArgs(int argc, char * argv[]);
void execute(void);

// Translate the input into a convenient represenation
void inputEdges(SingleSource & graph, HostHostToIpMap & ip,
                map<string, int> & nameToIndex, int numEdges);

// When we input edges, we translate the host labels into an internal
// numeric representation. When printing an error message, we want to
// translate the numbers involved back to the labels. This function
// serves that purpose. Note that this is a linear operation. But that
// is ok, because this function should only be used when presenting an
// error.
string reverseMap(int index, map<string,int> const & nameToIndex);

// Print all the routes that the command line options say we should.
void printAllRoutes(SingleSource & graph, HostHostToIpMap const & ip,
                    map<string, int> const & nameToIndex);
void printRoutesFromHost(int source, SingleSource const & graph,
                         HostHostToIpMap const & ip);

// Print all routes from a particular host to a particular host
void printRoutesToHost(int source, int dest, SingleSource const & graph,
                       HostHostToIpMap const & ip);

// Calculate the source ip address and the first hop ip address
// between a particular pair of hosts.
void calculateSourceInfo(int source, int dest,
                         SingleSource const & graph,
                         HostHostToIpMap const & ip,
                         string & OutSourceIp, string & OutFirstHopIp);

// Print a route from a host to a specific ip address on another host.
void printRouteToIp(string sourceIp, string firstHopIp, string destIp,
                    int cost);


//-----------------------------------------------------------------------------

int main(int argc, char * argv[])
{
    try
    {
        processArgs(argc, argv);
        execute();
    }
    catch(exception & error)
    {
        cerr << argv[0] << ": " << error.what() << endl;
        return 1;
    }
    return 0;
}

//-----------------------------------------------------------------------------

void processArgs(int argc, char * argv[])
{
    // The if/else/if/else construct is cumbersome. Is there a better way?
    for (int i = 1; i < argc; ++i)
    {
        if (strncmp(argv[i], "--source=", sizeof("--source=") - 1) == 0)
        {
            arg::runAll = false;
            arg::source = argv[i] + sizeof("--source=") - 1;
        }
        else if (strncmp(argv[i], "--all", sizeof("--all") - 1) == 0)
        {
            arg::runAll = true;
        }
        else if (strncmp(argv[i], "--compress", sizeof("--compress") - 1) == 0)
        {
            arg::compress = true;
        }
        else if (strncmp(argv[i], "--strong", sizeof("--strong") - 1) == 0)
        {
            arg::strong = true;
        }
        else if (strncmp(argv[i], "--weak", sizeof("--weak") - 1) == 0)
        {
            arg::strong = false;
        }
    }
}

//-----------------------------------------------------------------------------

void execute(void)
{
    map<string, int> nameToIndex;

    int i = 0;
    int numVertices = 0;
    int numEdges = 0;

    cin >> numVertices >> numEdges;
    SingleSource graph(numVertices);
    HostHostToIpMap ip(numVertices);

    // If a problem occurs, we need to translate from our internal ids
    // to labels recognizable by the user.
    try
    {
        inputEdges(graph, ip, nameToIndex, numEdges);
        printAllRoutes(graph, ip, nameToIndex);
    }
    catch(EdgeInsertException & error)
    {
        error.setMessage(reverseMap(error.getSource(), nameToIndex),
                         reverseMap(error.getDest(), nameToIndex),
                         intToString(error.getCost()));
        throw error;
    }
    catch (RouteNotFoundException & error)
    {
        error.setMessage(reverseMap(error.getSource(), nameToIndex),
                         reverseMap(error.getDest(), nameToIndex),
                         reverseMap(error.getFirstHop(), nameToIndex));
        throw error;
    }
}

//-----------------------------------------------------------------------------

string reverseMap(int index, map<string,int> const & nameToIndex)
{
    // linear search
    string result = "[ERROR-IN-ERROR: reverse mapping failed: index="
        + intToString(index) + "]";
    map<string,int>::const_iterator pos = nameToIndex.begin();
    map<string,int>::const_iterator limit = nameToIndex.end();
    for ( ; pos != limit; ++pos)
    {
        if (pos->second == index)
        {
            result = pos->first;
        }
    }
    return result;
}

//-----------------------------------------------------------------------------

void inputEdges(SingleSource & graph, HostHostToIpMap & ip,
                map<string, int> & nameToIndex, int numEdges)
{
    string firstHost, secondHost;
    string firstIp, secondIp;
    int weight = 0;
    int vertexCounter = 0;

    for (int i = 0; i < numEdges; ++i)
    {
        cin >> firstHost >> firstIp >> secondHost >> secondIp >> weight;

        map<string,int>::iterator firstPos = nameToIndex.find(firstHost);
        if (firstPos == nameToIndex.end())
        {
            firstPos = nameToIndex.insert(make_pair(firstHost,
                                                    vertexCounter)).first;
            ++vertexCounter;
            if (static_cast<size_t>(vertexCounter) > ip.size())
            {
                throw TooManyHostsException(static_cast<int>(ip.size()));
            }
        }

        map<string,int>::iterator secondPos = nameToIndex.find(secondHost);
        if (secondPos == nameToIndex.end())
        {
            secondPos = nameToIndex.insert(make_pair(secondHost,
                                                     vertexCounter)).first;
            ++vertexCounter;
            if (static_cast<size_t>(vertexCounter) > ip.size())
            {
                throw TooManyHostsException(static_cast<int>(ip.size()));
            }
        }

        graph.insertEdge(firstPos->second, secondPos->second, weight);
        ip[firstPos->second].insert(make_pair(secondPos->second,
                                              make_pair(firstIp,
                                                        secondIp)));
        ip[secondPos->second].insert(make_pair(firstPos->second,
                                               make_pair(secondIp,
                                                         firstIp)));
    }
}

//-----------------------------------------------------------------------------

void printAllRoutes(SingleSource & graph, HostHostToIpMap const & ip,
                    map<string, int> const & nameToIndex)
{
    if (arg::runAll)
    {
        map<int, string> indexToName;
        map<string, int>::const_iterator pos = nameToIndex.begin();
        map<string, int>::const_iterator limit = nameToIndex.end();
        for ( ; pos != limit; ++pos)
        {
            indexToName[pos->second] = pos->first;
        }
        for (int i = 0; i < graph.getVertexCount(); ++i)
        {
            cout << indexToName[i] << endl;
            graph.route(i);
            if (arg::compress)
            {
                Compressor temp;
                temp.compress(graph, ip);
            }
            else
            {
                printRoutesFromHost(i, graph, ip);
            }
            cout << "%%" << endl;
        }
    }
    else
    {
        map<string,int>::const_iterator sourcePos;
        sourcePos = nameToIndex.find(arg::source);
        if (sourcePos == nameToIndex.end())
        {
            throw InvalidSourceException(arg::source);
        }
        graph.route(sourcePos->second);
        if (arg::compress)
        {
            Compressor temp;
            temp.compress(graph, ip);
        }
        else
        {
            printRoutesFromHost(sourcePos->second, graph, ip);
        }
    }
}

//-----------------------------------------------------------------------------

void printRoutesFromHost(int source, SingleSource const & graph,
                         HostHostToIpMap const & ip)
{
    for (int i = 0; i < graph.getVertexCount(); ++i)
    {
        if (i != source)
        {
            printRoutesToHost(source, i, graph, ip);
        }
    }
}

//-----------------------------------------------------------------------------

void printRoutesToHost(int source, int dest, SingleSource const & graph,
                       HostHostToIpMap const & ip)
{
    string sourceIp;
    string firstHopIp;
    calculateSourceInfo(source, dest, graph, ip, sourceIp, firstHopIp);

    multimap< int, pair<string, string> >::const_iterator pos;
    pos = ip[dest].begin();
    multimap< int, pair<string, string> >::const_iterator limit;
    limit = ip[dest].end();
    string previous;

    for ( ; pos != limit; ++pos)
    {
        string const & destIp = pos->second.first;
        if (destIp != previous)
        {
            printRouteToIp(sourceIp, firstHopIp, destIp,
                           graph.getDistance(dest));
            previous = destIp;
        }
    }
}

//-----------------------------------------------------------------------------

void calculateSourceInfo(int source, int dest,
                         SingleSource const & graph,
                         HostHostToIpMap const & ip,
                         string & outSourceIp, string & outFirstHopIp)
{
    multimap< int, pair<string, string> >::const_iterator sourcePos;
    sourcePos = ip[source].find(graph.getFirstHop(dest));
    if (sourcePos == ip[source].end())
    {
        throw RouteNotFoundException(source, dest, graph.getFirstHop(dest));
    }
    outSourceIp = sourcePos->second.first;
    outFirstHopIp = sourcePos->second.second;
}

//-----------------------------------------------------------------------------

void printRouteToIp(string sourceIp, string firstHopIp, string destIp,
                    int cost)
{
    if (destIp != firstHopIp)
    {
        cout << "ROUTE DEST=" << destIp
             << " DESTTYPE=host DESTMASK=255.255.255.255 NEXTHOP="
             << firstHopIp << " COST=" << cost << " SRC="
             << sourceIp << endl;
    }
}

//-----------------------------------------------------------------------------



void printRouteToSubnet(string sourceIp, string firstHopIp, string destSubnet,
                        int netMaskSize, int cost)
{
    if (netMaskSize == 32)
    {
        printRouteToIp(sourceIp, firstHopIp, destSubnet, cost);
    }
    else
    {
        IPAddress netMask = 0xffffffff << (32 - netMaskSize);
        cout << "ROUTE DEST=" << destSubnet << " DESTTYPE=net DESTMASK="
             << ipToString(netMask) << " NEXTHOP=" << firstHopIp << " COST="
             << cost << " SRC=" << sourceIp << endl;
    }
}



//-----------------------------------------------------------------------------

string intToString(int num)
{
    ostringstream stream;
    stream << num;
    return stream.str();
}
//-----------------------------------------------------------------------------
