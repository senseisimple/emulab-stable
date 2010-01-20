#!/usr/bin/perl

$path = $ARGV[0];
$count = $ARGV[1];

sub getBw
{
    my $source = shift(@_);
    my $dest = shift(@_);
    my $result = 1000;
    my $file = "$path/end-$source/local/logs/$source-to-$dest-run-1.graph";
    open BW, "<".$file;
    my $bw = <BW>;
    chomp($bw);
    if ($bw =~ /1 ([0-9.]+) /)
    {
	$result = $1*1000;
    }
    return $result;
}

sub getDelay
{
    my $source = shift(@_);
    my $dest = shift(@_);
    my $result = 10000000;
    open DELAY, "<$path/end-$source/local/logs/$source-$dest.ping";
    my $delay;
    while ($delay = <DELAY>)
    {
	if ($delay =~ /time=([0-9.]+) ms/)
	{
	    if ($result > $1)
	    {
		$result = $1;
	    }
	}
    }
    return $result;
}

my $i = 1;
my $j = 1;

for ($i = 1; $i <= $count; ++$i)
{
    for ($j = 1; $j <= $count; ++$j)
    {
	if ($i != $j)
	{
	    my $bw = getBw($i, $j);
	    my $delay = getDelay($i, $j);
	    print "10.0.0.$i 10.0.0.$j $bw $delay 0\n";
	}
    }
}
