#!/usr/bin/perl -w

use lib '/usr/testbed/lib';

use libdb;
use English;
use strict;

sub usage() {
    print "Usage: $0 <pid> <eid>\n";
}

if (@ARGV < 2) {
    exit &usage;
}

my ($pid, $eid) = @ARGV;

print "pid: $pid\neid: $eid\n";

my $nodeos;
my $j;

foreach $j (ExpNodes($pid, $eid)) {
    print "stopping slothd on $j\n";
    `sudo ssh -q $j killall slothd`;
}

exit 0;
