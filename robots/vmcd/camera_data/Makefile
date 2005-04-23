#
# EMULAB-COPYRIGHT
# Copyright (c) 2005 University of Utah and the Flux Group.
# All rights reserved.
#

# Makefile for robots/vmcd/camera_data.
# See README.txt for directory contents.

SRCDIR = ..
analyze := $(SRCDIR)/dump_analyzer.py
w_plot := $(SRCDIR)/analysis_w_plot.awk
PARAMDIR = ../etc

default: plots

dollar = $
show: plots
	gv all_$(mag).eps & sleep 1
	for f in $(cam_plt); do gv $(value dollar)f & sleep 1; done
show-blend8: blend8
	gv all_blend8_$(mag).eps & sleep 1
	for f in $(cam_blp); do gv $(value dollar)f & sleep 1; done

clean:
	rm -f analysis_* *.error_lines *.grid_points *.tris_output *.eps \
	  *.pyc *_blend8 *_calpts

# Plot magnification factor for error_lines files.
magval := 50
##magval := 1
mag := mag$(magval)

# Camera numbers.
cams := 0 1 2 3 6 7
cam_out := $(foreach cam, $(cams), output_camera$(cam) )
cam_anl := $(foreach cam, $(cams), analysis_camera$(cam) )
analysis: analysis_all $(cam_anl)

cam_plt := $(foreach cam, $(cams), camera$(cam)_$(mag).eps )
plots: all_$(mag).eps $(cam_plt)

cam_anl := $(foreach cam, $(cams), analysis_camera$(cam)_blend8 )
cam_blo := $(foreach cam, $(cams), output_camera$(cam)_blend8 )
cam_blp := $(foreach cam, $(cams), camera$(cam)_blend8_$(mag).eps )
blend8: all_blend8_$(mag).eps $(cam_anl) $(cam_blp)

# Per-camera analysis.
analysis_camera%: output_camera%
	$(analyze) $^ > $@

# Cat the output files together in world coords for an overall analysis.
analysis_all: $(cam_out)
	$(analyze) -w $^ > $@
analysis_all_blend8: $(cam_blo)
	$(analyze) -w $^ > $@

# Detailed, but optional, control of plot bounds.  
# The Y axis is reversed in world coords.  (Grumble.)
all_bounds := [3:14] [15:5]
all_blend8_bounds := [3:14] [15:5]

# Generate plots through gnuplot.  
# "w_plot -v file=base." makes base.grid_points and base.error_lines .
%_$(mag).eps: analysis_%
	$(w_plot) -v file=$(@:eps=) -v mag=$(magval) < $<
	echo 'set output "$@"; \
	      set terminal postscript eps color; \
	      plot $(value $(subst _$(mag).eps,,$@)_bounds) \
		"$(@:eps=grid_points)" with points pt 7 ps 1, \
		"$(@:eps=error_lines)" with lines lt 1 lw 3' | gnuplot

# Extract a subset of ground truth as calibration points for error blending.
output_camera%_calpts: input_camera%_cal
	./subset_fd_output.awk -v subset=$< $(@:_calpts=) > $@

# Convert calibration points to mezz opts for the C error blending algorithm.
cam_dwp := $(foreach cam, $(cams), camera$(cam)_calpts)
calpts: $(cam_dwp)
camera%_calpts: analysis_camera%_calpts
	./cal_pts.py $< > $@

# Apply the Python error blender to generate and show the blending triangles.
camera%_blend8.tri_lines: analysis_camera%_calpts output_camera%
	./test_lin_blend2.py -t $^ > $@

# Apply the Python error blender to error-cancel the grid data.
output_camera%_blend8: analysis_camera%_calpts output_camera%
	./test_lin_blend2.py $^ > $@
