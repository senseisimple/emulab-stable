/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * routing.cc
 *
 * routes traffic between nodes (with weighted edges)
 * in the same manner as ns-2.
 *
 * crb May 7 2002
 *
 * This file largely based on code which was borrowed from ns-2.
 * As such, the following copyright applies to portions of this code:
 *
 * Copyright (c) 1991-1994 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and the Network Research Group at
 *	Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Routing code for general topologies based on min-cost routing algorithm in
 * Bertsekas' book.  Written originally by S. Keshav, 7/18/88
 * (his work covered by identical UC Copyright)
 */

#include <stdlib.h>
#include <stdio.h>

#include <list>
#include <algorithm>
using namespace std;

// Forward decls 
void routing_init();
void routing_insert( int sid, int tid, double cost, void * etc = NULL );
void routing_calc();
int  routing_lookup( int sid, int did, void ** etcReturn = NULL );

// bet ya didnt know that 0x3fff == infinity...
#define INFINITY	0x3fff

// for some bizarre reason, cost is a double, though it gets added to ints..
struct adj_entry {
  double cost;
  void * etc;
};

struct route_entry {
  int next_hop;
  void * etc;
};

static int size;
static adj_entry * adj;
static route_entry * route;

static inline adj_entry   & ADJ_REF( int s, int d ) { return adj[ s * size + d ]; }
static inline route_entry & ROUTE_REF( int s, int d ) { return route[ s * size + d ]; }

#define ADJ(i, j) ADJ_REF( i, j ).cost
#define ADJ_ENTRY(i, j) ADJ_REF( i, j ).etc
#define ROUTE(i, j) ROUTE_REF( i, j ).next_hop
#define ROUTE_ENTRY(i, j) ROUTE_REF( i, j ).etc

struct queued_adj_entry {
  int sid, tid;
  adj_entry adj;
};

static list< queued_adj_entry > * queued_adj_entries;

void routing_init()
{
  size  = 0;
  adj   = NULL;
  route = NULL;
  queued_adj_entries = new list< queued_adj_entry >();
}

// rather than inserting entries directly, and constantly growing
// the NxN array, I queue entries up until routes are calculated,
// at which time, a correctly sized array is allocated, and 
// the entries are loaded in.

void routing_insert( int sid, int tid, double cost, void * etc )
{
  sid++; 
  tid++;
  queued_adj_entry e;
  e.sid = sid;
  e.tid = tid;
  e.adj.cost = cost;
  e.adj.etc  = etc;
  queued_adj_entries->push_back( e );
  if ((sid + 1) > size) { size = sid + 1; }
  if ((tid + 1) > size) { size = tid + 1; }
}

static inline void add_to_adj( queued_adj_entry & qe )
{
  // printf("Adding %i -> %i.\n", qe.sid, qe.tid );
  ADJ_REF( qe.sid, qe.tid ) = qe.adj;
}

static void compute_routes();

void routing_calc()
{
  if (adj) { delete[] adj; }
  adj = new adj_entry[ size * size ];
  for (int i = 0; i < (size * size); i++) {
    adj[i].cost = INFINITY;
    adj[i].etc  = NULL;
  }

  for_each( queued_adj_entries->begin(), queued_adj_entries->end(), add_to_adj );
  delete queued_adj_entries;
  queued_adj_entries = new list< queued_adj_entry >();

  if (route) { delete[] route; }
  route = new route_entry[size * size];
  memset((char *)route, 0, size * size * sizeof(route_entry));
  compute_routes();
}

