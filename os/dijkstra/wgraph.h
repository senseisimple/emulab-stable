
/*      wgraph.h

        Header file for weighted graph type

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

#if __GNUC__ == 3 && __GNUC_MINOR__ > 0
#include <string>
#include<backward/pair.h>
using namespace std;
#endif

#define MAXV            20000           /* maximum number of vertices */
#define MAXDEGREE       50              /* maximum outdegree of a vertex */

typedef struct {
        int v;                          /* neighboring vertex */
        int weight;                     /* edge weight */
} edge;

typedef struct {
        edge edges[MAXV+1][MAXDEGREE];  /* adjacency info */
        int degree[MAXV+1];             /* outdegree of each vertex */
        int nvertices;                  /* number of vertices in the graph */
        int nedges;                     /* number of edges in the graph */
    // map from dest edge to (source ip, dest ip)
    std::multimap< int, pair<string, string> > ip[MAXV + 1];
} graph;

void initialize_graph(graph * g);
void read_graph(graph * g, bool directed);
void insert_edge(graph * g, int x, int y, bool directed, int w);
void delete_edge(graph * g, int x, int y, bool directed);
void print_graph(graph * g);
void initialize_search(graph * g);
void dfs(graph * g, int v);
void process_vertex(int v);
void process_edge(int x, int y);
void find_path(int start, int end, int parents[]);
void connected_components(graph * g);
