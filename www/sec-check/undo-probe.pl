#!/usr/bin/perl -wT

#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#

# undo-probe.pl - Common code to undo probes to setup/teardown pages.

# Often, an automated SQL injection probe DOESN'T FAIL, due to the page logic
# ignoring the probe value given for the input field, or not using the probe
# label unquoted in an SQL query.  E.g, the first beginexp that succeeds uses
# up the experiment name and blocks all other probes, hence the experiment has
# to be deleted before the next probe can be done.
# 
# This script assists with the timing problems of doing that.  One problem is
# that many setup pages return at the beginning of their actions, and leave
# spewlogfile, et. al. to show the results.  Wget doesn't wait for the spew to
# finish, so there's trouble if the probe script marches on to the next probe.
# Another problem is that the undo actions also take time, and we don't want
# to go on to the next probe until the cleanup action is finished, either.
# 
# The approach here is to check the database or file state on MYBOSS (the
# inner Emulab controller) and proceed when that has been changed.  Logic to
# do that for each of the object types we use to exercise the pages is in the
# cases below.
#

use English;
use Getopt::Std;

sub usage()
{
    print(STDERR
	  "Usage: undo-probe.pl -s|-t <type> <id>\n".
          "switches and arguments:\n".
          "-s      - Undo setup.\n".
          "-t      - Undo teardown\n".
          "<type>  - The type of object to remove or restore\n".
          "          (proj, group, user, exp, osid, imageid)\n".
          "<id>    - Identifier of the object.\n");
    exit(-1);
}
my  $optlist = "st";
%options = ();
if (! getopts($optlist, \%options)) {
    usage();
}

my $setup = 0;
my $teardown = 0;
if (defined($options{"s"})) {
    $setup = 1;
}
elsif (defined($options{"t"})) {
    $teardown = 1;
}
else {
    usage();
}

# Untaint the script arguments.
my $type = $ARGV[0];
if ($type =~ /^([\w]+)$/) {$type = $1;} else {die("Tainted argument $type!")};
my $id =   $ARGV[1];
if ($id =~ /^([\w\/]+)$/) {$id = $1;} else {die("Tainted argument $id!")};

# Experiment, id is pid/eid.
my $pid;
my $eid;
($pid, $eid) = split("/", $id)
    if ($type eq "exp");

sub untainted_envar($)
{
    my ($VAR) = @_;

    my $var = $ENV{"$VAR"};
    $var ne "" || die "*** \$$VAR must be set for ssh.\n";
    # Untaint envar.
    if ($var =~ /^([-\w\.\/]+)$/) {return $1;}
    else {die("Tainted \$$VAR $var!")};
}

my $myboss = untainted_envar("MYBOSS");	# Pipe db queries to $MYBOSS via ssh.
my $myops = untainted_envar("MYOPS"); # Some cmds have to run on $MYOPS via ssh.
my $srcdir = untainted_envar("SRCDIR");	# For supporting files.

# Must untaint the path to use pipes/backtick.
$ENV{'PATH'} = "/bin:/usr/bin";

# Turn off line buffering on output.
$| = 1;

# Really simple queries: a one-line result is returned.
sub squery($)
{
    my ($query) = @_;

    ##print $query . "\n";
    ### my @results = `ssh $myboss 'mysql tbdb -Be "$query"'`;
    my @results = `echo \"$query\" | ssh $myboss mysql tbdb`;

    my $res = $results[1];
    chomp($res);
    return $res;
}

# See whether an object is present in the db.
sub present($$)
{
    my ($type, $id) = @_;
    my $q;

    # Experiment: id is pid/eid.
    if ($type eq "exp") {
	$q = "select count(*) from experiments where pid='$pid' and eid='$eid'";
    }

    my $res = squery($q);
    ##print $res . "\n";
    return $res eq "1";
}

# Make sure the object has indeed been created(setup) or deleted(teardown),
# waiting a bit for it to get there if not.
my $retries = 10; # (in tenths of a minute.)
my $i;
for ($i=1; $i<=$retries; $i++) {
    last if ($setup == present($type, $id));
    print ".";
    sleep(6);
}
my $status = ($setup? ($i>$retries? "missing" : "present")
	      : ($i>$retries? "still here" : "removed") );
print "" . ($i>$retries? "*** " : "") . "$type $id is " . 
    $status . "\n";
exit(0) if ( $status eq "missing" || $status eq "still here" ); # Done.

# Do the cleanup.
my $cmd;
my $host = $myboss;
if ($type eq "exp") {
    if ($setup) { $cmd = "endexp -w $pid $eid"; }
    else {
	$host = $myops;
	$cmd = "startexp -w -f -E '$pid/$eid.' " . 
	    "-p $pid -e $eid shaped-2-nodes.ns";
	# Put the ns file on my myops homedir.
	system("scp $srcdir/shaped-2-nodes.ns $myops:");
    }
}
for ($i=1; $i<=$retries; $i++) {
    print "doing $cmd\n";
    print `ssh -n $host '$cmd'`;
    last if ( $? <= 0 );
    if ($i < $retries) {
	print "sleeping...";
	sleep(6);
    }
}
	
# Check and wait for completion.
for ($i=1; $i<=$retries; $i++) {
    last if ($teardown == present($type, $id));
    print ".";
    sleep(6);
}
print "" . ($i>$retries? "*** " : "") . "$type $id is " . 
    ($setup? ($i>$retries? "hung" : "cleaned up")
     : ($i>$retries? "not back yet" : "restored") ) . "\n";
