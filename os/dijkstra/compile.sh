#!/bin/sh

g++33 -ftemplate-depth-30 -O3 -I/home/duerig/include -o dijk Compressor.cc \
    NoneCompressor.cc SingleSource.cc TreeCompressor.cc \
    VoteIpTree.cc bitmath.cc dijkstra.cc OptimalIpTree.cc

g++33 -O3 -o check check-dijkstra.cc bitmath.cc

g++33 -O3 -o route-histogram route-histogram.cc
