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
use OSinfo;
use libdb qw(TBGetSiteVar DBQueryFatal TBResolveNextOSID);
use vars qw(@ISA @EXPORT @EXPORT_OK);

sub FD_ADDITIVE  { return "FD_ADDITIVE"; }
sub FD_FIRSTFREE { return "FD_FIRSTFREE"; }
sub FD_ONCEONLY  { return "FD_ONCEONLY"; }

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
my $print_shared = 1;
my $print_virtual = 1;
my $print_sim = 1;
my $genimode = 1;
my $delaycap_override = undef;
my $multiplex_override = undef;

our %nodeList = ();
our %linkList = ();
# Table of which types the user project is allowed to have.
# Keyed by type where 1 = allowed, 0 = denied, and ! exists = allowed
our %permissions = ();
# Map from auxtype names to real type names
our %auxtypemap = ();
# Map from type names to lists of features
our %typefeatures = ();
# Keyed by osids. Contains OsInfo structures
our %osinfo = ();
# Mapping between types and osids, and vice versa. Hash of arrays.
our %type_osid = ();
our %osid_type = ();
# Mapping between an osid and its subosids. Hash of arrays.
our %osid_subosid = ();

#
# Initialize nodes hash based on nodes, reservations, and node_status tables
#
sub LookupNodes()
{
    my $dbresult;
    my $row;
    $dbresult = DBQueryFatal("select * from nodes");
    while ($row = $dbresult->fetchrow_hashref()) {
	CreateNode($row);
    }

    # Bulk lookup on reserved table
    $dbresult = DBQueryFatal("select * from reserved");
    while ($row = $dbresult->fetchrow_hashref()) {
	$nodeList{$row->{"node_id"}}->node()->SetReservedRow($row);
    }

    $dbresult = DBQueryFatal("select node_id, status from node_status");
    while (my ($node_id, $status) = $dbresult->fetchrow()) {
	$nodeList{$node_id}->set_status($status);
    }

    $dbresult = DBQueryFatal("select * from widearea_nodeinfo");
    while ($row = $dbresult->fetchrow_hashref()) {
	$nodeList{$row->{'node_id'}}->set_widearea($row);
    }
}

#
# Initialize project permissions table if the user specified a project.
#
sub LookupPermissions()
{
    if (defined($user_project)) {
        # By default a type is allowed for every project. If a type is
        # in the permissions table, it is allowed only for those
        # projects which it is attached to.
	my $dbresult =
	    DBQueryFatal("select distinct type ".
			   "from nodetypeXpid_permissions");
	while (my ($type) = $dbresult->fetchrow_array()) {
	    $permissions{$type} = 0;
	}
	$dbresult =
	    DBQueryFatal("select type ".
			   "from nodetypeXpid_permissions".
			   "where pid='$user_project'");
	while (my ($type) = $dbresult->fetchrow_array()) {
	    $permissions{$type} = 1;
	}
    }
}

#
# Lookup global usage counts on virtual nodes.
#
sub LookupGlobalCounts()
{
    my $condition = " ";
    if (defined($exempt_eid)) {
	$condition = "and not (pid='$user_project' and eid='$exempt_eid') "
    }
    my $dbresult = 
	DBQueryFatal("select phys_nodeid,count(phys_nodeid) ".
		     "from reserved as r ".
		     "left join nodes as n on n.node_id=r.node_id ".
		     "where n.node_id!=n.phys_nodeid ".
		     $condition.
		     "group by phys_nodeid");
    while (my ($node_id, $count) = $dbresult->fetchrow_array()) {
	$nodeList{$node_id}->set_globalcount($count);
    }
}

#
# Auxtypes can be associated with the main type of a node or with the
# node itself. Lookup both of these cases.
#
sub LookupAuxtypes()
{
    my $dbresult;
    #
    # Read the auxtypes for each type.
    # 
    $dbresult = DBQueryFatal("select auxtype,type from node_types_auxtypes");
    while (my ($auxtype,$type) = $dbresult->fetchrow_array()) {
	$auxtypemap{$auxtype} = $type;
    }

    #
    # Read in the node_auxtypes table for each node.
    #
    $dbresult = DBQueryFatal("select node_id, type, count from node_auxtypes");
    while (my ($node_id, $type, $count) = $dbresult->fetchrow_array()) {
	$nodeList{$node_id}->addAuxtype($type, $count);
    }
}

