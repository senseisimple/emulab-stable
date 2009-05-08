#!/usr/bin/perl
package TestBed::TestExperiment::Test;
use SemiModern::Perl;
use TestBed::TestSuite::Experiment;
use Mouse;
use Data::Dumper;

has 'e'    => ( isa => 'TestBed::TestSuite::Experiment', is => 'rw');
has 'desc' => ( isa => 'Str', is => 'rw');
has 'ns'   => ( isa => 'Str', is => 'rw');
has 'proc' => ( isa => 'CodeRef', is => 'rw');
has 'test_count'   => ( isa => 'Any', is => 'rw');

sub tn {
  my ($pid, $gid, $eid, $ns, $sub, $test_count, $desc) = @_;
  my $e = TestBed::TestSuite::Experiment->new(
      'pid' => $pid,
      'gid' => $gid,
      'eid' => $eid);
  return TestBed::TestExperiment::Test->new(
    'e' => $e, 
    'ns' => $ns,
    'desc' => $desc,
    'proc' => $sub,
    'test_count' => $test_count );
}

sub prep {
  my $self = shift;
  $self->e->create_and_get_metadata($self->ns);
}

sub run {
  my $self = shift;
  my $e    = $self->e;
  my $proc = $self->proc;
    $e->swapin_wait;
    $proc->($e);
}

sub run_ensure_kill {
  my $self = shift;
  eval {
    $self->run;
  };
  my $run_exception = $@;
  eval {
    $self->kill;
  };
  my $kill_exception = $@;
  die $run_exception if $run_exception;
  die $kill_exception if $kill_exception;
  return 1;
}

sub kill {
  my $self = shift;
  $self->e->end;
}

1;
