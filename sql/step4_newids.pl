#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
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

my %new_osids    = ();
my %new_imageids = ();
my $index        = 100;

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

DBQueryFatal("drop table if exists ".
	     " temp_images, temp_os_info, temp_o2i, ".
	     " temp_osid_map, temp_partitions, temp_nodes, ".
	     " temp_attributes, temp_cur_reloads, temp_sched_reloads");

DBQueryFatal("create table temp_os_info like os_info");
DBQueryFatal("create table temp_images like images");
DBQueryFatal("create table temp_o2i (".
	     " `osid` int(8) unsigned NOT NULL default '0',".
	     " `type` varchar(30) NOT NULL default '', ".
	     " `imageid` int(8) unsigned NOT NULL default '0', ".
	     " PRIMARY KEY  (`osid`,`type`) ".
	     " ) ENGINE=MyISAM DEFAULT CHARSET=latin1");
DBQueryFatal("create table temp_osid_map (".
	     " `osid` int(8) unsigned NOT NULL default '0',".
	     " `btime` datetime NOT NULL default '1000-01-01 00:00:00', ".
	     " `etime` datetime NOT NULL default '9999-12-31 23:59:59', ".
	     " `nextosid` int(8) unsigned NOT NULL default '0', ".
	     "  PRIMARY KEY  (`osid`,`btime`,`etime`) ".
	     " ) ENGINE=MyISAM DEFAULT CHARSET=latin1");
DBQueryFatal("create table temp_partitions (".
	     " `node_id` varchar(32) NOT NULL default '', ".
	     " `partition` tinyint(4) NOT NULL default '0', ".
	     " `osid` int(8) unsigned default NULL, ".
	     " `imageid` int(8) unsigned default NULL, ".
	     " `imagepid` varchar(12) NOT NULL default '', ".
	     " PRIMARY KEY  (`node_id`,`partition`), ".
	     " KEY `osid` (`osid`) ".
	     " ) ENGINE=MyISAM DEFAULT CHARSET=latin1");
DBQueryFatal("create table temp_attributes like node_type_attributes");
DBQueryFatal("create table temp_nodes like nodes");
DBQueryFatal("create table temp_sched_reloads (".
	     " `node_id` varchar(32) NOT NULL default '', ".
	     " `image_id` int(8) unsigned NOT NULL default '0', ".
	     " `reload_type` enum('netdisk','frisbee') default NULL, ".
	     " PRIMARY KEY  (`node_id`) ".
	     " ) ENGINE=MyISAM DEFAULT CHARSET=latin1");
DBQueryFatal("create table temp_cur_reloads (".
	     " `node_id` varchar(32) NOT NULL default '', ".
	     " `image_id` int(8) unsigned NOT NULL default '0', ".
	     " `mustwipe` tinyint(4) NOT NULL default '0', ".
	     " PRIMARY KEY  (`node_id`) ".
	     " ) ENGINE=MyISAM DEFAULT CHARSET=latin1");

DBQueryFatal("lock tables emulab_indicies write," .
	     "         images write, os_info write, ".
	     "         osidtoimageid write, osid_map write, ".
	     "         partitions write, nodes write, ".
	     "         node_type_attributes write, ".
	     "         scheduled_reloads write, current_reloads write, ".
	     "         temp_images write, temp_osid_map write, ".
	     "         temp_o2i write, temp_os_info write, ".
	     "         temp_partitions write, temp_nodes write, ".
	     "         temp_attributes write, temp_cur_reloads write, ".
	     "         temp_sched_reloads write");

#
# Grab all known OSIDs now and assign new integer based IDs.
#
my $query_result = DBQueryFatal("select osid from os_info");

while (my ($osid) = $query_result->fetchrow_array()) {
    $new_osids{$osid} = $index++;
}

#
# Ditto for IMAGEIDs, but watch for ezid images, which share the same
# ID with the os_info table.
#
$query_result = DBQueryFatal("select imageid,ezid from images");

while (my ($imageid, $ezid) = $query_result->fetchrow_array()) {
    my $this_index = ($ezid ? $new_osids{$imageid} : $index++);
	
    $new_imageids{$imageid} = $this_index;
}

# os_info
DBQueryFatal("insert into temp_os_info select * from os_info");
DBQueryFatal("alter table temp_os_info ".
	     "add `old_osid` varchar(35) NOT NULL default '' after osid, ".
	     "add `old_nextosid` varchar(35) NOT NULL default '' ".
	     "after nextosid");
