#!/usr/bin/perl

use warnings;
use strict;

use POSIX qw(WIFEXITED WEXITSTATUS);

my $prefix=$ENV{PREFIX};
$prefix =~ s/\/$//;
my $perl_files_file = "$prefix/etc/daikonize.lst";
my $envsetup=$ENV{ENVSETUP};

# MODES:
#   stage1
#   stage2
#   analyze
#   restore -- restore the state to a non-daikonzes state
#   clean

# The real perl script is renamed to "*.real.pl"

# The return code is 
#   0 - No errors (but some files may have been skipped)
#   1 - Non-fatel errors
#   2 - Fatel errors, should not continue 

sub dir_file ($) {$_[0] =~ /^(.+)\/(.+)$/; return ($1,$2);}
sub lchdir (;$) {chdir $prefix; chdir $_[0] if defined $_[0];}

sub create_shell_wrapper ( $$$$$ );
sub in_stage ( $$ ) ;
sub restore ();
sub my_system ( $$ );

#
# These global variables and functions are used for error reporting
#

my $exit = 0;
our $path;
my (@skipped, @failed, @fatel, @success);

sub skip ( $ );  # File skipped
sub error ( $ ); # Error while processing file
sub fatel ( $ ); # Fatel error
sub check_fatel ( ;& );
sub check_exit (); # Report errors and exit

#
#
#

my @perl_files;
open F, $perl_files_file or die "Unable to open $perl_files_file.\n";
while (<F>) {
  chop;
  # FIXME: Check that each file contains a directory part and
  #        is not an absolute path.
  push @perl_files, $_;
}

my %dirs;
foreach (@perl_files) {my ($d,$f) = dir_file $_; $dirs{$d} = 1;}
my @dirs = sort keys %dirs;

my $mode = $ARGV[0] || '';

if ($mode eq "stage1") 
{
  lchdir;
  foreach $path (@perl_files) {
    eval {
      fatel "File \"$path\" already in a daikonized state.  Run \"daikonize restore\" first."
          if -e "$path.real.pl";
    };
    print STDERR $@ if (defined $@);
  }
  check_fatel;
  foreach $path (@perl_files) { 
    eval {
      my ($d,$f) = dir_file $path;
      lchdir $d;
      skip "$d/$f doesn't exist, skipping." unless -e $f;
      rename $f, "$f.real.pl" or fatel "Unable to rename $f to $f.real.pl";
      my_system "dfepl", "dfepl --absolute --dtrace-append $f.real.pl";
      create_shell_wrapper $d, $f, 'STAGE1', 'dtype-perl', 'daikon-untyped';
      push @success, $path; 
   };
    print STDERR $@ if (defined $@);
  }
  check_fatel {restore};
  foreach my $d (@dirs) {
    foreach (qw(daikon-instrumented daikon-output daikon-untyped)) {
      mkdir "$d/$_";
      chmod 0777, "$d/$_";
    }
  }
  check_exit;
}
elsif ($mode eq "stage2")
{
  my @files;
  foreach $path (@perl_files) {
    eval {
      my ($d,$f) = dir_file $path;
      lchdir $d;
      skip "File \"$path\" not instrumented, skipping."
          unless -e "daikon-instrumented/$f.real-main.types";
      fatel "File \"$path\" not in stage1."
          unless in_stage $f, 1;
      push @files, $path;
    };
    print STDERR $@ if (defined $@);
  }
  check_fatel;
  my @files2;
  foreach $path (@files) {
    eval {
      my ($d,$f) = dir_file $path;
      lchdir $d;
      my_system "dfepl -T", "dfepl --absolute --dtrace-append -T $f.real.pl";
      push @files2, $path;
    };
    print STDERR $@ if (defined $@);
  }
  check_fatel;
  foreach $path (@files2) {
    eval {
      my ($d,$f) = dir_file $path;
      lchdir $d;
      create_shell_wrapper $d, $f, 'STAGE2', 'dtrace-perl', 'daikon-instrumented';
      push @success, $path;
    };
    print STDERR $@ if (defined $@);
  }
  check_exit;
}
elsif ($mode eq "analyze")
{
  my @files;
  lchdir;
  foreach $path (@perl_files) {
    eval {
      my ($d,$f) = dir_file $path;
      lchdir $d;
      skip "Warning: File \"$path\" not instrumented, skipping."
          unless -e "daikon-output/$f.real-combined.dtrace";
      fatel "File $path not in stage2."
          unless in_stage $f, 2;
      push @files, $path;
    };
    print STDERR $@ if (defined $@);
  }
  check_fatel;
  foreach $path (@files) {
    eval {
      my ($d,$f) = dir_file $path;
      lchdir $d;
      chdir 'daikon-output';
      my_system "java daikon.Daikon", "java daikon.Daikon --omit_from_output 0r -o $f.inv $f.real-main.decls $f.real-combined.dtrace";
      push @success, $path;
    };
    print STDERR $@ if (defined $@);
  }
  check_exit;
}
elsif ($mode eq "restore")
{
  restore;
  exit 0;
}
elsif ($mode eq "clean")
{
  restore;
  lchdir;
  foreach (@perl_files) {
    my ($d,$f) = dir_file $_;
    system "rm -f $d/daikon-*/$f.*; rm -f $d/daikon-*/$f-*";
  }
  foreach (@dirs) {
    system "rmdir $_/daikon-*";
  }
  exit 0;
}
else
{
  print STDERR "Usage: daikonize stage1|stage2|analyze|restore|clean\n";
  exit 1;
}

