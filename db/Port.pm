#
# This is the Port class that represent a port in VLAN.
# It aims at providing all avaialble information of a
# port rather than just a string representation consisting
# of node and card or port only, which is used by most snmpit
# code.
#
# Previous string representation only has node and card for
# a PC port, which can not be unique when representing a port
# on a test node switch, such as our hp5406 nodes.
# A Port class instance uniquely identifies a port with
# all of its properties, such as node_id, card, port and iface.
#
# Some important terms used:
# - Tokens: seperated fields of a port, e.g. "node1", "card2", "iface3", etc.
# - Iface:  interface on a node, or a full representation of
#   an interface, say "iface2", "node1:iface4".
# - Triple: a port representation that uses three fields:
#   node, card and port, like "node1:card2.port1".
# - String: a string representation of a port, ususally a
#   combination of tokens, e.g. "node1:iface3", "node1:card3.port4".
#
# All code that needs to convert between different representations
# or merely parse tokens from string and vice-verse must
# use the converters provided in this class.
#
# EMULAB-COPYRIGHT
# Copyright (c) 2011 University of Utah and the Flux Group.
# All rights reserved.
#
package Port;

use strict;
use Exporter;
use vars qw(@ISA @EXPORT);

@ISA    = "Exporter";
@EXPORT = qw ( );

use libdb;
use English;
use Data::Dumper;

# Cache of port instances
# node:iface OR node:card.port => Port Instance
my %allports = ();

# Ends of wires, node:iface OR node:card.port => Port Instance
my %wiredports = ();

#
# check if a var is a Port instance
#
sub isPort($$)
{
	my ($c, $p) = @_;
	if (ref($p) eq "Port") {
		return 1;
	}
	return 0;
}

#
# Get the other end port of a wire by triple representation of this end port
# the classname can be ignored
# the representation can be a triple-port string or triple tokens
#
sub GetOtherEndByTriple($;$$$)
{
    my ($c, $node, $card, $port) = @_;
    my $str;

    if (!defined($node)) {
	$str = $c;
    } elsif (!defined($card)) {
	$str = $node;
    } elsif (!defined($port)) {
	$str = Port->Tokens2TripleString($c, $node, $card);
    } else {
	$str = Port->Tokens2TripleString($node, $card, $port);
    }

    if (exists($wiredports{$str})) {
	return $wiredports{$str};
    }

    my $p = Port->LookupByTriple($str);
    if (defined($p)) {

	$wiredports{$p->toTripleString()} = $p->getOtherEndPort();
	$wiredports{$p->toIfaceString()} = $p->getOtherEndPort();

	return $p->getOtherEndPort();
    } else {
	return undef;
    }
}

#
# Get the other end port of a wire by iface representation of this end port
# the classname can be ignored
# the representation can be a iface-port string or iface tokens
#
sub GetOtherEndByIface($;$$)
{
    my ($c, $node, $iface) = @_;
    my $str;
    my $p;
    
    if (defined($iface)) {
	$str = Port->Tokens2IfaceString($node, $iface);
    } elsif (!defined($node)) {
        $str = $c;
    } else {
        $str = $node;
        $p = Port->LookupByIface($str);
        if (!defined($p)) {
            $str = Port->Tokens2IfaceString($c, $node);
            $p = Port->LookupByIface($str);
            if (!defined($p)) {
                return undef;
            }
        }
    }
    
    if (!defined($p)) {
    	$p = Port->LookupByIface($str);
    }
    if (defined($p)) {
    
    	$wiredports{$p->toTripleString()} = $p->getOtherEndPort();
	$wiredports{$p->toIfaceString()} = $p->getOtherEndPort();

	return $p->getOtherEndPort();
    } else {
	return undef;
    }
}    


#
# Parse node:iface string into tokens
# classname can be ignored
#
sub ParseIfaceString($;$)
{
    my ($c, $striface) = @_;

    if (!defined($striface)) {
	$striface = $c;
    }

    if ($striface =~ /^(.+):(.+)$/) {
	return ($1, $2);
    }

    return (undef, undef);
}

