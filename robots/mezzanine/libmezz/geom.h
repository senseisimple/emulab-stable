/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

// geom.h - Some basic linear algebra of points, vectors, and lines.

#ifndef GEOM_H
#define GEOM_H

#include <math.h>
#include <string.h>

// Restricted to 2D - Points and vectors are arrays of 2 doubles.
// No dimensionality hierarchy, no operand checking, no homogeneous coordinates.
// Arrays are passed by reference in C - turned to pointers to their first element.
typedef double geomVec[2];
typedef double geomPt[2];
#define geomCopy(src,dst) bcopy(src, dst, sizeof dst);

// vecFrom2Pts - Subtract two points for vec from the first to the second.
void vecFrom2Pts(geomPt p1, geomPt p2, geomVec result);

// vecLength - Length of a vector.
double vecLength(geomVec v);

// vecNormalize - Returns a unit vector in the same direction.
// Result can be the same vector as the vector argument.
void vecNormalize(geomVec v, geomVec result);

// vecScale - Multiply the length.
// Result can be the same vector as the vector argument.
void vecScale(geomVec v, double scale, geomVec result);

// ptOffset - Offset a point by (a multiple of) a vector.
// Result can be the same point as the point argument.
void ptOffset(geomPt p, geomVec v, double scale, geomPt result);

// ptBlend - Blend between two points, 0.0 for first, 1.0 for the second.
// Result can be the same point as either of the two point arguments.b
void ptBlend(geomPt p1, geomPt p2, double t, geomPt result);

// Line - Line equation is Ax+By+C, == 0 on the line, positive inside.
struct geomLineStruct
{
  geomPt pt1, pt2;
  double A, B, C;
};
typedef struct geomLineStruct geomLineObj;
typedef geomLineObj *geomLine;

// Construct a line through two points.
void initGeomLine(geomPt pt1, geomPt pt2, geomLine self);

// Debug print function.
void prGeomLine(geomLine self);

// signedDist - Signed distance, positive inside (right side) of the line.
double signedDist(geomLine self, geomPt pt);

#endif
