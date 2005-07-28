// 
//   EMULAB-COPYRIGHT
//   Copyright (c) 2005 University of Utah and the Flux Group.
//   All rights reserved.
// 

#include <stdio.h>
#include "blend_tris.h"

// Initialize a BlendTri object.  It's easier to deposit the verts, target, and error
// vecs into this space, than to pass and copy them, so we assume that has been done.
void initBlendTri(blendTri self)
{
  int i;
  for ( i = 0; i < 3; i++ )
  {
    // Construct edge lines from each vertex to its clockwise neighbor.
    initGeomLine(self->verts[(i+1)%3], self->verts[(i+2)%3], &self->edges[i]);

    // Perpendicular distances from each vertex to the opposing edge line.
    self->dists[i] = signedDist(&self->edges[i], self->verts[i]);
  }
}

// Debug print function.
void prBlendTri(blendTri self)
{
  int i;

  printf("BlendTri=(\n");

  printf("  verts=(");
  for ( i = 0; i < 3; i++ )
    printf("(%g,%g)%s", self->verts[i][0], self->verts[i][1], i<2 ? ", " : "),\n");

  printf("  target=(");
  for ( i = 0; i < 3; i++ )
    printf("(%g,%g)%s", self->target[i][0], self->target[i][1], i<2 ? ", " : "),\n");

  printf("  error=(");
  for ( i = 0; i < 3; i++ )
    printf("(%g,%g)%s", self->error[i][0], self->error[i][1], i<2 ? ", " : "),\n");

  printf("  edges=(\n");
  for ( i = 0; i < 3; i++ )
  {
    printf("    ");
    prGeomLine(&self->edges[i]);
  }
  printf("  ),\n");

  printf("  dists=(%g,%g,%g)\n", self->dists[0], self->dists[1], self->dists[2]);

  printf(")\n");
}

// baryCoords - Compute the three barycentric coordinates of a point
// relative to the triangle.
//
// They vary linearly from 1.0 at a vertex to 0.0 at the edge of the
// triangle opposite the vertex, and always sum to 1 everywhere.
//
// They're good both for finding out whether you are inside a triangle, and
// for linearly blending values across the triangle.
//
void baryCoords(blendTri self, geomPt pixPt, double bcs[3])
{
  int i;
  for ( i = 0; i < 3; i++ )
    // Fraction of the distance all the way across.
    bcs[i] = signedDist(&self->edges[i], pixPt) / self->dists[i];
}

// errorBlend - Blend the error vector components linearly over the triangle.
// bcs arg is baryCoords of an image point.
void errorBlend(blendTri self, double bcs[3], geomVec result)
{
  int i;

  result[0] = result[1] = 0.0;	// Accumulate component vectors.
  for ( i = 0; i < 3; i++ )
    ptOffset(result, self->error[i], -bcs[i], result); // Negate error to cancel.
}
