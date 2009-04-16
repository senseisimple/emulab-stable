#!/usr/bin/perl
package TestBed::TestSuite;
use SemiModern::Perl;
use TestBed::TestSuite::Experiment;
use Data::Dumper;

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT = qw(e ep dpe);

=head1 NAME

TestBed::TestSuite

=over 4

=item C<ep()>

creates a new empty experiment, for calling experiement "class methods" on

=item C<e($pid, $eid)>

creates a new experiment with pid and eid

=item C<dpe($eid)>

new experiement takes one arg a eid and uses the default pid in TBConfig

=back

=cut

sub ep  { TestBed::TestSuite::Experiment->new }
sub e   { TestBed::TestSuite::Experiment->new('pid'=> shift, 'eid' => shift) }
sub dpe { TestBed::TestSuite::Experiment->new('pid'=> $TBConfig::DEFAULT_PID, 'eid' => shift) }

1;
