#!/usr/bin/perl -wT
#
# EMULAB-COPYRIGHT
# Copyright (c) 2010 University of Utah and the Flux Group.
# All rights reserved.
#
package libptopnew;

use strict;
use Exporter;
use lib "/usr/testbed/lib";
#use lib "@prefix@/lib";
use Node;
use libdb qw(TBGetSiteVar);
use vars qw(@ISA @EXPORT @EXPORT_OK);

my $PGENISUPPORT = 1;
my $OURDOMAIN = "jonlab.tbres.emulab.net";
my $MAINSITE  = 0;
#my $PGENISUPPORT = @PROTOGENI_SUPPORT@;
#my $OURDOMAIN = "@OURDOMAIN@";
#my $MAINSITE  = @TBMAINSITE@;
my $cmuuid = TBGetSiteVar('protogeni/cm_uuid');
my $cmurn = "";
if ($PGENISUPPORT) {
    require GeniHRN;
    require GeniXML;
    $cmurn = GeniHRN::Generate($OURDOMAIN, "authority", "cm");
}


@ISA = "Exporter";
@EXPORT = qw( );

my $user_project = undef;
my $exempt_eid = undef;
my $available_only = 0;
my $print_widearea = 0;
my $print_shared = 0;
my $genimode = 1;

our %nodeList = ();
our %linkList = ();
# Table of which types the user project is allowed to have.
# Keyed by type where 1 = allowed, 0 = denied, and ! exists = allowed
our %permissions = ();

sub Init()
{
    InitPermissions();
}

#
# Initialize project permissions table if the user specified a project.
#
sub InitPermissions()
{
    if (defined($user_project)) {
        # By default a type is allowed for every project. If a type is
        # in the permissions table, it is allowed only for those
        # projects which it is attached to.
	my $dbresult
	    = DBQueryFatal("select distinct type ".
			   "from nodetypeXpid_permissions");
	while (my ($type) = $dbresult->fetchrow_array) {
	    $permissions{$type} = 0;
	}
	$dbresult
	    = DBQueryFatal("select type ".
			   "from nodetypeXpid_permissions".
			   "where pid='$user_project'");
	while (my ($type) = $dbresult->fetchrow_array) {
	    $permissions{$type} = 1;
	}
    }
}

sub TypeAllowed($)
{
    my ($type) = @_;
    return (! defined($user_project)
	    || ! exists($permissions{$type})
	    || $permissions{$type});
}

# Accessors
sub Nodes() { return \%nodeList; }
sub Links() { return \%linkList; }

# Add new nodes and links
sub CreateNode($)
{
    my ($row) = @_;
    my $node = libptop::pnode->Create($row);
    Nodes()->{$node->name()} = $node;
    return $node;
}

###############################################################################
# Physical Nodes. These contain the all of the per-node state used to
# generate ptop or xml files.

package libptop::pnode;

use EmulabConstants;

sub Create($$)
{
    my ($class, $row) = @_;

    my $self = {};

    $self->{'NODE'} = Node->LookupRow($row);
    $self->{'PTYPES'} = [];

    bless($self, $class);
    return $self;
}

# Accessors
sub name($)       { return $_[0]->{'NODE'}->node_id(); }
sub node($)       { return $_[0]->{'NODE'}; }
sub type($)       { return $_[0]->node()->NodeTypeInfo(); }

