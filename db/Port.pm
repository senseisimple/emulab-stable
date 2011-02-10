#!/usr/bin/perl -wT
#
# EMULAB-COPYRIGHT
# Copyright (c) 2010 University of Utah and the Flux Group.
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

# Cache of port instances, node:iface OR node:card.port => Port Instance
my %allports = ();

# Ends of wires, node:iface OR node:card.port => node:card.port
# A better mapping should be triple/iface => Port instance, but the switch side
# port may not exist in interfaces table, so we can't create that object by
# LookupByXXX.
my %wiredports = ();


sub GetTheOtherEndByTriple($;$$$)
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
    if (defined($p) && $p != 0 && $p != -1) {

	$wiredports{$p->toTripleString()} = $p->getOtherEndTriple();
	$wiredports{$p->toIfaceString()} = $p->getOtherEndTriple();

	return $p->getOtherEndTriple();
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

    return undef;
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

    return undef;
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

sub LookupByIface($$;$)
{
    my ($class, $nodeid, $iface) = @_;
    my $striface;

    if (!defined($iface)) {
	$striface = $nodeid;
	($nodeid, $iface) = Port->ParseIfaceString($striface);
    } else {
	$striface = Tokens2IfaceString($class, $nodeid, $iface);
    }

    if (exists($allports{$striface})) {
	return $allports{$striface};
    }

    # all fields
    my $query_result = DBQueryWarn("select * from interfaces ".
		    "where node_id='$nodeid' AND iface='$iface'");
    return -1
	if (!$query_result);
    return 0
	if (!$query_result->numrows);

    my $rowref = $query_result->fetchrow_hashref();

    my $card  = $rowref->{'card'};
    my $port  = $rowref->{'port'};

    my $inst = {};
    $inst->{"INTERFACES_ROW"} = $rowref;

    # wire mapping
    $query_result =
	DBQueryWarn("select * from wires ".
		    "where node_id1='$nodeid' AND card1='$card' AND port1='$port'");
    return -1
	if (!$query_result);

    $inst->{"WIRE_END"} = "pc";
    
    if (!$query_result->numrows) {
	$query_result =
	    DBQueryWarn("select * from wires ".
		    "where node_id2='$nodeid' AND card2='$card' AND port2='$port'");
	return -1
	    if (!$query_result);
	return 0
	    if (!$query_result->numrows);
	$inst->{"WIRE_END"} = "switch";
    }

    $rowref = $query_result->fetchrow_hashref();
    $inst->{"WIRES_ROW"} = $rowref;

    bless($inst, $class);

    $allports{$striface} = $inst;
    $allports{Tokens2TripleString($class, $nodeid, $card, $port)} = $inst;

    return $inst;
}

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

    if (exists($allports{$strtriple})) {
	return $allports{$strtriple};
    }

    my $query_result =
	DBQueryWarn("select * from interfaces ".
		    "where node_id='$nodeid' AND card='$card' AND port='$port'");
    return -1
	if (!$query_result);
    return 0
	if (!$query_result->numrows);

    my $rowref = $query_result->fetchrow_hashref();

    my $iface  = $rowref->{'iface'};

    my $inst = {};
    $inst->{"INTERFACES_ROW"} = $rowref;

    # wire mapping
    $query_result =
	DBQueryWarn("select * from wires ".
		    "where node_id1='$nodeid' AND card1='$card' AND port1='$port'");
    return -1
	if (!$query_result);

    $inst->{"WIRE_END"} = "pc";
    
    if (!$query_result->numrows) {
	$query_result =
	    DBQueryWarn("select * from wires ".
		    "where node_id2='$nodeid' AND card2='$card' AND port2='$port'");
	return -1
	    if (!$query_result);
	return 0
	    if (!$query_result->numrows);
	$inst->{"WIRE_END"} = "switch";
    }

    $rowref = $query_result->fetchrow_hashref();
    $inst->{"WIRES_ROW"} = $rowref;

    bless($inst, $class);

    $allports{$strtriple} = $inst;
    $allports{Tokens2IfaceString($class, $nodeid, $iface)} = $inst;

    return $inst;
}

sub field($$)  { return ((! ref($_[0])) ? -1 : $_[0]->{'INTERFACES_ROW'}->{$_[1]}); }
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

sub wire_type($)   { return $_[0]->{'WIRES_ROW'}->{'type'}; }

sub other_end_node_id($)   
{ 
    my $self = $_[0];

    if ($self->wire_end() eq "pc") {
	return $self->{'WIRES_ROW'}->{'node_id2'}; 
    } else {
	return $self->{'WIRES_ROW'}->{'node_id1'};
    }
}

sub other_end_card($) 
{
    my $self = $_[0];

    if ($self->wire_end() eq "pc") {
	return $self->{'WIRES_ROW'}->{'card2'}; 
    } else {
	return $self->{'WIRES_ROW'}->{'card1'};
    }
}

sub other_end_port($) 
{
    my $self = $_[0];

    if ($self->wire_end() eq "pc") {
	return $self->{'WIRES_ROW'}->{'port2'}; 
    } else {
	return $self->{'WIRES_ROW'}->{'port1'};
    }
}

sub toIfaceString($) {
    return Tokens2IfaceString($_[0]->node_id(), $_[0]->iface());
}
    
sub toTripleString($) {
    return Tokens2TripleString($_[0]->node_id(), $_[0]->card(), $_[0]->port());
}

#
# Should not support.
#
sub toNodeCardString($) {
    return Tokens2IfaceString($_[0]->node_id(), $_[0]->card());
}

sub getOtherEndTriple($) {
    return Tokens2TripleString($_[0]->other_end_node_id(), $_[0]->other_end_card(), $_[0]->other_end_port());
}

return 1;
