// 
//   EMULAB-COPYRIGHT
//   Copyright (c) 2005 University of Utah and the Flux Group.
//   All rights reserved.
// 

#ifndef BLENDTRIS_H
#define BLENDTRIS_H

#include <stdio.h>
#include "geom.h"

// blendTri - Piecewise triangular linear blending using barycentric coordinates.
//
//                             Barycentric coordinate of vertex 0 / edge 0.
//          v0  --------- 1.0  100% of v0 data at v0.
//         /^ \         |
//        / |  \        |
//      e1  e1  e2      | 0.5   50% of v0 data halfway down the triangle.
//      /  dist  \      |
//     /    |     \     |
//   v2<--- e0 --- v1 --- 0.0    0% of v0 data on edge e0 from v1 to v2.
//
struct blendTriStruct  // There are three of everything here.
{
  // Image coordinates for the three vertices, in pixels, listed clockwise.
  geomPt verts[3];

  // Target world coordinates of vertices, in meters.
  geomPt target[3];

  // Error vectors in meters (difference from the target point to be corrected.)
  geomVec error[3];

  // Construct edge lines from each vertex to its clockwise neighbor.
  geomLineObj edges[3];

  // Perpendicular distances from each vertex to the opposing edge line.
  double dists[3];
};
typedef struct blendTriStruct blendTriObj;
typedef blendTriObj *blendTri;

// Initialize a BlendTri object.  It's easier to deposit the verts, target, and error
// vecs into this space, than to pass and copy them, so we assume that has been done.
void initBlendTri(blendTri self);

// Debug print function.
void prBlendTri(blendTri self);

// baryCoords - Compute the three barycentric coordinates of a point
// relative to the triangle.
//
// They vary linearly from 1.0 at a vertex to 0.0 at the edge of the
// triangle opposite the vertex, and always sum to 1 everywhere.
//
// They're good both for finding out whether you are inside a triangle, and
// for linearly blending values across the triangle.
//
void baryCoords(blendTri self, geomPt pixPt, double bcs[3]);

// errorBlend - Blend the error vector components linearly over the triangle.
// bcs arg is baryCoords of an image point.
void errorBlend(blendTri self, double bcs[3], geomVec result);

#endif
