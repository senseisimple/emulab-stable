#!/bin/sh

# UdpClient saves the throughput calculated after receiving each packet.

# This data is dumped in Throughput.log - it is converted to a data file for GNUplot
# using a python script makeGnuPlot.py

# The GNUplot file plot.gp displays the graph from data values in Output.log

# NOTE: If you want to run the client multiple times, save the Output.log with 
# another name, because it will be overwritten every time this shell script is run.

python makeGnuPlot.py Throughput.log Output.log
gnuplot -persist plot.gp
