#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
use English;

#
# TODO:
#  * group_policies table needs a webpage to make it easier?
#  * The images table uses $pid-$imagename for the unique imageid.
#  * Ditto for the os_info table.
#  * sql/database-fill-supplemental.sql. Ditto for install/images.
#  * motelogfiles table; Dave will handle this.
#  * reserved_pid in the nodes table; need a webpage to set that in bulk.
#  * imagepid in partitions table. Not sure how this used.
#  * wireless_stats; Dave will handle this.
#

use lib "/usr/testbed/lib";
use libdb;
use libtestbed;

#
# Untaint the path
# 
$ENV{'PATH'} = '/bin:/usr/bin:/usr/sbin';
delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};

#
# Grab the ids that were generated earlier with init_newids and save them.
#
my %gids = ();

my $query_result =
    DBQueryFatal("select pid,pid_idx,gid,gid_idx from groups");

while (my ($pid,$pid_idx,$gid,$gid_idx) = $query_result->fetchrow_array()) {
    $gids{"$pid:$gid"} = $gid_idx;
}

# And experiments
my %experiments = ();

$query_result =
    DBQueryFatal("select pid,eid,idx from experiments");

while (my ($pid,$eid,$idx) = $query_result->fetchrow_array()) {
    $experiments{"$pid:$eid"} = $idx;
}

#
# See if a table has been changed by looking for the existence of a field.
#
sub TableChanged($$)
{
    my ($table, $slot) = @_;

    my $describe_result =
	DBQueryFatal("describe $table $slot");

    return $describe_result->numrows;
}

#
# Change the primary key for a table
#
sub ChangePrimaryKey($$)
{
    my ($table, $newkey) = @_;

    DBQuery("alter table $table drop primary key");
    DBQueryFatal("alter table $table add PRIMARY KEY ($newkey)");
}

#
# Update table for pid/gid changes
#
sub UpdateTablePidGid($$$$$)
{
    my ($table, $newpidslot, $newgidslot, $oldpidslot, $oldgidslot) = @_;
    my $query_result;
    
    if (defined($oldgidslot)) {
	print "Updating '$newpidslot/$newgidslot' from ".
	    "'$oldpidslot/$oldgidslot' in table '$table'\n";

	$query_result = 
	    DBQueryFatal("select distinct $oldpidslot,$oldgidslot ".
			 "  from $table ".
			 "where ${newpidslot}='0' or ${newgidslot}='0'");
	
    }
    else {
	print "Updating '$newpidslot' from '$oldpidslot' in table '$table'\n";

	$query_result = 
	    DBQueryFatal("select distinct $oldpidslot from $table ".
		     "where ${newpidslot}='0'");
    }

    while (my ($oldpidvalue,$oldgidvalue) = $query_result->fetchrow_array()) {
	next
	    if (!defined($oldpidvalue) ||
		$oldpidvalue eq "+" || $oldpidvalue eq "-");

	if (!exists($gids{"$oldpidvalue:$oldpidvalue"})) {
	    print "No pid defined for $oldpidvalue\n";
	    next;
	}
	my $newpididx = $gids{"$oldpidvalue:$oldpidvalue"};

	if (defined($oldgidslot)) {
	    if (!exists($gids{"$oldpidvalue:$oldgidvalue"})) {
		print "No gid defined for $oldpidvalue:$oldgidvalue\n";
		next;
	    }
	    my $newgididx = $gids{"$oldpidvalue:$oldgidvalue"};

	    DBQueryFatal("update $table set ".
			 "  ${newpidslot}='${newpididx}', ".
			 "  ${newgidslot}='${newgididx}' ".
			 "where ${oldpidslot}='${oldpidvalue}' and ".
			 "      ${oldgidslot}='${oldgidvalue}'");
	}
	else {
	    DBQueryFatal("update $table set ${newpidslot}='${newpididx}' ".
			 "where ${oldpidslot}='${oldpidvalue}' and ".
			 "      ${newpidslot}='0'");
	}
    }
}

#
# Update table for pid/eid changes
#
sub UpdateTablePidEid($$$$)
{
    my ($table, $newslot, $oldpidslot, $oldeidslot) = @_;
    my $query_result;
    
    print "Updating '$newslot' from '$oldpidslot/$oldeidslot' in ".
	"table '$table'\n";

    $query_result = 
	DBQueryFatal("select distinct $oldpidslot,$oldeidslot from $table ".
		     "where ${newslot}='0'");

    while (my ($oldpidvalue,$oldeidvalue) = $query_result->fetchrow_array()) {
	next
	    if (!defined($oldpidvalue) || $oldpidvalue eq "" ||
		!defined($oldeidvalue) || $oldeidvalue eq "");

	if (!exists($experiments{"$oldpidvalue:$oldeidvalue"})) {
	    die("No mapping for $oldpidvalue/$oldeidvalue\n");
	}

	my $newidx = $experiments{"$oldpidvalue:$oldeidvalue"};

	DBQueryFatal("update $table set ".
		     "  ${newslot}='${newidx}' ".
		     "where ${oldpidslot}='${oldpidvalue}' and ".
		     "      ${oldeidslot}='${oldeidvalue}'");
    }
}

