#~!/usr/bin/perl

$path = $ARGV[0];
$source = $ARGV[1];
$clusterString = $ARGV[2];

sub printRow
{
    my $dest = shift(@_);
    print "10.0.0.$dest ";
    my $filename = "$path/end-$source/local/logs/$source-to-$dest-run-1.graph";
    open FILE, "<$filename";
    my $line;
    while ($line = <FILE>)
    {
	if ($line =~ /^[0-9]+ ([0-9.]+) $/)
	{
	    my $bw = $1 * 1000;
	    print $bw." ";
	}
    }
    print "\n";
}

my @clusters = split(/:/, $clusterString);

my $i = 0;
for ($i = 0; $i < scalar(@clusters); ++$i)
{
    my @nodes = split(/,/, $clusters[$i]);
    my $j = 0;
    for ($j = 0; $j < scalar(@nodes); ++$j)
    {
	printRow($nodes[$j]);
    }
    print "%%\n";
}

