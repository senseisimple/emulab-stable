/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef _SCORE_H
#define _SCORE_H

/*
 * 'optimal' score - computes a lower bound on the optimal score for this
 * mapping, so that if we find this score, we know we're done and can exit
 * early.
 */
#ifdef USE_OPTIMAL
#define OPTIMAL_SCORE(edges,nodes) (nodes*SCORE_PNODE + \
                                    nodes/opt_nodes_per_sw*SCORE_SWITCH + \
                                    edges*((SCORE_INTRASWITCH_LINK+ \
                                    SCORE_DIRECT_LINK*2)*4+\
                                    SCORE_INTERSWITCH_LINK)/opt_nodes_per_sw)
#else
#define OPTIMAL_SCORE(edges,nodes) 0
#endif

/*
 * Details about which violations are present
 */
class violated_info {

    /* Spit out the current violations (details only, not the total) */
    friend ostream &operator<<(ostream &o, const violated_info &vinfo) {
	o << "  unassigned: " << vinfo.unassigned << endl;
	o << "  pnode_load: " << vinfo.pnode_load << endl;
	o << "  no_connect: " << vinfo.no_connection << endl;
	o << "  link_users: " << vinfo.link_users << endl;
	o << "  bandwidth:  " << vinfo.bandwidth << endl;
	o << "  desires:    " << vinfo.desires << endl;
	o << "  vclass:     " << vinfo.vclass << endl;
	o << "  delay:      " << vinfo.delay << endl;
#ifdef FIX_PLINK_ENDPOINTS
	o << "  endpoints:  " << vinfo.incorrect_endpoints << endl;
#endif
	return o;
    }
    
    public: /* No real reason to hide this stuff */
	violated_info():
	    unassigned(0), pnode_load(0), no_connection(0), link_users(0),
	    bandwidth(0), desires(0), vclass(0), delay(0),
	    incorrect_endpoints(0) { }

	int unassigned;
	int pnode_load;
	int no_connection;
	int link_users;
	int bandwidth;
	int desires;
	int vclass;
	int delay;
	int incorrect_endpoints;
};

extern double score;
extern int violated;
extern violated_info vinfo;
extern bool allow_trivial_links;

/*
 * Scoring functions
 */
void init_score();
void remove_node(vvertex vv);
int add_node(vvertex vv,pvertex pv,bool deterministic);
double get_score();
double fd_score(tb_vnode &vnoder,tb_pnode &pnoder,int *fd_violated);
pvertex make_lan_node(vvertex vv);
void delete_lan_node(pvertex pv);

#endif
