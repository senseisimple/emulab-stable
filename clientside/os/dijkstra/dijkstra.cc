// dijkstra.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "lib.h"
#include "Exception.h"
#include "dijkstra.h"
#include "SingleSource.h"
#include "bitmath.h"
#include "Compressor.h"
#include "NoneCompressor.h"
#include "TreeCompressor.h"
#include "VoteIpTree.h"
#include "OptimalIpTree.h"
#include <cstring>

using namespace std;

// The command line options given to us.
namespace arg
{
    string source;
    bool runAll = true;
    bool strong = true;
    auto_ptr<Compressor> compress;
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
    arg::compress.reset(new NoneCompressor());
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
        else if (strncmp(argv[i], "--compress=vote",
                         sizeof("--compress=vote") - 1) == 0)
        {
            VoteIpTree temp;
            arg::compress.reset(new TreeCompressor(temp));
        }
        else if (strncmp(argv[i], "--compress=optimal",
                         sizeof("--compress=optimal") - 1) == 0)
        {
            OptimalIpTree temp;
            arg::compress.reset(new TreeCompressor(temp));
        }
        else if (strncmp(argv[i], "--compress=none",
                         sizeof("--compress=none") - 1) == 0)
        {
            arg::compress.reset(new NoneCompressor());
        }
/*
        else if (strncmp(argv[i], "--compress=rocket",
                         sizeof("--compress=rocket") - 1) == 0)
        {
            arg::compress.reset(new RocketCompressor());
        }
*/
        else if (strncmp(argv[i], "--compress",
                         sizeof("--compress") - 1) == 0)
        {
            VoteIpTree temp;
            arg::compress.reset(new TreeCompressor(temp));
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
            arg::compress->printRoutes(graph, ip);
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
        arg::compress->printRoutes(graph, ip);
    }
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
        if (netMaskSize <= 0)
        {
            netMask = 0;
        }
//        cerr << netMaskSize << ", " << netMask << ", " << (0xffffffff << 32) << endl;
        cout << "ROUTE DEST=" << destSubnet << " DESTTYPE=net DESTMASK="
             << ipToString(netMask) << " NEXTHOP=" << firstHopIp << " COST="
             << cost << " SRC=" << sourceIp << endl;
    }
}

//-----------------------------------------------------------------------------
