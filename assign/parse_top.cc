/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "port.h"

#include <hash_map>
#include <slist>
#include <queue>
#include <rope>

#include <boost/config.hpp>
#include <boost/utility.hpp>
#include <boost/property_map.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>

#include <iostream.h>

using namespace boost;

#include "common.h"
#include "vclass.h"
#include "delay.h"
#include "physical.h"
#include "virtual.h"
#include "parser.h"

extern name_vvertex_map vname2vertex;
extern name_name_map fixed_nodes;
extern name_count_map vtypes;
extern vvertex_vector virtual_nodes;

#define top_error(s) errors++;cerr << "TOP:" << line << ": " << s << endl

int parse_top(tb_vgraph &VG, istream& i)
{
  name_vclass_map vclass_map;
  string_vector parsed_line;
  int errors=0,line=0;
  int num_nodes = 0;
  char inbuf[1024];
  
  while (!i.eof()) {
    line++;
    i.getline(inbuf,1024);
    parsed_line = split_line(inbuf,' ');
    if (parsed_line.size() == 0) {continue;}

    crope command = parsed_line[0];

    if (command.compare("node") == 0) {
      if (parsed_line.size() < 3) {
	top_error("Bad node line, too few arguments.");
      } else {
	crope name = parsed_line[1];
	crope type = parsed_line[2];
	num_nodes++;
	tb_vnode *v = new tb_vnode();
	vvertex vv = add_vertex(VG);
	vname2vertex[name] = vv;
	virtual_nodes.push_back(vv);
	put(vvertex_pmap,vv,v);
	v->name = name;
	name_vclass_map::iterator dit = vclass_map.find(type);
	if (dit != vclass_map.end()) {
	  v->type="";
	  v->vclass = (*dit).second;
	} else {
	  v->type=type;
	  v->vclass=NULL;
	  if (vtypes.find(v->type) == vtypes.end()) {
	      vtypes[v->type] = 1;
	  } else {
	      vtypes[v->type]++;
	  }
	}
	v->fixed = false;	// this may get set to true later
#ifdef PER_VNODE_TT
	v->num_links = 0;
	v->total_bandwidth = 0;
#endif
	
	for (unsigned int i = 3;i < parsed_line.size();++i) {
	  crope desirename,desireweight;
	  if (split_two(parsed_line[i],':',desirename,desireweight,"0") == 1) {
	    top_error("Bad desire, missing weight.");
	  }
	  double gweight;
	  if (sscanf(desireweight.c_str(),"%lg",&gweight) != 1) {
	    top_error("Bad desire, bad weight.");
	    gweight = 0;
	  }
	  v->desires[desirename] = gweight;
	}
      }
    } else if (command.compare("link") == 0) {
      if (parsed_line.size() < 7) {
	top_error("Bad link line, too few arguments.");
      } else {
	crope name = parsed_line[1];
	crope src = parsed_line[2];
	crope dst = parsed_line[3];
	crope bw,bwunder,bwover;
	crope delay,delayunder,delayover;
	crope loss,lossunder,lossover;
	crope bwweight,delayweight,lossweight;
	string_vector parsed_delay,parsed_bw,parsed_loss;
	parsed_bw = split_line(parsed_line[4],':');
	bw = parsed_bw[0];
	if (parsed_bw.size() == 1) {
	  bwunder = "0";
	  bwover = "0";
	  bwweight = "1";
	} else if (parsed_bw.size() == 3) {
	  bwunder = parsed_bw[1];
	  bwover = parsed_bw[2];
	  bwweight = "1";
	} else if (parsed_bw.size() == 4) {
	  bwunder = parsed_bw[1];
	  bwover = parsed_bw[2];
	  bwweight = parsed_bw[3];
	} else {
	  top_error("Bad link line, bad bandwidth specifier.");
	}
	parsed_delay = split_line(parsed_line[5],':');
	delay = parsed_delay[0];
	if (parsed_delay.size() == 1) {
	  delayunder = "0";
	  delayover = "0";
	  delayweight = "1";
	} else if (parsed_delay.size() == 3) {
	  delayunder = parsed_delay[1];
	  delayover = parsed_delay[2];
	  delayweight = "1";
	} else if (parsed_delay.size() == 4) {
	  delayunder = parsed_delay[1];
	  delayover = parsed_delay[2];
	  delayweight = parsed_delay[3];
	} else {
	  top_error("Bad link line, bad delay specifier.");
	}
	parsed_loss = split_line(parsed_line[6],':');
	loss = parsed_loss[0];
	if (parsed_loss.size() == 1) {
	  lossunder = "0";
	  lossover = "0";
	  lossweight = "1";
	} else if (parsed_loss.size() == 3) {
	  lossunder = parsed_loss[1];
	  lossover = parsed_loss[2];
	  lossweight = "1";
	} else if (parsed_loss.size() == 4) {
	  lossunder = parsed_loss[1];
	  lossover = parsed_loss[2];
	  lossweight = parsed_loss[4];
	} else {
	  top_error("Bad link line, bad loss specifier.");
	}

	vedge e;
	vvertex node1 = vname2vertex[src];
	vvertex node2 = vname2vertex[dst];
	e = add_edge(node1,node2,VG).first;
	tb_vlink *l = new tb_vlink();
	l->src = node1;
	l->dst = node2;
	put(vedge_pmap,e,l);

	if ((sscanf(bw.c_str(),"%d",&(l->delay_info.bandwidth)) != 1) ||
	    (sscanf(bwunder.c_str(),"%d",&(l->delay_info.bw_under)) != 1) ||
	    (sscanf(bwover.c_str(),"%d",&(l->delay_info.bw_over)) != 1) ||
	    (sscanf(bwweight.c_str(),"%lg",&(l->delay_info.bw_weight)) != 1) ||
	    (sscanf(delay.c_str(),"%d",&(l->delay_info.delay)) != 1) ||
	    (sscanf(delayunder.c_str(),"%d",&(l->delay_info.delay_under)) != 1) ||
	    (sscanf(delayover.c_str(),"%d",&(l->delay_info.delay_over)) != 1) ||
	    (sscanf(delayweight.c_str(),"%lg",&(l->delay_info.delay_weight)) != 1) ||
	    (sscanf(loss.c_str(),"%lg",&(l->delay_info.loss)) != 1) ||
	    (sscanf(lossunder.c_str(),"%lg",&(l->delay_info.loss_under)) != 1) ||
	    (sscanf(lossover.c_str(),"%lg",&(l->delay_info.loss_over)) != 1) ||
	    (sscanf(lossweight.c_str(),"%lg",&(l->delay_info.loss_weight)) != 1)) {
	  top_error("Bad line line, bad delay characteristics.");
	}
	l->no_connection = false;
	l->name = name;
	l->allow_delayed = true;
#ifdef ALLOW_TRIVIAL_DEFAULT
	l->allow_trivial = true;
#else
	l->allow_trivial = false;
#endif
	l->emulated = false;
	
	for (unsigned int i = 7;i < parsed_line.size();++i) {
	  if (parsed_line[i].compare("nodelay") == 0) {
	    l->allow_delayed = false;
	  } else if (parsed_line[i].compare("emulated") == 0) {
	    l->emulated = true;
	  } else if (parsed_line[i].compare("trivial_ok") == 0) {
	    l->allow_trivial = true;
	  } else {
	    top_error("bad link line, unknown tag: " <<
		      parsed_line[i] << ".");
	  }
	}
	
#ifdef PER_VNODE_TT
	tb_vnode *vnode1 = get(vvertex_pmap,node1);
	tb_vnode *vnode2 = get(vvertex_pmap,node2);
	if (l->emulated) {
	    vnode1->total_bandwidth += l->delay_info.bandwidth;
	    vnode2->total_bandwidth += l->delay_info.bandwidth;
	} else {
	    vnode1->num_links++;
	    vnode2->num_links++;
	}
#endif
      }
    } else if (command.compare("make-vclass") == 0) {
      if (parsed_line.size() < 4) {
	top_error("Bad vclass line, too few arguments.");
      } else {
	crope name = parsed_line[1];
	crope weight = parsed_line[2];
	double gweight;
	if (sscanf(weight.c_str(),"%lg",&gweight) != 1) {
	  top_error("Bad vclass line, invalid weight.");
	  gweight = 0;
	}
	
	tb_vclass *v = new tb_vclass(name,gweight);
	vclass_map[name] = v;
	for (unsigned int i = 3;i<parsed_line.size();++i) {
	  v->add_type(parsed_line[i]);
	  // XXX - in order not to break vtypes, we don't put them in the
	  // vtypes map (otherwise, the type precheck would complain, but the
	  // top may still be mappable).
	  //vtypes.push_front(parsed_line[i]);
	}
      }
    } else if (command.compare("fix-node") == 0) {
      if (parsed_line.size() != 3) {
	top_error("Bad fix-node line, wrong number of arguments.");
      } else {
	crope virtualnode = parsed_line[1];
	crope physicalnode = parsed_line[2];
	fixed_nodes[virtualnode] = physicalnode;
      }
    } else {
      top_error("Unknown directive: " << command << ".");
    }
  }

  if (errors > 0) {exit(2);}
  
  return num_nodes;
}
