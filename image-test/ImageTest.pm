#
# EMULAB-COPYRIGHT
# Copyright (c) 2005-2006 University of Utah and the Flux Group.
# All rights reserved.
#

package ImageTest;
use Exporter;

@ISA = "Exporter";
@EXPORT = qw (test test_cmd test_ssh test_rcmd test_experiment exit_str
	      ERR_MASK ERR_NONE ERR_EXPECTED ERR_FAILED ERR_FATAL ERR_INT
	      STATUS_MASK STATUS_NONE STATUS_SWAPPEDIN STATUS_EXISTS STATUS_CLEANUP);

use IO::File;
use strict;

use vars qw(%parms %dependencies %tally);
use vars qw($eid $pid $datadir $resultsdir);
use vars qw(@mapping @nodes @pnodes %to_physical %from_physical);
use vars qw($EXPECTED $FAILED);
use vars qw(%ERR %STATUS);

sub true() {1}
sub false() {0}

#
# exit values, or two parts together
#

sub ERR_MASK     {7};
sub ERR_NONE     {0};
sub ERR_EXPECTED {1}; # expected failures
sub ERR_FAILED   {2}; # tests failed
sub ERR_FATAL    {3}; # fatal error
sub ERR_INT      {4}; # interrupted
%ERR = (ERR_NONE, 'ERR_NONE',
	ERR_EXPECTED, 'ERR_EXPECTED',
	ERR_FAILED, 'ERR_FAILED',
	ERR_FATAL, 'ERR_FATAL',
	ERR_INT, 'ERR_INT');

sub STATUS_MASK      {3 << 3};
sub STATUS_NONE      {0 << 3};
sub STATUS_SWAPPEDIN {1 << 3}; # experment still swapped in
sub STATUS_EXISTS    {2 << 3}; # experment still exists
sub STATUS_CLEANUP   {3 << 3}; # requires cleanup
%STATUS = (STATUS_NONE, 'STATUS_NONE',
	   STATUS_SWAPPEDIN, 'STATUS_SWAPPEDIN',
	   STATUS_EXISTS, 'STATUS_EXISTS',
	   STATUS_CLEANUP, 'STATUS_CLEANUP');

sub exit_str($) {
  my ($exit) = @_;
  return join(' ', $ERR{$exit &  ERR_MASK}, $STATUS{$exit & STATUS_MASK});
}

#
#
#

sub oneof ($$) {
  my ($what, $which) = @_;
  foreach (@{$parms{$which}}) {
    return true if ($_ eq $what);
  }
  return false;
}

#
# Performs a test on a swapped in experient.  Returns true if the test
# passed.
#
# Each test can depend and on any number of other tests.  The test
# will be skipped unless all the dependencies are satisfied.
#
# Examples:
#   test 'test 1', [], sub {...};
#   test 'test 2', ['test 1'], sub {...};
#   test 'login prompt', [], sub {
#     local $_ = cat "/var/log/tiplogs/$pnode.run";
#     /login\: /;
#   }
#
sub test ($$&) {
  my ($name,$requires,$test) = @_;

  if (oneof($name, 'skip')) {
    return 0;
  }

  $tally{total}++;

  my $deps_sat = 1;
  foreach (@$requires) {$deps_sat = 0 unless $dependencies{$_}}

  unless ($deps_sat) {
    print "Skipping test \"$name\" due to unsatisfied dependies.\n";
    return 0;
  }

  print "<=== Starting Test: $name\n";

  my $res;
  eval {$res = &$test(%parms)};

  if ($res) {
    $tally{passed}++;
    $dependencies{$name} = 1;
    print ">=== \"$name\" succeeded\n";
    return true;
  } elsif ($@) {
    if (oneof($name, 'ignore')) {
      $tally{expected}++;
      print $EXPECTED "$name\n";
      print ">=== \"$name\" died: $@ -- ignored";
    } else {
      $tally{failed}++;
      print $FAILED "$name\n";
      print ">*** \"$name\" died: $@";
    }
    return false;
  } else {
    if (oneof($name, 'ignore')) {
      $tally{expected}++;
      print $EXPECTED "$name\n";
      print ">=== \"$name\" failed -- ignored\n";
    } else {
      $tally{failed}++;
      print $FAILED "$name\n";
      print ">*** \"$name\" failed\n";
    }
    return false;
  }
}

