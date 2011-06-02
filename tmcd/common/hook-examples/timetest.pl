#!/usr/bin/perl

use strict;
use warnings;

my $debug = 0;
if (@ARGV >= 1 && $ARGV[0] eq '-d') {
    $debug = 1;
    shift @ARGV;
}

#die "Usage: $0 -d TOLERANCE boot\n" unless @ARGV == 1;

my $TOLERANCE = 0.5;
if (scalar(@ARGV) > 1) {
    $TOLERANCE = $ARGV[0];
}

my $NTP_SERVER = "ops.emulab.net";

my $offset;
my $output = '';
open NTP, "/usr/sbin/ntpdate -q ops.emulab.net | ";
while (<NTP>) {
    $output .= $_;
    next unless /.+?: [a-z]+ time server .+? offset ([-+.\d]+) sec/;
    $offset = $1;
}

unless (defined $offset) {
    print STDERR "*** ERROR: Unable to get offset from ntpdata\n";
    print STDERR "NOTE: ntpdate output\n";
    print STDERR $output;
    exit 2;
}

if (abs($offset) > $TOLERANCE) {
    print STDERR "*** ERROR: offset of $offset greater than $TOLERANCE\n";
    print "FAILED: offset of $offset greater than $TOLERANCE\n";
    exit 1;
} else {
    print "PASSED: offset of $offset within tolerance of $TOLERANCE\n";
}