#
# Parse node:card.port string into tokens
# can be called without the classname
#
sub ParseTripleString($;$)
{
    my ($c, $triplestring) = @_;

    if (!defined($triplestring)) {
	$triplestring = $c;
    }

    if ($triplestring =~ /^(.+):(.+)\.(.+)$/) {
	return ($1, $2, $3);
    }

    return (undef, undef, undef);
}

sub ParseCardPortString($;$)
{
    my ($c, $cp) = @_;

    if (!defiend($cp)) {
	$cp = $c;
    }

    # Should not include all fields
    if ($cp =~ /^(.+):(.+)[\/\.](.+)$/) {
	return (undef, undef);
    }

    if ($cp =~ /^(.+)\.(.+)$/ || $cp =~ /^(.+)\/(.+)$/) {
	return ($1, $2);
    }

    return (undef, undef);
}

sub Iface2Triple($;$)
{
    my ($c, $striface) = @_;

    if (!defined($striface)) {
	$striface = $c;
    }

    if (exists($allports{$striface})) {
	return $allports{$striface}->toTripleString();
    } else {
	my ($nodeid, $iface) = ParseIfaceString($striface);
	if (!defined($iface)) {
	    return undef;
	}

	my $port = Port->LookupByIface($nodeid, $iface);
	if (defined($port) && $port != 0 && $port != -1) {
	    return $port->toTripleString();
	} else {
	    return undef;
	}
    }
}

sub Triple2Iface($;$)
{
    my ($c, $strtriple) = @_;

    if (!defined($strtriple)) {
	$strtriple = $c;
    }

    if (exists($allports{$strtriple})) {
	return $allports{$strtriple}->toIfaceString();
    } else {
	my ($nodeid, $card, $port) = ParseTripleString($strtriple);
	if (!defined($card) || !defined($port) || !defined($nodeid)) {
	    return undef;
	}

	my $portInst = Port->LookupByTriple($nodeid, $card, $port);
	if (defined($portInst && $port != 0 && $port != -1)) {
	    return $portInst->toIfaceString();
	} else {
	    return undef;
	}
    }
}

sub Tokens2TripleString($$$;$)
{
    my ($c, $nodeid, $card, $port) = @_;

    if (!defined($port)) {
	$port = $card;
	$card = $nodeid;
	$nodeid = $c;
    }

    return "$nodeid:$card.$port";
}

sub Tokens2IfaceString($$;$)
{
    my ($c, $nodeid, $iface) = @_;

    if (!defined($iface)) {
	$iface = $nodeid;
	$nodeid = $c;
    }

    return "$nodeid:$iface";
}


# functions started with fake_ are used to handle
# a special 'node:card/port' representation.

sub fake_CardPort2Iface($$;$)
{
    my ($cn, $c, $p) = @_;

    if (!defined($p)) {
	$p = $c;
	$c = $cn;
    }
    return "$c/$p";
}

sub fake_TripleString2IfaceString($;$)
{
    my ($cn, $t) = @_;

    if (!defined($t)) {
	$t = $cn;
    }

    my ($n, $c, $p) = ParseTripleString($t);
    if (!defined($n) || !defined($c) || !defined($p)) {
        return undef;
    }

    return "$n:".fake_CardPort2Iface($c, $p);
}

sub fake_IfaceString2TripleTokens($;$)
{
    my ($cn, $i) = @_;

    if (!defined($i)) {
	$i = $cn;
    }

    my ($n, $iface) = ParseIfaceString($i);
    
    if (defined($iface) && $iface =~ /^(.+)\/(.+)$/) {
	return ($n, $1, $2);
    }    

    return (undef, undef, undef);
}

