#!/usr/bin/perl

#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#

#
# Common functions used by the scripts in the assign test harness
#

#
# List all files in the ptop/ directory that match a certain pattern (passed
# to a shell). Return a list.
#
sub lsptop($) {
    my ($pattern) = @_;
    my @filenames = `ls -1 ptop/$pattern`;
    @filenames = map { `basename $_` } @filenames;
    chomp @filenames;
    return @filenames;
}

#
# Same for top/
#
sub lstop($) {
    my ($pattern) = @_;
    my @filenames = `ls -1 top/$pattern`;
    @filenames = map { `basename $_` } @filenames;
    chomp @filenames;
    return @filenames;
}

#
# Filter a list, returning on the elements for which the given fucntion
# returns true
#
sub filter(&@) {
    my ($fref, @list) = @_;
    my @rv = ();
    foreach my $elt (@list) {
        if (&$fref($elt)) {
            push @rv, $elt;
        }
    }
    return @rv;
}

#
# Parse a file describing some tests to run - really, it's just some perl code
# to execute that's expected to set certain variables.
#
sub readtestfile($) {
    my ($testfile) = @_;
    # Put variable set in this file in a different package (package declaration
    # ends when the function goes out of scope)
    package CFG;
    # This is sort of like 'source'
    do($testfile);
    die "Error parsing $testfile: $@\n" if $@;
}

#
# Set some standard global variables
#
sub setglobals() {
    $::testdir = "tests/$CFG::name";
    $::outdir = "$testdir/out";
    $::statdir = "$testdir/stat";
}

#
# Extract some info (node counts, link couts, etc.) about each top and ptop
# file used by the currently loaded test. Expects to find this data in some
# auxiliary files, which it generates if they don't already exist. Returns two
# hashes, one for top files, one for ptop files, indexed by filename
#
sub readfileinfo() {
    my (%topinfo, %ptopinfo);
    foreach my $typeinfo ((['top',\@CFG::tops, \%topinfo],
                           ['ptop',\@CFG::ptops, \%ptopinfo])) {
        my ($type, $list, $hashref) = @$typeinfo;

        foreach my $file (@$list) {
            $$hashref{$file} = {};

            my $nodecountfile = "$type/counts/$file.nodes";
            if (!-e $nodecountfile) {
                print "(re)Generating $nodecountfile for $type/$file\n";
                system "egrep -c '^node \\w+ pc' $type/$file > $nodecountfile";
            }
            chomp(my $nodecount = `cat $nodecountfile`);
            $$hashref{$file}{'nodes'} = $nodecount;

            my $linkcountfile = "$type/counts/$file.links";
            if (!-e $linkcountfile) {
                print "(re)Generating $linkcountfile for $type/$file\n";
                system "egrep -c '^link' $type/$file > $linkcountfile";
            }
            chomp(my $linkcount = `cat $linkcountfile`);
            $$hashref{$file}{'links'} = $linkcount;

        }
    }
    return (\%topinfo, \%ptopinfo);
}

#
# Parse a few critical details out of an assing log file. Assumes that we
# already know that the run succeeded.
#
sub parseassignlog($) {
    my ($logfile) = @_;
    my %assigninfo;

    open LFH, "<$logfile" or die "Unable to open $logfile: $!\n";
    while (my $line = <LFH>) {
        chomp $line;
        if ($line =~ /BEST SCORE:\s+(\d*\.*\d+) in (\d+) iters and (\d*\.*\d+) seconds/) {
            ($assigninfo{'bestscore'},$assigninfo{'iters'},
             $assigninfo{'runtime'} ) = ($1,$2,$3);
        }
    }
    if (!exists($assigninfo{'runtime'})) {
        warn "Unable to find runtime in $logfile";
    }

    return \%assigninfo;
}

#
# Given a top and a ptop file (name), return the name of the specific test
#
sub testname($$) {
    my ($top,$ptop) = @_;
    return "$top+$ptop";
}

#
# List all tests for the currently loaded config. We sort them using an
# alphanumeric sort so taht they will typically range from easiest to
# hardest
#
sub enumeratetests($) {
    my @tests;
    foreach my $top (@CFG::tops) {
        foreach my $ptop (@CFG::ptops) {
            push @tests, testname($top,$ptop);
        }
    }
    return (sort { alphanum($a,$b) } @tests);
}

#
# Returns true if the given test (already run) passed
#
sub passed($) {
    my ($test) = @_;
    if (-e passedfile($test)) {
        return 1;
    } else {
        return 0;
    }
}

#
# Extract the top or ptop name from a test name
#
sub top($) {
    my ($test) = @_;
    my ($top,$ptop) = split /\+/, $test;
    return $top;
}
sub ptop($) {
    my ($test) = @_;
    my ($top,$ptop) = split /\+/, $test;
    return $ptop;
}

#
# Conveneince functions to return names of files for various tests
#
sub statfile($) {
    my ($statname) = @_;
    return "$::statdir/$statname";
}

sub logfile($) {
    my ($testname) = @_;
    return "$::outdir/$testname.log";
}

sub passedfile($) {
    my ($testname) = @_;
    return "$::outdir/$testname.passed";
}

sub failedfile($) {
    my ($testname) = @_;
    return "$::outdir/$testname.failed";
}
sub topfile($) { return "top/" . top($_[0]); }
sub ptopfile($) { return "ptop/" . ptop($_[0]); }

#
# Takes a filename and a reference to a list (which it assumes is a 2-d array),
# sorts the list by the first column, and writes to the file. Perfect for
# producing output for gnuplot
#
sub write_sortedfile($$) {
    my ($fname, $arrayref) = @_;
    my @sorted = sort { $a[0] <=> $b[0] } @$arrayref;
    open FH, ">$fname" or die "Unable to open $fname: $!\n";
    print FH join("\n",(map { join "\t", @$_ } @sorted));
    print FH "\n";
    close FH;
}

#
# Stuff for alphanumeric sorting - from DaveKoelle.com
#

sub alphanum {
  # split strings into chunks
  my @a = chunkify($_[0]);
  my @b = chunkify($_[1]);
  
  # while we have chunks to compare.
  while (@a && @b) {
    my $a_chunk = shift @a;
    my $b_chunk = shift @b;
    
    my $test =
        (($a_chunk =~ /\d/) && ($b_chunk =~ /\d/)) ? # if both are numeric
            $a_chunk <=> $b_chunk : # compare as numbers
            $a_chunk cmp $b_chunk ; # else compare as strings  
    
    # return comparison if not equal.
    return $test if $test != 0;
  }

  # return longer string.
  return @a <=> @b;
}

# split on numeric/non-numeric transitions
sub chunkify {
  my @chunks = split m{ # split on
    (?= # zero width
      (?<=\D)\d | # digit preceded by a non-digit OR
      (?<=\d)\D # non-digit preceded by a digit
    )
  }x, $_[0];
  return @chunks;
}

1;
