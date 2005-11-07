package ImageTest;
use Exporter;

@ISA = "Exporter";
@EXPORT = qw (test test_cmd test_ssh test_rcmd test_experiment
	      ERR_NONE ERR_FAILED ERR_SWAPIN ERR_FATAL ERR_CLEANUP);
use IO::File;
use strict;

use vars qw(%parms %dependencies %tally);
use vars qw($eid $pid $datadir $resultsdir);
use vars qw(@mapping @nodes @pnodes %to_physical %from_physical);
use vars qw($FAILED);

# exit values
sub ERR_NONE    {0};
sub ERR_FAILED  {1}; # tests failed
sub ERR_SWAPIN  {2}; # swapin failed
sub ERR_FATAL   {3}; # fatal error
sub ERR_CLEANUP {4}; # fatal error - cleanup needed

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
  $tally{total}++;

  my $deps_sat = 1;
  foreach (@$requires) {$deps_sat = 0 unless $dependencies{$_}}

  unless ($deps_sat) {
    print "Skipping test \"$name\" due to unsatisfied dependies.\n";
    return 0;
  }

  my $res;
  eval {$res = &$test(%parms)};

  if ($res) {
    $tally{passed}++;
    $dependencies{$name} = 1;
    print "\"$name\" succeeded\n";
    return 1;
  } elsif ($@) {
    $tally{failed}++;
    print $FAILED "$name\n";
    print "*** \"$name\" died: $@";
    return 0;
  } else {
    $tally{failed}++;
    print $FAILED "$name\n";
    print "*** \"$name\" failed\n";
    return 0;
  }
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
    print "Executing: $cmd\n";
    if (not defined $output_test) {
      system $cmd;
      return ($? >> 8 == 0);
    } else {
      local $/ = undef;
      my $F = new IO::File;
      open $F, "$cmd |" or return 0;
      local $_ = <$F>;
      close $F;
      return 0 unless ($? >> 8 == 0);
      open $F, ">$resultsdir/$name.out";
      print $F $_;
      close $F;
      my $res = &$output_test;
      print "*** output of \"$cmd\" did not match expected output\n" unless $res;
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
  test_cmd "ssh-$node", [], "ssh  -o BatchMode=yes -o StrictHostKeyChecking=no $node.$parms{eid}.$parms{pid} true";
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
	    "ssh -o BatchMode=yes $node.$parms{eid}.$parms{pid} $cmd", $output_test);
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

  test_rcmd "sudo-$node", [], $node, 'sudo touch /afile.txt';

  test_rcmd "hostname-$node" , [], $node, 'hostname', sub {
    /^$node\.$eid\.$pid/i || /^$pnode/i;
  };

  test "login_prompt-$node", [], sub {
    local $_ = cat "/var/log/tiplogs/$pnode.run";
    /login\: /;
  };

}

#
#
#
sub multi_node_tests () {

  #test_cmd 'linktest', [], "run_linktest.pl -v -e $pid/$eid";

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
  %tally = (total => 0, passed => 0, failed => 0);

  $eid = $parms{eid};
  $pid = $parms{pid};
  $datadir = $parms{datadir};
  $resultsdir = $parms{resultsdir};
  my $exit = 0;

  $SIG{__DIE__} = sub {
    return unless defined $^S && !$^S;
    $! = ERR_FATAL;
    die $_[0];
  };

  mkdir $resultsdir, 0777;

  open STDOUT, ">$resultsdir/log" or die;
  open STDERR, ">&STDOUT"         or die;

  $FAILED = new IO::File ">$resultsdir/failed-tests" or die;

  my ($F,$O);

  $F = new IO::File ">$resultsdir/parms" or die;
  foreach (sort keys %parms) {
    print $F "$_: $parms{$_}\n";
  }

  if ($parms{stages} =~ /c/) {

    $F = new IO::File "$datadir/nsfile.ns" or die;
    $O = new IO::File ">$resultsdir/nsfile.ns" or die;
    while (<$F>) {
      s/\@([^@]+)\@/$parms{lc $1}/g;
      print $O $_;
    }
    close $O;
    close $F;

    system("/usr/testbed/bin/startexp -w -i -f".
	   " -E \"Experiment For Testing Images\"".
	   " -p $pid -e $eid $resultsdir/nsfile.ns");
    if ($? >> 8 != 0) {
      print "*** Could not create experment\n";
      exit ERR_FATAL;
    }
  }

  my $swapin_success = 1;
  if ($parms{stages} =~ /s/) {
    $swapin_success = test_cmd 'swapin', [],
      "/usr/testbed/bin/swapexp -w -e $pid,$eid in";
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
	  $exit = ERR_FATAL
	}
      }
    }

    if ($parms{stages} =~ /[oe]/) {

      test_cmd 'loghole', [], "loghole -e $pid/$eid sync";
	
      foreach my $node (@nodes) {
	my $pnode = $to_physical{$node};
	system "cp -pr /var/log/tiplogs/$pnode.run $resultsdir/tiplog-$node";
	if ($? >> 8 != 0) {
	  print "*** WARNING: Unable to copy tiplog for node $node.\n";
	}
      }

      test_cmd 'swapout', [],
	"/usr/testbed/bin/swapexp -w -e $pid,$eid out";

      # FIXME: need proper way to get the log file
      test_scanlog 'error_free_swapout', ['swapout'],
	`ls -t /proj/$pid/exp/$eid/tbdata/swapexp.* | head -1`;
    }

    if ($parms{stages} =~ /t/) {

      print "\n";
      print "Num Tests:         $tally{total}\n";
      print "Passed:            $tally{passed}\n";
      print "Failed:            $tally{failed}\n";
      my $unex = $tally{total} - $tally{passed} - $tally{failed};
      print "Unable to Execute: $unex\n";

    }

  } else {

    $exit = ERR_SWAPIN;

  }

  if ($parms{stages} =~ /e/) {

    system("cp -pr /proj/$pid/exp/$eid $resultsdir/exp-data");
    if ($? >> 8 != 0) {
      print "*** Unable to copy exp data.  Not terminating exp\n";
      exit ERR_CLEANUP;
    }

    system("/usr/testbed/bin/endexp -w -e $pid,$eid");
    if ($? >> 8 != 0) {
      print "*** Could not terminate experiment.  Must do manually\n";
      exit ERR_CLEANUP;
    }
  }

  $exit = ERR_FAILED if $exit == 0 && $tally{failed} > 0;
  exit $exit;
}






















