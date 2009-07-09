#!/usr/bin/perl
package TestBed::TestSuite;
use SemiModern::Perl;
use TestBed::TestSuite::Experiment;
use TestBed::ParallelRunner;
use TestBed::ForkFramework;
use TestBed::XMLRPC::Client::Node;
use Data::Dumper;
use Tools;

my $error_sub = sub {
  use Carp qw(longmess);
  say "Caught here " . __FILE__;
  say(@_);
  say longmess;
  die @_;
};
#$SIG{ __DIE__ } = $error_sub;

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT = qw(e CartProd CartProdRunner concretize defaults override rege runtests prun prunout get_free_node_names);

sub e { TestBed::TestSuite::Experiment->new(_build_e_from_positionals(@_)); }

sub rege {
  return TestBed::ParallelRunner::build_executor(@_);
}

sub runtests { TestBed::ParallelRunner::runtests(@_); }

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

sub concretize {
 Tools::concretize(shift, %{ override( { @_ }, %{ $TBConfig::cmdline_defines } ) } );
}

sub defaults {
  my ($params, %defaults) = @_;
  return { %defaults, %{($params || {})} };
}

sub override {
  my ($params, %overrides) = @_;
  return { %{($params || {})}, %overrides };
}

sub prun {
  my $results = TestBed::ForkFramework::ForEach::worksubs( @_);
  die ("prun item failed", $results)  if ($results->has_errors);
  return $results;
}

sub prunout {
  my $results = prun(@_);
  return $results->sorted_results;
}

sub get_free_node_names {
  TestBed::XMLRPC::Client::Node->new()->get_free_node_names(@_);
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

=item C<rege($e, $ns_contents, &test_sub, $test_count, $desc, %options)>

registers experiement with parallel test running engine

=item C<runtests($concurrent_pre_runs, $concurrent_node_count_usage) >

allows a maximum of $concurrent_pre_runs during parallel execution
allows a maximum of $concurrent_nodes during parallel execution

start the execution of parallel tests, not needed 

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

=item C<prun(@anonymous_funcs)>

executes anonymous funcs in parallel dying if any fail

=item C<prunout(@anonymous_funcs)>

executes anonymous funcs in parallel returning the output results

=back

=cut

1;
