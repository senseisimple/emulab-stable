#!/usr/bin/perl -w

use lib '/usr/testbed/lib';

use libdb;
use English;
use Getopt::Long;
use strict;

# Turn off line buffering
$|=1;

sub usage() {
    print "Usage: $0 [-s] <pid> <eid>\n".
        "  -s    start slothd if not running.\n";
}

my %opts = ();

GetOptions(\%opts,'s');

if ($opts{"h"}) {
    exit &usage;
}

if (@ARGV < 1) {
    exit &usage;
}

my ($pid, $eid) = @ARGV;

print "pid: $pid\neid: $eid\n";

my $j;

foreach $j (ExpNodes($pid, $eid)) {
    my $sld;
    print "checking slothd on $j: ";
    $sld = `sudo ssh -q $j ps auxwww | grep slothd | grep -v grep`;
    if ($sld) {
        print "running.\n";
    }
    else {
        print "not running. ";
        if ($opts{"s"}) {
            `sudo ssh -q $j /etc/testbed/slothd -f`;
            print "started..";
        }
        print "\n";
    }
}

exit 0;