sub available($;$)
{
    my ($self, $tagRef) = @_;
    my $node = $self->node();
    my $reserved_pid = $node->reserved_pid();
    my $reserved_eid = $node->ReservationID();
    # A node is reserved to a project if it has a reserved_pid, and
    # that pid is not the user's pid.
    my $project_reserved = defined($reserved_pid)
			   && (! defined($user_project)
			       || $user_project ne $reserved_pid);
    # A node is reserved to an experiment if it has a reserved_eid,
    # a reserved_pid, and one of those is not the user's pid/eid.
    my $exp_reserved = defined($reserved_eid)
	               && defined($reserved_pid)
	               && (! defined($exempt_eid)
			   || $reserved_eid ne $exempt_eid
			   || $reserved_pid ne $user_project);
    my $isreserved = $project_reserved || $exp_reserved;

    my $typeallowed = (libptopnew::TypeAllowed($node->class())
		       && libptopnew::TypeAllowed($node->type()));

    # Nodes are free if they are nonlocal, or if they are up and
    # not-reserved, or if they are shared.
    #
    # And they must also be allowed for the current project by the
    # nodetypeXpid_permissions table.
    my $isfree = ((!$self->islocal()
		   || (! $isreserved && $self->isup())
		   || $self->isshared())
		  && $typeallowed);

    # And if they are a subnode, their parent must be available:

    # A bit of recursion to ensure that subnodes are only listed as
    # available if their parent is. The tags bit is to try to ensure
    # we don't loop forever if there is a subnode-loop. See also willPrint().
    if ($isfree && $node->issubnode()) {
	my %tags = {};
	if (defined($tagRef)) {
	    %tags = %{ $tagRef };
	}
	$tags{$self->name()} = 1;
	my $parent = $node->phys_nodeid();
	if (! exists($tags{$parent})) {
	    $isfree = $isfree && $nodeList{$parent}->available(\%tags);
	}
    }
    return $isfree;
}

sub addPType($$)
{
    my ($self, $newType) = @_;
    push(@{ $self->{'PTYPES'} }, $newType);
}

sub processSwitch($)
{
    my ($self) = @_;
    if (! $self->isswitch()) {
	return;
    }

    # Add switch and lan types
    $self->addPType(libptop::pnode_type->Create("switch", 1));
    if (!(defined($MAINSITE) && $MAINSITE
	  && $self->name() eq "procurve1")) {
	$self->addPType(libptop::pnode_type->Create("lan", undef, 1));
    }

    # Add real-switch feature
    #my $features = ["real-switch:0"];
}

sub processLocal($)
{
    my ($self) = @_;
    if (! $self->islocal()) {
	return;
    }
}

sub processWidearea($)
{
    my ($self) = @_;
    if (! $self->iswidearea()) {
	return;
    }
}

sub isswitch($)
{
    my ($self) = @_;
    my $role = $self->node()->role();
    return ($role eq 'testswitch' || $role eq 'widearea_switch'
	    || ($role eq 'testnodefoo' && $self->node()->isswitch()));
}

sub islocal($)
{
    my ($self) = @_;
    my $node = $self->node();
    my $isremotenode = $node->isremotenode();
    my $wa_attrvalue = $node->NodeTypeAttribute('dedicated_widearea');
    return ( $node->role() eq 'testnode'
	     && ((! defined($isremotenode) || $isremotenode == 0)
		 || (defined($wa_attrvalue) && $wa_attrvalue == 1)));
}

sub iswidearea($)
{
    my ($self) = @_;
    my $node = $self->node();
    my $isremotenode = $node->isremotenode();
    my $isvirtnode = $node-> isvirtnode();
    my $wa_attrvalue = $node->NodeTypeAttribute('dedicated_widearea');
    return ($node->role() eq 'testnode'
	    && defined($isremotenode)
	    && $isremotenode == 1
	    && (! defined($isvirtnode) || $isvirtnode == 0)
	    && $node->type() ne 'pcfedphys'
	    && (! defined($wa_attrvalue) || $wa_attrvalue == 0));
}

sub isshared($)
{
    my ($self) = @_;
    my $node = $self->node();
    # In shared mode, allow allocated nodes whose sharing_mode is set.
    return (defined($node->erole())
	    && $node->erole() eq "sharedhost"
	    && $self->isup());
}

sub isup($)
{
    my ($self) = @_;
    my $eventstate = $self->node()->eventstate();
    return defined($eventstate)
	&& ($eventstate eq TBDB_NODESTATE_ISUP()
	    || $eventstate eq TBDB_NODESTATE_PXEWAIT()
	    || $eventstate eq TBDB_NODESTATE_POWEROFF()
	    || $eventstate eq TBDB_NODESTATE_ALWAYSUP());
}

