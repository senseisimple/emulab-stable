/*      wgraph.c

        A generic weighted graph data type

        by Steven Skiena
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
#include <cstdio>
#include "wgraph.h"

using namespace std;

void initialize_graph(graph * g)
//graph *g;                             /* graph to initialize */
{

        int i;                          /* counter */

        g -> nvertices = 0;
        g -> nedges = 0;

        for (i=1; i<=MAXV; i++) g->degree[i] = 0;
}

void read_graph(graph * g, bool directed)
//graph *g;                               /* graph to initialize */
//bool directed;                                /* is this graph directed? */
{
        int i;                          /* counter */
        int m;                          /* number of edges */
        int x,y,w;                      /* placeholder for edge and weight */
        string xIp, yIp;

        initialize_graph(g);

//        scanf("%d %d\n",&(g->nvertices),&m);
        cin >> g->nvertices >> m;

        for (i=1; i<=m; i++) {
//                scanf("%d %d %d\n",&x,&y,&w);
            cin >> x >> xIp >> y >> yIp >> w;
            insert_edge(g,x+1,y+1,directed,w);
        }
}

void insert_edge(graph * g, int x, int y, bool directed, int w)
//graph *g;                               /* graph to initialize */
//int x, y;                               /* placeholder for edge (x,y) */
//bool directed;                          /* is this edge directed? */
//int w;                                  /* edge weight */
{
        if (g->degree[x] > MAXDEGREE)
                printf("Warning: insertion(%d,%d) exceeds degree bound\n",x,y);

        g->edges[x][g->degree[x]].v = y;
        g->edges[x][g->degree[x]].weight = w;
        /*g->edges[x][g->degree[x]].in = false;*/
        g->degree[x] ++;

        if (directed == false)
                insert_edge(g,y,x,true,w);
        else
                g->nedges ++;
}


void delete_edge(graph * g, int x, int y, bool directed)
//graph *g;                               /* graph to initialize */
//int x, y;                               /* placeholder for edge (x,y) */
//bool directed;                          /* is this edge directed? */
{
        int i;                          /* counter */

        for (i=0; i<g->degree[x]; i++)
                if (g->edges[x][i].v == y) {
                        g->degree[x] --;
                        g->edges[x][i] = g->edges[x][g->degree[x]];

                        if (directed == false)
                                delete_edge(g,y,x,true);

                        return;
                }

        printf("Warning: deletion(%d,%d) not found in g.\n",x,y);
}

void print_graph(graph * g)
//graph *g;                               /* graph to print */
{
        int i,j;                        /* counters */

        for (i=1; i<=g->nvertices; i++) {
                printf("%d: ",i);
                for (j=0; j<g->degree[i]; j++)
                        printf(" %d",g->edges[i][j].v);
                printf("\n");
        }
}

//bool processed[MAXV];           /* which vertices have been processed */
//bool discovered[MAXV];          /* which vertices have been found */
//int parent[MAXV];               /* discovery relation */


//void initialize_search(graph * g)
//graph *g;                               /* graph to traverse */
//{
//        int i;                          /* counter */

//        for (i=1; i<=g->nvertices; i++) {
//                processed[i] = false;
//                discovered[i] = false;
//                parent[i] = -1;
//        }
//}


//void dfs(graph * g, int v)
//graph *g;                               /* graph to traverse */
//int v;                                  /* vertex to start searching from */
//{
//        int i;                          /* counter */
//        int y;                          /* successor vertex */
//
//        discovered[v] = true;
//        process_vertex(v);
//
//      for (i=0; i<g->degree[v]; i++) {
//              y = g->edges[v][i].v;
//              if (discovered[y] == false) {
//                      parent[y] = v;
//                      dfs(g,y);
//              } else
//                      if (processed[y] == false)
//                              process_edge(v,y);
//      }
//
//      processed[v] = true;
//
//
void process_vertex(int v)
//int v;                                  /* vertex to process */
{
        printf(" %d",v);
}

void process_edge(int x, int y)
//int x,y;                                /* edge to process */
{
}




void find_path(int start, int end, int parents[])
//int start;                              /* first vertex on path */
//int end;                                /* last vertex on path */
//int parents[];                          /* array of parent pointers */
{
        if ((start == end) || (end == -1))
                printf("\n%d",start);
        else {
                find_path(start,parents[end],parents);
                printf(" %d",end);
        }
}

//void connected_components(graph * g)
//graph *g;                               /* graph to analyze */
//{
//        int c;                          /* component number */
//        int i;                          /* counter */
//
//        initialize_search(g);
//
//        c = 0;
//        for (i=1; i<=g->nvertices; i++)
//                if (discovered[i] == false) {
//                        c = c+1;
//                        printf("Component %d:",c);
//                        dfs(g,i);
//                        printf("\n");
//                }
//}