sub restore ()
{
  lchdir;
  foreach my $f (@perl_files) {
    return unless -e "$f.real.pl";
    rename "$f.real.pl", $f or print STDERR "Warning: Unable to restore file $f\n";
  }
}

sub create_shell_wrapper ($$$$$)
{
  my ($d, $f, $stage, $prog, $ed) = @_;
  open F, "$f.real.pl";
  $_ = <F>;
  my ($perl_ops) = /^\#\!.+\/perl (.+)/;
  open F, ">$f" or fatel "Unable to create file $f\n";
  print F "#!/bin/bash\n";
  print F "# $stage\n\n";
  print F ". $envsetup\n\n";
  print F "exec $prog $perl_ops $prefix/$d/$ed/$f.real.pl \"\$@\"\n";
  close F;
  chmod 0755, "$f";
}

sub in_stage ($$)
{
  my ($f, $stage) = @_;
  open F, "$f" or return 0;
  <F>;
  local $_ = <F>;    
  /^\# STAGE(.)/ or return 0;
  return $1 == $stage;
}

sub my_system ($$) {
  my ($c,$cmd) = @_;
  print STDERR "Running \"$c\" for $path.\n";
  system $cmd;
  fatel "\"$c\" failed for $path" unless WIFEXITED($?);
  error "\"$c\" failed for $path" unless WEXITSTATUS($?) == 0;
}

#
#
#

sub file_error ($$) {
  my ($e, $msg) = @_;
  if ($e > $exit) {$exit = $e}
  if ($e == 0) {
    push @skipped, $path;
    die "WARNING: $msg\n";
  } if ($e == 1) {
    push @failed, $path;
    die "ERROR: $msg\n";
  } else {
    push @fatel, $path;
    die "FATEL: $msg\n";
  }
}

sub skip ($) {file_error 0, $_[0];}
sub error ($) {file_error 1, $_[0];}
sub fatel ($) {file_error 2, $_[0];}

sub print_file_list ($@) {
  my ($msg, @files) = @_;
  return 0 unless (@files);
  print STDERR "$msg:\n";
  foreach my $f (sort @files) {
    print STDERR "  $f\n";
  }
}

sub check_fatel (;&) {
  my ($clean_up) = @_;
  return unless $exit >= 2;
  if ($clean_up) {&$clean_up}
  print_file_list "ERROR: There were fatel errors while processing the following files", @fatel; 
  print STDERR "ERROR: Exiting.\n";
  exit $exit;
}

sub check_exit () {
  check_fatel;
  print_file_list "WARNING: The following files where skipped", @skipped;
  print_file_list "WARNING: Unable to process the following files due to errors", @failed;
  if (@success) {
    print_file_list "The following files were successfully processed", @success;
    exit $exit;
  } else {
    print STDERR "ERROR: Unable to successfully process any files.\n" unless @success;
    exit 2;
  }
}

