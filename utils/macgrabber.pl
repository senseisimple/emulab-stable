#!/usr/bin/perl -w
use strict;

if (@ARGV != 3) {
	die "Usage: $0 <start_node> <end_node> <start_file>\n";
}

my ($startNode, $endNode, $startFile) = @ARGV;

# '#defines' - Change depeding on the machines you're harvesting MACs for
my $numCards = 5;

my $interfaceType = "fxp";
my $iface = "eth";
my $port = 1;
my $IPalias = "NULL";
my $currentSpeed = 100;
my $duplex = "full";

my $controlInterface = 0;
my $subnet = "155.101.132.";

my %cardmap = ( 0 => 2, 1 => 3, 2 => 4, 3 => 0, 4 => 1);

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
				$IP = $subnet . $i;
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
