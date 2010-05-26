#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#

sub printLog
{
    my $line = shift(@_);
    my $time = `date +%F_%H:%M:%S`;
    chomp($time);
    print STDERR  $time . " " . $line . "\n";
}

if (scalar(@ARGV) != 3)
{
    print STDERR "Usage: monitor.pl <barrier-name> <duration> <run-path>\n";
    exit(1);
}

$DELAY = 1;
$COUNT = 3000;

my $name = $ARGV[0];
my $duration = $ARGV[1];
my $runPrefix = $ARGV[2];

$timeout = $duration * 2;

printLog("Monitor Begin");

system("vmstat -n $DELAY $COUNT >& /local/logs/vmstat-cpu.out &");
system("vmstat -d -n $DELAY $COUNT >& /local/logs/vmstat-disk.out &");
system("vmstat -m -n $DELAY $COUNT >& /local/logs/vmstat-slab.out &");

printLog("Monitor Before server barrier");
system("$runPrefix/run-sync.pl 30 -n $name-server");

printLog("Monitor Before client barrier");
system("$runPrefix/run-sync.pl $timeout -n $name-client");

system("killall -9 vmstat");
printLog("Monitor End");