#
# Find features associated both with types and with nodes. Add the
# node features immediately and save the association between types and
# features. We will add those features to nodes when we iterate over
# all the nodes during processing.
#
sub LookupFeatures()
{
    my $dbresult;
    #
    # Read the features table for each type.
    # 
    $dbresult =
	DBQueryFatal("select type, feature, weight from node_type_features");
    while (my ($type, $feature, $weight) = $dbresult->fetchrow()) {
	if (! exists($typefeatures{$type})) {
	    $typefeatures{$type} = [];
	}
	push(@{ $typefeatures{$type} }, $feature.":".$weight);
    }

    #
    # Read the features table for each individual node
    #
    $dbresult =
	DBQueryFatal("select node_id, feature, weight from node_features");
    while (my ($node_id, $feature, $weight) = $dbresult->fetchrow()) {
	my $pnode = $nodeList{$node_id};
	if ($pnode->iswidearea()
	    || ($pnode->islocal() && ! $pnode->node()->sharing_mode())) {
	    $pnode->addFeatureString($feature.":".$weight);
	}
    }
}

#
# Bulk lookup on the os_info table. Find the mapping between types and
# osids and between osids and subosids.
#
sub LookupOsids()
{
    my $dbresult;
    my $row;
    # Bulk lookup on os_info table
    $dbresult = DBQueryFatal("select * from os_info");
    while ($row = $dbresult->fetchrow_hashref()) {
	my $os = OSinfo->LookupRow($row);
	$osinfo{$os->osid()} = $os;
    }

    #
    # Read the table of which image types are supported on which
    # hardware - we limit this to global images and ones that match
    # the PID (if given) We do this limiting for two reasons:
    # 1) To avoid an explosion in the number of features for nodes
    # 2) To avoid information leaks, allowing projects to see each
    #    other's images
    # 
    my $pidos = "";
    if (defined($user_project)) {
	$pidos = "or i.pid='$user_project'";
    }
    $dbresult =
	DBQueryFatal("select distinct oi.osid, oi.type ".
		     "from osidtoimageid as oi ".
		     "left join images as i on oi.imageid = i.imageid ".
		     "where i.global = 1 $pidos");
    while (my ($osid, $typename) = $dbresult->fetchrow()) {
	hashpush(\%type_osid, $typename, $osid);
	hashpush(\%osid_type, $osid, $typename);
    }

    #
    # We also have to resolve the 'generic' OSIDs, which use the
    # nextosid field to redirect to another OSID
    #
    $dbresult = DBQueryFatal("select osid from os_info where " .
			     "nextosid is not null");
    while (my ($osid) = $dbresult->fetchrow()) {
	#
	# Check to see if they were allowed to use the real OSID
	#
	my $realosid = TBResolveNextOSID($osid, $user_project, $exempt_eid);
	if (exists($osid_type{$realosid})) {
	    foreach my $type (@{ $osid_type{$realosid} }) {
		hashpush(\%type_osid, $type, $osid);
	    }
	}
    }

    #
    # For subOS support (i.e., vnode OSes running atop vhost OSes), we
    # have to check both the subosid and all the parent_osid it can
    # run on.
    #
    my $subosidquery = "select distinct o.osid,o.parent_osid ".
	"from os_submap as o " .
	"left join osidtoimageid as oi1 on o.osid = oi1.osid " .
	"left join osidtoimageid as oi2 on o.parent_osid = oi2.osid " .
	"left join images as i1 on oi1.imageid = i1.imageid ".
	"left join images as i2 on oi2.imageid = i2.imageid ".
	"where (i1.global = 1";
    if (defined($user_project)) {
	$subosidquery .= " or i1.pid='$user_project'";
    }
    $subosidquery .= ") and (i2.global = 1";
    if (defined($user_project)) {
	$subosidquery .= " or i2.pid='$user_project'";
    }
    $subosidquery .= ")";
    $dbresult = DBQueryFatal($subosidquery);
    while (my ($subosid,$osid) = $dbresult->fetchrow()) {
	hashpush(\%osid_subosid, $osid, $subosid);
    }
}

