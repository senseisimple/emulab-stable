/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "solution.h"
#include "vclass.h"
#include <string>
using namespace std;

bool compare_scores(double score1, double score2) {
    if ((score1 < (score2 + ITTY_BITTY)) && (score1 > (score2 - ITTY_BITTY))) {
	return 1;
    } else {
	return 0;
    }
}

/*
 * Print out the current solution
 */
void print_solution(const solution &s) {
    vvertex_iterator vit,veit;
    tb_vnode *vn;

    /*
     * Start by printing out all node mappings
     */
    cout << "Nodes:" << endl;
    tie(vit,veit) = vertices(VG);
    for (;vit != veit;++vit) {
	vn = get(vvertex_pmap,*vit);
	if (! s.is_assigned(*vit)) {
	    cout << "unassigned: " << vn->name << endl;
	} else {
	    cout << vn->name << " "
		<< get(pvertex_pmap,s.get_assignment(*vit))->name << endl;
	}
    }
    cout << "End Nodes" << endl;

    /*
     * Next, edges
     */
    cout << "Edges:" << endl;
    vedge_iterator eit,eendit;
    tie(eit,eendit) = edges(VG);
    for (;eit!=eendit;++eit) {
	tb_vlink *vlink = get(vedge_pmap,*eit);

	cout << vlink->name;

	if (vlink->link_info.type_used == tb_link_info::LINK_DIRECT) {
	    // Direct link - just need the source and destination
	    tb_plink *p = get(pedge_pmap,vlink->link_info.plinks.front());
	    tb_plink *p2 = get(pedge_pmap,vlink->link_info.plinks.back());
	    cout << " direct " << p->name << " (" <<
		p->srcmac << "," << p->dstmac << ") " <<
		p2->name << " (" << p2->srcmac << "," << p2->dstmac << ")";
	} else if (vlink->link_info.type_used ==
		tb_link_info::LINK_INTRASWITCH) {
	    // Intraswitch link - need to grab the plinks to both nodes
	    tb_plink *p = get(pedge_pmap,vlink->link_info.plinks.front());
	    tb_plink *p2 = get(pedge_pmap,vlink->link_info.plinks.back());
	    cout << " intraswitch " << p->name << " (" <<
		p->srcmac << "," << p->dstmac << ") " <<
		p2->name << " (" << p2->srcmac << "," << p2->dstmac << ")";
	} else if (vlink->link_info.type_used ==
		tb_link_info::LINK_INTERSWITCH) {
	    // Interswitch link - iterate through each intermediate link
	    cout << " interswitch ";
	    for (pedge_path::iterator it=vlink->link_info.plinks.begin();
		    it != vlink->link_info.plinks.end();++it) {
		tb_plink *p = get(pedge_pmap,*it);
		cout << " " << p->name << " (" << p->srcmac << "," <<
		    p->dstmac << ")";
	    }
	} else if (vlink->link_info.type_used == tb_link_info::LINK_TRIVIAL) {
	    // Trivial link - we really don't have useful information to
	    // print, but we'll fake a bunch of output here just to make it
	    // consistent with other (ie. intraswitch) output
	    vvertex vv = vlink->src;
	    tb_vnode *vnode = get(vvertex_pmap,vv);
	    pvertex pv = vnode->assignment;
	    tb_pnode *pnode = get(pvertex_pmap,pv);
	    cout << " trivial " <<  pnode->name << ":loopback" <<
		" (" << pnode->name << "/null,(null)) " <<
		pnode->name << ":loopback" << " (" << pnode->name <<
		"/null,(null)) ";
	} else {
	    // No mapping at all
	    cout << " Mapping Failed";
	}

	cout << endl;
    }

    cout << "End Edges" << endl;

    cout << "End solution" << endl;
}

/*
 * Print out a summary of the solution - mainly, we print out information from
 * the physical perspective. For example, now many vnodes are assigned to each
 * pnode, and how much total bandwidth each pnode is handling.
 */
void print_solution_summary(const solution &s)
{
  // First, print the number of vnodes on each pnode, and the total number of
  // pnodes used
  cout << "Summary:" << endl;

  // Go through all physical nodes
  int pnodes_used = 0;
  pvertex_iterator pit, pendit;
  tie(pit, pendit) = vertices(PG);
  for (;pit != pendit; pit++) {
    tb_pnode *pnode = get(pvertex_pmap,*pit);

    // We're going to treat switches specially
    bool isswitch = false;
    if (pnode->is_switch) {
      isswitch = true;
    }

    // Only print pnodes we are using, or switches we are going through
    if ((pnode->total_load > 0) ||
	(isswitch && (pnode->switch_used_links > 0))) {

      // What we really want to know is how many PCs, etc. were used, so don't
      // count switches
      if (!pnode->is_switch) {
	pnodes_used++;
      }

      // Print name, number of vnodes, and some bandwidth numbers
      cout << pnode->name << " " << pnode->total_load << " vnodes, " <<
	pnode->nontrivial_bw_used << " nontrivial BW, " <<
	pnode->trivial_bw_used << " trivial BW, type=" << pnode->current_type
	<< endl;

      // Go through all links on this pnode
      poedge_iterator pedge_it,end_pedge_it;
      tie(pedge_it,end_pedge_it) = out_edges(*pit,PG);
      for (;pedge_it!=end_pedge_it;++pedge_it) {
	tb_plink *plink = get(pedge_pmap,*pedge_it);

	tb_pnode *link_dst = get(pvertex_pmap,source(*pedge_it,PG));
	if (link_dst == pnode) {
	  link_dst = get(pvertex_pmap,target(*pedge_it,PG));
	}

	// Ignore links we aren't using
	if ((plink->emulated == 0) && (plink->nonemulated == 0)) {
	  continue;
	}

	// For switches, only print inter-switch links
	if (isswitch && (!link_dst->is_switch)) {
	  continue;
	}
	cout << "    " << plink->bw_used << " " << plink->name << endl;
      }

      // Print out used local additive features
      node_feature_set::iterator feature_it;
      for (feature_it = pnode->features.begin();
	  feature_it != pnode->features.end();++feature_it) {
	if (feature_it->is_local() && feature_it->is_l_additive()) {
	    double total = feature_it->cost();
	    double used = feature_it->used();
	  cout << "    " << feature_it->name() << ": used=" << used <<
	      " total=" << total << endl;
	}
      }
    }
  }
  cout << "Total physical nodes used: " << pnodes_used << endl;

  cout << "End summary" << endl;
}