sub willPrint($;$)
{
    my ($self, $tagRef) = @_;
    my $node = $self->node();

    # In geni mode, disallow nodes tagged protogeni_exclude from being printed.
    my $geniExclude = 0;
    $node->NodeAttribute("protogeni_exclude", \$geniExclude);
    my $geniok = (! defined($geniExclude) || $geniExclude == 0);
    my $result = ($self->isswitch()
		  || $self->islocal()
		  || ($self->iswidearea() && $print_widearea)
		  || ($self->isshared() && $print_shared))
	&& (! $available_only || $self->available($)))
	&& (! $genimode || $geniok);

    # A bit of recursion to ensure that subnodes are only printed if
    # their parent is. The tags bit is to try to ensure we don't loop
    # forever if there is a subnode-loop. See also available()).
    if ($result && $node->issubnode()) {
	my %tags = {};
	if (defined($tagRef)) {
	    %tags = %{ $tagRef };
	}
	$tags{$self->name()} = 1;
	my $parent = $node->phys_nodeid();
	if (! exists($tags{$parent})) {
	    $result = $result && $nodeList{$parent}->willPrint(\%tags);
	}
    }
    return $result;
}

sub toString($)
{
    my ($self) = @_;
    my $result = "node " . $self->name();

    foreach my $type (@{ $self->{'PTYPES'} }) {
	$result .= " " . $type->toString();
    }
    return $result;
}

sub toXML($$)
{
    my ($self, $parent) = @_;
    my $xml = GeniXML::AddElement("node", $parent);
    my $urn = GeniHRN::Generate($OURDOMAIN, "node", $self->name());
    if (GeniXML::IsVersion0($xml)) {
	GeniXML::SetText("component_manager_uuid", $xml, $cmurn);
	GeniXML::SetText("component_uuid", $xml, $urn);
    } else {
	GeniXML::SetText("component_manager_id", $xml, $cmurn);
	GeniXML::SetText("component_id", $xml, $urn);
    }
    GeniXML::SetText("component_name", $xml, $self->name());

    foreach my $type (@{ $self->{'PTYPES'} }) {
	$type->toXML($xml);
    }
}

###############################################################################
# Physical Node Type. These are the types which are printed out in the
# ptopgen file. Note that there is not a one-to-one correspondence
# between these types and the 'type' of the node in the database.

package libptop::pnode_type;

sub Create($$;$$)
{
    my ($class, $name, $slots, $isstatic) = @_;

    my $self = {};

    $self->{'NAME'} = $name;
    $self->{'SLOTS'} = $slots;
    $self->{'ISSTATIC'} = 0;
    if (defined($isstatic)) {
	$self->{'ISSTATIC'} = 1;
    }

    bless($self, $class);
    return $self;
}

# Accessors
sub name($)       { return $_[0]->{'NAME'}; }
sub slots($)      { return $_[0]->{'SLOTS'}; }
sub isstatic($)   { return $_[0]->{'ISSTATIC'}; }

sub toString($)
{
    my ($self) = @_;
    my $result = "";
    if ($self->isstatic()) {
	$result .= "*";
    }
    $result .= $self->name().":";
    if (defined($self->slots())) {
	$result .= $self->slots();
    } else {
	$result .= "*";
    }
    return $result;
}

sub toXML($$)
{
    my ($self, $parent) = @_;
    my $slots = $self->slots();
    if (! defined($slots)) {
	$slots = "unlimited";
    }
    my $xml;
    if (GeniXML::IsVersion0($parent)) {
	$xml = GeniXML::AddElement("node_type", $parent);
	GeniXML::SetText("type_name", $xml, $self->name());
	GeniXML::SetText("type_slots", $xml, $slots);
	if ($self->isstatic()) {
	    GeniXML::SetText("static", $xml, "true");
	}
    } else {
	my $sliverxml;
	if ($self->name() eq "pc") {
	    $sliverxml = GeniXML::AddElement("sliver_type", $parent);
	    GeniXML::SetText("name", $sliverxml, "raw-pc");
	    # TODO: osids
	}
	if ($self->name() eq "pcvm") {
	    $sliverxml = GeniXML::AddElement("sliver_type", $parent);
	    GeniXML::SetText("name", $sliverxml, "emulab-openvz");
	}
	$xml = GeniXML::AddElement("hardware_type", $parent);
	GeniXML::SetText("name", $xml, $self->name());
	my $elab = GeniXML::AddElement("node_type", $xml, $GeniXML::EMULAB_NS);
	GeniXML::SetText("type_slots", $elab, $slots);
	if ($self->isstatic()) {
	    GeniXML::SetText("static", $elab, "true");
	}
    }
}

1;