#
# Class method
# Find the port instance by a given string, if no port found from DB, 
# make a fake one.
#
# This is useful when a port string is derived from the switch, but
# it is not stored in DB. (eg. listing all ports on a switch)
#
sub LookupByStringForced($$)
{
    my ($class, $str) = @_;
    my $inst = {};
    
    my $p = Port->LookupByIface($str);
    if (defined($p)) {
	return $p;
    }
    else {
	$p = Port->LookupByTriple($str);
	if (defined($p)) {
	    return $p;
	}
    }
    
    my $iface;
    my ($nodeid, $card, $port) = Port->ParseTripleString($str);
    if (!defined($port)) {
	($nodeid, $card, $port) = Port->fake_IfaceString2TripleTokens($str);
	if (!defined($port)) {
	    ($nodeid, $iface) = Port->ParseIfaceString($str);
	    if (!defined($iface)) {
		$port = undef;
	    } else {
		$port = 1;
		$card = $iface;
	    }
	} else {
	    $iface = Port->fake_CardPort2Iface($card, $port);
	}
    } else {
	$iface = Port->fake_CardPort2Iface($card, $port);
    }
    
    if (defined($port)) {
	my $rowref = {};
	
	$inst->{"RAW_STRING"} = $str;
	$inst->{"FORCED"} = 1;
	$inst->{"HAS_FIELDS"} = 1;
	
	$rowref->{'iface'} = $iface;
	$rowref->{'node_id'} = $nodeid;
	$rowref->{'card'} = $card;
	$rowref->{'port'} = $port;
	$rowref->{'mac'} = "";
	$rowref->{'IP'} = "";
	$rowref->{'role'} = "";
	$rowref->{'interface_type'} = "";
	$rowref->{'mask'} = "";
	$rowref->{'uuid'} = "";
	$inst->{"INTERFACES_ROW"} = $rowref;
    }
    else {
	$inst->{"RAW_STRING"} = $str;
	$inst->{"FORCED"} = 1;
	$inst->{"HAS_FIELDS"} = 0;
    }
    
    #
    # We should determine this according to the query result
    # in nodes table by nodeid.
    #
    $inst->{"WIRE_END"} = "switch";
    
    bless($inst, $class);
    return $inst;
}

#
# Class method
# get the port instance according to its iface-string representation
#
sub LookupByIface($$;$)
{
    my ($class, $nodeid, $iface, $force) = @_;
    my $striface="";

    if (!defined($iface)) {
	$striface = $nodeid;
	($nodeid, $iface) = Port->ParseIfaceString($striface);	
    } else {
	$striface = Tokens2IfaceString($class, $nodeid, $iface);	
    }
    
    if (!defined($striface) || !defined($nodeid)) {
        return undef;
    }

    if (exists($allports{$striface})) {
	return $allports{$striface};
    }

    # all fields
    my $query_result = DBQueryWarn("select * from interfaces ".
		    "where node_id='$nodeid' AND iface='$iface'");
    return undef
	if (!$query_result);
    
    if (!$query_result->numrows) {
	my ($n, $c, $p) = fake_IfaceString2TripleTokens($class, $striface);
	if (defined($p)) {
	    return LookupByTriple($class, $n, $c, $p);
	} else {
	    return undef;
	}
    }

    my $rowref = $query_result->fetchrow_hashref();

    my $card  = $rowref->{'card'};
    my $port  = $rowref->{'port'};

    my $inst = {};
    $inst->{"INTERFACES_ROW"} = $rowref;

    # wire mapping
    $query_result =
	DBQueryWarn("select * from wires ".
		    "where node_id1='$nodeid' AND card1='$card' AND port1='$port'");
    return undef
	if (!$query_result);

    $inst->{"WIRE_END"} = "pc";
    
    if (!$query_result->numrows) {
	$query_result =
	    DBQueryWarn("select * from wires ".
		    "where node_id2='$nodeid' AND card2='$card' AND port2='$port'");
	return undef
	    if (!$query_result);
	return undef
	    if (!$query_result->numrows);
	$inst->{"WIRE_END"} = "switch";
    }

    $rowref = $query_result->fetchrow_hashref();
    $inst->{"WIRES_ROW"} = $rowref;
    $inst->{"FORCED"} = 0;
    $inst->{"HAS_FIELDS"} = 1;

    bless($inst, $class);

    $allports{$striface} = $inst;
    $allports{Tokens2TripleString($class, $nodeid, $card, $port)} = $inst;

    return $inst;
}

