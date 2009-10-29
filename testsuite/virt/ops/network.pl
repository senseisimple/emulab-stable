#!/usr/bin/perl

if (scalar(@ARGV) != 5)
{
    print STDERR "Usage: network.pl <proj> <exp> <pair-count> <bw> <delay>\n";
}

$proj = $ARGV[0];
$exp = $ARGV[1];
$count = $ARGV[2];
$bw = $ARGV[3];
$delay = $ARGV[4];

for ($i = 1; $i <= $count; ++$i)
{
#    $command = "/usr/testbed/bin/tevc -e $proj/$exp now link-$i modify delay=$delay";
#    print $command."\n";
#    system($command);
    $command = "/usr/testbed/bin/tevc -e $proj/$exp now link-$i modify bandwidth=$bw";
    print $command."\n";
    system($command);
}
