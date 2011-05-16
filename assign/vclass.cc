/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

static const char rcsid[] = "$Id: vclass.cc,v 1.13 2009-05-20 18:06:08 tarunp Exp $";

#include "port.h"

#include <stdlib.h>

#include <boost/config.hpp>
#include <boost/utility.hpp>
#include BOOST_PMAP_HEADER
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>

using namespace boost;

#include "common.h"
#include "vclass.h"
#include "delay.h"
#include "physical.h"
#include "virtual.h"

name_vclass_map vclass_map;

void tb_vclass::add_type(fstring type)
{
  members[type]=0;
  if (dominant.empty() ||
      (members[dominant] == 0)) {
    dominant = type;
  }
}

bool tb_vclass::has_type(fstring type) const {
    return (members.find(type) != members.end());
}


double tb_vclass::assign_node(fstring type)
{
  double new_score = score;
  members[type] += 1;

  if (type != dominant) {
    if (members[dominant] != 0) {
      new_score = weight;
    }
    if (members[type] > members[dominant]) {
      dominant = type;
    }
  }

  double delta = new_score-score;
  score=new_score;
  return delta;
}

double tb_vclass::unassign_node(fstring type)
{
  double new_score = 0;
  members[type] -= 1;

  int curmax = 0;
  for (members_map::iterator dit=members.begin();
       dit != members.end();++dit) {
    int n = (*dit).second;
    if (n > curmax) {
      if (curmax != 0) {
	new_score = weight;
      }
      curmax = n;
      dominant = (*dit).first;
    }
  }
  
  double delta = new_score-score;
  score=new_score;
  return delta;
}

fstring tb_vclass::choose_type() const
{
  // This may take some tweaking - i.e. might want to make more
  // efficient, although members is usually a very small hash.
  if (RANDOM()%2 == 0) {
    return dominant;
  }
  int r = RANDOM()%members.size();
  members_map::const_iterator dit;
  for (dit=members.begin();dit != members.end();++dit) {
    if (r == 0) break;
    r--;
  }
  return (*dit).first;
}

bool tb_vclass::empty() const {
    members_map::const_iterator dit;
    for (dit=members.begin();dit != members.end();++dit) {
        if ((*dit).second > 0) {
            return false;
        }
    }
    return true;
}
