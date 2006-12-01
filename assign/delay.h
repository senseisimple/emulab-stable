/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2002-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef __DELAY_H
#define __DELAY_H

// For DBL_MAX
#include <float.h>
// For fabs()
#include <math.h>

#include <iostream>
using namespace std;

inline double double_distance(double a,double b) {
  if (b == 0) {
    if (a == 0)
      return 0;
    else
      return DBL_MAX;
  }
  return fabs((double)a/(double)b - 1.0);
}
inline double delay_distance(int a, int b) {
  return double_distance((double)a,(double)b);
}
inline double bandwidth_distance(int a,int b) {
  return double_distance((double)a,(double)b);
}
inline double loss_distance(double a,double b) {
  return double_distance(a,b);
}

class tb_delay_info {
public:
  int bandwidth;
  int delay;
  double loss;
  int bw_under,bw_over;
  int delay_under,delay_over;
  double loss_under,loss_over;
  double bw_weight,delay_weight,loss_weight;

  double distance(tb_delay_info &target) {
      // ricci - hack to try to remove this behavior
      return 0;
    if (((bw_under != -1) && (target.bandwidth < bandwidth-bw_under)) ||
	((bw_over != -1) && (target.bandwidth > bandwidth+bw_over)) ||
	((delay_under != -1) && (target.delay < delay-delay_under)) ||
	((delay_over != -1) && (target.delay > delay+delay_over)) ||
	((loss_under != -1) && (target.loss < loss-loss_under)) ||
	((loss_over != -1) && (target.loss > loss+loss_over))) {
      return -1;
    }
    return bandwidth_distance(target.bandwidth,bandwidth)*bw_weight+
      delay_distance(target.delay,delay)*delay_weight+
      loss_distance(target.loss,loss)*loss_weight;
  }
  
  friend ostream &operator<<(ostream &o, const tb_delay_info& delay)
  {
    o << "tb_delay_info: bw=" << delay.bandwidth << "+" <<
      delay.bw_over << "-" << delay.bw_under << "/" << delay.bw_weight;
    o << " delay=" << delay.delay << "+" << delay.delay_over <<
      "-" << delay.delay_under << "/" << delay.delay_weight;
    o << " loss=" << delay.loss << "+" << delay.loss_over << "-" <<
      delay.loss_under << "/" << delay.loss_weight;
    o << endl;
    return o;
  }
};

#endif

