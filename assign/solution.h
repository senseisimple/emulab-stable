/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Functions used for printing out a solution, or intermediary steps on the way
 * to a solution
 */

#ifndef __SOLUTION_H
#define __SOLUTION_H

#include "port.h"

#include "delay.h"
#include "physical.h"
#include "virtual.h"
#include "maps.h"

/* 
 * External globals
 */

/* From assign.cc */
extern node_map absassignment;
extern assigned_map absassigned;
extern tb_pgraph PG;

/* Print a solution */
void print_solution();

/* Print a summary of the solution */
void print_solution_summary();

/* Check to see if two scores are, for all intents and purposes, the same */
bool compare_scores(double score1, double score2);

/* The amount by twhich two scores can differ and still look the same - should
 * be << than the smallest possible weight
 */
const double ITTY_BITTY = 0.00001;

/*
 * These structs are used for traversing edges, etc. to produce graphviz
 * outputs. They are used as functors.
 */
struct pvertex_writer {
  void operator()(ostream &out,const pvertex &p) const;
};
struct vvertex_writer {
  void operator()(ostream &out,const vvertex &v) const;
};
struct pedge_writer {
  void operator()(ostream &out,const pedge &p) const;
};
struct sedge_writer {
  void operator()(ostream &out,const sedge &s) const;
};
struct svertex_writer {
  void operator()(ostream &out,const svertex &s) const;
};
struct vedge_writer {
  void operator()(ostream &out,const vedge &v) const;
};
struct graph_writer {
  void operator()(ostream &out) const;
};
struct solution_edge_writer {
  void operator()(ostream &out,const vedge &v) const;
};
struct solution_vertex_writer {
  void operator()(ostream &out,const vvertex &v) const;
};
#endif
