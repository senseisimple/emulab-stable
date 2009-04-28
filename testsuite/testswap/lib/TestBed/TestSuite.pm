#!/usr/bin/perl
package TestBed::TestSuite;
use SemiModern::Perl;
use TestBed::TestSuite::Experiment;
use Data::Dumper;
use Tools;

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT = qw(e ep dpe dpge CartProd CartProdRunner concretize defaults);

=head1 NAME

TestBed::TestSuite

=over 4

=item C<ep()>

creates a new empty experiment, for calling experiement "class methods" on

=item C<e($pid, $eid)>

creates a new experiment with pid and eid

=item C<dpe($eid)>

new experiement takes one arg a eid and uses the default pid in TBConfig

=item C<CartProd($hashref)> Cartesian Product Runner

my $config = {
  'OS'       => [qw( AOS BOS COS )],
  'HARDWARE' => [qw( AHW BHW CHW )],
  'LINKTYPE' => [qw( ALT BLT CLT )],
};

returns [ { OS => 'AOS', HARDWARE => 'AHW', LINKTYPE => 'ALT' },
          { OS => 'BOS', HARDWARE => 'AHW', LINKTYPE => 'ALT' },
          ...
        ]


=item C<CartProdRunner($sub, $hashref)> Cartesian Product Runner

my $config = {
  'OS'       => [qw( AOS BOS COS )],
  'HARDWARE' => [qw( AHW BHW CHW )],
  'LINKTYPE' => [qw( ALT BLT CLT )],
};

CartProdRunner(\&VNodeTest::VNodeTest, $config);

=back

=cut

sub ep  { TestBed::TestSuite::Experiment->new }
sub e   { TestBed::TestSuite::Experiment->new('pid'=> shift, 'eid' => shift) }
sub dpe { TestBed::TestSuite::Experiment->new('pid'=> $TBConfig::DEFAULT_PID, 'eid' => shift) }
sub dpge {
    my $gid = (!defined($TBConfig::DEFAULT_GID) 
	       || $TBConfig::DEFAULT_GID eq '') 
	       ? $TBConfig::DEFAULT_PID : $TBConfig::DEFAULT_GID;
    TestBed::TestSuite::Experiment->new('pid' => $TBConfig::DEFAULT_PID,
					'gid' => $gid,
					'eid' => shift)
}

sub CartProd {
  my ($config, %options) = @_;
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
    @a = map { if ($options{'filter'}->($_)) {
      push @newa, $_;}
    } @a;
    @a = @newa;
  }

  if (exists $options{'generator'}) {
    @a = map &{$options{'generator'}}, @a;
  }
  @a;
}

sub CartProdRunner {
  my ($proc, $config, %options) = @_;
  for (CartProd($config, %options)) { $proc->($_); }
}

sub defaults {
  my ($params, %defaults) = @_;
  +{ %defaults, %{($params || {})} };
}
1;