DBQueryFatal("update temp_os_info set old_osid=osid");
DBQueryFatal("update temp_os_info set old_nextosid=nextosid");
DBQueryFatal("alter table temp_os_info add KEY old_osid (`old_osid`)");
DBQueryFatal("alter table temp_os_info drop PRIMARY KEY");
DBQueryFatal("alter table temp_os_info drop osid");
DBQueryFatal("alter table temp_os_info drop nextosid");
DBQueryFatal("alter table temp_os_info ".
	     "add `osid` int(8) unsigned NOT NULL default '0' ".
	     "after pid_idx, ".
	     "add `nextosid` int(8) unsigned default NULL ".
	     "after op_mode");

#
# Update the table with the new osids.
#
$query_result = DBQueryFatal("select old_osid,old_nextosid from temp_os_info");

while (my ($old_osid,$old_nextosid) = $query_result->fetchrow_array()) {
    my $new_osid = $new_osids{$old_osid};
    my $new_nextosid = "NULL";

    if (defined($old_nextosid) && $old_nextosid ne "") {
	if (!exists($new_osids{$old_nextosid})) {
	    print "*** $old_nextosid in $old_osid record not defined!\n";
	    $new_nextosid = 0;
	}
	else {
	    $new_nextosid = $new_osids{$old_nextosid};
	}
    }
    DBQueryFatal("update temp_os_info set ".
		 "    osid=$new_osid, nextosid=$new_nextosid ".
		 "where old_osid='$old_osid'");
}
DBQueryFatal("alter table temp_os_info add PRIMARY KEY (`osid`)");

# images
DBQueryFatal("insert into temp_images select * from images");
DBQueryFatal("alter table temp_images ".
	     "add `old_imageid` varchar(45) NOT NULL default '' ".
	     "after imageid");
DBQueryFatal("update temp_images set old_imageid=imageid");
DBQueryFatal("alter table temp_images add KEY old_imageid (`old_imageid`)");
DBQueryFatal("alter table temp_images drop PRIMARY KEY");
DBQueryFatal("alter table temp_images drop imageid");
DBQueryFatal("alter table temp_images ".
	     "add `imageid` int(8) unsigned NOT NULL default '0' ".
	     " after gid, ".
	     "drop part1_osid, drop part2_osid, drop part3_osid, ".
	     "drop part4_osid, drop default_osid, ".
	     "add `part1_osid` int(8) unsigned default NULL ".
	     " after loadlength, ".
	     "add `part2_osid` int(8) unsigned default NULL ".
	     " after part1_osid, ".
	     "add `part3_osid` int(8) unsigned default NULL ".
	     " after part2_osid, ".
	     "add `part4_osid` int(8) unsigned default NULL ".
	     " after part3_osid, ".
	     "add `default_osid` int(8) unsigned NOT NULL default '0' ".
	     " after part4_osid");
    
#
# Assign new unique IDs. Watch for ezid images, which use the same
# ID for both the imageid and osid.
#
$query_result =
    DBQueryFatal("select imageid,part1_osid,part2_osid, ".
		 "  part3_osid, part3_osid, default_osid ".
		 "from images");

while (my ($old_imageid, $old_part1, $old_part2,
	   $old_part3, $old_part4, $old_default)
       = $query_result->fetchrow_array()) {

    my $new_imageid = $new_imageids{$old_imageid};
    my $new_part1   = "NULL";
    my $new_part2   = "NULL";
    my $new_part3   = "NULL";
    my $new_part4   = "NULL";
    my $new_default = "NULL";

    if (defined($old_part1) && $old_part1 ne "") {
	$new_part1 = $new_osids{$old_part1};
	if (!defined($new_part1)) {
	    print "*** $old_part1 in image $old_imageid record not defined!\n";
	    $new_part1 = "NULL";
	}
    }
    if (defined($old_part2) && $old_part2 ne "") {
	$new_part2 = $new_osids{$old_part2};
	if (!defined($new_part2)) {
	    print "*** $old_part2 in image $old_imageid record not defined!\n";
	    $new_part2 = "NULL";
	}
    }
    if (defined($old_part3) && $old_part3 ne "") {
	$new_part3 = $new_osids{$old_part3};
	if (!defined($new_part3)) {
	    print "*** $old_part3 in image $old_imageid record not defined!\n";
	    $new_part3 = "NULL";
	}
    }
    if (defined($old_part4) && $old_part4 ne "") {
	$new_part4 = $new_osids{$old_part4};
	if (!defined($new_part4)) {
	    print "*** $old_part4 in image $old_imageid record not defined!\n";
	    $new_part4 = "NULL";
	}
    }
    if (defined($old_default) && $old_default ne "") {
	$new_default = $new_osids{$old_default};
	if (!defined($new_default)) {
	    print "*** $old_default in image $old_imageid record not defined!\n";
	    $new_default = "NULL";
	}
    }
	
    DBQueryFatal("update temp_images set imageid=$new_imageid, ".
		 "  part1_osid=$new_part1,part2_osid=$new_part2, ".
		 "  part3_osid=$new_part3,part3_osid=$new_part3, ".
		 "  default_osid=$new_default ".
		 "where old_imageid='$old_imageid'");
}
DBQueryFatal("alter table temp_images add PRIMARY KEY (`imageid`)");

