/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

// geom.c - Some basic linear algebra of points, vectors, and lines.

#include <stdio.h>
#include "geom.h"

// vecFrom2Pts - Subtract two points for vec from the first to the second.
void vecFrom2Pts(geomPt p1, geomPt p2, geomVec result)
{
  result[0] = p1[0] - p2[0];
  result[1] = p1[1] - p2[1];
}

// vecLength - Length of a vector.
double vecLength(geomVec v)
{
  return hypot(v[0], v[1]);
}

// vecNormalize - Returns a unit vector in the same direction.
// Result can be the same vector as the vector argument.
void vecNormalize(geomVec v, geomVec result)
{
  double norm = vecLength(v);
  result[0] = v[0]/norm;
  result[1] = v[1]/norm;
}

// vecScale - Multiply the length.
// Result can be the same vector as the vector argument.
void vecScale(geomVec v, double scale, geomVec result)
{
  result[0] = v[0] * scale;
  result[1] = v[1] * scale;
}

// ptOffset - Offset a point by (a multiple of) a vector.
// Result can be the same point as the point argument.
void ptOffset(geomPt p, geomVec v, double scale, geomPt result)
{
  result[0] = p[0] + v[0] * scale;
  result[1] = p[1] + v[1] * scale;
}

// ptBlend - Blend between two points, 0.0 for first, 1.0 for the second.
// Result can be the same point as either of the two point arguments.b
void ptBlend(geomPt p1, geomPt p2, double t, geomPt result)
{
  geomVec offset;
  vecFrom2Pts(p1, p2, offset);
  ptOffset(p1, offset, t, result);
}

// Construct a line through two points.
void initGeomLine(geomPt pt1, geomPt pt2, geomLine self)
{
  geomVec perp;

  geomCopy(pt1, self->pt1);
  geomCopy(pt2, self->pt2);

  // (A,B) is the vector perpendicular to right side of line, (deltaY, -deltaX).
  perp[0] = pt2[1]-pt1[1];
  perp[1] = pt1[0]-pt2[0];
  vecNormalize(perp, perp);
  self->A = perp[0];
  self->B = perp[1];

  // C is the negative of the distance of the line from the origin.
  self->C = 0.0;                    // Small chicken-and-egg problem...
  self->C = -signedDist(self,pt1);
}

// Debug print function.
void prGeomLine(geomLine self)
{
  printf("Line=(pt1=(%g,%g), pt2=(%g,%g), A=%g, B=%g, C=%g)\n",
         self->pt1[0], self->pt1[1], self->pt2[0], self->pt2[1],
         self->A, self->B, self->C );
}

// signedDist - Signed distance, positive inside (right side) of the line.
double signedDist(geomLine self, geomPt pt)
{
  return self->A*pt[0] + self->B*pt[1] + self->C;   // [x y 1] . [A B C]
}
