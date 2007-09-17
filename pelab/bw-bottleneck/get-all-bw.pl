#!/usr/bin/perl

if ($ARGV != 4)
{
    die("Usage: get-all-bw.pl <pid> <eid> <node-count> <duration>");
}

$pid = $ARGV[0];
$eid = $ARGV[1];
$nodeCount = $ARGV[2];
$duration = $ARGV[3];

for ($i = 1; $i <= $nodeCount; ++$i)
{
    system("perl gather-dest.pl $pid $eid $nodeCount $i $duration");
}
