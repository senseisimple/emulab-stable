<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#
class Group
{
    var	$group;

    #
    # Constructor by lookup on unique index.
    #
    function Group($gid_idx) {
	$safe_gid_idx = addslashes($gid_idx);

	$query_result =
	    DBQueryWarn("select * from groups ".
			"where gid_idx='$safe_gid_idx'");

	if (!$query_result || !mysql_num_rows($query_result)) {
	    $this->group = NULL;
	    return;
	}
	$this->group = mysql_fetch_array($query_result);
    }

    # Hmm, how does one cause an error in a php constructor?
    function IsValid() {
	return !is_null($this->group);
    }

    # Lookup by gid_idx.
    function Lookup($gid_idx) {
	$foo = new Group($gid_idx);

	if ($foo->IsValid())
	    return $foo;
	return null;
    }

    # Backwards compatable lookup by pid,gid. Will eventually flush this.
    function LookupByPidGid($pid, $gid) {
	$safe_pid = addslashes($pid);
	$safe_gid = addslashes($gid);

	$query_result =
	    DBQueryWarn("select gid_idx from groups ".
			"where pid='$safe_pid' and gid='$safe_gid'");

	if (!$query_result || !mysql_num_rows($query_result)) {
	    return null;
	}
	$row = mysql_fetch_array($query_result);
	$idx = $row['gid_idx'];

	$foo = new Group($idx); 

	if ($foo->IsValid())
	    return $foo;
	
	return null;
    }
    
    #
    # Refresh an instance by reloading from the DB.
    #
    function Refresh() {
	if (! $this->IsValid())
	    return -1;

	$gid_idx = $this->gid_idx();

	$query_result =
	    DBQueryWarn("select * from groups where gid_idx='$gid_idx'");
    
	if (!$query_result || !mysql_num_rows($query_result)) {
	    $this->group = NULL;
	    return -1;
	}
	$this->group = mysql_fetch_array($query_result);
	return 0;
    }

    # accessors
    function field($name) {
	return (is_null($this->group) ? -1 : $this->group[$name]);
    }
    function pid()	        { return $this->field("pid"); }
    function gid()	        { return $this->field("gid"); }
    function pid_idx()          { return $this->field("pid_idx"); }
    function gid_idx()          { return $this->field("gid_idx"); }
    function leader()           { return $this->field("leader"); }
    function created()          { return $this->field("created"); }
    function description()      { return $this->field("description"); }
    function unix_gid()         { return $this->field("unix_gid"); }
    function unix_name()        { return $this->field("unix_name"); }
    function expt_count()       { return $this->field("expt_count"); }
    function expt_last()        { return $this->field("expt_last"); }
    function wikiname()         { return $this->field("wikiname"); }
    function mailman_password() { return $this->field("mailman_password"); }


    #
    # Class function to create new group and return object.
    #
    function NewGroup($project, $gid, $leader, $description, $unix_name) {
	global $TBBASE, $TBMAIL_APPROVAL, $TBMAIL_AUDIT, $TBMAIL_WWW;
	global $MIN_UNIX_GID;

	# Every group gets a new unique index.
	$gid_idx = TBGetUniqueIndex('next_gid');

	# If project is not defined, then creating initial project group.
	if (! $project) {
	    $pid = $gid;
	    $pid_idx = $gid_idx;
	}
	else {
	    $pid = $project->pid();
	    $pid_idx = $project->pid_idx();
	}
	
	# Get me an unused unix id. Nice query, eh? Basically, find 
	# unused numbers by looking at existing numbers plus one, and check
	# to see if that number is taken. 
	$query_result =
	    DBQueryWarn("select g.unix_gid + 1 as start from groups as g ".
			"left outer join groups as r on ".
			"  g.unix_gid + 1 = r.unix_gid ".
			"where g.unix_gid>$MIN_UNIX_GID and ".
			"      g.unix_gid<50000 and ".
			"      r.unix_gid is null limit 1");

	if (!$query_result || !mysql_num_rows($query_result)) {
	    TBERROR("Could not find an unused unix_gid!", 0);
	    return null;
	}
	$row = mysql_fetch_row($query_result);
	$unix_gid = $row[0];

	if (!DBQueryWarn("insert into groups set ".
			 " pid='$pid', gid='$gid', ".
			 " leader='" . $leader->uid() . "'," .
			 " created=now(), ".
			 " description='$description', ".
			 " unix_name='$unix_name', ".
			 " gid_idx=$gid_idx, ".
			 " pid_idx=$pid_idx, ".
			 " unix_gid=$unix_gid")) {
	    return null;
	}

	if (! DBQueryWarn("insert into group_stats (pid, gid, gid_idx) ".
			  "values ('$pid', '$gid', $gid_idx)")) {
	    DBQueryFatal("delete from groups where gid_idx='$gid_idx'");
	    return null;
	}
	$newgroup = Group::Lookup($gid_idx);
	if (! $newgroup)
	    return null;

	return $newgroup;
    }

    #
    # Delete a group.
    #
    function Delete() {
	$gid_idx = $this->gid_idx();

	DBQueryWarn("delete from group_stats where gid_idx='$gid_idx'");
	DBQueryWarn("delete from groups where gid_idx='$gid_idx'");
	return 0;
    }

    #
    # This is strictly for initialization of a testbed.
    #
    function Initialize($uid, $pid) {
	$emulabgroup = Group::LookupByPidGid($TBOPSPID, $TBOPSPID);

	if (! $emulabgroup) {
	    TBERROR("Group::Initialize: Could not find $TBOPSPID!", 1);
	}

	$user = User::LookupByUid($uid);

	if (! $user) {
	    TBERROR("Group::Initialize: Could not find user $uid!", 1);
	}

	if ($emulabgroup->AddNewMember($user) < 0) {
	    TBERROR("Group::Initialize: Could not add $uid to $TBOPSPID!", 1);
	}

	# Inline approval; usually done with modgroups.
	DBQueryFatal("update group_membership set date_approved=now(), ".
		     "  trust='" . TBDB_TRUSTSTRING_GROUPROOT . "' ".
		     "where uid='$uid' and pid='$TBOPSPID'");

	DBQueryFatal("update group_membership set date_approved=now(), ".
		     "  trust='" . TBDB_TRUSTSTRING_PROJROOT . "' ".
		     "where uid='$uid' and pid='$pid'");

	return 0;
    }

    #
    # Add *new* member to group; starts out with trust=none.
    #
    function AddNewMember($user) {
	$uid     = $user->uid();
	$uid_idx = $user->uid_idx();
	$pid     = $this->pid();
	$pid_idx = $this->pid_idx();
	$gid     = $this->gid();
	$gid_idx = $this->gid_idx();
	    
	if (!DBQueryWarn("insert into group_membership ".
			 "(uid, uid_idx, gid, gid_idx, pid, pid_idx, ".
			 " trust, date_applied) ".
			 "values ('$uid', '$uid_idx', '$gid', '$gid_idx', ".
			 "        '$pid', '$pid_idx', ".
			 "        'none', now())")) {
	    return -1;
	}
	return 0;
    }

    #
    # Check if user is a member of this group.
    #
    function IsMember($user) {
	$uid     = $user->uid();
	$uid_idx = $user->uid_idx();
	$gid     = $this->gid();
	$gid_idx = $this->gid_idx();

	$query_result =
	    DBQueryFatal("select trust from group_membership ".
			 "where uid_idx='$uid_idx' and gid_idx='$gid_idx'");

	return mysql_num_rows($query_result);
    }
}