#
# Class method
# get the port instance according to its triple-string representation
#
sub LookupByTriple($$;$$)
{
    my ($class, $nodeid, $card, $port) = @_;

    my $strtriple;

    if (!defined($card)) {
	$strtriple = $nodeid;
	($nodeid, $card, $port) = Port->ParseTripleString($strtriple);
    } else {
	$strtriple = Tokens2TripleString($class, $nodeid, $card, $port);
    }
    
    if (!defined($strtriple) || !defined($nodeid) || !defined($card)) {
        return undef;
    }

    if (exists($allports{$strtriple})) {
	return $allports{$strtriple};
    }

    # wire mapping:
    my $query_result =
	DBQueryWarn("select * from wires ".
		    "where node_id1='$nodeid' AND card1='$card' AND port1='$port'");
    return undef
	if (!$query_result);

    my $inst = {};
    $inst->{"WIRE_END"} = "pc";
    
    if (!$query_result->numrows) {
	$query_result =
	    DBQueryWarn("select * from wires ".
		    "where node_id2='$nodeid' AND card2='$card' AND port2='$port'");
	return undef
	    if (!$query_result);
	return undef
	    if (!$query_result->numrows);
	$inst->{"WIRE_END"} = "switch";
    }

    my $rowref = $query_result->fetchrow_hashref();
    $inst->{"WIRES_ROW"} = $rowref;

    $query_result =
	DBQueryWarn("select * from interfaces ".
		    "where node_id='$nodeid' AND card='$card' AND port='$port'");
    return undef
	if (!$query_result);
    if (!$query_result->numrows) {
	$rowref = {};
	my $iface = fake_CardPort2Iface($card, $port);
	$rowref->{'iface'} = $iface;
	$rowref->{'node_id'} = $nodeid;
	$rowref->{'card'} = $card;
	$rowref->{'port'} = $port;
	$rowref->{'mac'} = "";
	$rowref->{'IP'} = "";
	$rowref->{'role'} = "";
	$rowref->{'interface_type'} = "";
	$rowref->{'mask'} = "";
	$rowref->{'uuid'} = "";
    } else {
	$rowref = $query_result->fetchrow_hashref();
    }

    my $iface = $rowref->{'iface'};

    $inst->{"INTERFACES_ROW"} = $rowref;
    $inst->{"FORCED"} = 0;
    $inst->{"HAS_FIELDS"} = 1;

    # wire mapping
    
    bless($inst, $class);

    $allports{$strtriple} = $inst;
    $allports{Tokens2IfaceString($class, $nodeid, $iface)} = $inst;

    return $inst;
}

sub LookupByIfaces($@)
{
    my ($c, @ifs) = @_;

    return map(Port->LookupByIface($_), @ifs);
}

sub LookupByTriples($@)
{
    my ($c, @ifs) = @_;

    return map(Port->LookupByTriple($_), @ifs);
}

#
# Class method
# Get all ports by their wire type
#
sub LookupByWireType($$)
{
	my ($c, $wt) = @_;
	my @ports = ();
	
	my $result = DBQueryFatal("SELECT node_id1, card1, port1, " .
		"node_id2, card2, port2 FROM wires WHERE type='$wt'");

	if ($result) {
		while (my @row = $result->fetchrow()) {
			my ($node_id1, $card1, $port1, $node_id2, $card2, $port2)  = @row;
			my $p1 = Port->LookupByTriple($node_id1, $card1, $port1);
			if (defined($p1)) {
				push @ports, $p1;
			}
			my $p2 = Port->LookupByTriple($node_id2, $card2, $port2);
			if (defined($p2)) {
				push @ports, $p2;
			}
		}
	}
	
	return @ports;
}

sub field($$)  { return (((! ref($_[0])) || ($_[0]->{'HAS_FIELDS'} == 0)) ? 
    -1 : $_[0]->{'INTERFACES_ROW'}->{$_[1]}); }
