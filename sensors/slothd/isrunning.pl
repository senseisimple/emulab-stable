#!/usr/bin/perl -w

use lib '/usr/testbed/lib';

use libdb;
#use Mysql;
use English;
use Getopt::Long;
use strict;

# Turn off line buffering
$|=1;

my $t = 120;

sub usage() {
    die("Usage: $0 [-s] [<pid> <eid>]
  -s    start slothd if not running.
If no pid/eid are given, do all nodes that haven't reported slothd
data in the last $t minutes.\n"); }

my %opts = ();

GetOptions(\%opts,'s');

if ($opts{"h"}) {
    usage();
}

my @nodes = ();

if (@ARGV > 0) {
    my ($pid, $eid) = @ARGV;
    @nodes = ExpNodes($pid, $eid);
} else {
    DBQueryFatal("drop table if exists idletemp2");
    #(print "create temporary table idletemp2
    DBQueryFatal("create temporary table idletemp2
select pid,eid,r.node_id,max(tstamp) as t from reserved as r 
left join node_idlestats as n on r.node_id=n.node_id 
where r.node_id not like \"sh%\" and r.node_id not like \"wireless%\" 
and r.node_id not like \"%ron%\" 
group by pid,eid,node_id 
having t is null or (unix_timestamp(now())-unix_timestamp(t) >= $t*60) 
order by pid,eid,node_id");
    # We now have a table that says the last time each node reported
    # for all nodes that haven't reported in last $t minutes.
    # (Note: Don't change group by above to pid,eid!)
    my $r = DBQueryFatal("select node_id from idletemp2");
    while (my %row = $r->fetchhash) {
	push(@nodes,$row{"node_id"});
    }
}

sub pcnum { substr($a,2) <=> substr($b,2) }

foreach my $n (sort pcnum @nodes) {
    print "checking slothd on $n: ";
    #print "\n"; next;
    print check($n);
    print "\n";
}

exit 0;

sub check {
    #my $ssh="sshtb -o \"FallBackToRsh no\" -o \"ConnectionAttempts 3\" -q";
    my $ssh="sshtb -q";
    my $node = shift;
    my $cmd1 = "ps auxwww | grep slothd | grep -v grep";
    my $cmd2 = "/etc/testbed/slothd -f";
    # Run an ssh command in a child process, protected by an alarm to
    # ensure that the ssh is not hung up forever if the machine is in
    # some funky state.
    my $str = "";
    my $syspid = fork();
    if ($syspid) {
	# parent
        local $SIG{ALRM} = sub { kill("TERM", $syspid); };
        alarm 5;
	#print "$syspid - Alarm set.\n";
        waitpid($syspid, 0);
	my $rv = $?;
	#print "$syspid - Done waiting. Got '$rv'\n";
        alarm 0;
	if ($rv == 15) { 
	    #print "Node is wedged.\n"; 
	    $str="unreachable"; 
	} elsif ($rv == 256) { 
	    #print "Node is not running sshd.\n"; 
	    $str="SSH not available"; 
	} elsif ($rv == 512) { 
	    $str="not running, couldn't start slothd"; 
	} elsif ($rv == 0) {
	    $str="running";
	} elsif ($rv == 1) {
	    $str="not running";
	} elsif ($rv == 2) {
	    $str="not running, started";
	} else {
	    $str="I don't know what happened...$rv";
	}
    } else {
	# child
	$str = `sudo $ssh $node $cmd1`;
	#print "$syspid - ssh succeeded:'$str'\n";
	if ($str) {
	    #print "running.\n";
	    exit(0);
	} else {
	    #print "not running. ";
	    if ($opts{"s"}) {
		$str = `sudo $ssh $node $cmd2`;
		if ($str) {
		    #print "(start returned '$str') ";
		}
		#print "started...";
		exit(2);
	    }
	    #print "\n";
	    exit(1);
	}
	exit(-1);
    }
    return $str;
}
