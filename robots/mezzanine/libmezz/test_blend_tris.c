/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

// test_blend_tris.c - Light testing of the blend_tris.[hc] code.

#include <stdio.h>
#include "blend_tris.h"

int main(int argc, char** argv)
{
    // Notice the negative image Y coords.
    geomPt p[3] = {{50,-50}, {100,-100}, {0,-100}};   // Image verts in pixels.
    geomPt t[3] = {{0,0}, {1,-1}, {-1,-1}};	      // Target verts in meters.
    geomVec e[3] = {{.02,.04}, {.12,.16}, {.20,.24}}; // Vertex error vecs in meters.

    blendTriObj to, *tri = &to;
    geomPt bpt = {50,-75};
    double b[3];
    geomVec eb;

    geomCopy(p, tri->verts);
    geomCopy(t, tri->target);
    geomCopy(e, tri->error);
    initBlendTri(tri);

    printf("tri is "); prBlendTri(tri);

    baryCoords(tri, bpt, b);
    printf("b = baryCoords(tri, (50,-75)) is [%g,%g,%g],\n%s\n", b[0], b[1], b[2],
	   "  should be [.5, .25, .25]");

    errorBlend(tri, b, eb);
    printf("eb = errorBlend(tri, b) is [%g,%g],\n%s\n", eb[0], eb[1],
	   "  should be [-.09, -.12] = [.01+.03+.05, .02+.04+.06]");

    return 0;
}