DBQueryFatal("replace into emulab_indicies (name, idx) ".
	     "values ('next_osid', $index), ('next_imageid',$index)");

# osidtoimageid
$query_result = DBQueryFatal("select * from osidtoimageid");

while (my ($old_osid, $type, $old_imageid) = $query_result->fetchrow_array()) {
    my $new_osid    = $new_osids{$old_osid};
    my $new_imageid = $new_imageids{$old_imageid};

    if (!defined($new_osid) || !defined($new_imageid)) {
	$new_osid = "";
	$new_imageid = "";
	print "*** osidtoimageid: ".
	    "$old_osid ($new_osid), $type, $old_imageid ($new_imageid)\n";
	next;
    }
    DBQueryFatal("insert into temp_o2i ".
		 " values($new_osid, '$type', $new_imageid)");
}

# osid_map
$query_result = DBQueryFatal("select * from osid_map");

while (my ($old_osid, $btime, $etime, $old_nextosid) =
       $query_result->fetchrow_array()) {
    my $new_osid  = $new_osids{$old_osid};
    my $new_next  = $new_osids{$old_nextosid};

    if (!defined($new_osid) || !defined($new_next)) {
	$new_osid = "";
	$new_next = "";
	print "*** osid_map: ".
	    "$old_osid ($new_osid), $old_nextosid ($new_next)\n";
	next;
    }
    DBQueryFatal("insert into temp_osid_map ".
		 " values($new_osid, '$btime', '$etime', $new_next)");
}

# scheduled_reloads
$query_result = DBQueryFatal("select * from scheduled_reloads");

while (my ($node_id, $old_imageid, $reload_type) =
       $query_result->fetchrow_array()) {
    my $new_imageid = $new_imageids{$old_imageid};

    if (!defined($new_imageid)) {
	$new_imageid = "";
	print "*** scheduled_reloads: $node_id ".
	    "$old_imageid ($new_imageid)\n";
	next;
    }
    DBQueryFatal("insert into temp_sched_reloads ".
		 " values('$node_id', $new_imageid, '$reload_type')");
}

# scheduled_reloads
$query_result = DBQueryFatal("select * from current_reloads");

while (my ($node_id, $old_imageid, $mustwipe) =
       $query_result->fetchrow_array()) {
    my $new_imageid = $new_imageids{$old_imageid};

    if (!defined($new_imageid)) {
	$new_imageid = "";
	print "*** current_reloads: ".
	    "$node_id: $old_imageid ($new_imageid)\n";
	next;
    }
    DBQueryFatal("insert into temp_cur_reloads ".
		 " values('$node_id', $new_imageid, '$mustwipe')");
}

# partitions
$query_result = DBQueryFatal("select * from partitions");

while (my ($node_id, $partition, $old_osid, $old_imageid, $imagepid) =
       $query_result->fetchrow_array()) {
    my $new_osid    = $new_osids{$old_osid};
    my $new_imageid = (defined($old_imageid) ?
		       $new_imageids{$old_imageid} : "NULL");
    
    if (!defined($new_osid) ||
	(defined($old_imageid) && !defined($new_imageid))) {
	$new_osid = "";
	$new_imageid = "";
	print "*** partition: $node_id ".
	    "$old_osid ($new_osid), $old_imageid ($new_imageid)\n";
	next;
    }
    DBQueryFatal("insert into temp_partitions ".
		 " values('$node_id', $partition, ".
		 "        $new_osid, $new_imageid, '$imagepid')");
}

