#!/usr/bin/perl -w

#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#

use strict;

# biosgrabber.pl, a new script to harvest BIOS version numbers (for BIOSen that
# print their version number to the serial console) from cature logs. It prints
# out SQL commands to update the database stdout. You'll want to change the
# parseVersion function, especially the regular expression in it, to match the
# BIOS version string printed by your machines' BIOS

if (@ARGV != 3) {
	die "Usage: $0 <start_node> <end_node> <start_file>\n";
}

my ($startNode, $endNode, $startFile) = @ARGV;

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
	my $version = parseVersion($filename);
	if (!$version) {
		warn "No MACs found for ${nodeType}${i} in file $filename\n";
	} else {
		print qq|UPDATE nodes SET bios_version='$version' | .
			qq|WHERE node_id=${nodeType}${i};\n|;
	}
}

sub parseVersion {
	my ($filename) = @_;

	if (!open(LOG,$filename)) {
		warn "Unable to open logfile $filename ... skipping\n";
		return ();
	}

	my $version = "";

	while (<LOG>) {
		chomp;
		# XXX: The following regexp is specific to the Intel ISP1100
		if (/\033\[.{4}(TR440BXA)\.(86B).(\d{4})\.(P\d{2})\.(\d{10})\033/) {
			$version = "$1.$2.$3.$4.$5";
		}
	}
	return $version;
}
