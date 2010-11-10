/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

static const char rcsid[] = "$Id: parse_top.cc,v 1.42 2009-05-20 18:06:08 tarunp Exp $";

#include "port.h"

#include <boost/config.hpp>
#include <boost/utility.hpp>
#include <boost/property_map.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>

#include <iostream>

#include <stdio.h>

using namespace boost;

#include "common.h"
#include "vclass.h"
#include "delay.h"
#include "physical.h"
#include "virtual.h"
#include "parser.h"
#include "anneal.h"
#include "string.h"

extern name_vvertex_map vname2vertex;
extern name_name_map fixed_nodes;
extern name_name_map node_hints;
extern name_count_map vtypes;
extern name_list_map vclasses;
extern vvertex_vector virtual_nodes;

#define top_error(s) errors++;cout << "TOP:" << line << ": " << s << endl
#define top_error_noline(s) errors++;cout << "TOP: " << s << endl

// Used to do late binding of subnode names to vnodes, so that we're no
// dependant on their ordering in the top file, which can be annoying to get
// right.
// Returns the number of errors found
int bind_top_subnodes(tb_vgraph &vg) {
    int errors = 0;

    // Iterate through all vnodes looking for ones that are subnodes
    vvertex_iterator vit,vendit;
    tie(vit,vendit) = vertices(vg);
    for (;vit != vendit;++vit) {
	tb_vnode *vnode = get(vvertex_pmap, *vit);
	if (!vnode->subnode_of_name.empty()) {
	    if (vname2vertex.find(vnode->subnode_of_name)
		    == vname2vertex.end()) {
		top_error_noline(vnode->name << " is a subnode of a " <<
			"non-existent node, " << vnode->subnode_of_name << ".");
		continue;
	    }
	    vvertex parent_vv = vname2vertex[vnode->subnode_of_name];
	    vnode->subnode_of = get(vvertex_pmap,parent_vv);
	    vnode->subnode_of->subnodes.push_back(vnode);
	}
    }

    return errors;
}

extern name_vclass_map vclass_map;

