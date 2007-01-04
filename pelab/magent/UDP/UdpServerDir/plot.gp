set xlabel "Time( micro sec )"
set ylabel "Bandwidth( Kilo bits per sec )"

plot [0:10000000] [0:20000] 'ServerOutput.log' with lines

