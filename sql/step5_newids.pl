#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007, 2009 University of Utah and the Flux Group.
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

# users
DBQueryFatal("lock tables users write, user_stats write");
if (! TableChanged("users", "uid_uuid")) {
    DBQueryFatal("alter table users ".
	"add `uid_uuid` varchar(40) NOT NULL default '' after uid_idx, ".
	"add KEY uid_uuid (`uid_uuid`)");
}
if (! TableChanged("user_stats", "uid_uuid")) {
    DBQueryFatal("alter table user_stats ".
	"add `uid_uuid` varchar(40) NOT NULL default '' after uid_idx, ".
	"add KEY uid_uuid (`uid_uuid`)");
}
my $query_result = DBQueryFatal("select uid_idx from users ".
				"where uid_uuid=''");
while (my ($uid_idx) = $query_result->fetchrow_array()) {
    my $uuid = NewUUID();

    if (!defined($uuid)) {
	die("*** $0:\n".
	    "    Could not generate a UUID for user: $uid_idx\n");
    }
    DBQueryFatal("update users set uid_uuid='$uuid' where uid_idx='$uid_idx'");
    DBQueryFatal("update user_stats set uid_uuid='$uuid' ".
		 "where uid_idx='$uid_idx'");
    select(undef, undef, undef, 0.02);
}
$query_result = DBQueryFatal("select uid_idx from user_stats ".
			     "where uid_uuid=''");
while (my ($uid_idx) = $query_result->fetchrow_array()) {
    my $uuid = NewUUID();

    if (!defined($uuid)) {
	die("*** $0:\n".
	    "    Could not generate a UUID for user: $uid_idx\n");
    }
    DBQueryFatal("update user_stats set uid_uuid='$uuid' ".
		 "where uid_idx='$uid_idx'");
    select(undef, undef, undef, 0.02);
}
DBQueryFatal("unlock tables");

# groups (no point in doing the project).
DBQueryFatal("lock tables groups write, group_stats write");
if (! TableChanged("groups", "gid_uuid")) {
    DBQueryFatal("alter table groups ".
	"add `gid_uuid` varchar(40) NOT NULL default '' after gid_idx, ".
	"add KEY gid_uuid (`gid_uuid`)");
}
if (! TableChanged("group_stats", "gid_uuid")) {
    DBQueryFatal("alter table group_stats ".
	"add `gid_uuid` varchar(40) NOT NULL default '' after gid_idx, ".
	"add KEY gid_uuid (`gid_uuid`)");
}
$query_result = DBQueryFatal("select gid_idx from groups ".
			     "where gid_uuid=''");
while (my ($gid_idx) = $query_result->fetchrow_array()) {
    my $uuid = NewUUID();

    if (!defined($uuid)) {
	die("*** $0:\n".
	    "    Could not generate a UUID for group: $gid_idx\n");
    }
    DBQueryFatal("update groups set gid_uuid='$uuid' ".
		 "where gid_idx='$gid_idx'");
    DBQueryFatal("update group_stats set gid_uuid='$uuid' ".
		 "where gid_idx='$gid_idx'");
    select(undef, undef, undef, 0.02);
}
$query_result = DBQueryFatal("select gid_idx from group_stats ".
			     "where gid_uuid=''");
while (my ($gid_idx) = $query_result->fetchrow_array()) {
    my $uuid = NewUUID();

    if (!defined($uuid)) {
	die("*** $0:\n".
	    "    Could not generate a UUID for group: $gid_idx\n");
    }
    DBQueryFatal("update group_stats set gid_uuid='$uuid' ".
		 "where gid_idx='$gid_idx'");
    select(undef, undef, undef, 0.02);
}
DBQueryFatal("unlock tables");

# experiments 
DBQueryFatal("lock tables experiments write, experiment_stats write");
if (! TableChanged("experiments", "eid_uuid")) {
    DBQueryFatal("alter table experiments ".
	"add `eid_uuid` varchar(40) NOT NULL default '' after eid, ".
	"add KEY eid_uuid (`eid_uuid`)");
}
if (! TableChanged("experiment_stats", "eid_uuid")) {
    DBQueryFatal("alter table experiment_stats ".
	"add `eid_uuid` varchar(40) NOT NULL default '' after eid, ".
	"add KEY eid_uuid (`eid_uuid`)");
}
$query_result = DBQueryFatal("select idx from experiments ".
			     "where eid_uuid=''");
while (my ($exptidx) = $query_result->fetchrow_array()) {
    my $uuid = NewUUID();

    if (!defined($uuid)) {
	die("*** $0:\n".
	    "    Could not generate a UUID for exptidx: $exptidx\n");
    }
    DBQueryFatal("update experiments set eid_uuid='$uuid' ".
		 "where idx='$exptidx'");
    DBQueryFatal("update experiment_stats set eid_uuid='$uuid' ".
		 "where exptidx='$exptidx'");
    select(undef, undef, undef, 0.02);
}
$query_result = DBQueryFatal("select exptidx from experiment_stats ".
			     "where eid_uuid=''");
while (my ($exptidx) = $query_result->fetchrow_array()) {
    my $uuid = NewUUID();

    if (!defined($uuid)) {
	die("*** $0:\n".
	    "    Could not generate a UUID for exptidx: $exptidx\n");
    }
    DBQueryFatal("update experiment_stats set eid_uuid='$uuid' ".
		 "where exptidx='$exptidx'");
    select(undef, undef, undef, 0.02);
}
DBQueryFatal("unlock tables");

# images
DBQueryFatal("lock tables images write");
if (! TableChanged("images", "uuid")) {
    DBQueryFatal("alter table images ".
	"add `uuid` varchar(40) NOT NULL default '' after imageid, ".
	"add KEY uuid (`uuid`)");
}
$query_result = DBQueryFatal("select imageid from images where uuid=''");
while (my ($imageid) = $query_result->fetchrow_array()) {
    my $uuid = NewUUID();

    if (!defined($uuid)) {
	die("*** $0:\n".
	    "    Could not generate a UUID for imageid: $imageid\n");
    }
    DBQueryFatal("update images set uuid='$uuid' ".
		 "where imageid='$imageid'");
    select(undef, undef, undef, 0.02);
}
DBQueryFatal("unlock tables");

# os_info
DBQueryFatal("lock tables os_info write");
if (! TableChanged("os_info", "uuid")) {
    DBQueryFatal("alter table os_info ".
	"add `uuid` varchar(40) NOT NULL default '' after osid, ".
	"add KEY uuid (`uuid`)");
}
$query_result = DBQueryFatal("select osid from os_info where uuid=''");
while (my ($osid) = $query_result->fetchrow_array()) {
    my $uuid = NewUUID();

    if (!defined($uuid)) {
	die("*** $0:\n".
	    "    Could not generate a UUID for osid: $osid\n");
    }
    DBQueryFatal("update os_info set uuid='$uuid' ".
		 "where osid='$osid'");
    select(undef, undef, undef, 0.02);
}
DBQueryFatal("unlock tables");