int parse_top(tb_vgraph &vg, istream& input)
{
  string_vector parsed_line;
  int errors=0,line=0;
  int num_nodes = 0;
  char inbuf[1024];
  
  while (!input.eof()) {
    line++;
    input.getline(inbuf,1024);
    parsed_line = split_line(inbuf,' ');
    if (parsed_line.size() == 0) {continue;}

    string command = parsed_line[0];

    if (command == string("node")) {
      if (parsed_line.size() < 3) {
	top_error("Bad node line, too few arguments.");
      } else {
	string name = parsed_line[1];
	string unparsed_type = parsed_line[2];

	// Type might now a have a 'number of slots' assoicated with it
	string type;
	string typecount_str;
	split_two(unparsed_type,':',type,typecount_str,"1");

	int typecount;
	if (sscanf(typecount_str.c_str(),"%i",&typecount) != 1) {
	    top_error("Bad type slot count.");
	    typecount = 1;
	}

	num_nodes++;
        tb_vclass *vclass;
	
	name_vclass_map::iterator dit = vclass_map.find(type);
	if (dit != vclass_map.end()) {
	  type = "";
	  vclass = (*dit).second;
	} else {
	  vclass = NULL;
	  if (vtypes.find(type) == vtypes.end()) {
	      vtypes[type] = typecount;
	  } else {
	      vtypes[type] += typecount;
	  }
	}

	tb_vnode *v = new tb_vnode(name,type,typecount);
	v->vclass = vclass;
	vvertex vv = add_vertex(vg);
	vname2vertex[name] = vv;
	virtual_nodes.push_back(vv);
	put(vvertex_pmap,vv,v);
	
	for (unsigned int i = 3;i < parsed_line.size();++i) {
	  string desirename,desireweight;
	  if (split_two(parsed_line[i],':',desirename,desireweight,"0") == 1) {
	      // It must be a flag?
	      if (parsed_line[i] == string("disallow_trivial_mix")) {
		  v->disallow_trivial_mix = true;
	      } else {
		  top_error("Unknown flag or bad desire (missing weight)");
	      }
	  } else {
	      if (desirename == string("subnode_of")) {
		  // Okay, it's not a desire, it's a subnode declaration
		  if (!v->subnode_of_name.empty()) {
		      top_error("Can only be a subnode of one node");
		      continue;
		  }
		  v->subnode_of_name = desireweight;

	      } else {
		  double gweight;
		  if (sscanf(desireweight.c_str(),"%lg",&gweight) != 1) {
		      top_error("Bad desire, bad weight.");
		      gweight = 0;
		  }
		  tb_node_featuredesire node_fd(desirename, gweight);
		  node_fd.add_desire_user(gweight);
		  v->desires.push_front(node_fd);
	      }
	  }
	}
	v->desires.sort();
      }
    } else if (command == string("link")) {
      if (parsed_line.size() < 8) {
	top_error("Bad link line, too few arguments.");
      } else {
	string name = parsed_line[1];
	string src = parsed_line[2];
	string dst = parsed_line[3];
	string link_type = parsed_line[7];
	string bw,bwunder,bwover;
	string delay,delayunder,delayover;
	string loss,lossunder,lossover;
	string bwweight,delayweight,lossweight;
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
	// Check to make sure the nodes in the link actually exist
	if (vname2vertex.find(src) == vname2vertex.end()) {
	  top_error("Bad link line, non-existent node.");
	  continue;
	}
	if (vname2vertex.find(dst) == vname2vertex.end()) {
	  top_error("Bad link line, non-existent node.");
	  continue;
	}

	vvertex node1 = vname2vertex[src];
	vvertex node2 = vname2vertex[dst];
	e = add_edge(node1,node2,vg).first;
	tb_vlink *l = new tb_vlink();
	l->src = node1;
	l->dst = node2;
	l->type = link_type;
	put(vedge_pmap,e,l);

        // Special flag: treat a bandwidth of '*' specially
        if (!strcmp(bw.c_str(),"*")) {
            l->delay_info.bandwidth = -2; // Special flag
            l->delay_info.adjust_to_native_bandwidth = true;
        } else {
            if (sscanf(bw.c_str(),"%d",&(l->delay_info.bandwidth)) != 1) {
                top_error("Bad line line, bad bandwidth characteristics.");
            }
        }

        // Scan in the rest of the delay_info structure
	if ((sscanf(bwunder.c_str(),"%d",&(l->delay_info.bw_under)) != 1) ||
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
	l->no_connection = true;
	l->name = name;
	l->allow_delayed = true;
#ifdef ALLOW_TRIVIAL_DEFAULT
	l->allow_trivial = true;
#else
	l->allow_trivial = false;
#endif
	l->emulated = false;
	l->fix_src_iface = false;
	l->fix_dst_iface = false;
		
	for (unsigned int i = 8;i < parsed_line.size();++i) {
	  string stag, svalue;
	  split_two(parsed_line[i],':',stag,svalue,"");
	  if (parsed_line[i] == string("nodelay")) {
	    l->allow_delayed = false;
	  } else if (parsed_line[i] == string("emulated")) {
	    l->emulated = true;
	  } else if (parsed_line[i] == string("trivial_ok")) {
	    l->allow_trivial = true;
	  } else if (stag == string("fixsrciface")) {
            l->fix_src_iface = true;
	    l->src_iface = svalue;
    	  } else if (stag == string("fixdstiface")) {
            l->fix_dst_iface = true;
	    l->dst_iface = svalue;
	  } else {
	    top_error("bad link line, unknown tag: " <<
		      parsed_line[i] << ".");
	  }
	}

	tb_vnode *vnode1 = get(vvertex_pmap,node1);
	tb_vnode *vnode2 = get(vvertex_pmap,node2);
#ifdef PER_VNODE_TT
	if (l->emulated) {
	    if (!l->allow_trivial) {
		vnode1->total_bandwidth += l->delay_info.bandwidth;
		vnode2->total_bandwidth += l->delay_info.bandwidth;
	    }
	} else {
	    vnode1->num_links++;
	    vnode2->num_links++;
	    vnode1->link_counts[link_type]++;
	    vnode2->link_counts[link_type]++;
	}
#endif
        
        // Some sanity checks: this combination is illegal for now
        if (l->delay_info.adjust_to_native_bandwidth && (l->allow_trivial ||
                    l->emulated)) {
            top_error("Auto-assigning bandwidth on trivial or emulated links"
                      " not allowed!");
        }
	
      }
    } else if (command == string("make-vclass")) {
      if (parsed_line.size() < 4) {
	top_error("Bad vclass line, too few arguments.");
      } else {
	string name = parsed_line[1];
	string weight = parsed_line[2];
	double gweight;
	if (sscanf(weight.c_str(),"%lg",&gweight) != 1) {
	  top_error("Bad vclass line, invalid weight.");
	  gweight = 0;
	}
	
	tb_vclass *v = new tb_vclass(name,gweight);
	vclass_map[name] = v;
	for (unsigned int i = 3;i<parsed_line.size();++i) {
	  v->add_type(parsed_line[i]);
	  vclasses[name].push_back(parsed_line[i]);
	}
      }
    } else if (command == string("fix-node")) {
      if (parsed_line.size() != 3) {
	top_error("Bad fix-node line, wrong number of arguments.");
      } else {
	string virtualnode = parsed_line[1];
	string physicalnode = parsed_line[2];
	fixed_nodes[virtualnode] = physicalnode;
      }
    } else if (command == string("node-hint")) {
      if (parsed_line.size() != 3) {
	top_error("Bad node-hint line, wrong number of arguments.");
      } else {
	string virtualnode = parsed_line[1];
	string physicalnode = parsed_line[2];
	node_hints[virtualnode] = physicalnode;
      }
    } else {
      top_error("Unknown directive: " << command << ".");
    }
  }

  errors += bind_top_subnodes(vg);

  if (errors > 0) {exit(EXIT_FATAL);}
  
  return num_nodes;
}
