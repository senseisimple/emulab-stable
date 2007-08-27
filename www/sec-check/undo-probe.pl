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
	  "Usage: undo-probe.pl -s|-t [-p <pid>] <type> <id>\n".
          "switches and arguments:\n".
          "-s      - Undo setup.\n".
          "-t      - Undo teardown.\n".
	  "-n      - \"Nuke mode\" for unapproved users after joinproject.\n".
          "-p <pid>  Project in which the object resides.\n".
          "          (Projects and users are not in projects.)\n".
          "<type>  - The type of object to remove or restore.\n".
          "          (proj, group, user, exp, osid, imageid)\n".
          "<id>    - Identifier (name) of the object.\n");
    exit(-1);
}
my  $optlist = "stnp:";
%options = ();
if (! getopts($optlist, \%options)) {
    usage();
}

my $setup = 0;
my $teardown = 0;
my $pid;
my $nuke_mode = "";
if (defined($options{"s"})) {
    $setup = 1;
}
elsif (defined($options{"t"})) {
    $teardown = 1;
}
elsif (defined($options{"n"})) {
    # This is used after joinproject.  "nuke mode" removes a not-yet-approved user.
    $nuke_mode = "-n";
}
else {
    usage();
}
if (defined($options{"p"})) {
    $pid = $options{"p"};
    if ($pid =~ /^([-_\w\d]+)$/) {$pid = $1;} else {die("Tainted argument $pid!")};
}

# Untaint the script arguments.
my $type = $ARGV[0];
if ($type =~ /^([-_\w\d]+)$/) {$type = $1;} else {die("Tainted argument $type!")};
my $id =   $ARGV[1];
if ($id =~ /^([-_\w\d]+)$/) {$id = $1;} else {die("Tainted argument $id!")};

sub untainted_envar($)
{
    my ($VAR) = @_;

    my $var = $ENV{"$VAR"};
    $var ne "" || die "\n*** \$$VAR must be set for ssh.\n\n";
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

    if ($type eq "proj") {
	$q = "select count(*) from projects where pid='$id'";
    } 
    elsif ($type eq "group") {
	$q = "select count(*) from groups where pid='$pid' and gid='$id'";
    }
    elsif ($type eq "user") {
	$q = "select count(*) from users where uid='$id'";
    }
    elsif ($type eq "exp") {
	$q = "select count(*) from experiments where pid='$pid' and eid='$id'";
    }
    elsif ($type eq "osid") {
	$q = "select count(*) from os_info where pid='$pid' and osname='$id'";
    }
    elsif ($type eq "imageid") {
	$q = "select count(*) from images where pid='$pid' and imagename='$id'";
    }
    else {
	print "*** Invalid type: present(type=$type, id=$id), pid=$pid\n\n";
	exit(-1);
    }
    
    my $res = squery($q);
    ##print $res . "\n";
    return $res eq "1";
}

# Parts of this have never been needed.
sub not_yet()
{
    print "*** Never needed to restore $type $id during teardown before. ***\n\n";
    exit(-1);
}

# Make sure the object has indeed been created(setup) or deleted(teardown),
# waiting a bit for it to get there if not.
my $present = present($type, $id);
print "\n$type $id is " . ($present ? "still here" : "not here") . ", waiting.\n"
    if ( $setup != $present );
my $retries = 10; # (in tenths of a minute.)
my $i;
for ($i=1; $i<=$retries; $i++) {
    last if ($setup == present($type, $id));
    print ".";
    sleep(6);
}
my $status = ($setup ? ($i>$retries? "missing" : "present")
	      : ($i>$retries? "still here" : "removed") );
print "\n*** $type $id is " . $status . ". *** \n";
if ( $status eq "missing" || $status eq "still here" ) { # Done.
    print "\n";
    exit(0);
}

# Do the cleanup.  Lots of nice delete scripts; restoring is harder.
my $cmd;
my $utsb = "/usr/testbed/sbin";
my $host = $myboss;
my $ssh  = 1;
my $post_query;
if ($type eq "proj") {
    if ($setup) { $cmd = "$utsb/wap $utsb/rmproj $id"; } else { not_yet(); }
} 
elsif ($type eq "group") {
    if ($setup) { $cmd = "$utsb/wap $utsb/rmgroup $id"; } else { not_yet(); }
}
elsif ($type eq "user") {
    if ($setup) {
	$cmd = "$utsb/wap $utsb/rmuser $nuke_mode $id";
	# Clean up archived users or we won't be allowed to use the uid again.
	$post_query = "delete from users where uid='$id' and status='archived'";
    } else { not_yet(); }
}
elsif ($type eq "exp") {
    $ssh = 1;
    if ($setup) { $cmd = "endexp -w $pid/$id"; }
    else {
	$host = $myops;
	$cmd = "startexp -w -f -E '$pid/$id.' " . 
	    "-p $pid -e $id shaped-2-nodes.ns";
	# Put the ns file on my myops homedir.
	system("scp $srcdir/shaped-2-nodes.ns $myops:");
    }
}
elsif ($type eq "osid") {
    if ($setup) {
	squery("delete from os_info where pid='$pid' and osname='$id'");
    }
    else { not_yet(); }
}
elsif ($type eq "imageid") {
    if ($setup) {
	squery("delete from images where pid='$pid' and imagename='$id'");
	# May be an _ez image with the an osid by the same name.
	squery("delete from os_info where pid='$pid' and osname='$id'");
    }
    else { not_yet(); }
}
else {
    print "*** Invalid type=$type, id=$id, pid=$pid ***\n\n";
    exit(-1);
}

if (defined($cmd)) {
    $cmd = "ssh -n $host '$cmd'"
	if $ssh;
    for ($i=1; $i<=$retries; $i++) {
	print "doing $cmd\n";
	print `$cmd`;
	last if ( $? <= 0 );
	if ($i < $retries) {
	    print "sleeping...";
	    sleep(6);
	}
    }
}
	
if  (defined($post_query)) {
    squery($post_query);
}

# Check and wait for completion.
for ($i=1; $i<=$retries; $i++) {
    last if ($teardown == present($type, $id));
    print ".";
    sleep(6);
}
print "*** $type $id is now " . 
    ($setup? ($i>$retries? "hung" : "cleaned up")
     : ($i>$retries? "not back yet" : "restored") ) . ". ***\n\n";
