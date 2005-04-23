# 
#   EMULAB-COPYRIGHT
#   Copyright (c) 2005 University of Utah and the Flux Group.
#   All rights reserved.
# 

import geom

# BlendTri - Piecewise triangular linear blending using barycentric coordinates.
#
#
#                             Barycentric coordinate of vertex 0 / edge 0.
#          v0  --------- 1.0  100% of v0 data at v0.
#         /^ \         |
#        / |  \        |
#      e1  e1  e2      | 0.5   50% of v0 data halfway down the triangle.
#      /  dist  \      |
#     /    |     \     |
#   v2<--- e0 --- v1 --- 0.0    0% of v0 data on edge e0 from v1 to v2.
#
class BlendTri:

    def __init__(self, verts, target, error): # There are three of everything here.

        # Image coordinates for the three vertices, in pixels, listed clockwise.
        self.verts = verts

        # Target world coordinates of vertices, in meters.
        self.target = target

        # Error vector in meters (difference from the target point to be corrected.)
        self.error = error

        # Construct edge lines from each vertex to its clockwise neighbor.
        self.edges = [geom.Line(verts[(i+1)%3], verts[(i+2)%3]) for i in range(3)]

        # Perpendicular distances from each vertex to the opposing edge line.
        self.dists = [e.signedDist(v) for v,e in zip(self.verts,self.edges)]

        pass

    def __str__(self):
        return 'BlendTri=('+\
               'verts='+str(self.verts)+\
               ', target='+str(self.target)+\
               ', error='+str(self.error)+\
               ', edges=('+string.join([str(l) for l in self.edges], ', ')+')'+\
               ', dists='+str(self.dists)+\
               ')'

    # baryCoords - Compute the three barycentric coordinates of a point
    # relative to the triangle.
    #
    # They vary linearly from 1.0 at a vertex to 0.0 at the edge of the
    # triangle opposite the vertex, and always sum to 1 everywhere.
    #
    # They're good both for finding out whether you are inside a triangle, and
    # for linearly blending values across the triangle.
    #
    def baryCoords(self, pixPt):
        return [e.signedDist(pixPt) / d  # Fraction of dist all the way across.
                for e,d in zip(self.edges, self.dists)]

    # errorBlend - Blend the error vector components linearly over the triangle.
    def errorBlend(self,bcs):           # Arg is baryCoords of an image point.
        sumVec = (0.0, 0.0)
        for bc,errVec in zip(bcs,self.error):
            sumVec = geom.ptOffset(sumVec, errVec, -bc) # Negate the error.
            pass
        return sumVec

    pass # end of BlendTri

def testBlendTri():
    # Notice the negative image Y coords.
    p0,p1,p2 = p = ((50,-50), (100,-100), (0,-100))  # Image verts in pixels.
    t0,t1,t2 = t = ((0,0), (1,-1), (-1,-1))          # Target verts in meters.
    e0,e1,e2 = e = ((.02,.04), (.12,.16), (.20,.24)) # Vertex error vecs in meters.
    tri = BlendTri(p, t, e)
    print 'tri is '+str(tri)

    b = tri.baryCoords((50,-75))
    print "b = tri.baryCoords((50,-75)) is "+str(b)+",\n should be [.5, .25, .25]"
    eb = tri.errorBlend(b)
    print "eb = tri.errorBlend(b) is "+str(eb)+\
          ", \n should be [-.09, -.12] = [.01+.03+.05, .02+.04+.06]"
    pass
