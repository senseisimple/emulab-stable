## GNUPlot commands

set title ''
set terminal postscript eps 20

set output "dur_vs_fin.eps"

set xlabel 'Finish Time (seconds)' font "Times,20"
set ylabel 'Duration (microseconds)' font "Times,20"

#set format x "  %g"
#set grid xtics ytics

set size 1.5,1.0
set key top right

set xrange [0:]
set yrange [0:]

plot 'values.txt' title "Cumulative Lag" with lines
