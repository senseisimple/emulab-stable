#!/usr/bin/perl -w

#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#


#
# fillwires.pl - script to fill out the wires table for testbed nodes
#

#
# '#defines' - Edit these, depending on your nodes
#

# At Utah, we have lower numbered wires on the bottom of the rack: so, as
# wire numbers increment, node numbers decrement. If you have the opposite,
# change this variable to +1;
$node_increment = -1;

# The number of experimental interfaces on each node
$experimental_interfaces = 4;

# Since the wires on the back of the node may not correspond directly to the
# actual interface numbering (ie. the lowest numbered wire may not be eth0),
# we have the map below. In the example below, the lowest numbered wire on each
# node corresponds to eth3, the next to eth4, etc. There should be as many
# enries in this table as the number in experimental_interfaces variable above.
# Note that on the right side, there is no 2. This is because, with these nodes,
# eth2 is the control net.
%cardmap = ( 0 => 3,
	     1 => 4,
	     2 => 0,
	     3 => 1);

# The interface number of the control net
$control_net = 2;

#
# end of defines
#

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
	$wires_per_node = $experimental_interfaces;
	if ($control_net == 0) {
	    $card_start = 1;
	} else {
	    $card_start = 0;
	}
} elsif ($type eq "Control") {
	$wires_per_node = 1;
	$card_start = $control_net;

	# We don't use the cardmap for the control net
	undef %cardmap;
} else {
	die "Sorry, $type is not a supported wire type\n";
}

# Parse the start_node they gave us
$start_node =~ /^(\D+)(\d+)$/;
my $node_type = $1;
my $node_num = $2 - $node_increment; # Will be decremented the
				     # first the through the for loop

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
		$node_num += $node_increment;
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
