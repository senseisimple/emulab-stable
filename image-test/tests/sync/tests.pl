#
# EMULAB-COPYRIGHT
# Copyright (c) 2005-2006 University of Utah and the Flux Group.
# All rights reserved.
#

sub compare_file($@) {
  my $n = shift; 
  my $f = new IO::File "/proj/$pid/exp/$eid/tmp/node${n}res" or
      die "*** Unable to open result file for node$n.\n";
  while (<$f>) {
    chop;
    my $expected = shift;
    die "*** Results file for node$n did not match expected output\n"
	unless $_ eq $expected;
  }
  die "*** Results file for node$n did not match expected output\n"
      if <$f>;
  return 1;
}

print "Sleeping 45 seconds...\n";
sleep 45;

test 'sync1', [], sub {
  compare_file 0, 0,1,2;
  compare_file 1, 0,1,2;
  compare_file 2, 0,1,2;
};

print "Sleeping 30 seconds...\n";
sleep 30;

test 'sync2', [], sub {
  compare_file 3, 3,4;
  compare_file 4, 3,4;
};