sub node_id($) { return field($_[0], 'node_id'); }
sub card($)    { return field($_[0], 'card'); }
sub port($)    { return field($_[0], 'port'); }
sub iface($)   { return field($_[0], 'iface'); }
sub mac($)     { return field($_[0], 'mac'); }
sub IP($)      { return field($_[0], 'IP'); }
sub role($)    { return field($_[0], 'role'); }
sub interface_type($)    { return field($_[0], 'interface_type'); }
sub mask($)    { return field($_[0], 'mask'); }
sub uuid($)    { return field($_[0], 'uuid'); }

sub wire_end($) { return $_[0]->{'WIRE_END'}; }
sub is_switch_side($) { return $_[0]->wire_end() eq "switch"; }

sub wire_type($)   { return $_[0]->{'WIRES_ROW'}->{'type'}; }

sub is_forced($) { return $_[0]->{"FORCED"};}
sub has_fields($) { return $_[0]->{"HAS_FIELDS"};}
sub raw_string($) { return $_[0]->{"RAW_STRING"};}

sub switch_node_id($)
{
    my $self = shift;
    if ($self->is_switch_side()) {
	return $self->node_id();
    } else {
	return $self->other_end_node_id();
    }
}

sub switch_card($)
{
    my $self = shift;
    if ($self->is_switch_side()) {
	return $self->card();
    } else {
	return $self->other_end_card();
    }
}

sub switch_port($)
{
    my $self = shift;
    if ($self->is_switch_side()) {
	return $self->port();
    } else {
	return $self->other_end_port();
    }
}

sub switch_iface($)
{
    my $self = shift;
    if ($self->is_switch_side()) {
	return $self->iface();
    } else {
	return $self->other_end_iface();
    }
}

sub pc_node_id($)
{
    my $self = shift;
    if (!$self->is_switch_side()) {
	return $self->node_id();
    } else {
	return $self->other_end_node_id();
    }
}

sub pc_card($)
{
    my $self = shift;
    if (!$self->is_switch_side()) {
	return $self->card();
    } else {
	return $self->other_end_card();
    }
}

sub pc_port($)
{
    my $self = shift;
    if (!$self->is_switch_side()) {
	return $self->port();
    } else {
	return $self->other_end_port();
    }
}

sub pc_iface($)
{
    my $self = shift;
    if (!$self->is_switch_side()) {
	return $self->iface();
    } else {
	return $self->other_end_iface();
    }
}

#
# get node_id field of the other end Port instance
#
sub other_end_node_id($)   
{ 
    my $self = $_[0];
    
    if ($self->is_forced) {
	if ($self->has_fields()) {
	    return $self->node_id();
	} else {
	    return $self->raw_string();
	}
    }

    if ($self->wire_end() eq "pc") {
	return $self->{'WIRES_ROW'}->{'node_id2'}; 
    } else {
	return $self->{'WIRES_ROW'}->{'node_id1'};
    }
}

#
# get card field of the other end Port instance
#
sub other_end_card($) 
{
    my $self = $_[0];
    
    if ($self->is_forced) {
	if ($self->has_fields()) {
	    return $self->card();
	} else {
	    return $self->raw_string();
	}
    }

    if ($self->wire_end() eq "pc") {
	return $self->{'WIRES_ROW'}->{'card2'}; 
    } else {
	return $self->{'WIRES_ROW'}->{'card1'};
    }
}

#
# get port field of the other end Port instance
#
sub other_end_port($) 
{
    my $self = $_[0];
    
    if ($self->is_forced) {
	if ($self->has_fields()) {
	    return $self->port();
	} else {
	    return $self->raw_string();
	}
    }

    if ($self->wire_end() eq "pc") {
	return $self->{'WIRES_ROW'}->{'port2'}; 
    } else {
	return $self->{'WIRES_ROW'}->{'port1'};
    }
}

