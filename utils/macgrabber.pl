#!/usr/bin/perl -w

#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#

use strict;

# macgrabber.pl, a new script to harvest MAC address (as printed out by Mike's
# 'whoami' kernel) from cature logs. It prints out SQL commands to insert the
# addresses to stdout. Change the values in the '#defines' section below
# depeding on the nodes you're harvesting info for.

if (@ARGV != 3) {
	die "Usage: $0 <start_node> <end_node> <start_file>\n";
}

my ($startNode, $endNode, $startFile) = @ARGV;

#
# '#defines' - Change depeding on the machines you're harvesting MACs for
#

# Number of cards (incl. control net) in each node.
my $numCards = 5;

# Interface type (from the interface_types) table of the cards
my $interfaceType = "fxp";

# Which of the interfaces (as printed by whoami) is the control net
my $controlInterface = 0;

# Subnet for the control net
my $subnet = "155.101.132.";

# Where the last octet of the nodes' IP addresses will start. For example,
# if pc1 is 155.101.132.1, this should be 1. If pc1 is 155.101.143.131, this
# should be 131.
my $base = 1;

# Mapping of interface numbers, as printed by the whoami kernel, to 
# cannonical database order. For example, on Utah's pc850's, what
# showmac reports as eth0 is saved as eth2 in the database (since this
# is its name under Linux 2.4.x . In general, the best way to get this
# mapping is to boot both the whoami kernel and a Linux kernel, and
# compare the ordering. There should be as many entries in this list
# as recorded in $numCards above.
my %cardmap = ( 0 => 2,
				1 => 3,
				2 => 4,
				3 => 0,
				4 => 1 );

# You can leave the following alone, unless you're sure you need to change them
my $iface = "eth";
my $port = 1;
my $IPalias = "NULL";
my $currentSpeed = 100;
my $duplex = "full";

# end of '#defines'

$startNode =~ /^(\D+)(\d+)$/;
my ($nodeType,$startNum) = ($1,$2);

if ((!defined $nodeType) || (!defined $startNum)) {
	die "Invalid start_node: $startNode\n";
}

$endNode =~ /^\D+(\d+)$/;
my $endNum = $1;

if (!defined $endNum) {
	die "Invalid end_node: $endNode\n";
}


$startFile =~ /^(\D*)(\d+)(\D*)$/;
my ($filePrefix,$fileNum,$fileSuffix) = ($1,$2,$3);

if (!defined $fileNum) {
	die "Invalid start_file: $startFile\n";
}

for (my $i = $startNum; $i <= $endNum; $i++) {
	my $filename = $filePrefix . $fileNum++ . $fileSuffix;
	my @MACs = parseMac($filename);
	if (!@MACs) {
		warn "No MACs found for ${nodeType}${i} in file $filename\n";
	} else {
		for (my $j = 0; $j < @MACs; $j++) {
			my $IP;
			if ($j == $controlInterface) {
				$IP = $subnet . ($base + $i -1);
			} else {
				$IP = "";
			}
			print qq|INSERT INTO interfaces VALUES ("${nodeType}${i}",$cardmap{$j}, | .
				qq|$port,"$MACs[$j]","$IP",$IPalias,"$interfaceType", | .
				qq|"${iface}$cardmap{$j}",$currentSpeed,"$duplex");\n|;
		}
	}
}

sub parseMac {
	my ($filename) = @_;

	if (!open(LOG,$filename)) {
		warn "Unable to open logfile $filename ... skipping\n";
		return ();
	}

	my @MACs = ();

	while (<LOG>) {
		chomp;
		if (/^\s*eth0\s+[\dabcdef]{12}?\s*$/) {
			my @tmpMACs = ();
			for (my $i = 0; $i < $numCards; $i++) {
				if (/^\s*eth\d\s+([\dabcdef]{12}?)\s*$/) {
					push @tmpMACs, $1;
					$_ = <LOG>;
				} else {
					warn "Incorrect or missing card MAC in file $filename\n";
					last;
				}
				if ($i == ($numCards -1)) {
					@MACs = @tmpMACs;
				}
			}
		}
	}
	return @MACs;
}
