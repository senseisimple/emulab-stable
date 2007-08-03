#!/usr/bin/perl -w

# Usage: download-logs.pl <pid> <eid>

if (@ARGV != 3)
{
    die("Invalid number of arguments");
}

$pid = $ARGV[0];
$eid = $ARGV[1];
$nodeCount = $ARGV[2];

sub copyFiles
{
    my $source = $_[0];
    my $dest1 = $_[1];
    my $dest2 = $_[2];
    system("mkdir -p $prefix/dump/$source/$dest1/$dest2");
    system("cp $prefix/node-$source/local/logs/dump-$source-$dest1-$dest2.dump $prefix/dump/$source/$dest1/$dest2/source.dump");
    system("cp $prefix/node-$dest1/local/logs/dump-$source-$dest1-$dest2.dump $prefix/dump/$source/$dest1/$dest2/dest1.dump");
    system("cp $prefix/node-$dest2/local/logs/dump-$source-$dest1-$dest2.dump $prefix/dump/$source/$dest1/$dest2/dest2.dump");

#    system("perl dump2filter.pl $prefix/dump/$source/$dest1/$dest2");
}

print("/usr/testbed/bin/loghole -e $pid/$eid sync -P\n");
system("/usr/testbed/bin/loghole -e $pid/$eid sync -P");

$prefix = "/proj/$pid/exp/$eid/logs";

for ($source = 1; $source <= $nodeCount; ++$source)
{
    for ($dest1 = 1; $dest1 <= $nodeCount; ++$dest1)
    {
	for ($dest2 = $dest1 + 1; $dest2 <= $nodeCount; ++$dest2)
	{
	    if ($source != $dest1 && $source != $dest2)
	    {
		copyFiles($source, $dest1, $dest2);
	    }
	}
    }
}

system("/usr/testbed/bin/loghole -e $pid/$eid archive");
