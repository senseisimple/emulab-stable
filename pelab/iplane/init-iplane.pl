#!/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2007 University of Utah and the Flux Group.
# All rights reserved.
#

use Socket;

# node_mapping is the reverse mapping of the one in init-elabnodes.
# The keys are the PlanetLab node names. The values are the Emulab node names.
my %node_mapping = ();

my $eprefix = "elab-";
my $pprefix = "planet-";

$pid = $ARGV[0];
$eid = $ARGV[1];

@nodelist = split('\s+', `cat /proj/$pid/exp/$eid/tmp/node_list`);
chomp(@nodelist);

foreach my $mapping (@nodelist) {
    if ($mapping =~ /^(${pprefix}[\d]+)=([\w]*)$/) {
	# vnode is the virtual node name in emulab.
	my $vnode = $1;
	# pnode is the planetlab node name.
	my $pnode = $2.".emulab.net";
	$vnode =~ s/${pprefix}/${eprefix}/;
	$pnode = gethostbyname($pnode);
	$pnode = inet_ntoa($pnode);
	$node_mapping{$pnode} = $vnode
    }
}

$command = "./query_iplane_client iplane.cs.washington.edu 1 iplane_pairwise.rb \""
           .join(" ", keys(%node_mapping))."\"";

print $command."\n";

@replylist = `$command`;

print "$pid $eid\n";

foreach my $reply (@replylist) {
    if ($reply =~ /\s*source=([0-9.]*)\s*dest=([0-9.]*)\s*latency=([0-9.]*)\s*/) {
	my $source = $1;
	my $dest = $2;
	my $latency = $3;
	print $node_mapping{$source}." ".$node_mapping{$dest}." "."10000"." ".($latency/2)." 0.0\n";
	print $node_mapping{$dest}." ".$node_mapping{$source}." "."10000"." ".($latency/2)." 0.0\n";
    }
}
