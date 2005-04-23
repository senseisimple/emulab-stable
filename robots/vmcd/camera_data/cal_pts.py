#!/usr/local/bin/python
# 
#   EMULAB-COPYRIGHT
#   Copyright (c) 2005 University of Utah and the Flux Group.
#   All rights reserved.
# 
# cal_pts.py - Convert dump analysis data into mezzanine.opt calibration pts.
#
# To use this program, first make a file_dumper input file specifying the
# calibration data at the center, edge midpoints, and corner points in meters.
# The required order is left-to-right, bottom-to-top, so the lower-left corner
# comes first on the bottom row, then the middle and top rows.  Gather these
# points using file_dumper, and analyze them through dump_analyzer.
#
# Usage: There are is one filename arg:
#
# ptAnal - The dump_analyzer output file for the center, edge, and corner pts.
#          The order must be as above.  The section headers give the target
#          world coordinates in meters.  The mean_[ab][xy] give Mezzanine
#          fiducial blob coordinates in pixels, and the mean_[xy]_offset tells
#          the world coordinate error in meters (difference from the target)
#          to be canceled out at the center pixel between the fiducial blobs.

import sys
import getopt
import string
import re

import geom
import blend_tris
import read_analysis

opts,args = getopt.getopt(sys.argv[1:], 'd')
debug = False
for o,a in opts:
    if o == "-d":
        debug = True
        pass
    pass
if len(args) != 1:
    print "Read the comments for usage."
    pass

# Read in the calibration points.
nPts = 9
loc,target,offset = read_analysis.data(args[0], nPts)
if debug:
    print "loc "+str(loc)
    print "target "+str(target)

# Print out the dewarp points section of the mezzanine.opt file.
for i in range(len(target)):
    print "dewarp.wpos[%d] = (%.3f, %.3f)"%(i, target[i][0], target[i][1])
    ##print "dewarp.epos[%d] = (%.3f, %.3f)"%(i, offset[i][0], offset[i][1])
    print "dewarp.ipos[%d] = (%d, %d)"%(i, loc[i][0]+.5, loc[i][1]+.5)
    print ""
    pass
