/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "solution.h"

/*
 * Print out the current solution
 */
void print_solution()
{
    vvertex_iterator vit,veit;
    tb_vnode *vn;

    /*
     * Start by printing out all node mappings
     */
    cout << "Nodes:" << endl;
    tie(vit,veit) = vertices(VG);
    for (;vit != veit;++vit) {
	vn = get(vvertex_pmap,*vit);
	if (! vn->assigned) {
	    cout << "unassigned: " << vn->name << endl;
	} else {
	    cout << vn->name << " "
		<< get(pvertex_pmap,vn->assignment)->name << endl;
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

	if (vlink->link_info.type == tb_link_info::LINK_DIRECT) {
	    // Direct link - just need the source and destination
	    tb_plink *p = get(pedge_pmap,vlink->link_info.plinks.front());
	    tb_plink *p2 = get(pedge_pmap,vlink->link_info.plinks.back());
	    cout << " direct " << p->name << " (" <<
		p->srcmac << "," << p->dstmac << ") " <<
		p2->name << " (" << p2->srcmac << "," << p2->dstmac << ")";
	} else if (vlink->link_info.type == tb_link_info::LINK_INTRASWITCH) {
	    // Intraswitch link - need to grab the plinks to both nodes
	    tb_plink *p = get(pedge_pmap,vlink->link_info.plinks.front());
	    tb_plink *p2 = get(pedge_pmap,vlink->link_info.plinks.back());
	    cout << " intraswitch " << p->name << " (" <<
		p->srcmac << "," << p->dstmac << ") " <<
		p2->name << " (" << p2->srcmac << "," << p2->dstmac << ")";
	} else if (vlink->link_info.type == tb_link_info::LINK_INTERSWITCH) {
	    // Interswitch link - interate through each intermediate link
	    cout << " interswitch ";
	    for (pedge_path::iterator it=vlink->link_info.plinks.begin();
		    it != vlink->link_info.plinks.end();++it) {
		tb_plink *p = get(pedge_pmap,*it);
		cout << " " << p->name << " (" << p->srcmac << "," <<
		    p->dstmac << ")";
	    }
	} else if (vlink->link_info.type == tb_link_info::LINK_TRIVIAL) {
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

void pvertex_writer::operator()(ostream &out,const pvertex &p) const {
    tb_pnode *pnode = get(pvertex_pmap,p);
    out << "[label=\"" << pnode->name << "\"";
    crope style;
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
    if (plink->type == tb_plink::PLINK_INTERSWITCH) {
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
    crope style;
    crope color;
    crope label;
    switch (linfo.type) {
	case tb_link_info::LINK_UNKNOWN: style="dotted";color="red"; break;
	case tb_link_info::LINK_DIRECT: style="dashed";color="black"; break;
	case tb_link_info::LINK_INTRASWITCH:
	    style="solid";color="black";
	    label=get(pvertex_pmap,linfo.switches.front())->name;
	    break;
	case tb_link_info::LINK_INTERSWITCH:
	    style="solid";color="blue";
	    label="";
	    for (pvertex_list::const_iterator it=linfo.switches.begin();
		    it!=linfo.switches.end();++it) {
		label += get(pvertex_pmap,*it)->name;
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
    crope label=vnode->name;
    crope color;
    if (absassigned[v]) {
	label += " ";
	label += get(pvertex_pmap,absassignment[v])->name;
	color = "black";
    } else {
	color = "red";
    }
    crope style;
    if (vnode->fixed) {
	style="dashed";
    } else {
	style="solid";
    }
    out << "[label=\"" << label << "\" color=" << color <<
	" style=" << style << "]";
}
