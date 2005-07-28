/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

// test_geom.c - Light testing of the geom.[hc] code.

#include <stdio.h>
#include "geom.h"

int main(int argc, char** argv)
{
  geomLineObj lo, *l = &lo;
  geomPt p1 = {-1,0}, p2 = {0,1};
  geomPt p3 = {0,0}, p4 = {1,1}, p5 = {-1,1};

  initGeomLine(p1, p2, l);
  printf("l is "); prGeomLine(l);
  printf("signedDist(l, (0,0)) is %g, should be .707\n", signedDist(l, p3));
  printf("signedDist(l, (1,1)) is %g, should be .707\n", signedDist(l, p4));
  printf("signedDist(l, (-1,1)) is %g, should be -.707\n", signedDist(l, p5));

  return 0;
}
