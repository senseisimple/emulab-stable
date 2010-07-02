#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
package TestBed::Wrap::loghole;
use SemiModern::Perl;
use TBConfig;
use Data::Dumper;
use Tools;
use Tools::TBSSH;

=pod

loghole -e proj/expt [args ...]
=cut

=head1 NAME

TestBed::Wrap::loghole

=over 4

=item C<loghole($pid, $eid, $arg)>

executes loghole on $pid and $eid with $arg string such as "now link1 down"
by sshing to ops

=back

=cut

sub loghole {
  my ($e, @args) = @_;
  my ($pid, $eid) = ($e->pid, $e->eid);
  my $cmd = 'PATH=/usr/testbed/bin:$PATH loghole ' . "-e $pid/$eid " . join(" ", @args);
  #say $cmd;
  Tools::TBSSH::cmdsuccess($TBConfig::OPS_SERVER, $cmd);
}

1;
