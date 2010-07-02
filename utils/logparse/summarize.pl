#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#

#
# Check a list of Emulab records, optionally adding fixup records
# to take care of inconsistencies.
#

use Getopt::Std;
use tbmail;

sub usage()
{
    exit(1);
}
my $optlist = "hl:";

my $tolist = 10;

my $expts = 0;
my $swapins = 0;
my $first;
my $last;
my %peruser = ();
my %perproj = ();
my %pernode = ();
my %byeid = ();

my %iscreate = (
    PRELOAD()     => 1,
    CREATE1()     => 1,
    CREATE2()     => 1,
    BATCHCREATE() => 1,
);
my %isswapin = (
    CREATE1()     => 1,
    CREATE2()     => 1,
    SWAPIN()      => 1,
    BATCHCREATE() => 1,
    BATCHSWAPIN() => 1,
);

sub dorecord($);

#
# Parse command arguments.
#
%options = ();
if (! getopts($optlist, \%options)) {
    usage();
}
if (defined($options{"h"})) {
    usage();
}
if (defined($options{"l"})) {
    $tolist = $options{"l"};
}

my $lineno = 1;
while (my $line = <STDIN>) {
    my $rec = parserecord($line);
    if ($rec) {
	dorecord($rec);
    } else {
	print STDERR "*** Bad record on line $lineno:\n";
	print STDERR "    '$line'\n";
    }
    $lineno++;
}

@projlist = sort { $perproj{$b} <=> $perproj{$a} } keys(%perproj);
@userlist = sort { $peruser{$b} <=> $peruser{$a} } keys(%peruser);
@nodelist = sort { $pernode{$b} <=> $pernode{$a} } keys(%pernode);
@eidlist = sort { $byeid{$b} <=> $byeid{$a} } keys(%byeid);

#
# And the stats
#
print "$expts experiments, $swapins swapins in ", $lineno - 1, " records\n";
print "  First: ", scalar(localtime($first)), "\n";
print "  Last:  ", scalar(localtime($last)), "\n";
print "Top $tolist projects (swapins):\n";
my $did = 0;
for $proj (@projlist) {
    printf "%20s: %d\n", $proj, $perproj{$proj};
    last if (++$did == $tolist);
}
print "Top $tolist users (swapins):\n";
$did = 0;
for $user (@userlist) {
    printf "%20s: %d\n", $user, $peruser{$user};
    last if (++$did == $tolist);
}

# goofy shit
print "Top $tolist nodes (swapins):\n";
$did = 0;
for $node (@nodelist) {
    printf "%20s: %d\n", $node, $pernode{$node};
    last if (++$did == $tolist);
}
print "Top $tolist experiment names:\n";
$did = 0;
for $eid (@eidlist) {
    printf "%20s: %d\n", $eid, $byeid{$eid};
    last if (++$did == $tolist);
}

sub dorecord($) {
    my $rec = shift;
    my ($stamp, $pid, $eid, $uid, $action, $msgid, @nodes) = @{$rec};
    if ($iscreate{$action}) {
	$expts++;
	$first = $stamp if (!$first);
	$last = $stamp;
	$byeid{$eid}++;
    }
    if ($isswapin{$action}) {
	$swapins++;
	$peruser{$uid}++;
	$perproj{$pid}++;
	map { $pernode{$_}++ } @nodes;
    }
}
