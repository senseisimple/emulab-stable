
/*      dijkstra.c

        Compute shortest paths in weighted graphs using Dijkstra's algorithm

        by: Steven Skiena
        date: March 6, 2002
*/

/*
Copyright 2003 by Steven S. Skiena; all rights reserved.

Permission is granted for use in non-commerical applications
provided this copyright notice remains intact and unchanged.

This program appears in my book:

"Programming Challenges: The Programming Contest Training Manual"
by Steven Skiena and Miguel Revilla, Springer-Verlag, New York 2003.

See our website www.programming-challenges.com for additional information.

This book can be ordered from Amazon.com at

http://www.amazon.com/exec/obidos/ASIN/0387001638/thealgorithmrepo/

*/


#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <sstream>
#include <cstdio>
#include <cstdlib>

#include "wgraph.h"

using namespace std;

#define MAXINT  100007

int parent[MAXV];               /* discovery relation */
int firstHop[MAXV];
int distanceList[MAXV];             /* distance vertex is from start */



void dijkstra(graph *g, int start)           /* WAS prim(g,start) */
{
        int i,j;                        /* counters */
        bool intree[MAXV];              /* is the vertex in the tree yet? */
        int v;                          /* current vertex to process */
        int w;                          /* candidate next vertex */
        int weight;                     /* edge weight */
        int dist;                       /* best current distance from start */

        for (i=1; i<=g->nvertices; i++) {
                intree[i] = false;
                distanceList[i] = MAXINT;
                parent[i] = -1;
        }

        distanceList[start] = 0;
        v = start;
        firstHop[v] = v;

        while (intree[v] == false) {
                intree[v] = true;
                for (i=0; i<g->degree[v]; i++) {
                        w = g->edges[v][i].v;
                        weight = g->edges[v][i].weight;
/* CHANGED */           if (distanceList[w] > (distanceList[v]+weight)) {
/* CHANGED */                   distanceList[w] = distanceList[v]+weight;
/* CHANGED */                   parent[w] = v;
                                if (firstHop[v] == start)
                                {
                                    firstHop[w] = w;
                                }
                                else
                                {
                                    firstHop[w] = firstHop[v];
                                }
                        }
                }

                v = 1;
                dist = MAXINT;
                for (i=1; i<=g->nvertices; i++)
                        if ((intree[i] == false) && (dist > distanceList[i])) {
                                dist = distanceList[i];
                                v = i;
                        }
        }
/*for (i=1; i<=g->nvertices; i++) printf("%d %d\n",i,distanceList[i]);*/
}

void print_route(graph * g, int source, int dest)
{
    multimap< int, pair<string, string> >::iterator sourcePos;
    sourcePos = g->ip[source].find(firstHop[dest]);
    if (sourcePos == g->ip[source].end())
    {
        cerr << "ddijk: internal error: route not found" << endl;
        exit(1);
    }
    string sourceIp = sourcePos->second.first;
    string firstHopIp = sourcePos->second.second;

    multimap< int, pair<string, string> >::iterator pos;
    pos = g->ip[dest].begin();
    multimap< int, pair<string, string> >::iterator limit;
    limit = g->ip[dest].end();
    string previous;

    for ( ; pos != limit; ++pos)
    {
        if (pos->second.first != firstHopIp && pos->second.first != previous)
        {
            cout << "ROUTE DEST=" << pos->second.first
                 << " DESTTYPE=host DESTMASK=255.255.255.255 NEXTHOP="
                 << firstHopIp << " COST=" << distanceList[dest] << " SRC="
                 << sourceIp << endl;
            previous = pos->second.first;
        }
    }
}

int main(int argc, char * argv[])
{
    if (argc == 2)
    {
        string arg(argv[1]);
        string firstHost, secondHost;
        string firstIp, secondIp;
        map<string, int> nameToIndex;
        int weight = 0;

        graph g;
        int i = 0;
        int vertexCounter = 1;
        int numEdges = 0;

        initialize_graph(&g);
        cin >> g.nvertices >> numEdges;

        for (i = 1; i <= numEdges; ++i)
        {
            cin >> firstHost >> firstIp >> secondHost >> secondIp >> weight;
            map<string,int>::iterator firstPos = nameToIndex.find(firstHost);
            if (firstPos == nameToIndex.end())
            {
                firstPos = nameToIndex.insert(make_pair(firstHost,
                                                        vertexCounter)).first;
                ++vertexCounter;
            }
            map<string,int>::iterator secondPos = nameToIndex.find(secondHost);
            if (secondPos == nameToIndex.end())
            {
                secondPos = nameToIndex.insert(make_pair(secondHost,
                                                         vertexCounter)).first;
                ++vertexCounter;
            }
            insert_edge(&g, firstPos->second, secondPos->second, false,
                        weight);
            g.ip[firstPos->second].insert(make_pair(secondPos->second,
                                                    make_pair(firstIp,
                                                              secondIp)));
            g.ip[secondPos->second].insert(make_pair(firstPos->second,
                                                     make_pair(secondIp,
                                                               firstIp)));
        }

        map<string,int>::iterator sourcePos = nameToIndex.find(arg);
        if (sourcePos == nameToIndex.end())
        {
            cerr << "Invalid source name in command line" << endl;
            return 1;
        }

//        read_graph(&g,false);
        dijkstra(&g,sourcePos->second);

        for (i=1; i<=g.nvertices; i++)
        {
            if (i != sourcePos->second)
            {
                print_route(&g,sourcePos->second,i);
            }
//            find_path(source,i,parent);
        }
        return 0;
    }
    else
    {
        cerr << "Incorrect number of arguments" << endl;
        return 1;
    }

}



