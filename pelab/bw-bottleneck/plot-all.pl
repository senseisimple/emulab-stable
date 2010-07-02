$limit = @ARGV;
for ($i = 0; $i < $limit; ++$i)
{
    $file = $ARGV[$i];
    $file =~ /([0-9a-zA-Z\-]+)(\.[0-9a-zA-Z\-]*)?$/;
    $name = $1;
    system("rm plot.gpl");
    open(PLOT, ">plot.gpl");
    print PLOT "set title \"$name\"\n";
    print PLOT "set xlabel \"# of streams\"\n";
    print PLOT "set ylabel \"Throuhput (Mbps)\"\n";
    print PLOT "set yrange [0:]\n";
    print PLOT "set nokey\n";
#    print PLOT "set term postscript enhanced eps 20 \"NimbusSanL-Regu\" fontfile \"/usr/local/share/texmf-dist/fonts/type1/urw/helvetic/uhvr8a.pfb\"\n";
    print PLOT "set term postscript\n";
    print PLOT "set output \"$name.ps\"\n";
    print PLOT "plot \"$file\" with lines\n";
    system("gnuplot plot.gpl");
}
