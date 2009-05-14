#!/usr/bin/perl
package TestBed::TestSuite;
use SemiModern::Perl;
use TestBed::TestSuite::Experiment;
use TestBed::ParallelRunner;
use Data::Dumper;
use Tools;

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT = qw(e CartProd CartProdRunner concretize defaults override rege runtests);

sub e { TestBed::TestSuite::Experiment->new(_build_e_from_positionals(@_)); }
sub rege {
  my $e;
  if (@_ == 4)    { $e = e(); }
  elsif (@_ == 5) { $e = e(shift); }
  elsif (@_ == 6) { $e = e(shift, shift); }
  elsif (@_ == 7) { $e = e(shift, shift, shift); }
  else { die 'Too many args to rege'; }
  return TestBed::ParallelRunner::add_experiment($e, @_);
}
sub runtests { TestBed::ParallelRunner::runtests; }


sub _build_e_from_positionals {
  if (@_ == 0) { return {}; }
  if (@_ == 1) { return { 'eid' => shift }; }
  if (@_ == 2) { return { 'pid' => shift, 'eid' => shift }; }
  if (@_ == 3) { return { 'pid' => shift, 'gid' => shift, 'eid' => shift }; }
  if (@_ >  3) { die 'Too many args to e'; }
}

sub CartProd {
  my $config = shift;
  my %options;
  if (@_ == 1) {
    $options{'filter'} = $_[0];
  }
  else {
    %options = @_;
  }

  #say Dumper($config);
  my @a;
  while (my ($k, $v) = each %$config) {
    if ( @a ) {
      my @b;
      #say "induct";
      map {
        my $c = {$k => $_};
        push @b, map {{ %$_, %$c}} @a;
      } @$v;
      #say Dumper(\@b);
      @a = @b;
    }
    else {
      #say "basis";
      @a =  map {{$k => $_}} @$v;
      #say Dumper(\@a);
    }
  }

  if (exists $options{'filter'}) {
    my @newa;
    @a = map { 
      my $result = $options{'filter'}->($_);
      if ( defined $result ) {
        if ( ref $result ) {
          push @newa, $result;
        }
        elsif ( $result ) {
          push @newa, $_;
        }
      }
    } @a;
    @a = @newa;
  }

  if (exists $options{'generator'}) {
    @a = map &{$options{'generator'}}, @a;
  }
  @a;
}

sub CartProdRunner {
  my $proc = shift;
  for (CartProd(@_)) { $proc->($_); }
}

sub defaults {
  my ($params, %defaults) = @_;
  return { %defaults, %{($params || {})} };
}

sub override {
  my ($params, %overrides) = @_;
  return { %{($params || {})}, %overrides };
}

=head1 NAME

TestBed::TestSuite

=over 4

=item C<e()>
creates a new empty experiment, for calling experiement "class methods" on

=item C<e($eid)>

creates a new experiment with eid and uses the default pid and gid in TBConfig

=item C<e($pid, $eid)>

creates a new experiment with pid and eid and uses the default gid in TBConfig

=item C<e($pid, $gid, $eid)>

creates a new experiment with pid, gid, and eid

=item C<CartProd($hashref)> Cartesian Product Runner

=item C<CartProd($hashref, &filter_gen_func)> Cartesian Product Runner

my $config = {
  'OS'       => [qw( AOS BOS COS )],
  'HARDWARE' => [qw( AHW BHW CHW )],
  'LINKTYPE' => [qw( ALT BLT CLT )],
};

returns [ { OS => 'AOS', HARDWARE => 'AHW', LINKTYPE => 'ALT' },
          { OS => 'BOS', HARDWARE => 'AHW', LINKTYPE => 'ALT' },
          ...
        ]

takes an optional sub ref that acts as a filter
if &filter_gen_func returns undef or 0 the case is dropped from the result.
if &filter_gen_func returns a hash ref the has ref becomes the new case resutl.

=item C<CartProdRunner($sub, $hashref)> Cartesian Product Runner

my $config = {
  'OS'       => [qw( AOS BOS COS )],
  'HARDWARE' => [qw( AHW BHW CHW )],
  'LINKTYPE' => [qw( ALT BLT CLT )],
};

CartProdRunner(\&VNodeTest::VNodeTest, $config);

=item C<defaults($hashref, %defaults)> provides default hash entries for a params hash ref

returns a modified hash ref

=item C<override($hashref, %overrides)> provides hash entry overrides for a params hash ref

returns a modified hash ref

=back

=cut

1;
