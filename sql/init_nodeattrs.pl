#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#
use English;

use lib "/usr/testbed/lib";
use libdb;
use libtestbed;

#
# Untaint the path
# 
$ENV{'PATH'} = '/bin:/usr/bin:/usr/sbin';
delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};

my $query_result =
    DBQueryFatal("select * from node_types");

sub AddAttr($$$$)
{
    my ($nodetype, $attrkey, $attrtype, $attrvalue) = @_;

    print("replace into node_type_attributes ".
	  "(type, attrkey, attrvalue, attrtype) values ".
	  "('$nodetype', '$attrkey', '$attrvalue', '$attrtype');\n");
}

while (my $rowref = $query_result->fetchrow_hashref()) {
    my $type = $rowref->{'type'};

    AddAttr($type, "default_osid", "string", $rowref->{'osid'})
	if (defined($rowref->{'osid'}) && $rowref->{'osid'} ne "");
    AddAttr($type, "rebootable", "boolean", $rowref->{'isrebootable'})
	if (defined($rowref->{'isrebootable'}));

    next
	if ($rowref->{'isvirtnode'} ||
	    $rowref->{'isjailed'} ||
	    $rowref->{'isdynamic'} ||
	    $rowref->{'issubnode'} ||
	    $rowref->{'isplabdslice'} ||
	    $rowref->{'issimnode'});

    AddAttr($type, "processor", "string", $rowref->{'proc'})
	if (defined($rowref->{'proc'}));
    AddAttr($type, "frequency", "integer", $rowref->{'speed'})
	if (defined($rowref->{'speed'}));
    AddAttr($type, "memory", "integer", $rowref->{'RAM'})
	if (defined($rowref->{'RAM'}));
    AddAttr($type, "disksize", "float", $rowref->{'HD'})
	if (defined($rowref->{'HD'}));
    # Kill this?
    AddAttr($type, "max_interfaces", "integer", $rowref->{'max_interfaces'})
	if (defined($rowref->{'max_interfaces'}));
    # Kill this?
    AddAttr($type, "control_network", "integer", $rowref->{'control_net'})
	if (defined($rowref->{'control_net'}));
    AddAttr($type, "power_delay", "integer", $rowref->{'power_time'})
	if (defined($rowref->{'power_time'}));
    AddAttr($type, "default_imageid", "string", $rowref->{'imageid'})
	if (defined($rowref->{'imageid'}) && $rowref->{'imageid'} ne "");
    AddAttr($type, "imageable", "boolean", $rowref->{'imageable'})
	if (defined($rowref->{'imageable'}) && $rowref->{'imageable'});
    AddAttr($type, "delay_capacity", "integer", $rowref->{'delay_capacity'})
	if (defined($rowref->{'delay_capacity'}));
    AddAttr($type, "virtnode_capacity", "integer",
	    $rowref->{'virtnode_capacity'})
	if (defined($rowref->{'virtnode_capacity'}));
    AddAttr($type, "control_interface", "string", $rowref->{'control_iface'})
	if (defined($rowref->{'control_iface'}));
    AddAttr($type, "disktype", "string", $rowref->{'disktype'})
	if (defined($rowref->{'disktype'}));
    AddAttr($type, "bootdisk_unit", "integer", $rowref->{'bootdisk_unit'})
	if (defined($rowref->{'bootdisk_unit'}));
    AddAttr($type, "delay_osid", "string", $rowref->{'delay_osid'})
	if (defined($rowref->{'delay_osid'}) && $rowref->{'delay_osid'} ne "");
    AddAttr($type, "jail_osid", "string", $rowref->{'jail_osid'})
	if (defined($rowref->{'jail_osid'}) && $rowref->{'jail_osid'} ne "");
    # Kill
    # modelnetcore_osid
    # modelnetedge_osid
    AddAttr($type, "pxe_boot_path", "string", $rowref->{'pxe_boot_path'})
	if (defined($rowref->{'pxe_boot_path'}) &&
	    $rowref->{'pxe_boot_path'} ne "");
    AddAttr($type, "simnode_capacity", "integer",$rowref->{'simnode_capacity'})
	if (defined($rowref->{'simnode_capacity'}));
    AddAttr($type, "trivlink_maxspeed", "integer",
	    $rowref->{'trivlink_maxspeed'})
	if (defined($rowref->{'trivlink_maxspeed'}));
    AddAttr($type, "bios_waittime", "integer", $rowref->{'bios_waittime'})
	if (defined($rowref->{'bios_waittime'}));
    AddAttr($type, "adminmfs_osid", "string", $rowref->{'adminmfs_osid'})
	if (defined($rowref->{'adminmfs_osid'}));
    AddAttr($type, "diskloadmfs_osid", "string", $rowref->{'diskloadmfs_osid'})
	if (defined($rowref->{'diskloadmfs_osid'}));

}
