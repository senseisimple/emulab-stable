#!/usr/bin/perl -w

use lib '/usr/testbed/lib';

use libdb;
use English;
use Getopt::Long;
use strict;

sub usage() {
    print "Usage: $0 [-e <pid> <eid>] | [vnode ...]\n";
}

my %opts = ();
my $pid;
my $eid;
my @nodes = ();

if (@ARGV < 1) {
    exit &usage;
}

my $linpath = "~kwebb/src/testbed/sensors/slothd/linux/slothd";
my $fbsdpath = "~kwebb/src/testbed/sensors/slothd/fbsd/slothd";
my $linrcpath = "~kwebb/src/testbed/tmcd/linux/rc.testbed";
my $fbsdrcpath = "~kwebb/src/testbed/tmcd/freebsd/rc.testbed";

my $SSH = "sudo ssh -q";
my $SCP = "sudo scp -q -o Loglevel=QUIET";

GetOptions(\%opts,'h','e=s');

if ($opts{"e"}) {
    $pid = $opts{"e"};
    $eid = shift;
    @nodes = ExpNodes($pid, $eid);
    print "pid: $pid\n";
    print "eid: $eid\n";
} else {
    @nodes = @ARGV;
}

print "Installing on the following nodes: @nodes\n";

my $j;
foreach $j (@nodes) {
    InstallSlothd($j);
}

sub InstallSlothd($) {
    my $node = shift;
    my $nodeos;
    
    print "Executing for $node ...\n";
    $nodeos = `$SSH $node uname -s`;
    
    if ($nodeos =~ /inux/) {
        print "Node type is linux\n";
        `$SCP $linpath $node:/etc/rc.d/testbed`;
        `$SCP $linrcpath $node:/etc/rc.d/testbed`;    
    }
    else {
        print "Node type is FreeBSD\n";
        `$SCP $fbsdpath $node:/etc/testbed`;
        `$SCP $fbsdrcpath $node:/etc/testbed`;    
    }
    print "starting slothd on $node\n";
    `$SSH $node /etc/testbed/slothd`;
}

exit 0;
