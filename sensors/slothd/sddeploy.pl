#!/usr/bin/perl -w

use lib '/usr/testbed/lib';

use libdb;
use English;
use Getopt::Long;
use strict;

sub usage() {
    print "Usage: $0 [-a] | [-e <pid> <eid>] | [vnode ...]\n";
}

my %opts = ();
my $pid;
my $eid;
my @nodes = ();
my @failed = ();

if (@ARGV < 1) {
    exit &usage;
}

my $linpath = "~kwebb/src/testbed/sensors/slothd/linux/slothd";
my $fbsdpath = "~kwebb/src/testbed/sensors/slothd/fbsd/slothd";
my $linrcpath = "~kwebb/src/testbed/tmcd/linux/rc.testbed";
my $fbsdrcpath = "~kwebb/src/testbed/tmcd/freebsd/rc.testbed";

my $SSH = "sudo ssh -q";
my $SCP = "sudo scp -q -o Loglevel=QUIET";

GetOptions(\%opts,'h','f=s','e=s','a');

if ($opts{"f"}) {
    open(NODEFILE, "<$opts{'f'}");
    while (<NODEFILE>) {
        chomp;
        push @nodes, $_;
    }
}

elsif ($opts{"a"}) {
    my $qres = DBQuery("select a.node_id from reserved as a "
                       . "left join nodes as b "
                       . "on a.node_id = b.node_id "
                       . "where b.type = 'pc850' or b.type = 'pc600'");
    if ($qres) {
        while(my @row = $qres->fetchrow_array()) {
            push @nodes, $row[0];
        }
    }
}

elsif ($opts{"e"}) {
    $pid = $opts{"e"};
    $eid = shift;
    @nodes = ExpNodes($pid, $eid);
    print "pid: $pid\n";
    print "eid: $eid\n";
} 

else {
    @nodes = @ARGV;
}

print "Installing on the following nodes: @nodes\n";

my $j;
foreach $j (@nodes) {
    if (InstallSlothd($j)) {
        push @failed, $j;
    }
}

if (@failed) {
    print "failed nodes: @failed\n\n";
}

exit 0;

sub InstallSlothd($) {
    my $node = shift;
    my $nodeos = "";
    
    print "Executing for $node ...\n";
    print "pinging node..\n";
    `ping -q -c 2 $node > /dev/null`;
    if ($?) {
        print "host down.. skipping.\n";
        return 1;
    }

    $nodeos = `$SSH $node uname -s`;
    
    if ($nodeos =~ /linux/i) {
        print "Node type is linux\n";
        print "stopping slothd on $node\n";
        `$SSH $node killall slothd`;
        sleep 1;
        `$SCP $linpath $node:/etc/rc.d/testbed`;
        `$SCP $linrcpath $node:/etc/rc.d/testbed`;
    }
    elsif ($nodeos =~ /freebsd/i) {
        print "Node type is FreeBSD\n";
        print "stopping slothd on $node\n";
        `$SSH $node killall slothd`;
        sleep 1;
        `$SCP $fbsdpath $node:/etc/testbed`;
        `$SCP $fbsdrcpath $node:/etc/testbed`;    
    }
    else {
        print "Unknown node OS: $nodeos: not deploying.\n";
        return 1;
    }

    print "starting slothd on $node\n";
    `$SSH $node /etc/testbed/slothd -f`;
    if ($?) {
        return 1;
    }
    return 0;
}

exit 0;
