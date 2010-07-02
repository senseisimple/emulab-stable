#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006-2008 University of Utah and the Flux Group.
# All rights reserved.
#
use English;

use lib "/usr/testbed/lib";
use libdb;
use libtestbed;
use User;

#
# Untaint the path
# 
$ENV{'PATH'} = '/bin:/usr/bin:/usr/sbin';
delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};

#
# Grab the ids that were generated earlier with init_newids and save them.
#
my %uids = ();

# Ick, this turns up in various places!
$uids{'root'} = 0;

my $query_result =
    DBQueryFatal("select uid,uid_idx from users");

while (my ($uid,$uid_idx) = $query_result->fetchrow_array()) {
    $uids{$uid} = $uid_idx;
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
# Update table
#
sub UpdateTable($$$)
{
    my ($table, $newslot, $oldslot) = @_;

    print "Updating '$newslot' from '$oldslot' in table '$table'\n";

    my $query_result =
	DBQueryFatal("select distinct $oldslot from $table ".
		     "where ${newslot}='0' or ${newslot} is null");

    while (my ($oldvalue) = $query_result->fetchrow_array()) {
	next
	    if (!defined($oldvalue));
	
	my $idx = $uids{$oldvalue};

	#
	# Create an archived user as a placeholder for this person.
	# This can be a problem if a uid was deleted and then reused.
	#
	if (!defined($idx)) {
	    print "*** Creating archived user for '$oldvalue'\n";
	    
	    my %newuser_args = ("notes" =>
				'Deleted user restored as Archived');
	    
	    my $newuser = User->Create($oldvalue,
				       $User::NEWUSER_FLAGS_ARCHIVED|
				       $User::NEWUSER_FLAGS_NOUUID,
				       \%newuser_args);
	    
	    $idx = $newuser->uid_idx();
	    $uids{$oldvalue} = $idx;
	}
	
	DBQueryFatal("update $table set ${newslot}='${idx}' ".
		     "where ${oldslot}='${oldvalue}' and ".
		     "      (${newslot}='0' or ${newslot} is null)");
    }
}

# user_sslcerts
if (! TableChanged("user_sslcerts", "uid_idx")) {
    DBQueryFatal("alter table user_sslcerts add ".
		 "  uid_idx mediumint(8) unsigned NOT NULL default '0' ".
		 "after uid");
}
UpdateTable("user_sslcerts", "uid_idx", "uid");

# user_sfskeys
if (! TableChanged("user_sfskeys", "uid_idx")) {
    DBQueryFatal("alter table user_sfskeys add ".
		 "  uid_idx mediumint(8) unsigned NOT NULL default '0' ".
		 "after uid");
    UpdateTable("user_sfskeys", "uid_idx", "uid");
    DBQueryFatal("alter table user_sfskeys drop PRIMARY KEY");
    DBQueryFatal("alter table user_sfskeys add PRIMARY KEY (uid_idx,comment)");
    DBQueryFatal("alter table user_sfskeys add INDEX uid (uid,comment)");
}
UpdateTable("user_sfskeys", "uid_idx", "uid");

# user_pubkeys
if (! TableChanged("user_pubkeys", "uid_idx")) {
    DBQueryFatal("alter table user_pubkeys add ".
		 "  uid_idx mediumint(8) unsigned NOT NULL default '0' ".
		 "after uid");
    UpdateTable("user_pubkeys", "uid_idx", "uid");
    DBQueryFatal("alter table user_pubkeys add INDEX uid (uid,idx)");
    DBQueryFatal("alter table user_pubkeys drop PRIMARY KEY");
    DBQueryFatal("alter table user_pubkeys add PRIMARY KEY (uid_idx,idx)");
}
UpdateTable("user_pubkeys", "uid_idx", "uid");

# Projects table.
if (! TableChanged("projects", "head_idx")) {
    DBQueryFatal("alter table projects add ".
		 "  head_idx mediumint(8) unsigned NOT NULL default '0' ".
		 "after head_uid");
}
UpdateTable("projects", "head_idx", "head_uid");

# Groups table.
if (! TableChanged("groups", "leader_idx")) {
    DBQueryFatal("alter table groups add ".
		 "  leader_idx mediumint(8) unsigned NOT NULL default '0' ".
		 "after leader");
}
UpdateTable("groups", "leader_idx", "leader");

# experiments table
if (! TableChanged("experiments", "creator_idx")) {
    DBQueryFatal("alter table experiments add ".
		 "  creator_idx mediumint(8) unsigned NOT NULL default '0' ".
		 "after gid");
    DBQueryFatal("alter table experiments add ".
		 "  swapper_idx mediumint(8) unsigned default NULL ".
		 "after creator_idx");
}
UpdateTable("experiments", "creator_idx", "expt_head_uid");
UpdateTable("experiments", "swapper_idx", "expt_swap_uid");

# experiment_stats table
if (! TableChanged("experiment_stats", "creator_idx")) {
    DBQueryFatal("alter table experiment_stats add ".
		 "  creator_idx mediumint(8) unsigned NOT NULL default '0' ".
		 "after creator");
}
UpdateTable("experiment_stats", "creator_idx", "creator");

# experiment_templates table
if (! TableChanged("experiment_templates", "uid_idx")) {
    DBQueryFatal("alter table experiment_templates add ".
		 "  uid_idx mediumint(8) unsigned NOT NULL default '0' ".
		 "after uid");
}
UpdateTable("experiment_templates", "uid_idx", "uid");

# experiment_template_metadata_items table
if (! TableChanged("experiment_template_metadata_items", "uid_idx")) {
    DBQueryFatal("alter table experiment_template_metadata_items add ".
		 "  uid_idx mediumint(8) unsigned NOT NULL default '0' ".
		 "after uid");
}
UpdateTable("experiment_template_metadata_items", "uid_idx", "uid");

# experiment_template_instances table
if (! TableChanged("experiment_template_instances", "uid_idx")) {
    DBQueryFatal("alter table experiment_template_instances add ".
		 "  uid_idx mediumint(8) unsigned NOT NULL default '0' ".
		 "after uid");
}
UpdateTable("experiment_template_instances", "uid_idx", "uid");

# datapository_databases table
if (! TableChanged("datapository_databases", "uid_idx")) {
    DBQueryFatal("alter table datapository_databases add ".
		 "  uid_idx mediumint(8) unsigned NOT NULL default '0' ".
		 "after uid");
}
UpdateTable("datapository_databases", "uid_idx", "uid");

# images table
if (! TableChanged("images", "creator_idx")) {
    DBQueryFatal("alter table images add ".
		 "  creator_idx mediumint(8) unsigned NOT NULL default '0' ".
		 "after creator");
}
UpdateTable("images", "creator_idx", "creator");

# os_info table
if (! TableChanged("os_info", "creator_idx")) {
    DBQueryFatal("alter table os_info add ".
		 "  creator_idx mediumint(8) unsigned NOT NULL default '0' ".
		 "after creator");
}
UpdateTable("os_info", "creator_idx", "creator");

# knowledge_base_entries
if (! TableChanged("knowledge_base_entries", "creator_idx")) {
    DBQueryFatal("alter table knowledge_base_entries add ".
		 "  creator_idx mediumint(8) unsigned NOT NULL default '0' ".
		 "after creator_uid");
    DBQueryFatal("alter table knowledge_base_entries add ".
		 "  modifier_idx mediumint(8) unsigned NOT NULL default '0' ".
		 "after modifier_uid");
    DBQueryFatal("alter table knowledge_base_entries add ".
		 "  archiver_idx mediumint(8) unsigned NOT NULL default '0' ".
		 "after archiver_uid");
}
UpdateTable("knowledge_base_entries", "creator_idx", "creator_uid");
UpdateTable("knowledge_base_entries", "modifier_idx", "modifier_uid");
UpdateTable("knowledge_base_entries", "archiver_idx", "archiver_uid");

# mailman_listnames
if (! TableChanged("mailman_listnames", "owner_idx")) {
    DBQueryFatal("alter table mailman_listnames add ".
		 "  owner_idx mediumint(8) unsigned NOT NULL default '0' ".
		 "after owner_uid");
}
UpdateTable("mailman_listnames", "owner_idx", "owner_uid");

# node_history
if (! TableChanged("node_history", "uid_idx")) {
    DBQueryFatal("alter table node_history add ".
		 "  uid_idx mediumint(8) unsigned NOT NULL default '0' ".
		 "after uid");
}
UpdateTable("node_history", "uid_idx", "uid");

# nodelog
if (! TableChanged("nodelog", "reporting_idx")) {
    DBQueryFatal("alter table nodelog add ".
		 "  reporting_idx mediumint(8) unsigned NOT NULL default '0' ".
		 "after reporting_uid");
}
UpdateTable("nodelog", "reporting_idx", "reporting_uid");

# testbed_stats
if (! TableChanged("testbed_stats", "uid_idx")) {
    DBQueryFatal("alter table testbed_stats add ".
		 "  uid_idx mediumint(8) unsigned NOT NULL default '0' ".
		 "after uid");
}
UpdateTable("testbed_stats", "uid_idx", "uid");

# widearea_accounts
if (! TableChanged("widearea_accounts", "uid_idx")) {
    DBQueryFatal("alter table widearea_accounts add ".
		 "  uid_idx mediumint(8) unsigned NOT NULL default '0' ".
		 "after uid");
}
UpdateTable("widearea_accounts", "uid_idx", "uid");

# widearea_nodeinfo
if (! TableChanged("widearea_nodeinfo", "contact_idx")) {
    DBQueryFatal("alter table widearea_nodeinfo add ".
		 "  contact_idx mediumint(8) unsigned NOT NULL default '0' ".
		 "after contact_uid");
}
UpdateTable("widearea_nodeinfo", "contact_idx", "contact_uid");

