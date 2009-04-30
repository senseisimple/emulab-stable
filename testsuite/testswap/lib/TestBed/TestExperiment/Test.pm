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

sub tn {
  my ($pid, $gid, $eid, $desc, $ns, $sub) = @_;
  my $e = TestBed::TestSuite::Experiment->new(
      'pid' => $pid,
      'gid' => $gid,
      'eid' => $eid,
      'ns' => $ns);
  TestBed::TestExperiment::Test->new(
    'e' => $e, 
    'ns' => $ns,
    'desc' => $desc,
    'proc' => $sub );
}

sub prep {
  my $self = shift;
  my $a = $self->e->create_and_get_metadata($self->ns);
  #say Dumper($a);
  $a;
}

sub run {
  my $self = shift;
  my $e = $self->e;
  my $proc = $self->proc;
  $proc->($e);
}

sub kill {
  shift->e->end;
}

1;
