#!/usr/bin/perl -w

if (@ARGV != 10) {
	die "Usage: $0 <start_cable> <end_cable> <type> <length> <start_node> <switch_name> <start_module> <start_port> <min_port> <max_port>\n";
}

# Grab arguments
my ($start_cable,$end_cable, $type, $length, $start_node, $switch_name,
	$start_module, $start_port, $min_port, $max_port) = @ARGV;

# Different cable types have slightly different numbering schemes - mainly,
# the number of wires per node. $card_start hold the base card number (eg.
# experimental wires connect to eth0-eth3, control network wires connect to 
# eth4) 
my ($wires_per_node, $card_start);
if ($type eq "Node") {
	$wires_per_node = 4;
	$card_start = 0;
	# Since the card numbers (as probed at boot time) may not correspond
	# to the left-to-right order we're using for cable numbers, we
	# use this map to match to reality
	# FIXME: Placeholder until we get the real thing figured out
	%cardmap = ( 0 => 0, 1 => 1, 2 => 2, 3 => 3);
} elsif ($type eq "Control") {
	$wires_per_node = 1;
	$card_start = 4;
} else {
	die "Sorry, $type is not a supported wire type\n";
}

# Parse the start_node they gave us
$start_node =~ /^(\D+)(\d+)$/;
my $node_type = $1;
my $node_num = $2 +1; # Will be decremented the first the through the for loop

if ((!$node_type) || (!$node_num)) {
	die "Invalid start node: $start_node\n";
}

my $switch_module  = $start_module;
my $switch_port = $start_port;

for (my $i = 0; $i <= ($end_cable - $start_cable); $i++ ) {

	my $cable = $start_cable + $i;

	# We assume that you start wiring from the bottom of the rack -
	# change the next line to $node_num++ if wiring from the top
	if (!($i % $wires_per_node)) {
		$node_num--;
	}
	my $node1 = $node_type . $node_num;

	# Get the card number
	my $card1;
	if (! defined %cardmap) {
		$card1 = $card_start + $i % $wires_per_node;
	} else {
		$card1 = $cardmap{$i % $wires_per_node} + $card_start;
	}
	my $port1 = 1;

	my $node2 = $switch_name;
	my $card2 = $switch_module;
	my $port2 = $switch_port;

	# Wrap around to the next module on the switch if necessary 
	$switch_port++;
	if ($switch_port > $max_port) {
		$switch_module++;
		$switch_port = $min_port;
	}

	print qq|INSERT INTO wires VALUES ($cable,$length,"$type","$node1",| .
		qq|"$card1",$port1,"$node2", $card2, $port2);\n|;

}
