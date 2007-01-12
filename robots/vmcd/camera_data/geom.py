# 
#   EMULAB-COPYRIGHT
#   Copyright (c) 2005 University of Utah and the Flux Group.
#   All rights reserved.
# 
# geom.py - Some basic linear algebra of points, vectors, and lines.
# Points and vectors are lists or tuples of numbers.

import math

# vecFrom2Pts - Subtract two points for vec from the first to the second.
def vecFrom2Pts(p1, p2):                # Any dimension.
    return [j-i for i,j in zip(p1,p2)]

# vecLength - Length of a vector.  (math.hypot does just the 2-D case.)
def vecLength(v):                       # Any dimension.
    return math.sqrt(sum([i*i for i in v]))

# vecNormalize - Returns a unit vector in the same direction.
def vecNormalize(v):                       # Any dimension.
    norm = vecLength(v)
    return [i/norm for i in v]

# vecScale - Multiply the length.
def vecScale(v, scale):                 # Any dimension.
    return [i*scale for i in v]

# ptOffset - Offset a point by (a multiple of) a vector.
def ptOffset(p, v, scale=1.0):          # Any dimension.
    return [i + scale*j for i,j in zip(p,v)]

# ptBlend - Blend between two points, 0.0 for first, 1.0 for the second.
def ptBlend(p1, p2, t=0.5):             # Any dimension.
    return ptOffset(p1, vecFrom2Pts(p1, p2), t)

# Line - Line equation is Bx-Ay+C, == 0 on the line, positive inside.
# [A,B] is a vector perpendicular to the line, positive to the right.
# Notice that this is not the Ax+By+C form you learned in Algebra class.
# It makes for efficient calculation of the distance from a point to a line.
class Line:

    # Construct a line through two points.
    def __init__(self, pt1, pt2):
        self.pt1 = pt1
        self.pt2 = pt2

        # (A,B) is perpendicular vec to right side of line, (deltaY, -deltaX).
        self.A, self.B = vecNormalize((pt2[1]-pt1[1], pt1[0]-pt2[0]))

        # C is the negative of the distance of the line from the origin.
        self.C = 0.0                    # Small chicken-and-egg problem...
        self.C = -self.signedDist(pt1)

        pass

    def __str__(self):                  # Called by str().
        return 'Line=('+\
               'pt1='+str(self.pt1)+\
               ', pt2='+str(self.pt2)+\
               ', A='+str(self.A)+\
               ', B='+str(self.B)+\
               ', C='+str(self.C)+\
               ')'

    # signedDist - Signed distance, positive inside (right side) of the line.
    def signedDist(self, pt):
        return self.A*pt[0] + self.B*pt[1] + self.C   # [x y 1] . [A B C]

    pass

def testLine():
    l=Line((-1,0),(0,1))
    print "l is "+str(l)
    print "l.signedDist((0,0)) is "+str(l.signedDist((0,0)))+", should be .707"
    print "l.signedDist((1,1)) is "+str(l.signedDist((1,1)))+", should be .707"
    print "l.signedDist((-1,1)) is "+str(l.signedDist((-1,1)))+", should be -.707"
    pass