# Delays table.
DBQueryFatal("lock tables delays write");
if (! TableChanged("delays", "exptidx")) {
    DBQueryFatal("alter table delays ".
		 "add `exptidx` int(11) NOT NULL default '0' after iface1");
    DBQueryFatal("alter table delays add INDEX exptidx (exptidx)");
}
UpdateTablePidEid("delays", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# elabinelab_vlans table.
DBQueryFatal("lock tables elabinelab_vlans write");
if (! TableChanged("elabinelab_vlans", "exptidx")) {
    DBQueryFatal("alter table elabinelab_vlans ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("elabinelab_vlans", "exptidx", "pid", "eid");
    DBQueryFatal("alter table elabinelab_vlans drop PRIMARY KEY");
    DBQueryFatal("alter table elabinelab_vlans add UNIQUE KEY ".
		 "pideid (`pid`,`eid`,`inner_id`)");
    DBQueryFatal("alter table elabinelab_vlans add PRIMARY KEY ".
		 "(`exptidx`,`inner_id`)");
}
UpdateTablePidEid("elabinelab_vlans", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# event_groups table.
DBQueryFatal("lock tables event_groups write");
if (! TableChanged("event_groups", "exptidx")) {
    DBQueryFatal("alter table event_groups ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("event_groups", "exptidx", "pid", "eid");
    DBQueryFatal("alter table event_groups add UNIQUE KEY ".
		 "pideid (`pid`,`eid`,`idx`)");
    DBQueryFatal("alter table event_groups drop PRIMARY KEY");
    DBQueryFatal("alter table event_groups add PRIMARY KEY ".
		 "(`exptidx`,`idx`)");
}
UpdateTablePidEid("event_groups", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# eventlist table.
DBQueryFatal("lock tables eventlist write");
if (! TableChanged("eventlist", "exptidx")) {
    DBQueryFatal("alter table eventlist ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("eventlist", "exptidx", "pid", "eid");
    DBQueryFatal("alter table eventlist add UNIQUE KEY ".
		 "pideid (`pid`,`eid`,`idx`)");
    DBQueryFatal("alter table eventlist drop PRIMARY KEY");
    DBQueryFatal("alter table eventlist add PRIMARY KEY ".
		 "(`exptidx`,`idx`)");
}
UpdateTablePidEid("eventlist", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# experiment_templates
DBQueryFatal("lock tables experiment_templates write");
if (! TableChanged("experiment_templates", "exptidx")) {
    DBQueryFatal("alter table experiment_templates ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    DBQueryFatal("alter table experiment_templates ".
		 "  add INDEX exptidx (exptidx)");
}
UpdateTablePidEid("experiment_templates", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# experiment_templates
DBQueryFatal("lock tables experiment_templates write");
if (! TableChanged("experiment_templates", "pid_idx")) {
    DBQueryFatal("alter table experiment_templates ".
     "add pid_idx mediumint(8) unsigned NOT NULL default '0' after uid_idx, ".
     "add gid_idx mediumint(8) unsigned NOT NULL default '0' after pid_idx");
}
UpdateTablePidGid("experiment_templates",
		  "pid_idx", "gid_idx", "pid", "gid");
DBQueryFatal("unlock tables");

# firewall_rules
DBQueryFatal("lock tables firewall_rules write");
if (! TableChanged("firewall_rules", "exptidx")) {
    DBQueryFatal("alter table firewall_rules ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("firewall_rules", "exptidx", "pid", "eid");
    DBQueryFatal("alter table firewall_rules add KEY ".
		 "pideid (`pid`,`eid`,`fwname`,`ruleno`)");
    DBQueryFatal("alter table firewall_rules drop PRIMARY KEY");
    DBQueryFatal("alter table firewall_rules add PRIMARY KEY ".
		 "(`exptidx`,`fwname`,`ruleno`)");
}
UpdateTablePidEid("firewall_rules", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# firewalls
DBQueryFatal("lock tables firewalls write");
if (! TableChanged("firewalls", "exptidx")) {
    DBQueryFatal("alter table firewalls ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("firewalls", "exptidx", "pid", "eid");
    DBQueryFatal("alter table firewalls add KEY ".
		 "pideid (`pid`,`eid`,`fwname`)");
    DBQueryFatal("alter table firewalls drop PRIMARY KEY");
    DBQueryFatal("alter table firewalls add PRIMARY KEY ".
		 "(`exptidx`,`fwname`)");
}
UpdateTablePidEid("firewalls", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# ipport_ranges
DBQueryFatal("lock tables ipport_ranges write");
if (! TableChanged("ipport_ranges", "exptidx")) {
    DBQueryFatal("alter table ipport_ranges ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("ipport_ranges", "exptidx", "pid", "eid");
    DBQueryFatal("alter table ipport_ranges add UNIQUE KEY ".
		 "pideid (`pid`,`eid`)");
    DBQueryFatal("alter table ipport_ranges drop PRIMARY KEY");
    DBQueryFatal("alter table ipport_ranges add PRIMARY KEY (`exptidx`)");
}
UpdateTablePidEid("ipport_ranges", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# ipsubnets
DBQueryFatal("lock tables ipsubnets write");
if (! TableChanged("ipsubnets", "exptidx")) {
    DBQueryFatal("alter table ipsubnets ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("ipsubnets", "exptidx", "pid", "eid");
    DBQueryFatal("alter table ipsubnets add KEY pideid (`pid`,`eid`)");
    DBQueryFatal("alter table ipsubnets add KEY exptidx (`exptidx`)");
}
UpdateTablePidEid("ipsubnets", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# linkdelays table.
DBQueryFatal("lock tables linkdelays write");
if (! TableChanged("linkdelays", "exptidx")) {
    DBQueryFatal("alter table linkdelays ".
		 "add `exptidx` int(11) NOT NULL default '0' after type");
    DBQueryFatal("alter table linkdelays add INDEX exptidx (exptidx)");
}
UpdateTablePidEid("linkdelays", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# next_reserve table.
DBQueryFatal("lock tables next_reserve write");
if (! TableChanged("next_reserve", "exptidx")) {
    DBQueryFatal("alter table next_reserve ".
		 "add `exptidx` int(11) NOT NULL default '0' after node_id");
}
UpdateTablePidEid("next_reserve", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# nseconfigs table.
DBQueryFatal("lock tables nseconfigs write");
if (! TableChanged("nseconfigs", "exptidx")) {
    DBQueryFatal("alter table nseconfigs ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("nseconfigs", "exptidx", "pid", "eid");
    DBQueryFatal("alter table nseconfigs add UNIQUE KEY ".
		 "pideid (`pid`,`eid`,`vname`)");
    DBQueryFatal("alter table nseconfigs drop PRIMARY KEY");
    DBQueryFatal("alter table nseconfigs add PRIMARY KEY ".
		 "(`exptidx`,`vname`)");
}
UpdateTablePidEid("nseconfigs", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# nsfiles
DBQueryFatal("lock tables nsfiles write");
if (! TableChanged("nsfiles", "exptidx")) {
    DBQueryFatal("alter table nsfiles ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("nsfiles", "exptidx", "pid", "eid");
    DBQueryFatal("alter table nsfiles add UNIQUE KEY pideid (`pid`,`eid`)");
    DBQueryFatal("alter table nsfiles drop PRIMARY KEY");
    DBQueryFatal("alter table nsfiles add PRIMARY KEY (`exptidx`)");
}
UpdateTablePidEid("nsfiles", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# plab_slice_nodes table.
DBQueryFatal("lock tables plab_slice_nodes write");
if (! TableChanged("plab_slice_nodes", "exptidx")) {
    DBQueryFatal("alter table plab_slice_nodes ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    DBQueryFatal("alter table plab_slice_nodes add INDEX exptidx (exptidx)");
}
UpdateTablePidEid("plab_slice_nodes", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# plab_slices
DBQueryFatal("lock tables plab_slices write");
if (! TableChanged("plab_slices", "exptidx")) {
    DBQueryFatal("alter table plab_slices ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("plab_slices", "exptidx", "pid", "eid");
    DBQueryFatal("alter table plab_slices add UNIQUE KEY ".
		 "pideid (`pid`,`eid`)");
    DBQueryFatal("alter table plab_slices drop PRIMARY KEY");
    DBQueryFatal("alter table plab_slices add PRIMARY KEY (`exptidx`)");
}
UpdateTablePidEid("plab_slices", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# port_registration table.
DBQueryFatal("lock tables port_registration write");
if (! TableChanged("port_registration", "exptidx")) {
    DBQueryFatal("alter table port_registration ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("port_registration", "exptidx", "pid", "eid");
    DBQueryFatal("alter table port_registration add UNIQUE KEY ".
		 "pideid (`pid`,`eid`,`service`)");
    DBQueryFatal("alter table port_registration drop PRIMARY KEY");
    DBQueryFatal("alter table port_registration add PRIMARY KEY ".
		 "(`exptidx`,`service`)");
}
UpdateTablePidEid("port_registration", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# traces
DBQueryFatal("lock tables traces write");
if (! TableChanged("traces", "exptidx")) {
    DBQueryFatal("alter table traces ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("traces", "exptidx", "pid", "eid");
    DBQueryFatal("alter table traces add KEY exptidx (`exptidx`)");
}
UpdateTablePidEid("traces", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# tunnels table.
DBQueryFatal("lock tables tunnels write");
if (! TableChanged("tunnels", "exptidx")) {
    DBQueryFatal("alter table tunnels ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("tunnels", "exptidx", "pid", "eid");
    DBQueryFatal("alter table tunnels add UNIQUE KEY ".
		 "pideid (`pid`,`eid`,`node_id`,`vname`)");
    DBQueryFatal("alter table tunnels drop PRIMARY KEY");
    DBQueryFatal("alter table tunnels add PRIMARY KEY ".
		 "(`exptidx`,`node_id`,`vname`)");
}
UpdateTablePidEid("tunnels", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# v2pmap table.
DBQueryFatal("lock tables v2pmap write");
if (! TableChanged("v2pmap", "exptidx")) {
    DBQueryFatal("alter table v2pmap ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("v2pmap", "exptidx", "pid", "eid");
    DBQueryFatal("alter table v2pmap add UNIQUE KEY ".
		 "pideid (`pid`,`eid`,`vname`)");
    DBQueryFatal("alter table v2pmap drop PRIMARY KEY");
    DBQueryFatal("alter table v2pmap add PRIMARY KEY ".
		 "(`exptidx`,`vname`)");
}
UpdateTablePidEid("v2pmap", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# virt_agents table.
DBQueryFatal("lock tables virt_agents write");
if (! TableChanged("virt_agents", "exptidx")) {
    DBQueryFatal("alter table virt_agents ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("virt_agents", "exptidx", "pid", "eid");
    DBQueryFatal("alter table virt_agents add UNIQUE KEY ".
		 "pideid (`pid`,`eid`,`vname`,`vnode`)");
    DBQueryFatal("alter table virt_agents drop PRIMARY KEY");
    DBQueryFatal("alter table virt_agents add PRIMARY KEY ".
		 "(`exptidx`,`vname`,`vnode`)");
}
UpdateTablePidEid("virt_agents", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# virt_firewalls table.
DBQueryFatal("lock tables virt_firewalls write");
if (! TableChanged("virt_firewalls", "exptidx")) {
    DBQueryFatal("alter table virt_firewalls ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("virt_firewalls", "exptidx", "pid", "eid");
    DBQueryFatal("alter table virt_firewalls add UNIQUE KEY ".
		 "pideid (`pid`,`eid`,`fwname`)");
    DBQueryFatal("alter table virt_firewalls drop PRIMARY KEY");
    DBQueryFatal("alter table virt_firewalls add PRIMARY KEY ".
		 "(`exptidx`,`fwname`)");
}
UpdateTablePidEid("virt_firewalls", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# virt_lan_lans table.
DBQueryFatal("lock tables virt_lan_lans write");
if (! TableChanged("virt_lan_lans", "exptidx")) {
    DBQueryFatal("alter table virt_lan_lans ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("virt_lan_lans", "exptidx", "pid", "eid");
    DBQueryFatal("alter table virt_lan_lans add UNIQUE KEY ".
		 "idx (`pid`,`eid`,`idx`)");
    DBQueryFatal("alter table virt_lan_lans drop PRIMARY KEY");
    DBQueryFatal("alter table virt_lan_lans add PRIMARY KEY ".
		 "(`exptidx`,`idx`)");
}
UpdateTablePidEid("virt_lan_lans", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# virt_lan_member_settings table.
DBQueryFatal("lock tables virt_lan_member_settings write");
if (! TableChanged("virt_lan_member_settings", "exptidx")) {
    DBQueryFatal("alter table virt_lan_member_settings ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("virt_lan_member_settings", "exptidx", "pid", "eid");
    DBQueryFatal("alter table virt_lan_member_settings add UNIQUE KEY ".
		 "pideid (`pid`,`eid`,`vname`,`member`,`capkey`)");
    DBQueryFatal("alter table virt_lan_member_settings drop PRIMARY KEY");
    DBQueryFatal("alter table virt_lan_member_settings add PRIMARY KEY ".
		 "(`exptidx`,`vname`,`member`,`capkey`)");
}
UpdateTablePidEid("virt_lan_member_settings", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# virt_lan_settings table.
DBQueryFatal("lock tables virt_lan_settings write");
if (! TableChanged("virt_lan_settings", "exptidx")) {
    DBQueryFatal("alter table virt_lan_settings ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("virt_lan_settings", "exptidx", "pid", "eid");
    DBQueryFatal("alter table virt_lan_settings add UNIQUE KEY ".
		 "pideid (`pid`,`eid`,`vname`,`capkey`)");
    DBQueryFatal("alter table virt_lan_settings drop PRIMARY KEY");
    DBQueryFatal("alter table virt_lan_settings add PRIMARY KEY ".
		 "(`exptidx`,`vname`,`capkey`)");
}
UpdateTablePidEid("virt_lan_settings", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# virt_lans table.
DBQueryFatal("lock tables virt_lans write");
if (! TableChanged("virt_lans", "exptidx")) {
    DBQueryFatal("alter table virt_lans ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("virt_lans", "exptidx", "pid", "eid");
    DBQueryFatal("alter table virt_lans add KEY ".
		 "pideid (`pid`,`eid`,`vname`,`vnode`)");
    DBQueryFatal("alter table virt_lans add UNIQUE KEY ".
		 "vport (`pid`,`eid`,`vname`,`vnode`,`vport`)");
    DBQueryFatal("alter table virt_lans add PRIMARY KEY ".
		 "(`exptidx`,`vname`,`vnode`,`vport`)");
}
UpdateTablePidEid("virt_lans", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# virt_node_desires table.
DBQueryFatal("lock tables virt_node_desires write");
if (! TableChanged("virt_node_desires", "exptidx")) {
    DBQueryFatal("alter table virt_node_desires ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("virt_node_desires", "exptidx", "pid", "eid");
    DBQueryFatal("alter table virt_node_desires add UNIQUE KEY ".
		 "pideid (`pid`,`eid`,`vname`,`desire`)");
    DBQueryFatal("alter table virt_node_desires drop PRIMARY KEY");
    DBQueryFatal("alter table virt_node_desires add PRIMARY KEY ".
		 "(`exptidx`,`vname`,`desire`)");
}
UpdateTablePidEid("virt_node_desires", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# virt_node_startloc table.
DBQueryFatal("lock tables virt_node_startloc write");
if (! TableChanged("virt_node_startloc", "exptidx")) {
    DBQueryFatal("alter table virt_node_startloc ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("virt_node_startloc", "exptidx", "pid", "eid");
    DBQueryFatal("alter table virt_node_startloc add UNIQUE KEY ".
		 "pideid (`pid`,`eid`,`vname`)");
    DBQueryFatal("alter table virt_node_startloc drop PRIMARY KEY");
    DBQueryFatal("alter table virt_node_startloc add PRIMARY KEY ".
		 "(`exptidx`,`vname`)");
}
UpdateTablePidEid("virt_node_startloc", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# virt_node_startloc table.
DBQueryFatal("lock tables virt_node_startloc write");
if (! TableChanged("virt_node_startloc", "exptidx")) {
    DBQueryFatal("alter table virt_node_startloc ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("virt_node_startloc", "exptidx", "pid", "eid");
    DBQueryFatal("alter table virt_node_startloc add UNIQUE KEY ".
		 "pideid (`pid`,`eid`,`vname`)");
    DBQueryFatal("alter table virt_node_startloc drop PRIMARY KEY");
    DBQueryFatal("alter table virt_node_startloc add PRIMARY KEY ".
		 "(`exptidx`,`vname`)");
}
UpdateTablePidEid("virt_node_startloc", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# virt_nodes table.
DBQueryFatal("lock tables virt_nodes write");
if (! TableChanged("virt_nodes", "exptidx")) {
    DBQueryFatal("alter table virt_nodes ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("virt_nodes", "exptidx", "pid", "eid");
    DBQueryFatal("alter table virt_nodes add UNIQUE KEY ".
		 "pideid (`pid`,`eid`,`vname`)");
    DBQueryFatal("alter table virt_nodes add PRIMARY KEY ".
		 "(`exptidx`,`vname`)");
}
UpdateTablePidEid("virt_nodes", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# virt_parameters table.
DBQueryFatal("lock tables virt_parameters write");
if (! TableChanged("virt_parameters", "exptidx")) {
    DBQueryFatal("alter table virt_parameters ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("virt_parameters", "exptidx", "pid", "eid");
    DBQueryFatal("alter table virt_parameters add UNIQUE KEY ".
		 "pideid (`pid`,`eid`,`name`)");
    DBQueryFatal("alter table virt_parameters drop PRIMARY KEY");
    DBQueryFatal("alter table virt_parameters add PRIMARY KEY ".
		 "(`exptidx`,`name`)");
}
UpdateTablePidEid("virt_parameters", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# virt_programs table.
DBQueryFatal("lock tables virt_programs write");
if (! TableChanged("virt_programs", "exptidx")) {
    DBQueryFatal("alter table virt_programs ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("virt_programs", "exptidx", "pid", "eid");
    DBQueryFatal("alter table virt_programs add UNIQUE KEY ".
		 "pideid (`pid`,`eid`,`vnode`,`vname`)");
    DBQueryFatal("alter table virt_programs drop PRIMARY KEY");
    DBQueryFatal("alter table virt_programs add PRIMARY KEY ".
		 "(`exptidx`,`vnode`,`vname`)");
}
UpdateTablePidEid("virt_programs", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# virt_routes table.
DBQueryFatal("lock tables virt_routes write");
if (! TableChanged("virt_routes", "exptidx")) {
    DBQueryFatal("alter table virt_routes ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("virt_routes", "exptidx", "pid", "eid");
    DBQueryFatal("alter table virt_routes add UNIQUE KEY ".
		 "pideid (`pid`,`eid`,`vname`,`src`,`dst`)");
    DBQueryFatal("alter table virt_routes drop PRIMARY KEY");
    DBQueryFatal("alter table virt_routes add PRIMARY KEY ".
		 "(`exptidx`,`vname`,`src`,`dst`)");
}
UpdateTablePidEid("virt_routes", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# virt_simnode_attributes table.
DBQueryFatal("lock tables virt_simnode_attributes write");
if (! TableChanged("virt_simnode_attributes", "exptidx")) {
    DBQueryFatal("alter table virt_simnode_attributes ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("virt_simnode_attributes", "exptidx", "pid", "eid");
    DBQueryFatal("alter table virt_simnode_attributes add UNIQUE KEY ".
		 "pideid (`pid`,`eid`,`vname`)");
    DBQueryFatal("alter table virt_simnode_attributes drop PRIMARY KEY");
    DBQueryFatal("alter table virt_simnode_attributes add PRIMARY KEY ".
		 "(`exptidx`,`vname`)");
}
UpdateTablePidEid("virt_simnode_attributes", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# virt_tiptunnels table.
DBQueryFatal("lock tables virt_tiptunnels write");
if (! TableChanged("virt_tiptunnels", "exptidx")) {
    DBQueryFatal("alter table virt_tiptunnels ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("virt_tiptunnels", "exptidx", "pid", "eid");
    DBQueryFatal("alter table virt_tiptunnels add UNIQUE KEY ".
		 "pideid (`pid`,`eid`,`host`,`vnode`)");
    DBQueryFatal("alter table virt_tiptunnels drop PRIMARY KEY");
    DBQueryFatal("alter table virt_tiptunnels add PRIMARY KEY ".
		 "(`exptidx`,`host`,`vnode`)");
}
UpdateTablePidEid("virt_tiptunnels", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# virt_trafgens table.
DBQueryFatal("lock tables virt_trafgens write");
if (! TableChanged("virt_trafgens", "exptidx")) {
    DBQueryFatal("alter table virt_trafgens ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("virt_trafgens", "exptidx", "pid", "eid");
    DBQueryFatal("alter table virt_trafgens add UNIQUE KEY ".
		 "pideid (`pid`,`eid`,`vnode`,`vname`)");
    DBQueryFatal("alter table virt_trafgens drop PRIMARY KEY");
    DBQueryFatal("alter table virt_trafgens add PRIMARY KEY ".
		 "(`exptidx`,`vnode`,`vname`)");
}
UpdateTablePidEid("virt_trafgens", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# virt_user_environment table.
DBQueryFatal("lock tables virt_user_environment write");
if (! TableChanged("virt_user_environment", "exptidx")) {
    DBQueryFatal("alter table virt_user_environment ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("virt_user_environment", "exptidx", "pid", "eid");
    DBQueryFatal("alter table virt_user_environment add UNIQUE KEY ".
		 "pideid (`pid`,`eid`,`idx`)");
    DBQueryFatal("alter table virt_user_environment drop PRIMARY KEY");
    DBQueryFatal("alter table virt_user_environment add PRIMARY KEY ".
		 "(`exptidx`,`idx`)");
}
UpdateTablePidEid("virt_user_environment", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# virt_vtypes table.
DBQueryFatal("lock tables virt_vtypes write");
if (! TableChanged("virt_vtypes", "exptidx")) {
    DBQueryFatal("alter table virt_vtypes ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("virt_vtypes", "exptidx", "pid", "eid");
    DBQueryFatal("alter table virt_vtypes add UNIQUE KEY ".
		 "pideid (`pid`,`eid`,`name`)");
    DBQueryFatal("alter table virt_vtypes add PRIMARY KEY ".
		 "(`exptidx`,`name`)");
}
UpdateTablePidEid("virt_vtypes", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# vis_graphs table.
DBQueryFatal("lock tables vis_graphs write");
if (! TableChanged("vis_graphs", "exptidx")) {
    DBQueryFatal("alter table vis_graphs ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("vis_graphs", "exptidx", "pid", "eid");
    DBQueryFatal("alter table vis_graphs add UNIQUE KEY ".
		 "pideid (`pid`,`eid`)");
    DBQueryFatal("alter table vis_graphs drop PRIMARY KEY");
    DBQueryFatal("alter table vis_graphs add PRIMARY KEY (`exptidx`)");
}
UpdateTablePidEid("vis_graphs", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# vis_nodes table.
DBQueryFatal("lock tables vis_nodes write");
if (! TableChanged("vis_nodes", "exptidx")) {
    DBQueryFatal("alter table vis_nodes ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("vis_nodes", "exptidx", "pid", "eid");
    DBQueryFatal("alter table vis_nodes add UNIQUE KEY ".
		 "pideid (`pid`,`eid`,`vname`)");
    DBQueryFatal("alter table vis_nodes drop PRIMARY KEY");
    DBQueryFatal("alter table vis_nodes add PRIMARY KEY ".
		 "(`exptidx`,`vname`)");
}
UpdateTablePidEid("vis_nodes", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# vlans table.
DBQueryFatal("lock tables vlans write");
if (! TableChanged("vlans", "exptidx")) {
    DBQueryFatal("alter table vlans ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    UpdateTablePidEid("vlans", "exptidx", "pid", "eid");
    DBQueryFatal("alter table vlans add KEY exptidx (`exptidx`,`virtual`)");
}
UpdateTablePidEid("vlans", "exptidx", "pid", "eid");
DBQueryFatal("unlock tables");

# datapository_databases
if (! TableChanged("datapository_databases", "pid_idx")) {
    DBQueryFatal("alter table datapository_databases ".
     "add pid_idx mediumint(8) unsigned NOT NULL default '0' after uid, ".
     "add gid_idx mediumint(8) unsigned NOT NULL default '0' after pid_idx");
}
UpdateTablePidGid("datapository_databases",
		  "pid_idx", "gid_idx", "pid", "gid");

# experiments
if (! TableChanged("experiments", "pid_idx")) {
    DBQueryFatal("alter table experiments ".
     "add elabinelab_exptidx int(11) default NULL after elabinelab_eid, ".
     "add pid_idx mediumint(8) unsigned NOT NULL default '0' after eid, ".
     "add gid_idx mediumint(8) unsigned NOT NULL default '0' after pid_idx");
    UpdateTablePidGid("experiments",
		      "pid_idx", "gid_idx", "pid", "gid");
    DBQueryFatal("alter table experiments add UNIQUE KEY ".
		 "pideid (`pid`,`eid`)");
    DBQueryFatal("alter table experiments add UNIQUE KEY ".
		 "pididxeid (`pid_idx`,`eid`)");
    DBQueryFatal("alter table experiments drop PRIMARY KEY");
    DBQueryFatal("alter table experiments add PRIMARY KEY (`idx`)");
    DBQueryFatal("alter table experiments drop KEY idx");
}
UpdateTablePidGid("experiments",
		  "pid_idx", "gid_idx", "pid", "gid");
UpdateTablePidEid("experiments",
		  "elabinelab_exptidx", "pid", "elabinelab_eid");

# group_stats
if (! TableChanged("group_stats", "pid_idx")) {
    DBQueryFatal("alter table group_stats ".
	 "add pid_idx mediumint(8) unsigned NOT NULL default '0' after gid");
}
UpdateTablePidGid("group_stats",
		  "pid_idx", undef, "pid", undef);

# images
if (! TableChanged("images", "pid_idx")) {
    DBQueryFatal("alter table images ".
     "add pid_idx mediumint(8) unsigned NOT NULL default '0' after imagename,".
     "add gid_idx mediumint(8) unsigned NOT NULL default '0' after pid_idx");
    UpdateTablePidGid("images",
		      "pid_idx", "gid_idx", "pid", "gid");
    DBQueryFatal("alter table images drop PRIMARY KEY");
    DBQueryFatal("alter table images drop KEY imageid");
    DBQueryFatal("alter table images add PRIMARY KEY (`imageid`)");
    DBQueryFatal("alter table images add UNIQUE KEY pid (`pid`,`imagename`)");
}
UpdateTablePidGid("images",
		  "pid_idx", "gid_idx", "pid", "gid");

# group_policies
if (! TableChanged("group_policies", "pid_idx")) {
    DBQueryFatal("alter table group_policies ".
     "add pid_idx mediumint(8) unsigned NOT NULL default '0' after gid, ".
     "add gid_idx mediumint(8) unsigned NOT NULL default '0' after pid_idx");
    DBQueryFatal("update group_policies set gid_idx=0,pid_idx=0 ".
		 "where pid='-'");
    DBQueryFatal("update group_policies set gid_idx=999999,pid_idx=999999 ".
		 "where pid='+'");
    UpdateTablePidGid("group_policies",
		      "pid_idx", "gid_idx", "pid", "gid");
    DBQueryFatal("alter table group_policies add UNIQUE KEY ".
		 "pid (`pid`,`gid`,`policy`,`auxdata`)");
    DBQueryFatal("alter table group_policies drop PRIMARY KEY");
    DBQueryFatal("alter table group_policies add PRIMARY KEY ".
		 "(`gid_idx`,`policy`,`auxdata`)");
}
DBQueryFatal("update group_policies set gid_idx=0,pid_idx=0 ".
	     "where pid='-'");
DBQueryFatal("update group_policies set gid_idx=999999,pid_idx=999999 ".
	     "where pid='+'");
UpdateTablePidGid("group_policies",
		  "pid_idx", "gid_idx", "pid", "gid");

# nodetypeXpid_permissions
if (! TableChanged("nodetypeXpid_permissions", "pid_idx")) {
    DBQueryFatal("alter table nodetypeXpid_permissions ".
	 "add pid_idx mediumint(8) unsigned NOT NULL default '0' after pid");
    UpdateTablePidGid("nodetypeXpid_permissions",
		      "pid_idx", undef, "pid", undef);
    DBQueryFatal("alter table nodetypeXpid_permissions add UNIQUE KEY ".
		 "typepid (`type`,`pid`)");
    DBQueryFatal("alter table nodetypeXpid_permissions drop PRIMARY KEY");
    DBQueryFatal("alter table nodetypeXpid_permissions add PRIMARY KEY ".
		 "(`type`,`pid_idx`)");
}
UpdateTablePidGid("nodetypeXpid_permissions",
		  "pid_idx", undef, "pid", undef);

# os_info
if (! TableChanged("os_info", "pid_idx")) {
    DBQueryFatal("alter table os_info ".
	"add pid_idx mediumint(8) unsigned NOT NULL default '0' after pid");
    UpdateTablePidGid("os_info",
		      "pid_idx", undef, "pid", undef);
    DBQueryFatal("alter table os_info drop PRIMARY KEY");
    DBQueryFatal("alter table os_info drop KEY osid");
    DBQueryFatal("alter table os_info add PRIMARY KEY (`osid`)");
    DBQueryFatal("alter table os_info add UNIQUE KEY pid (`pid`,`osname`)");
}
UpdateTablePidGid("os_info",
		  "pid_idx", undef, "pid", undef);

# last_reservation
if (! TableChanged("last_reservation", "pid_idx")) {
    DBQueryFatal("alter table last_reservation ".
	"add pid_idx mediumint(8) unsigned NOT NULL default '0' after pid");
    UpdateTablePidGid("last_reservation",
		      "pid_idx", undef, "pid", undef);
    DBQueryFatal("alter table last_reservation add UNIQUE KEY ".
		 "pid (`node_id`,`pid`)");
    DBQueryFatal("alter table last_reservation drop PRIMARY KEY");
    DBQueryFatal("alter table last_reservation add PRIMARY KEY ".
		 "(`node_id`,`pid_idx`)");
}
UpdateTablePidGid("last_reservation",
		  "pid_idx", undef, "pid", undef);

# reserved table
DBQueryFatal("lock tables reserved write");
if (! TableChanged("reserved", "exptidx")) {
    DBQueryFatal("alter table reserved ".
		 "add `exptidx` int(11) NOT NULL default '0' after eid");
    DBQueryFatal("alter table reserved ".
		 "add `old_exptidx` int(11) NOT NULL ".
		 "default '0' after old_eid");
    UpdateTablePidEid("reserved", "exptidx", "pid", "eid");
    UpdateTablePidEid("reserved", "old_exptidx", "old_pid", "old_eid");
    DBQueryFatal("alter table reserved add UNIQUE KEY ".
		 "vname2 (`exptidx`,`vname`)");
    DBQueryFatal("alter table reserved add KEY ".
		 "old_exptidx (`old_exptidx`)");
}
UpdateTablePidEid("reserved", "exptidx", "pid", "eid");
UpdateTablePidEid("reserved", "old_exptidx", "old_pid", "old_eid");
DBQueryFatal("unlock tables");

# experiment_stats table.
DBQueryFatal("lock tables experiment_stats write");
if (! TableChanged("experiment_stats", "pid_idx")) {
    DBQueryFatal("alter table experiment_stats ".
	"add pid_idx mediumint(8) unsigned NOT NULL default '0' after pid, ".
        "add gid_idx mediumint(8) unsigned NOT NULL default '0' after gid");
    UpdateTablePidGid("experiment_stats",
		      "pid_idx", "gid_idx", "pid", "gid");
#    DBQueryFatal("alter table experiment_stats drop KEY pideid");
    DBQueryFatal("alter table experiment_stats drop KEY exptidx");
    DBQueryFatal("alter table experiment_stats add KEY ".
		 "pideid (`pid`,`eid`)");
    DBQueryFatal("alter table experiment_stats drop PRIMARY KEY");
    DBQueryFatal("alter table experiment_stats add PRIMARY KEY ".
		 "(`exptidx`)");
}
UpdateTablePidGid("experiment_stats",
		  "pid_idx", "gid_idx", "pid", "gid");
DBQueryFatal("unlock tables");

