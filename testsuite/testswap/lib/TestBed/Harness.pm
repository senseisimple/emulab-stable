#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
package TAP::Parser::Iterator::StdOutErr;
use strict;
use warnings;
use vars qw($VERSION @ISA);

use TAP::Parser::Iterator::Process ();
use IO::Select;

@ISA = 'TAP::Parser::Iterator::Process';

sub _initialize {
    my $self = shift;
    $self->{out}        = shift || die "Need out";
    $self->{err}        = shift || die "Need err";
    $self->{sel}        = IO::Select->new( $self->{out}, $self->{err} );
    $self->{pid}        = shift || die "Need pid";
    $self->{exit}       = undef;
    $self->{chunk_size} = 65536;
    return $self;
}

package TestBed::Harness;
use SemiModern::Perl;
use TAP::Harness;
require Exporter;
our @ISA = qw(Exporter TAP::Harness);
our @EXPORT = qw(runharness);
use TestBed::TestSuite;
use TestBed::ForkFramework;
use TestBed::ParallelRunner;

sub build_TAP_stream {
  use TestBed::TestSuite;
  my ($in, $out, $err, $pid) = TestBed::ForkFramework::fork_child_redir(sub { $TestBed::ParallelRunner::GlobalRunner->runtests; });
  return TAP::Parser::Iterator::StdOutErr->new($out, $err, $pid);
}

sub parser_args_callback {
  my $args = shift;
  my $ref = $args->{source};
  
  if (ref $ref and $ref->isa('TestBed::ParallelRunner')) {
    delete $args->{source};
    $args->{'stream'} = build_TAP_stream;
  }
  $args;
}

sub split_t_pm {
  my @t;
  my @pm;
  map {
    if (/\.pm$/) { push @pm, glob($_); }
    elsif (/\.t$/) { push @t, glob($_); }
  } @_;
  (\@t, \@pm);
}

sub runharness {
  my @parts = my ($ts, $pms) = split_t_pm(@_);
  for (@$pms) { 
    eval "require \'$_\';"; 
    die $@ if $@;
  }

  my %harness_args = (
      verbosity => 1,
      lib     => [ '.', 'lib', 'blib/lib' ],
      );

  my $harness = TAP::Harness->new( \%harness_args );
  $harness->callback('parser_args', \&parser_args_callback);
  push @$ts, [TestBed::ParallelRunner->new(), 'Parallel Tests'] if @$pms;
  $harness->runtests(@$ts);
}


=head1 NAME

TestBed::Harness

=over 4

=item C<< build_TAP_stream >>

given a TestBed::ParallelRunner returns a TAP stream

=item C<< runharness(@test_file_names) >>

ex. runharness( 't/lib/*.t', 't/xmlrpc/*.t' 'test/BasicTopologies.pm' ) 

runs the specified test in a TAP::harness
pushes a ParallelRunner on if .pm parallel test modules are specified

=item C<< split_t_pm(@test_file_names) >>

splits test filenames into to lists based on .t and .pm extensions

=item C<< parser_args_callback >>

TAP::Harness parser_args callback that allow for special processing (pre_running) of parallel tests

=back

=cut

1;