#
#
#
sub sys (@) {
  print "<- Executing: ", join(' ', @_), "\n";
  system @_;
  print ">- Done\n";
  return $? >> 8 == 0;
}

#
# Performs a test by executing a command and optionally checking the output
#
# Example :
#   test_cmd "ssh $node", [], "ssh $node.$eid.$pid true";
#
sub test_cmd ($$$;&) {
  my ($name,$requires,$cmd,$output_test) = @_;
  test $name, $requires, sub {
    if (not defined $output_test) {
      return sys($cmd);
    } else {
      local $/ = undef;
      my $F = new IO::File;
      print "<- Executing: $cmd\n";
      open $F, "$cmd |" or return false;
      local $_ = <$F>;
      close $F;
      print ">- Done\n";
      return 0 unless ($? >> 8 == 0);
      open $F, ">$resultsdir/$name.out";
      print $F $_;
      close $F;
      my $res = &$output_test;
      print "*** Output of \"$cmd\" did not match expected output\n" unless $res;
      return $res;
    }
  };
}

#
# Test that ssh is working on a node
#
# Example:
#   test_ssh 'node';
#
sub test_ssh ($) {
  my ($node) = @_;
  test_cmd "ssh-$node", [], "ssh-node $node true";
}

#
# Performs a test by executing a remote command and optionally
# checking the output.  test_ssh must be executed on the node before
# any remote commands for the node.
#
# Examples:
#   test_rcmd 'sudo', [], 'node', 'sudo touch /afile.txt';
#   test_rcmd 'hostname' , [], 'node', 'hostname', sub {/^node\.$eid\.$pid/};
#
sub test_rcmd ($$$$;&) {
  my ($name,$requires,$node,$cmd,$output_test) = @_;
  &test_cmd($name, ["ssh-$node", @$requires],
	    "ssh-node $node $cmd", $output_test);
}

#
#
#
sub test_traceroute ($$@) {
  my ($src,$dest,@path) = @_;
  test_rcmd "traceroute-${src}_$dest", [], $src, "/usr/sbin/traceroute $dest",
    sub {
      local @_ = split /\n/;
      if (@_+0 != @path+0) {
	printf "*** traceroute $src->$dest: expected %d hops but got %d.\n",
	  @path+0, @_+0;
	return 0;
      }
      for (my $i = 0; $i < @_; $i++) {
	local $_ = $_[$i];
	my ($n) = /^\s*\d+\s*(\S+)/;
	next if $n eq $path[$i];
	printf "*** traceroute $src->$dest: expected %s for hop %d but got %s\n",
	  $path[0], $i+1, $n;
	return 0;
      }
      return 1;
    };
}


#
# Scans a log file for any errors or serious warnings.  The log file
# may also be a pipe as the string is passed directly to open.
#
# Example:
#   test_scanlog 'error free log', [], 'log';
#
sub test_scanlog ($$$) {
  my ($name,$requires,$log) = @_;
  test $name, $requires, sub {
    my $F = new IO::File;
    open $F, $log or die "Unable to open \"$log\" for reading.";
    my $errors = 0;
    while (<$F>) {
      next unless /^\*\*\* /;
      print;
      $errors = 1;
    }
    close $F;
    return $errors == 0;
  };
}

#
#
#
sub cat ($) {
  open F, $_[0];
  local $/ = undef;
  local $_ = <F>;
  close F;
  return $_;
};

#
#
#
sub single_node_tests ($) {

  my ($node) = @_;
  my ($pnode) = $to_physical{$node};

  test_ssh $node;

  return if $parms{skip_std_tests};

  test_rcmd "sudo-$node", [], $node, 'sudo touch /afile.txt';

  test_rcmd "hostname-$node" , [], $node, 'hostname', sub {
    /^$node\.$eid\.$pid/i || /^$pnode/i;
  };

  test "login_prompt-$node", [], sub {
    local $_ = cat "/var/log/tiplogs/$pnode.run";
    /login\: /;
  };

  test "proj_mount-$node", ["ssh-$node"], sub {
    sys "ssh-node $node touch $resultsdir/working/$node"
      or return false;
    return -e "$resultsdir/working/$node";
  }

}