// The actual algorithm. Joy.
static void compute_routes()
{
  int n = size;
  int* parent = new int[n];
  double* hopcnt = new double[n];
  
  /* do for all the sources */
  int k;
  for (k = 1; k < n; ++k) {
    int v;
    for (v = 0; v < n; v++)
      parent[v] = v;
    
    /* set the route for all neighbours first */
    for (v = 1; v < n; ++v) {
      if (parent[v] != k) {
	hopcnt[v] = ADJ(k, v);
	if (hopcnt[v] != INFINITY) {
	  ROUTE(k, v) = v;
	  ROUTE_ENTRY(k, v) = ADJ_ENTRY(k, v);
	}
      }
    }
    for (v = 1; v < n; ++v) {
      /*
       * w is the node that is the nearest to the subtree
       * that has been routed
       */
      int o = 0;
      /* XXX */
      hopcnt[0] = INFINITY;
      int w;
      for (w = 1; w < n; w++)
	if (parent[w] != k && hopcnt[w] < hopcnt[o])
	  o = w;
      parent[o] = k;
      /*
       * update distance counts for the nodes that are
       * adjacent to o
       */
      if (o == 0)
	continue;
      for (w = 1; w < n; w++) {
	if (parent[w] != k &&
	    hopcnt[o] + ADJ(o, w) < hopcnt[w]) {
	  ROUTE(k, w) = ROUTE(k, o);
	  ROUTE_ENTRY(k, w) = 
	    ROUTE_ENTRY(k, o);
	  hopcnt[w] = hopcnt[o] + ADJ(o, w);
	}
      }
    }
  }
  /*
   * The route to yourself is yourself.
   */
  for (k = 1; k < n; ++k) {
    ROUTE(k, k) = k;
    ROUTE_ENTRY(k, k) = 0; // This should not matter
  }
  
  delete[] hopcnt;
  delete[] parent;
}

int routing_lookup(int sid, int did, void ** etcReturn) {
  int src = sid+1;
  int dst = did+1;
  if (route == 0) {
    printf("routes not yet computed\n");
    return (-1);
  }
  if (src >= size || dst >= size) {
    printf("node out of range\n");
    return (-2);
  }
  if (etcReturn) {
    *etcReturn = ROUTE_ENTRY( src, dst );
  }
  return ROUTE(src, dst) - 1;
}

void routing_printall()
{
  int s;
  if (route == 0) {
    printf("routes not yet computed\n");
    return;
  }
  for (s = 1; s < size; s++) {
    int d;
    for (d = 1; d < size; d++) {
      // Do not print routes for adj or self.
      if ( s != d && 
	   ROUTE( s, d ) > 0 ) {
	printf("route %i to %i via %i\n", s - 1, d - 1, ROUTE( s, d ) - 1 );
      }
    }
  }
}

int main()
{
  routing_init();

  while(1) {
    char line[1024];
    fgets( line, 1024, stdin );
    if (line[0] == 'i' || line[0] == 'I') {
      char foo;
      int s,t;
      float cost = 1.0f; // 1 is ns's default, as well.
      sscanf( line, "%c %i %i %f", &foo, &s, &t, &cost );
      routing_insert( s, t, cost );
      if (line[0] == 'I') { routing_insert( t, s, cost ); }
    } else if (line[0] == 'c') {
      routing_calc();
    } else if (line[0] == 'C') {
      routing_calc();
      routing_printall();
      break;
    } else if (line[0] == 'l') {
      char foo;
      int s,t;
      sscanf( line, "%c %i %i", &foo, &s, &t );
      printf( "result: %i\n", routing_lookup(s,t) );
    } else if (line[0] == 'p') {
      routing_printall();
    } else if (line[0] == 'q') {
      break;
    } else if (line[0] == 'h') {
      printf( "'i <s> <t> <c>' - inserts adj from s to t, cost c\n" 
	      "'I <s> <t> <c>' - as above, but inserts adj from t to s, too\n"
	      "'c' - calculates routes\n"
	      "'C' - calculates routes, prints all, and quits\n"
	      "'l <s> <t>' - looks up calculated route from s to t\n"
	      "'p' - prints all routes\n"
	      "'q' - quits\n" );
    } else {
      printf( "Unknown command. 'h' for help.\n" );
    }
  }
  exit(0);
}