# node type attributes
DBQueryFatal("insert into temp_attributes select * from node_type_attributes");

$query_result =
    DBQueryFatal("select type, attrkey, attrvalue from node_type_attributes ".
		 "where attrkey='adminmfs_osid' or ".
		 "      attrkey='diskloadmfs_osid' or ".
		 "      attrkey='default_osid' or ".
		 "      attrkey='default_imageid' or ".
		 "      attrkey='delay_osid' or ".
		 "      attrkey='jail_osid'");

while (my ($type, $key, $val) = $query_result->fetchrow_array()) {
    $new_val = ($key eq "default_imageid" ?
		$new_imageids{$val} : $new_osids{$val});

    if (!defined($new_val)) {
	print "*** No attrvalue for $type,$key,$val\n";
	next;
    }

    DBQueryFatal("update temp_attributes set ".
		 "   attrvalue='$new_val', attrtype='integer' ".
		 "where type='$type' and attrkey='$key'");
}

# nodes
DBQueryFatal("insert into temp_nodes select * from nodes");
DBQueryFatal("alter table temp_nodes ".
	     "drop def_boot_osid, drop next_boot_osid, ".
	     "drop temp_boot_osid, drop osid, ".
	     "add `def_boot_osid` int(8) unsigned default NULL ".
	     " after role, ".
	     "add `temp_boot_osid` int(8) unsigned default NULL ".
	     " after def_boot_cmd_line, ".
	     "add `next_boot_osid` int(8) unsigned default NULL ".
	     " after temp_boot_osid, ".
	     "add `osid` int(8) unsigned default NULL ".
	     " after ipodhash");

$query_result =
    DBQueryFatal("select node_id, def_boot_osid, temp_boot_osid, ".
		 "  next_boot_osid, osid ".
		 "from nodes");

while (my ($node_id, $old_def, $old_temp, $old_next, $old_osid) =
       $query_result->fetchrow_array()) {
    my $new_def  = "NULL";
    my $new_temp = "NULL";
    my $new_next = "NULL";
    my $new_osid = "NULL";

    if (defined($old_def) && $old_def ne "") {
	$new_def = $new_osids{$old_def};
	if (!defined($new_def)) {
	    print "*** $node_id no new osid for $old_def\n";
	    $new_def = "NULL";
	}
    }
    if (defined($old_temp) && $old_temp ne "") {
	$new_temp = $new_osids{$old_temp};
	if (!defined($new_temp)) {
	    print "*** $node_id no new osid for $old_temp\n";
	    $new_temp = "NULL";
	}
    }
    if (defined($old_next) && $old_next ne "") {
	$new_next = $new_osids{$old_next};
	if (!defined($new_next)) {
	    print "*** $node_id no new osid for $old_next\n";
	    $new_next = "NULL";
	}
    }
    if (defined($old_osid) && $old_osid ne "") {
	$new_osid = $new_osids{$old_osid};
	if (!defined($new_osid)) {
	    print "*** $node_id no new osid for $old_osid\n";
	    $new_osid = "NULL";
	}
    }
    
    DBQueryFatal("update temp_nodes set ".
		 "  def_boot_osid=$new_def, temp_boot_osid=$new_temp, ".
		 "  next_boot_osid=$new_next, osid=$new_osid ".
		 "where node_id='$node_id'");
}

# Make these tables live!
DBQueryFatal("unlock tables");
if (1) {
    DBQueryFatal("rename table ".
		 "images to backup_images, temp_images to images, ".
		 "os_info to backup_os_info, temp_os_info to os_info, ".
		 "osidtoimageid to backup_o2i, temp_o2i to osidtoimageid, ".
		 "osid_map to backup_os_map, temp_osid_map to osid_map, ".
		 "partitions to backup_partitions, ".
		 "      temp_partitions to partitions,".
		 "scheduled_reloads to backup_scheduled_reloads, ".
		 "      temp_sched_reloads to scheduled_reloads,".
		 "current_reloads to backup_current_reloads, ".
		 "      temp_cur_reloads to current_reloads,".
		 "nodes to backup_nodes, temp_nodes to nodes, ".
		 "node_type_attributes to backup_attributes, ".
		 "	temp_attributes to node_type_attributes");
}