#
# Lookup both wires and interfaces. Let nodes know which other nodes
# they are connected to for later use when adding features.
#
sub LookupLinks()
{
    my $dbresult;
    my $row;
    $dbresult =
	DBQueryFatal("select w.type, w.node_id1, w.card1, w.port1, ".
		     "w.node_id2, w.card2, w.port2, i.iface ".
		     "from wires as w ".
		     "left join interfaces as i ".
		     "on w.node_id1=i.node_id and w.card1=i.card ".
		     "and w.port1=i.port ".
		     "where w.logical=0");
    while (my ($type, $node_id1, $card1, $port1,
	       $node_id2, $card2, $port2, $iface1) = $dbresult->fetchrow()) {
	if ($type ne 'Unused') {
	    my $node1 = $nodeList{$node_id1};
	    my $node2 = $nodeList{$node_id2};
	    if (defined($node1)) {
		$node1->addConnection($node_id2);
	    } else {
		print STDERR "Unknown node: $node_id1\n";
	    }
	    if (defined($node2)) {
		$nodeList{$node_id2}->addConnection($node_id1);
	    } else {
		print STDERR "Unknown node: $node_id2\n";
	    }
	}
    }
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

sub TypeAllowed($)
{
    my ($type) = @_;
    return (! defined($user_project)
	    || ! exists($permissions{$type})
	    || $permissions{$type});
}

# Push a value onto an array contained within a hash
sub hashpush($$$)
{
    my ($hashref, $key, $value) = @_;
    if (! exists($hashref->{$key})) {
	$hashref->{$key} = [$value];
    } else {
	push(@{ $hashref->{$key} }, $value);
    }
}

###############################################################################
# Physical Nodes. These contain the all of the per-node state used to
# generate ptop or xml files.

package libptop::pnode;

use libdb qw(TBOSID TB_OPSPID);

sub Create($$)
{
    my ($class, $row) = @_;

    my $self = {};

    $self->{'NODE'} = Node->LookupRow($row);
    $self->{'PTYPES'} = [];
    $self->{'FEATURES'} = [];
    $self->{'FLAGS'} = {};
    $self->{'GLOBALCOUNT'} = undef;
    $self->{'AUXTYPES'} = {};
    $self->{'CONNECTIONS'} = {};
    $self->{'STATUS'} = undef;
    $self->{'WIDEAREA'} = undef;

    bless($self, $class);
    return $self;
}

# Accessors
sub name($)        { return $_[0]->{'NODE'}->node_id(); }
sub node($)        { return $_[0]->{'NODE'}; }
sub type($)        { return $_[0]->node()->NodeTypeInfo(); }
sub globalcount($) { return $_[0]->{'GLOBALCOUNT'}; }
sub status($)      { return $_[0]->{'STATUS'}; }

sub set_globalcount { $_[0]->{'GLOBALCOUNT'} = $_[1]; }
sub set_status($)   { $_[0]->{'STATUS'} = $_[1]; }
sub set_widearea($) { $_[0]->{'WIDEAREA'} = $_[1]; }

sub widearea($$)
{
    my ($self, $key) = @_;
    my $result = undef;
    if (defined($self->{'WIDEAREA'})) {
	$result = $self->{'WIDEARE'}->{$key};
    }
    return $result;
}

sub isreserved($)
{
    my ($self) = @_;
    my $node = $self->node();
    my $pre_pid = $node->reserved_pid();
    my $reserved_eid = $node->eid();
    my $reserved_pid = $node->pid();
    # A node is reserved to a project if it has a reserved_pid, and
    # that pid is not the user's pid.
    my $pre_reserved = defined($pre_pid)
	               && (! defined($user_project)
			   || $user_project ne $pre_pid);
    # A node is reserved to an experiment if it has a reserved_eid,
    # a reserved_pid, and one of those is not the user's pid/eid.
    my $exp_reserved = defined($reserved_eid)
	               && defined($reserved_pid)
	               && (! defined($exempt_eid)
			   || $reserved_eid ne $exempt_eid
			   || $reserved_pid ne $user_project);
    return $pre_reserved || $exp_reserved;
}

sub available($;$)
{
    my ($self, $tagRef) = @_;
    my $node = $self->node();

    my $typeallowed = (libptopnew::TypeAllowed($node->class())
		       && libptopnew::TypeAllowed($node->type()));

    # Nodes are free if they are nonlocal, or if they are up and
    # not-reserved, or if they are shared.
    #
    # And they must also be allowed for the current project by the
    # nodetypeXpid_permissions table.
    my $isfree = ((!$self->islocal()
		   || (! $self->isreserved() && $self->isup())
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

sub addPType($$$;$)
{
    my ($self, $newname, $newvalue, $newstatic) = @_;
    push(@{ $self->{'PTYPES'} },
	 libptop::pnode_type->Create($newname, $newvalue, $newstatic));
}

sub addFeature($$$;$$)
{
    my ($self, $newname, $newvalue, $newflag, $newvolatile) = @_;
    push(@{ $self->{'FEATURES'} },
	 libptop::feature->Create($newname, $newvalue,
				  $newflag, $newvolatile));
}

sub addFeatureString($$)
{
    my ($self, $newfeature) = @_;
    push(@{ $self->{'FEATURES'} },
	 libptop::feature->CreateFromString($newfeature));
}

sub addFlag($$$)
{
    my ($self, $key, $value) = @_;
    $self->{'FLAGS'}->{$key} = $value;
}

sub addAuxtype($$$)
{
    my ($self, $key, $value) = @_;
    $self->{'AUXTYPES'}->{$key} = $value;
}

sub addConnection($$)
{
    my ($self, $name) = @_;
    $self->{'CONNECTIONS'}->{$name} = 1;
}

sub processSwitch($)
{
    my ($self) = @_;
    if (! $self->isswitch()) {
	return;
    }

    # Add switch and lan types
    $self->addPType("switch", 1);
    if (!(defined($MAINSITE) && $MAINSITE && $self->name() eq "procurve1")) {
	$self->addPType("lan", undef, 1);
    }

    # Add real-switch feature
    $self->addFeature('real-switch', 0);
}

sub processLocal($)
{
    my ($self) = @_;
    if (! $self->islocal()) {
	return;
    }
    my $node = $self->node();
    my $type = $self->type();
    # XXX temporary hack until node reboot avoidance 
    # is available. Nodes running the FBSD-NSE image
    # will have a feature def-osid-fbsd-nse 0.0
    # This is used by assign to prefer these pnodes
    # first before using others.
    if($node->def_boot_osid() && 
       $node->def_boot_osid() eq  TBOSID(TB_OPSPID, "FBSD-NSE")) { 
	$self->addFeature('FBSD-NSE', 0.0);
    }
    $self->addPType($node->type(), 1);
    # Might be equal, which assign would sum as two, not one!
    #
    # XXX: Temporary hack - don't mark switches that are testnodes
    # as having class 'switch' - assign treats those specially. We
    # use the knowledge that 'real' switches don't hit this point!
    #
    if ($node->type() ne $node->class() && $node->class() ne "switch") {
	$self->addPType($node->class(), 1);
    }

    $self->addDelayCapacity();
    if ($node->sharing_mode()) {
	$self->addShared();
    }
    $self->processAuxtypes();
    $self->processOsFeatures();
    $self->processConnections();
    # This is for the case that we are modifying an existing experiment - tell
    # assign to prefer nodes the user has already allocated
    if (defined($exempt_eid) && $available_only
	&& defined($node->eid()) && defined($node->pid())) {
	$self->addFeature('already_reserved', 0);
    }
    $self->processSubnode();
}

sub addDelayCapacity($)
{
    my ($self) = @_;
    my $delay = $self->type()->delay_capacity();
    if (defined($delaycap_override) &&
	$delaycap_override > 0 &&
	$delaycap_override < $delay) {
	$delay = $delaycap_override
    }
    $self->addPType("delay", $delay);
    $self->addPType("delay-".$self->node()->type(), $delay);
    
}

#
# Shared mode features
#
sub addShared($)
{
    my ($self) = @_;
    my $node = $self->node();
    #
    # Add a feature that says this node should not be picked
    # unless the corresponding desire is in the vtop. This
    # allows the node to be picked, subject to other type constraints.
    #
    $self->addFeature('pcshared', 1.0, undef, 1);

    #
    # The pool daemon may override the share weight.
    #
    my $sharedweight = undef;
    $node->NodeAttribute("shared_weight", \$sharedweight);
    if (defined($sharedweight)) {
	$self->addFeature('shareweight', $sharedweight);
    } else {
	#
	# The point of this feature is to have assign favor shared nodes
	# that already have nodes on them, so that they are well packed.
	# Shared nodes with just a few vnodes on them are avoided so that
	# they will free up eventually. 
	#
	my $maxvnodes = 10;
	my $weight    = 0.5;
	my $gcount    = 0.0;
	if (defined($self->globalcount())) {
	    $gcount = $self->globalcount();
	}

	if (exists($self->{'AUXTYPES'}->{'pcvm'})) {
	    $maxvnodes = $self->{'AUXTYPES'}->{'pcvm'};
	}

	#
	# No point in the feature if no room left. 
	#
	if ($maxvnodes > $gcount) {
	    my $factor = ($gcount / $maxvnodes);
	    if ($factor < 0.25) {
		$weight = 0.8;
	    }
	    elsif ($factor > 0.75) {
		$weight = 0.1;
	    }
	    else {
		$weight = 0.3;
	    }
	    #addFeature('shareweight', $weight);
	}
    }
}

#
# Add any auxiliary types
#
sub processAuxtypes($)
{
    my ($self) = @_;
    my $node = $self->node();
    my $needvirtgoo = 0;
    foreach my $auxtype (keys(%{ $self->{'AUXTYPES'} })) {
	my $count = $self->{'AUXTYPES'}->{$auxtype};
	my $realtypename;

	# Map an auxtype back to its real type, unless it is a real type.
	if (defined($auxtypemap{$auxtype})) {
	    $realtypename = $auxtypemap{$auxtype};
	} else {
	    $realtypename = $auxtype;
	}
	my $realtype = NodeType->Lookup($realtypename);
	my $is_virtual = ($realtype->isvirtnode() && $count > 0);
	if (! $is_virtual) {
	    $self->addPType($auxtype, $count);
	} elsif ($print_virtual) {
	    $needvirtgoo = 1;
	    #
	    # If the node is shared, must subtract the current global count
	    # from the max first, to see if there is any room left.
	    #
	    if ($node->sharing_mode() && defined($self->globalcount())) {
		$count -= $self->globalcount();
	    }
	    if (defined($multiplex_override)
		&& $multiplex_override <= $count) {
		$count = $multiplex_override;
	    }

	    #
	    # Add in machine specific auxtypes that use the same count.
	    #
	    $self->addPType($node->type()."-vm", $count);

	    # And a legacy type.
	    my $legacy_type = $node->type();
	    if (($legacy_type =~ s/pc/pcvm/)) {
		$self->addPType($legacy_type, $count);
	    }
	    $self->addPType($auxtype, $count);
	}
    }
    if ($needvirtgoo) {
	$self->processVirtGoo();
    }
    my $simcap = $self->type()->simnode_capacity();
    my $needsim = ($print_sim && defined($simcap) && $simcap > 0);
    if ($needsim) {
	$self->processSim($simcap);
    }
    if ($needsim || $needvirtgoo) {
	$self->addPType("lan", undef, 1);
    }
    if (($needvirtgoo && ! $node->sharing_mode())
	|| $needsim) {
	$self->processCpuRam();
    }
}

sub processVirtGoo($)
{
    my ($self) = @_;
    # Add trivial bw spec., but only if the node type has it
    my $trivspeed = $self->type()->trivlink_maxspeed();
    if ($trivspeed) {
	$self->addFlag("trivial_bw", $trivspeed);
    }
    if (! $self->node()->sharing_mode()) {
	# This number can be use for fine-tuning packing
	$self->addFeature('virtpercent', 100, libptopnew::FD_ADDITIVE());
    }
    # Put this silly feature in so that we can try to keep vnodes
    # on the same pnode they were before - but only if updating
    if (defined($exempt_eid)) {
	$self->addFeature($self->node()->node_id(), 0.0);
    }
}

sub processSim($$)
{
    my ($self, $simcap) = @_;
    #
    # Use user specified multiplex factor
    #
    my $cap = $simcap;
    if (defined($multiplex_override) && $multiplex_override <= $simcap) {
	$cap = $multiplex_override;
    }
    $self->addPType("sim", $cap);
    # Add trivial bw spec.
    $self->addFlag("trivial_bw", 100000);
}

sub processCpuRam($)
{
    my ($self) = @_;
    my $cpu_speed = $self->type()->frequency();
    my $ram = $self->type()->memory();
    # Add CPU and RAM information
    $self->addFeature('cpu', $cpu_speed, libptopnew::FD_ADDITIVE())
	if (defined($cpu_speed));
    $self->addFeature('ram', $ram, libptopnew::FD_ADDITIVE())
	if (defined($ram));
    $self->addFeature('cpupercent', 92, libptopnew::FD_ADDITIVE()); # XXX Hack
    $self->addFeature('rampercent', 80, libptopnew::FD_ADDITIVE()); # XXX Hack
}

#
# Add in OS features.
#
sub processOsFeatures($)
{
    my ($self) = @_;
    my $node = $self->node();
    if ($node->sharing_mode()) {
	#
	# A shared node is running just one OS, and we put that in
	# so that the user can specify which of the current VM types
	# is wanted.
	#
	my $osid = $node->def_boot_osid();
	$self->addFeature('OS-'.$osid, 0.5);
	# Add any subOSes the shared node osid can support
	if (defined($osid_subosid{$osid})) {
	    foreach my $subosid (@{ $osid_subosid{$osid} }) {
		$self->addFeature('OS-'.$osid.'-'.$subosid, 0);
	    }
	}
    } elsif (exists($type_osid{$node->type()})) {
	#
	# Add in features for all of the OSes that this node (as
	# evidenced by its type) can support
	#
	foreach my $o1 (@{ $type_osid{$node->type()} }) {
	    $self->addFeature('OS-'.$o1, 0);
	    foreach my $o2 (@{ $osid_subosid{$o1} }) {
		$self->addFeature('OS-'.$o1.'-'.$o2, 0);
	    }
	}
    } elsif (! $self->type()->imageable() &&
	     defined($self->type()->default_osid())) {
	#
	# If node is not imageable (and thus no entries in osidtoimageid,
	# then assume it always has its default OSID loaded and ready to
	# go, so that assign will agree to the allocation (assign_wrapper
	# adds a desire that says it has to be running the OSID the user
	# has selected, or the default OSID from the node_types table).
	#
	$self->addFeature('OS-'.$self->type()->default_osid(), 0);
    }
}

sub processConnections($)
{
    my ($self) = @_;
    if (! $self->node()->sharing_mode()) {
	foreach my $name (keys(%{ $self->{'CONNECTIONS'} })) {
	    $self->addFeature('connected-to-'.$name, 0.0);
	}
    }
}

sub processSubnode($)
{
    my ($self) = @_;
    my $node = $self->node();
    if ($node->issubnode()) {
	$self->addFlag('subnode_of', $node->phys_nodeid());
    }
}

sub processWidearea($)
{
    my ($self) = @_;
    my $node = $self->node();
    if (! $self->iswidearea()) {
	return;
    }

    #
    # Grab the global allocation count of vnodes on this pnode (if there
    # is one). This modifies the counts below.  If the count has already
    # been reached, then do not put this node into the ptop file.
    #
    my $maxvnodes = $self->type()->GetAttribute("global_capacity");
    
    if (defined($maxvnodes) && defined($self->globalcount())) {
	$maxvnodes -= $self->globalcount();
	if ($maxvnodes <= 0) {
	    return;
	}
    }

    #
    # Mark any nodes that are not up with a feature, so that they won't
    # normally get assigned. We want to include them, though, because we
    # allow people to do fix-node to down nodes
    #
    if (! defined($self->status())) {
	print STDERR "Widearea node does not have a status: ".
	    $node->node_id()."\n";
	return;
    }
    if (! defined($node->eid()) || ! defined($node->pid())) {
	print STDERR "Widearea node is not reserved to an eid or pid".
	    $node->node_id()."\n";
	return;
    }
    if ($self->status() ne 'up' ||
	($node->eid() eq NODEDEAD_EID() || $node->pid() eq NODEDEAD_PID())) {
	if ($genimode) {
	    return;
	}
	$self->addFeature('down', 1);
    }

    my $site = $self->widearea('site');
    if ($site) {
	$self->addFeature($site, 0.99, libptopnew::FD_FIRSTFREE());
    }

    #
    # Add any auxiliary types.
    #
    foreach my $auxtype (keys(%{ $self->{'AUXTYPES'} })) {
	my $count = $self->{'AUXTYPES'}->{$auxtype};
	if (defined($maxvnodes) && $maxvnodes < $count) {
	    $count = $maxvnodes;
	}
	if (defined($multiplex_override) && $multiplex_override < $count) {
	    $count = $multiplex_override;
	}
	$self->addPType($auxtype, $count);
    }

    # Add trivial bw spec.
    $self->addFlag("trivial_bw", 400000);

    # Indicate that these nodes are beautiful and unique snowflakes
    $self->addFlag("unique", 1);

    $self->processOsFeatures();

    #
    # Put in a feature indicating whether or not this node has a bandwidth
    # cap
    #
    my $bwlimit = $self->widearea('bwlimit');
    if (!defined($bwlimit) || $bwlimit eq "-1") {
	$self->addFeature('nobwlimit', 0);
    } else {
	$self->addFeature('bwlimit', 0);
    }

}

sub processTypeFeatures($)
{
    my ($self) = @_;
    if ($self->iswidearea()
	|| ($self->islocal() && ! $self->node()->sharing_mode())) {
	if (exists($typefeatures{$self->node()->type()})) {
	    foreach my $feature (@{ $typefeatures{$self->node()->type()} }) {
		$self->addFeatureString($feature);
	    }
	}
    }
}

sub toString($)
{
    my ($self) = @_;
    my $result = "node " . $self->name();

    # Print types
    foreach my $type (@{ $self->{'PTYPES'} }) {
	$result .= " " . $type->toString();
    }
    $result .= " -";
    # Print features
    foreach my $feature (@{ $self->{'FEATURES'} }) {
	$result .= " " . $feature->toString();
    }
    $result .= " -";
    foreach my $flag (keys(%{ $self->{'FLAGS'} })) {
	my $value = $self->{'FLAGS'}->{$flag};
	# Special case for the unique flag. :)
	if ($flag eq "unique") {
	    $result .= " $flag";
	} else {
	    $result .= " $flag:$value";
	}
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

    # Add types
    foreach my $type (@{ $self->{'PTYPES'} }) {
	$type->toXML($xml, $self->type());
    }
    # Add features
    foreach my $feature (@{ $self->{'FEATURES'} }) {
	$feature->toXML($xml);
    }

    if (! GeniXML::IsVersion0($xml)) {
	foreach my $flag (keys(%{ $self->{'FLAGS'} })) {
	    my $value = $self->{'FLAGS'}->{$flag};
	    if ($flag eq 'trivial_bw') {
		my $triv = GeniXML::AddElement("trivial_bandwidth", $xml,
					       $GeniXML::EMULAB_NS);
		GeniXML::SetText("value", $triv, $value);
	    } elsif ($flag eq 'subnode_of') {
		my $suburn = GeniHRN::Generate($OURDOMAIN, "node", $value);
		my $relation = GeniXML::AddElement("relation", $xml);
		GeniXML::SetText("type", $relation, "subnode_of");
		GeniXML::SetText("component_id", $relation, $suburn);
	    } elsif ($flag eq 'unique') {
		GeniXML::AddElement("unique", $xml, $GeniXML::EMULAB_NS);
	    } elsif ($flag eq 'disallow_trivial_mix') {
		GeniXML::AddElement("disallow_trivial_mix", $xml,
				    $GeniXML::EMULAB_NS);
	    }
	}
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
    my ($self, $parent, $mainType) = @_;
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
	    $self->osidToXML($sliverxml, $mainType);
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

sub osidToXML($$$)
{
    my ($self, $parent, $type) = @_;
    my $default = $type->default_osid();
    foreach my $osid (@{ $type_osid{$type->type()} }) {
	my $os = $osinfo{$osid};
	if (! defined($os)) {
	    print STDERR "Undefined osid: ".$osid."\n";
	    next;
	}
	if ($os->protogeni_export() ||
	    (defined($default) && $default eq $type->type())) {
	    # Fill out new disk image tag
	    my $disk = GeniXML::AddElement("disk_image", $parent);
	    GeniXML::SetText("name", $disk, $os->osname());
	    GeniXML::SetText("os", $disk, $os->OS());
	    GeniXML::SetText("version", $disk, $os->version());
	    GeniXML::SetText("description", $disk, $os->description());
	    if (defined($default) && $default eq $type->type()) {
		GeniXML::SetText("default", $disk, "true");
	    }
	}
    }
}

###############################################################################
# Physical Node Type. These are the types which are printed out in the
# ptopgen file. Note that there is not a one-to-one correspondence
# between these types and the 'type' of the node in the database.

package libptop::feature;

sub CreateFromString($$)
{
    my ($class, $str) = @_;

    my $self = libptop::feature->Create($class, undef, undef);

    my ($name, $value) = split(/:/, $str, 2);
    my $flags = "";
    if ($name =~ /^\?\+/) {
	$self->set_additive();
	$name = substr($name, 2);
    } elsif ($name =~ /^\*&/) {
	$self->set_firstfree();
	$name = substr($name, 2);
    } elsif ($name =~ /^\*!/) {
	$self->set_onceonly();
	$name = substr($name, 2);
    } elsif ($value >= 1.0) {
	$self->set_violatable();
    }
    $self->{'NAME'} = $name;
    $self->{'VALUE'} = $value;

    return $self;
}

sub Create($$$;$$)
{
    my ($class, $name, $value, $flag, $violatable) = @_;

    my $self = {};

    $self->{'NAME'} = $name;
    $self->{'VALUE'} = $value;
    $self->{'VIOLATABLE'} = 0;
    $self->{'ADDITIVE'} = 0;
    $self->{'FIRSTFREE'} = 0;
    $self->{'ONCEONLY'} = 0;

    bless($self, $class);

    if (defined($violatable)) {
	$self->set_violatable();
    }
    if (defined($flag)) {
	if ($flag eq libptopnew::FD_ADDITIVE()) {
	    $self->set_additive();
	} elsif ($flag eq libptopnew::FD_FIRSTFREE()) {
	    $self->set_firstfree();
	} elsif ($flag eq libptopnew::FD_ONCEONLY()) {
	    $self->set_onceonly();
	}
    }

    return $self;
}

sub set_violatable($)  { $_[0]->{'VIOLATABLE'} = 1; }
sub set_additive($)    { $_[0]->{'ADDITIVE'} = 1; }
sub set_firstfree($)   { $_[0]->{'FIRSTFREE'} = 1; }
sub set_onceonly($)    { $_[0]->{'ONCEONLY'} = 1; }

sub violatable($)      { return $_[0]->{'VIOLATABLE'}; }
sub additive($)        { return $_[0]->{'ADDITIVE'}; }
sub firstfree($)       { return $_[0]->{'FIRSTFREE'}; }
sub onceonly($)        { return $_[0]->{'ONCEONLY'}; }

sub toString($)
{
    my ($self) = @_;
    my $result = "";
    if ($self->violatable()) {
	# No representation as flat string
    }
    if ($self->additive()) {
	$result .= "?+";
    } elsif ($self->firstfree()) {
	$result .= "*&";
    } elsif ($self->onceonly()) {
	$result .= "*!";
    }
    $result .= $self->{'NAME'}.':'.$self->{'VALUE'};
    return $result;
}

sub toXML($$)
{
    my ($self, $parent) = @_;
    if (! GeniXML::IsVersion0($parent)) {
	my $xml = GeniXML::AddElement("fd", $parent, $GeniXML::EMULAB_NS);
	GeniXML::SetText("name", $xml, $self->{'NAME'});
	GeniXML::SetText("weight", $xml, $self->{'VALUE'});
	if ($self->violatable()) {
	    GeniXML::SetText("violatable", $xml, "true");
	}
	if ($self->additive()) {
	    GeniXML::SetText("local_operator", $xml, "+");
	} elsif ($self->firstfree()) {
	    GeniXML::SetText("global_operator", $xml, "FirstFree");
	} elsif ($self->onceonly()) {
	    GeniXML::SetText("global_operator", $xml, "OnceOnly");
	}
    }
}

1;