void pvertex_writer::operator()(ostream &out,const pvertex &p) const {
    tb_pnode *pnode = get(pvertex_pmap,p);
    out << "[label=\"" << pnode->name << "\"";
    fstring style;
    if (pnode->types.find("switch") != pnode->types.end()) {
	out << " style=dashed";
    } else if (pnode->types.find("lan") != pnode->types.end()) {
	out << " style=invis";
    }
    out << "]";
}

void vvertex_writer::operator()(ostream &out,const vvertex &v) const {
    tb_vnode *vnode = get(vvertex_pmap,v);
    out << "[label=\"" << vnode->name << " ";
    if (vnode->vclass == NULL) {
	out << vnode->type;
    } else {
	out << vnode->vclass->name;
    }
    out << "\"";
    if (vnode->fixed) {
	out << " style=dashed";
    }
    out << "]";
}

void pedge_writer::operator()(ostream &out,const pedge &p) const {
    out << "[";
    tb_plink *plink = get(pedge_pmap,p);
#ifdef VIZ_LINK_LABELS
    out << "label=\"" << plink->name << " ";
    out << plink->delay_info.bandwidth << "/" <<
	plink->delay_info.delay << "/" << plink->delay_info.loss << "\"";
#endif
    if (plink->is_type == tb_plink::PLINK_INTERSWITCH) {
	out << " style=dashed";
    }
    tb_pnode *src = get(pvertex_pmap,source(p,PG));
    tb_pnode *dst = get(pvertex_pmap,target(p,PG));
    if ((src->types.find("lan") != src->types.end()) ||
	    (dst->types.find("lan") != dst->types.end())) {
	out << " style=invis";
    }
    out << "]";
}

void sedge_writer::operator()(ostream &out,const sedge &s) const {
    tb_slink *slink = get(sedge_pmap,s);
    pedge_writer pwriter;
    pwriter(out,slink->mate);
}

void svertex_writer::operator()(ostream &out,const svertex &s) const {
    tb_switch *snode = get(svertex_pmap,s);
    pvertex_writer pwriter;
    pwriter(out,snode->mate);
}

void vedge_writer::operator()(ostream &out,const vedge &v) const {
    tb_vlink *vlink = get(vedge_pmap,v);
    out << "[";
#ifdef VIZ_LINK_LABELS
    out << "label=\"" << vlink->name << " ";
    out << vlink->delay_info.bandwidth << "/" <<
	vlink->delay_info.delay << "/" << vlink->delay_info.loss << "\"";
#endif
    if (vlink->emulated) {
	out << "style=dashed";
    }
    out <<"]";
}

void graph_writer::operator()(ostream &out) const {
    out << "graph [size=\"1000,1000\" overlap=\"false\" sep=0.1]" << endl;
}

void solution_edge_writer::operator()(ostream &out,const vedge &v) const {
    tb_link_info &linfo = get(vedge_pmap,v)->link_info;
    out << "[";
    string style;
    string color;
    string label;
    switch (linfo.type_used) {
	case tb_link_info::LINK_UNMAPPED: style="dotted";color="red"; break;
	case tb_link_info::LINK_DIRECT: style="dashed";color="black"; break;
	case tb_link_info::LINK_INTRASWITCH:
	    style="solid";color="black";
	    label=get(pvertex_pmap,linfo.switches.front())->name.c_str();
	    break;
	case tb_link_info::LINK_INTERSWITCH:
	    style="solid";color="blue";
	    label="";
	    for (pvertex_list::const_iterator it=linfo.switches.begin();
		    it!=linfo.switches.end();++it) {
		label += get(pvertex_pmap,*it)->name.c_str();
		label += " ";
	    }
	    break;
	case tb_link_info::LINK_TRIVIAL: style="dashed";color="blue"; break;
    }
    out << "style=" << style << " color=" << color;
    if (label.size() != 0) {
	out << " label=\"" << label << "\"";
    }
    out << "]";
}

void solution_vertex_writer::operator()(ostream &out,const vvertex &v) const {
    tb_vnode *vnode = get(vvertex_pmap,v);
    string label = vnode->name.c_str();
    string color;
    if (my_solution.is_assigned(v)) {
	label += " ";
	label += get(pvertex_pmap,my_solution.get_assignment(v))->name.c_str();
	color = "black";
    } else {
	color = "red";
    }
    string style;
    if (vnode->fixed) {
	style="dashed";
    } else {
	style="solid";
    }
    out << "[label=\"" << label << "\" color=" << color <<
	" style=" << style << "]";
}