#
#
#
sub multi_node_tests () {

  #test_cmd 'linktest', [], "run_linktest.pl -v -e $pid/$eid";

  return if $parms{skip_std_tests};

  sleep 10;
  test_cmd 'linktest1', [], "run_linktest.pl -v -L 1 -l 1 -e $pid/$eid";
  sleep 2;
  test_cmd 'linktest2', [], "run_linktest.pl -v -L 2 -l 2 -e $pid/$eid";
  sleep 2;
  test_cmd 'linktest3', [], "run_linktest.pl -v -L 3 -l 3 -e $pid/$eid";
  sleep 2;
  test_cmd 'linktest4', [], "run_linktest.pl -v -L 4 -l 4 -e $pid/$eid";

}

#
# Creats and swaps in a test experment in and performs tests on the
# images after it is done.  When all the tests are done swap the experment out,
# copy the experment data dir to EXP-YYYYMMDDHHMM and terminate the experiment.
#
# Will fork a child to do the actual work and return the pid of the child
#
# Expects a hash of parmaters as input for example:
#   test_experiment
#     pid => 'tbres',
#     eid => 'it-single',
#     os => 'RHL73-STD',
#     hardware => 'pc850',
#     datadir => '...',
#     resultsdir => '...',
#
# The .ns file for the experiment is expected to be named "<datadir>/nsfile.ns".
# The file will be scanned and any instances of @PARM@ will be substituted
# for the value of PARM.  Any image specific tests should be located in
# "<datadir>/tests.ns".
#
sub test_experiment (%) {

  {
    my $pid = fork();
    die unless defined $pid;

    return $pid if $pid != 0; # child
  }

  %parms = @_;
  %dependencies = ();
  %tally = (total => 0, passed => 0, expected => 0, failed => 0);

  $eid = $parms{eid};
  $pid = $parms{pid};
  $datadir = $parms{datadir};
  $resultsdir = $parms{resultsdir};

  my $err = ERR_NONE;
  my $status = STATUS_NONE;

  $SIG{__DIE__} = sub {
    return unless defined $^S && !$^S;
    $! = (ERR_FATAL | $status);
    die $_[0];
  };

  $SIG{INT} = 'IGNORE';

  $SIG{TERM} = sub {
    print "TERMINATING\n";
    exit (ERR_INT | $status);
  };

  mkdir $resultsdir, 0777;
  chdir $resultsdir;

  mkdir "working", 0777;
  mkdir "bin",     0777;

  #sleep 5;
  #exit 0;

  $ENV{PATH} = "$resultsdir/bin:$ENV{PATH}";

  open STDOUT, ">log"      or die;
  open STDERR, ">&STDOUT"  or die;

  my ($F,$O);

  $F = new IO::File ">pid" or die;
  print $F "$$\n";
  close $F;

  $FAILED = new IO::File ">failed-tests" or die;
  $EXPECTED = new IO::File ">failed-but-ignored" or die;

  $F = new IO::File ">parms" or die;
  foreach (sort keys %parms) {
    unless (ref $parms{$_}) {
      print $F "$_: $parms{$_}\n";
    } else {
      print $F "$_: @{$parms{$_}}\n";
    }
  }
  close $F;

  $F = new IO::File ">bin/ssh-node" or die;
  print $F "#!/bin/sh\n";
  print $F "\n";
  print $F 'cmd=$1',"\n";
  print $F 'shift', "\n";
  print $F join(' ',
		'ssh', "-x",
		"-o BatchMode=yes", "-o StrictHostKeyChecking=no",
		"-o UserKnownHostsFile=$resultsdir/working/known_hosts",
		"\$cmd.$parms{eid}.$parms{pid}", '"$@"', "\n");
  close $F;
  chmod 0755, "bin/ssh-node";

  if ($parms{stages} =~ /c/) {

    $F = new IO::File "$datadir/nsfile.ns" or die;
    $O = new IO::File ">nsfile.ns" or die;
    while (<$F>) {
      s/\@([^@]+)\@/$parms{lc $1}/g;
      print $O $_;
    }
    close $O;
    close $F;

    $status = STATUS_EXISTS;
    sys("/usr/testbed/bin/startexp -w -i -f".
	" -E \"Experiment For Testing Images\"".
	" -p $pid -e $eid nsfile.ns");
    if ($? >> 8 != 0) {
      print "*** Could not create experment\n";
      exit (ERR_FATAL | STATUS_NONE);
    }
  }

  my $swapin_success = 1;
  if ($parms{stages} =~ /s/) {
    $status = STATUS_SWAPPEDIN;
    $swapin_success = test_cmd 'swapin', [],
      "/usr/testbed/bin/swapexp -w -e $pid,$eid in";
    $status = STATUS_EXISTS unless $swapin_success;
  }

  if ($swapin_success) {

    if ($parms{stages} =~ /s/) {

      # FIXME: need proper way to get the log file
      test_scanlog 'error_free_swapin', [],
	`ls -t /proj/$pid/exp/$eid/tbdata/swapexp.* | head -1`;
    }

    local (@mapping, @nodes, @pnodes, %to_physical, %from_physical);

    open F, "/usr/testbed/bin/expinfo -m $pid $eid | ";

    while (<F>) {
      last if /^---/;
    }

    while (<F>) {
      next unless /\w/;
      local @_ = split /\s+/;
      push @mapping, [@_];
      push @nodes, $_[0];
      push @pnodes, $_[3];
      $to_physical{$_[0]} = $_[3];
      $from_physical{$_[3]} = $_[0];
    }

    if ($parms{stages} =~ /t/) {

      foreach my $node (@nodes) {
	single_node_tests($node);
      }

      if (@nodes > 1) {
	multi_node_tests();
      }

      if (-e "$datadir/tests.pl") {
	
	do "$datadir/tests.pl";
	
	if ($@) {
	  print "*** Unable to complete tests: $@";
	  print "*** Results may not be accurate.\n";
	  $err = ERR_FATAL
	}
      }
    }

    if ($parms{stages} =~ /[oe]/ && 
	(!$parms{dont_swapout_unexpected} || $tally{failed} == 0)) {
      
      test_cmd 'loghole', [], "loghole -e $pid/$eid sync";
	
      foreach my $node (@nodes) {
	my $pnode = $to_physical{$node};
	sys "cp -pr /var/log/tiplogs/$pnode.run tiplog-$node"
	  or print "*** WARNING: Unable to copy tiplog for node $node.\n";
      }

      test_cmd 'swapout', [],
	"/usr/testbed/bin/swapexp -w -e $pid,$eid out"
	  and $status = STATUS_EXISTS;

      # FIXME: need proper way to get the log file
      test_scanlog 'error_free_swapout', ['swapout'],
	`ls -t /proj/$pid/exp/$eid/tbdata/swapexp.* | head -1`;
    }

  } else {

    $err = ERR_FATAL;

  }

  $err = ERR_FAILED   if $err == ERR_NONE && $tally{failed} > 0;
  $err = ERR_EXPECTED if $err == ERR_NONE && $tally{expected} > 0;

  if ($parms{stages} =~ /e/ &&
      (!$parms{dont_swapout_unexpected} || $tally{failed} == 0)) {

    sys("cp -pr /proj/$pid/exp/$eid exp-data");
    if ($? >> 8 != 0) {
      print "*** Unable to copy exp data.  Not terminating exp\n";
      exit ($err | STATUS_CLEANUP);
    }

    sys("/usr/testbed/bin/endexp -w -e $pid,$eid");
    if ($? >> 8 != 0) {
      print "*** Could not terminate experiment.  Must do manually\n";
      exit ($err | STATUS_CLEANUP);
    }

    $status = STATUS_NONE;
  }

  if ($parms{stages} =~ /t/) {

    print "\n";
    print "Num Tests:           $tally{total}\n";
    print "Passed:              $tally{passed}\n";
    print "Expected Failures:   $tally{expected}\n";
    print "Unexpected Failures: $tally{failed}\n";
    my $unex = $tally{total} - $tally{passed} - $tally{expected} - $tally{failed};
    print "Unable to Execute:   $unex\n";

  }

  exit ($err | $status);
}






















