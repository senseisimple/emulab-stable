#!/usr/bin/perl

$proj = $ARGV[0];
$exp = $ARGV[1];

die "usage: $0 PROJ EXP\n" unless @ARGV == 2;

sub psystem(@) {
  print join(' ', @_);
  print "\n";
  system(@_);
  die unless $? == 0;
}

# reset the links
# wait only on the reset event
psystem("/usr/testbed/bin/tevc -e $proj/$exp now elabc clear");
psystem("/usr/testbed/bin/tevc -w -e $proj/$exp now elabc reset");
psystem("/usr/testbed/bin/tevc -e $proj/$exp now elabc create");

