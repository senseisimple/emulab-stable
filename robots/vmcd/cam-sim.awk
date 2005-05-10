#! /usr/bin/awk -f

#
# EMULAB-COPYRIGHT
# Copyright (c) 2005 University of Utah and the Flux Group.
# All rights reserved.
#

# 
# cam-sim.awk - Simulate imaging of a floor grid into an overhead wide-angle camera.
# The default output is rows and then columns of grid points for gnuplot.
# An optional output simulates file_dumper grid points.
# A blank line separates the points along a row or column from the next one.
# 
# Option vars, set with -v var=arg:
#  -v floor=1 - Output floor coordinates for the grid in meters.
#  -v warp=1 - Apply a radial distortion to warp the ccd coords.  
#              Value is the warp factor, 1.0 is a simple cosine.
#  -v mm=1 - Output ccd coords in mm from the optical axis instead of in pixels.
#  -v pts=1 - Output just the grid points in row-major order.
#  -v fd=1 - file_dumper output, grid points in pixels plus world coords in meters.
#
# Commands:  shell			   gnuplot
#  cam-sim.awk -v floor=1 > grid.data	   plot "grid.data" with lines
#  cam-sim.awk > nowarp.data		   plot "nowarp.data" with lines
#  cam-sim.awk -v warp=1 > warp1.data	   plot "warp1.data" with lines
#  cam-sim.awk -v warp=1.414 > warp2.data  plot "warp2.data" with lines

BEGIN {
    # Assumptions: 
    # Floor units are meters, CCD units are pixels or millimeters.
    # The grid origin is under the camera at the center.
    # The CCD origin is at the lower left pixel of the image.

    # Do everything with matching aspect ratios for slight simplicity.
    aspectX = 4; aspectY = 3; aspectRatio = aspectX / aspectY;

    # The floor grid, with the camera above its center point.
    floorX = 3.000; floorY = floorX / aspectRatio; gridStep = 0.250; 
    halfX = floorX / 2; halfY = floorY / 2;
    
    # The optical center of the camera lens in meters above the floor.
    ocHeight = 2.500;

    # The CCD image sensor is 1/3 inch diagonal 640x480 with a 4x3 aspect ratio.
    ccdDiag = 1/3 * 25.4; xPix = 640; yPix = 480;
    diagAngle = atan2(aspectY, aspectX);
    ccdX = ccdDiag * cos(diagAngle); ccdY = ccdDiag * sin(diagAngle);
    if ( !fd ) print "# ccdDiag", ccdDiag, "ccdX", ccdX, "ccdY", ccdY;
    ## ccdDiag 8.46667 ccdX 6.77333 ccdY 5.08

    # The lens is a varifocal wide-angle, with a focal length of 2.8 to 6 mm.
    # Calculate a focal length to give a field-of-view that covers the whole grid.
    focalLength = (ccdX/2) * ocHeight / halfX;
    if ( !fd ) print "# focalLength", focalLength;

    # Put a grid line crossing directly under the camera.
    stepsHalfX = int(halfX/gridStep); stepsHalfY = int(halfY/gridStep);
    ptsX = 2 * stepsHalfX + 1; ptsY = 2 * stepsHalfY + 1; 
    if ( !fd ) print "# rows(ptsY)", ptsY, "cols(ptsX)", ptsX;

    # Visit the grid points.
    if ( !fd ) print "# warp", warp;
    for ( i = 0; i < ptsY; i++ ) { # Rows of the grid.
	for ( j = 0; j < ptsX; j++ )
	    point(i,j);
	print "";		# Blank line separator.
    }
    if ( ! pts && !fd ) for ( j = 0; j < ptsX; j++ ) { # Columns of the grid.
	for ( i = 0; i < ptsY; i++ )
	    point(i,j);
	print "";
    }
}

function point(i,j) {
    gridX = (j - stepsHalfX)*gridStep;
    gridY = (i - stepsHalfY)*gridStep;
      
    if ( floor ) {
	print gridX, gridY;
	return;
    }

    pixXmm = focalLength * gridX / ocHeight;
    pixYmm = focalLength * gridY / ocHeight;

    if ( warp ) {
	# Model barrel lens distortion as the cosine of the off-axis angle.
	f = cos(warp * atan2(sqrt(gridX^2+gridY^2), ocHeight));
	pixXmm = pixXmm * f; 
	pixYmm = pixYmm * f;
    }

    pixX = pixXmm * xPix/ccdX + xPix/2;
    pixY = pixYmm * yPix/ccdY + yPix/2;

    if ( fd ) {			# Simulate file_dumper output.
      printf "\n+++\nsection: (%.3f, %.3f)\n", gridX, gridY;
      frames++;
      printf "frame %d (timestamp 1109888759.935184):\n", frames;
      printf "a(%.6f,%.6f) b(%.6f,%.6f) -- wc(%.6f,%.6f,%.6f)\n+++\n", 
	pixX, pixY+6, pixX, pixY-6, gridX, gridY, 0.0;
    }
    else if ( mm ) 
	print pixXmm, pixYmm;
    else {
	print pixX, pixY;
    }
}
