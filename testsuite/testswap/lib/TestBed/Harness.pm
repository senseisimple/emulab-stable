use SemiModern::Perl;
use TAP::Harness;
require Exporter;
our @ISA = qw(Exporter TAP::Harness);
our @EXPORT = qw(runharness);
use TestBed::TestSuite;

sub parser_args_callback {
  my $args = shift;
  my $ref = $args->{source};
  
  if (ref $ref and $ref->isa('TestBed::ParallelRunner')) {
    delete $args->{source};
    $args->{'stream'} = $ref->build_TAP_stream;
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
  for (@$pms) { eval "require \'$_\';"; }

  my %harness_args = (
      verbosity => 1,
      lib     => [ '.', 'lib', 'blib/lib' ],
      );

  my $harness = TAP::Harness->new( \%harness_args );
  $harness->callback('parser_args', \&parser_args_callback);
  push @$ts, TestBed::ParallelRunner->new() if @$pms;
  $harness->runtests(@$ts);
}

1;
