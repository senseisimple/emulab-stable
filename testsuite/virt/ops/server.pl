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

if (scalar(@ARGV) != 4)
{
    print STDERR "Usage: server.pl <barrier-name> <pair-number> <duration> <run-path>\n";
    exit(1);
}

$name = $ARGV[0];
$num = $ARGV[1];
$duration = $ARGV[2];
$runPrefix = $ARGV[3];

$timeout = $duration * 2;

printLog("$num Server Begin");

system("/usr/local/etc/emulab/emulab-iperf -s -w 256k -p 4000 &");

printLog("$num Server Before server barrier");
system("$runPrefix/run-sync.pl 30 -n $name-server");

printLog("$num Server Before client barrier");
system("$runPrefix/run-sync.pl $timeout -n $name-client");

system("killall -9 iperf");
printLog("$num Server End");
