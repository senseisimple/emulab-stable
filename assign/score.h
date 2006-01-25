/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2005 University of Utah and the Flux Group.
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
	o << "  unassigned:  " << vinfo.unassigned << endl;
	o << "  pnode_load:  " << vinfo.pnode_load << endl;
	o << "  no_connect:  " << vinfo.no_connection << endl;
	o << "  link_users:  " << vinfo.link_users << endl;
	o << "  bandwidth:   " << vinfo.bandwidth << endl;
	o << "  desires:     " << vinfo.desires << endl;
	o << "  vclass:      " << vinfo.vclass << endl;
	o << "  delay:       " << vinfo.delay << endl;
	o << "  trivial mix: " << vinfo.trivial_mix << endl;
	o << "  subnodes:    " << vinfo.subnodes << endl;
	o << "  max_types:   " << vinfo.max_types << endl;
#ifdef FIX_PLINK_ENDPOINTS
	o << "  endpoints:   " << vinfo.incorrect_endpoints << endl;
#endif
	return o;
    }
    
    public: /* No real reason to hide this stuff */
	violated_info():
	    unassigned(0), pnode_load(0), no_connection(0), link_users(0),
	    bandwidth(0), desires(0), vclass(0), delay(0),
	    incorrect_endpoints(0), trivial_mix(0), subnodes(0), max_types(0)
	{ }
	
	int count_violations() const {
	    return unassigned + pnode_load + no_connection + link_users + bandwidth
		+ desires + vclass + delay + incorrect_endpoints + trivial_mix 
		+ subnodes + max_types;
	}

	int unassigned;
	int pnode_load;
	int no_connection;
	int link_users;
	int bandwidth;
	int desires;
	int vclass;
	int delay;
	int incorrect_endpoints;
	int trivial_mix;
	int subnodes;
	int max_types;
};

extern double score;
extern int violated;
extern violated_info vinfo;
extern bool allow_trivial_links;
extern bool greedy_link_assignment;

/*
 * Vector used to hold several possible link resolutions
 */
typedef vector<tb_link_info> resolution_vector;

/*
 * Scoring functions
 */
void init_score();
void remove_node(vvertex vv);
int add_node(vvertex vv,pvertex pv,bool deterministic, bool is_fixed, bool skip_links = false);
double get_score();
pvertex make_lan_node(vvertex vv);
void delete_lan_node(pvertex pv);

/*
 * Functions specifically for dealing with links
 */
void score_link_info(vedge ve, tb_pnode *src_pnode, tb_pnode *dst_pnode,
	tb_vnode *src_vnode, tb_vnode *dst_vnode);
void unscore_link_info(vedge ve, tb_pnode *src_pnode, tb_pnode *dst_pnode,
	tb_vnode *src_vnode, tb_vnode *dst_vnode);
float find_link_resolutions(resolution_vector &resolutions, pvertex pv,
    pvertex dest_pv, tb_vlink *vlink, tb_pnode *pnode, tb_pnode *dest_pnode,
    bool flipped);
void resolve_link(vvertex vv, pvertex pv, tb_vnode *vnode, tb_pnode *pnode,
    bool deterministic, hash_set<const tb_vlink*, 
    hashptr<const tb_vlink*> > &seen_loopback_links, vedge edge);
void resolve_links(vvertex vv, pvertex pv, tb_vnode *vnode, tb_pnode *pnode,
    bool deterministic);
void mark_vlink_unassigned(tb_vlink *vlink);
#endif