#
# get iface field of the other end Port instance
#
sub other_end_iface($)
{
    my $self = $_[0];
    
    if ($self->is_forced) {
	if ($self->has_fields()) {
	    return $self->iface();
	} else {
	    return $self->raw_string();
	}
    }

    if ($self->wire_end() eq "pc") {
	return Port->LookupByTriple(
	    $self->{'WIRES_ROW'}->{'node_id2'},
	    $self->{'WIRES_ROW'}->{'card2'},
	    $self->{'WIRES_ROW'}->{'port2'})->iface(); 
    } else {
	return Port->LookupByTriple(
	    $self->{'WIRES_ROW'}->{'node_id1'},
	    $self->{'WIRES_ROW'}->{'card1'},
	    $self->{'WIRES_ROW'}->{'port1'})->iface(); 
    }
}

#
# to the 'iface' string representation
#
sub toIfaceString($) {
    my $self = shift;
    if (!$self->has_fields()) {
	return $self->raw_string();
    }
    return Tokens2IfaceString($self->node_id(), $self->iface());
}
    
#
# to the 'triple' string representation
# 
sub toTripleString($) {
    my $self = shift;
    if (!$self->has_fields()) {
	return $self->raw_string();
    }
    return Tokens2TripleString($self->node_id(), $self->card(), $self->port());
}

#
# to default string representation, which default is 'triple'
# 
sub toString($) {
    my $self = shift;
    if (!$self->has_fields()) {
	return $self->raw_string();
    }
    return $self->toTripleString();
}

#
# convert to the old "node:card" string
#
sub toNodeCardString($) {
    return Tokens2IfaceString($_[0]->node_id(), $_[0]->card());
}

#
# get port instance according to the given node_id
# 
sub getEndByNode($$) {
	my ($self, $n) = @_;
	if ($n eq $self->node_id()) {
		return $self;
	} elsif ($n eq $self->other_end_node_id()) { 
		return $self->getOtherEndPort();
	} else {
		return undef;
	}
}

#
# get the other side's triple string representation
#
sub getOtherEndTripleString($) {
    return Tokens2TripleString($_[0]->other_end_node_id(), $_[0]->other_end_card(), $_[0]->other_end_port());
}

#
# get the other side's iface string representation
#
sub getOtherEndIfaceString($) {
    return Tokens2IfaceString($_[0]->other_end_node_id(), $_[0]->other_end_iface());
}

#
# get the other side of a port instance, according to 'wires' DB table
#
sub getOtherEndPort($) {
    $my pt = Port->LookupByTriple($_[0]->getOtherEndTripleString());
    if (defined($pt)) {
	return $pt;
    } else {
	return $self;
    }
}

#
# get the PC side of a port instance
#
sub getPCPort($) {
    my $self = $_[0];
    
    if ($self->is_forced) {
	return $self;
    }

    if ($self->wire_end() eq "pc") {
	return $self;
    } else {
	return $self->getOtherEndPort();
    }
}

#
# get the switch side of a port instance
#
sub getSwitchPort($) {
    my $self = $_[0];
    
    if ($self->is_forced) {
	return $self;
    }

    if ($self->wire_end() ne "pc") {
	return $self;
    } else {
	return $self->getOtherEndPort();
    }
}

#
# convert an array of ports into string, in iface representation
#
sub toIfaceStrings($@)
{
	my ($c, @pts) = @_;
	if (@pts != 0) {
	    if (Port->isPort($pts[0])) {
		return join(" ", map($_->toIfaceString(), @pts)); 
	    }
	}
	return join(" ", @pts);
}

#
# convert an array of ports into string, in triple-filed representation
#
sub toTripleStrings($@)
{
	my ($c, @pts) = @_;
	if (@pts != 0) {
	    if (Port->isPort($pts[0])) {
		return join(" ", map($_->toTripleString(), @pts)); 
	    }
	}
	return join(" ", @pts);
}

#
# convert an array of ports into string, in default string representation
#
sub toStrings($@)
{
	my ($c, @pts) = @_;
	if (@pts != 0) {
	    if (Port->isPort($pts[0])) {
		return join(" ", map($_->toString(), @pts)); 
	    }
	}
	return join(" ", @pts);
}
return 1;
