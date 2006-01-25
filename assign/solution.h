/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003-2006 University of Utah and the Flux Group.
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
 * Stucture to hold a potential solution
 */
class solution {
    public:
    explicit solution() : vnode_assignments(), vtype_assignments(),
	vlink_assignments() { ; };
    ~solution() {;};
    
    // Copy constructor and operator
    solution(const solution &other) : vnode_assignments(other.vnode_assignments),
	vtype_assignments(other.vtype_assignments) { ; };
    
    solution &operator=(const solution &other) {
	// TODO: make sure copying the assignments doesn't alias!
	this->vnode_assignments = other.vnode_assignments;
	this->vtype_assignments = other.vtype_assignments;
    };
    
    inline bool is_assigned(const vvertex &vv) const {
	//return vnode_is_assigned[vv];
	node_map::const_iterator it = vnode_assignments.find(vv);
	if (it == vnode_assignments.end()) {
	    return false;
	} else {
	    return true;
	}
    };
    inline bool link_is_assigned(const vedge &ve) const {
	link_map::const_iterator it = vlink_assignments.find(ve);
	if (it == vlink_assignments.end()) {
	    return false;
	} else {
	    return true;
	}
    };
    
    inline pvertex get_assignment(const vvertex &vv) const {
	node_map::const_iterator it = vnode_assignments.find(vv);
	return it->second;
    };
    inline fstring get_vtype_assignment(const vvertex &vv) const {
	type_map::const_iterator it = vtype_assignments.find(vv);
	return it->second;
	//return vtype_assignments[vv];
    };
    inline tb_link_info get_link_assignment(const vedge &ve) const {
	link_map::const_iterator it = vlink_assignments.find(ve);
	return it->second;
    };
    
    inline void set_assignment(const vvertex &vv, const pvertex &pv) {
	vnode_assignments[vv] = pv;
    }
    
    inline void clear_assignment(const vvertex &vv) {
	vnode_assignments.erase(vv);
    }
    
    inline void set_link_assignment(const vedge &ve, const tb_link_info &info) {
	vlink_assignments[ve] = info;
    }
    
    inline void clear_link_assignment(const vedge &ve) {
	vlink_assignments.erase(ve);
    }
    
    inline void set_vtype_assignment(const vvertex &vv, const fstring t) {
	vtype_assignments[vv] = t;
    }
    
    private:
    /*
     * These variables store the actual solution
     */
    // The vvertex -> pnode : assignment
    node_map vnode_assignments;
    // vvertex -> bool : is the vnode assigned?
    //assigned_map vnode_is_assigned;
    // vtype -> ptype : what type has the ptype taken on?
    type_map vtype_assignments;
    // vedge -> link_info
    link_map vlink_assignments;
};

/* 
 * External globals
 */

/* From assign.cc */
extern tb_pgraph PG;

/* Print a solution */
void print_solution(const solution &s);

/* Print a summary of the solution */
void print_solution_summary(const solution &s);

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
    solution_vertex_writer(const solution &s) : my_solution(s) { ; };
  void operator()(ostream &out,const vvertex &v) const;
  private:
  solution_vertex_writer(); // Hide it
  const solution my_solution;
};
#endif
