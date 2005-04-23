#!/usr/local/bin/python
# 
#   EMULAB-COPYRIGHT
#   Copyright (c) 2005 University of Utah and the Flux Group.
#   All rights reserved.
# 
# test_lin_blend3.py - 9 point concentric version.
# Quick prototype of piecewise triangular linear blending.
#
# The object is to apply error corrections from the center, edge-midpoints,
# and corners of a camera grid to the whole grid.
#
# To use this program, first make a file_dumper input file specifying the
# calibration data at the center, edge midpoints, and corner points in meters.
# The required order is left-to-right, bottom-to-top, so the lower-left corner
# comes first on the bottom row, then the middle and top rows.  Gather these
# points using file_dumper, and analyze them through dump_analyzer.
#
# Gather a finer grid of points to be corrected using file_dumper.  (If the
# corners and origin above are part of the fine grid, you can extract a subset
# of this file_dumper data and analyze it to make the above file.)
#
# Usage: There are two filename args:
#
# ptAnal - The dump_analyzer output file for the center and corner points.
#          The order must be as above.  The section headers give the target
#          world coordinates in meters.  The mean_[ab][xy] give Mezzanine
#          fiducial blob coordinates in pixels, and the mean_[xy]_offset tells
#          the world coordinate error in meters (difference from the target)
#          to be canceled out at the center pixel between the fiducial blobs.
#
# gridData - The file_dumper output file containing data to be corrected.
#          Each data frame contains blob [ab] coordinates and world
#          coordinates.  The blob coordinates are passed through the error
#          correction blend, producing a world coordinate offset which is
#          added to the and new world coordinates in the data frame line,
#          which is then output.  Everything else streams straight through.

import sys
import getopt
import string
import re

import geom
import blend_tris
import read_analysis

opts,args = getopt.getopt(sys.argv[1:], 'td')
printTris = False
debug = False
for o,a in opts:
    if o == "-t":
        printTris = True
        pass
    if o == "-d":
        debug = True
        pass
    pass
if len(args) != 2:
    print "Read the comments for usage."
    pass

# Read in the calibration points.
nPts = 9
loc,target,offset = read_analysis.data(args[0], nPts)

# XXX Horrid Hack Warning - We need right-handed coordinates,
# but the image Y coordinate goes down from 0 at the top.
# Negate it internally.
for l in loc:
    l[1] = l[1] * -1.0

if debug:
    print "loc "+str(loc)
    print "target "+str(target)
    print "offset "+str(offset)

# Make the triangles.
#
# The required point order is left-to-right, bottom-to-top, so the lower-left
# corner comes first on the bottom row, then the middle and top rows.
# Triangles are generated clockwise from the bottom row, first inner and then
# outer.  Vertices are listed clockwise from the center in each triangle, so
# edges will have the inside on the right.
#
#  p6 ------ p7 ----- p8
#   |      / | \      |
#   | t6  /  |  \  t7 |
#   |    /   |   \    |
#   |   /    |    \   |
#   |  /  t2 | t3  \  |
#   | /      |      \ |
#  p3 ------ p4 ----- p5
#   | \      |      / |
#   |  \  t1 | t0  /  |
#   |   \    |    /   |
#   |    \   |   /    |
#   | t5  \  |  /  t4 |
#   |      \ | /      |
#  p0 ------ p1 ----- p2
#
def mkTri(i0, i1, i2):
    return blend_tris.BlendTri((loc[i0], loc[i1], loc[i2]),
                               (target[i0], target[i1], target[i2]),
                               (offset[i0], offset[i1], offset[i2]))

triangles = [ mkTri(4,5,1), mkTri(4,1,3), mkTri(4,3,7), mkTri(4,7,5),
              mkTri(2,1,5), mkTri(0,3,1), mkTri(6,7,3), mkTri(8,5,7) ]

# Optionally output only gnuplot lines for the triangles.
if printTris:                       
    for tri in triangles:
        for v in tri.target:
            print '%f, %f'%tuple(v)
        print "%f, %f\n"%tuple(tri.target[0])
        pass
    sys.exit(0)
    pass

#================================================================

# Regexes for parsing file_dumper output.
fpnum = "\s*(\-*\d+\.\d+)\s*"
reDumperSection = re.compile("section:\s*\("+fpnum+","+fpnum+"\)")
reFrameData_line = re.compile("(\[[0-9]+\] a\("+fpnum+","+fpnum+"\)\s*"
                              "b\("+fpnum+","+fpnum+"\)\s*-- wc)"
                              "\("+fpnum+","+fpnum+","+fpnum+"\)\s*")
gridData = file(args[1])
gridLine = gridData.readline()
##print "gridLine: "+gridLine
while gridLine != "":
    # Chop the newline.
    gridLine = gridLine.strip('\n')
    ##print "gridLine = "+gridLine

    m1 = reFrameData_line.match(gridLine)
    if m1 == None:
        # Everything else streams straight through.
        print gridLine
    else:
        # Frame data.
        lineHead = m1.group(1)
        data = [float(f) for f in m1.groups()[1:]]

        # XXX Horrid Hack Warning - We need right-handed coordinates,
        # but the image Y coordinate goes down from 0 at the top.
        # Negate it internally.
        pixLoc = geom.ptBlend((data[0], -data[1]), (data[2], -data[3]))
        wcLoc = (data[4], data[5])
        wAng = data[6]

        # Find the quadrant by looking at the center edges of the triangles.
        # We can actually blend linearly past the outer edges...
        for iTri in range(nPts-1):

            # Get the barycentric coords of the image point.
            bcs = triangles[iTri].baryCoords(pixLoc)

            # In the "concentric" layout, the first four triangles are inside,
            # so we pay attention to all 3 of their edges.  The second four
            # triangles are on the outside, so two of their edges are outside.
            if iTri <= 3 and bcs[0] >= 0.0 and bcs[1] >= 0.0 and bcs[2] >= 0.0 \
               or iTri >= 4 and bcs[0] >= 0.0:

                # This is the triangle containing this image point.
                newLoc = geom.ptOffset(wcLoc, triangles[iTri].errorBlend(bcs))
                ##print "pixLoc %s, iTri %d, bcs %s"%(pixLoc, iTri, bcs)
                ##print "triangles[%d] %s"%(iTri, triangles[iTri])
                print "%s(%f,%f,%f)"%(lineHead, newLoc[0], newLoc[1], wAng)

                break
            pass
        pass

    gridLine = gridData.readline()
    pass
