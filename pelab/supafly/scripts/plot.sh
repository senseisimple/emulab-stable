#!/bin/sh

#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

## GNUPlot commands
gnuplot << EOF

set title ''
set terminal postscript eps 20

set output "$1.eps"

set xlabel 'Timestamps as diffs (seconds)' font "Times,20"
set ylabel 'Duration (seconds)' font "Times,20"

#set format x "  %g"
#set grid xtics ytics

set size 1.5,1.0
set key top right

#set xrange [$2:$3]
#set yrange [$4:$5]

#set xrange [0:]
#set yrange [0:]

set xrange [0:30]
set yrange [0:1.2]

plot "$1.dat" title "$1 Lag" with linespoints pt 3
EOF
