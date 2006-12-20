<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#
class Project
{
    var	$project;
    var $group;

    #
    # Constructor by lookup on unique index.
    #
    function Project($pid_idx) {
	$safe_pid_idx = addslashes($pid_idx);

	$query_result =
	    DBQueryWarn("select * from projects ".
			"where pid_idx='$safe_pid_idx'");

	if (!$query_result || !mysql_num_rows($query_result)) {
	    $this->project = NULL;
	    return;
	}
	$this->project = mysql_fetch_array($query_result);
	$this->group   = null;
    }

    # Hmm, how does one cause an error in a php constructor?
    function IsValid() {
	return !is_null($this->project);
    }

    # Lookup by pid_idx.
    function Lookup($pid_idx) {
	$foo = new Project($pid_idx);

	if (! $foo->IsValid()) {
	    # Try lookup by plain uid.
	    $foo = Project::LookupByPid($pid_idx);
	    
	    if (!$foo || !$foo->IsValid())
		return null;

	    # Return here, in case I add a cache and forget to do this.
	    return $foo;
	}
	return $foo;
    }

    # Backwards compatable lookup by pid. Will eventually flush this.
    function LookupByPid($pid) {
	$safe_pid = addslashes($pid);

	$query_result =
	    DBQueryWarn("select pid_idx from projects where pid='$safe_pid'");

	if (!$query_result || !mysql_num_rows($query_result)) {
	    return null;
	}
	$row = mysql_fetch_array($query_result);
	$idx = $row['pid_idx'];

	$foo = new Project($idx); 

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

	$pid_idx = $this->pid_idx();

	$query_result =
	    DBQueryWarn("select * from projects where pid_idx='$pid_idx'");
    
	if (!$query_result || !mysql_num_rows($query_result)) {
	    $this->project = NULL;
	    return -1;
	}
	$this->project = mysql_fetch_array($query_result);
	$this->group   = null;
	return 0;
    }

    # accessors
    function field($name) {
	return (is_null($this->project) ? -1 : $this->project[$name]);
    }
    function pid_idx()	     { return $this->field("pid_idx"); }
    function pid()	     { return $this->field("pid"); }
    function created()       { return $this->field("created"); }
    function expires()       { return $this->field("expires"); }
    function name()          { return $this->field("name"); }
    function URL()           { return $this->field("URL"); }
    function funders()       { return $this->field("funders"); }
    function addr()          { return $this->field("addr"); }
    function head_uid()      { return $this->field("head_uid"); }
    function num_members()   { return $this->field("num_members"); }
    function num_pcs()       { return $this->field("num_pcs"); }
    function num_sharks()    { return $this->field("num_sharks"); }
    function num_pcplab()    { return $this->field("num_pcplab"); }
    function num_ron()       { return $this->field("num_ron"); }
    function why()           { return $this->field("why"); }
    function control_node()  { return $this->field("control_node"); }
    function approved()      { return $this->field("approved"); }
    function inactive()      { return $this->field("inactive"); }
    function date_inactive() { return $this->field("date_inactive"); }
    function public()        { return $this->field("public"); }
    function public_whynot() { return $this->field("public_whynot"); }
    function expt_count()    { return $this->field("expt_count"); }
    function expt_last()     { return $this->field("expt_last"); }
    function pcremote_ok()   { return $this->field("pcremote_ok"); }
    function default_user_interface()
	                     { return $this->field("default_user_interface"); }
    function linked_to_us()  { return $this->field("linked_to_us"); }
    function cvsrepo_public(){ return $this->field("cvsrepo_public"); }

    function unix_gid() {
	$group = $this->LoadGroup();
	
	return $group->unix_gid();
    }
    function unix_name() {
	$group = $this->LoadGroup();

	return $group->unix_name();
    }

    #
    # At some point we will stop passing pid and start using pid_idx.
    # Use this function to avoid having to change a bunch of code twice.
    #
    function URLParam() {
	return $this->pid();
    }

    #
    # Class function to create new project and return object.
    #
    function NewProject($pid, $leader, $args) {
	global $TBBASE, $TBMAIL_APPROVAL, $TBMAIL_AUDIT, $TBMAIL_WWW;
	
	#
	# The array of inserts is assumed to be safe already. Generate
	# a list of actual insert clauses to be joined below.
	#
	$insert_data = array();
	
	foreach ($args as $name => $value) {
	    $insert_data[] = "$name='$value'";
	}

	# First create the underlying default group for the project.
	if (! ($newgroup = Group::NewGroup(null, $pid, $leader,
					   'Default Group', $pid))) {
	    return null;
	}

	# Every project gets a new unique index, which comes from the group.
	$pid_idx = $newgroup->gid_idx();

	# Now tack on other stuff we need.
	$insert_data[] = "pid='$pid'";
	$insert_data[] = "pid_idx='$pid_idx'";
	$insert_data[] = "head_uid='" . $leader->uid() . "'";
	$insert_data[] = "created=now()";

	# Insert into DB. Should probably lock the table ...
	if (!DBQuerywarn("insert into projects set ".
			 implode(",", $insert_data))) {
	    $newgroup->Delete();
	    return null;
	}

	if (! DBQueryWarn("insert into project_stats (pid, pid_idx) ".
			  "values ('$pid', $pid_idx)")) {
	    $newgroup->Delete();
	    DBQueryFatal("delete from projects where pid_idx='$pid_idx'");
	    return null;
	}
	$newproject = Project::Lookup($pid_idx);
	if (! $newproject)
	    return null;

	#
	# The creator of a group is not automatically added to the group,
	# but we do want that for a new project. 
	#
	if ($newgroup->AddNewMember($leader) < 0) {
	    $newgroup->Delete();
	    DBQueryWarn("delete from project_stats where pid_idx=$pid_idx");
	    DBQueryWarn("delete from projects where pid_idx=$pid_idx");
	    return null;
	}

	return $newproject;
    }

    #
    # Access Check, which for now uses the global function to avoid duplication
    # until all code is changed.
    #
    function AccessCheck($user, $access_type) {
	return TBProjAccessCheck($user->uid(),
				 $this->pid(), $this->pid(),
				 $access_type);
    }

    #
    # Load the default group for a project lazily.
    #
    function LoadGroup() {
	# Note: pid_idx=gid_idx for the default group
	$gid_idx = $this->pid_idx();

	if (! ($group = Group::Lookup($gid_idx))) {
	    TBERROR("Project::LoadGroup: Could not load group $gid_idx!", 1);
	}
	$this->group = $group;
	return $group;
    }

    #
    # Return user object for leader.
    #
    function GetLeader() {
	$head_uid = $this->head_uid();

	if (! ($leader = User::Lookup($head_uid))) {
	    TBERROR("Could not find user object for $head_uid", 1);
	}
	return $leader;
    }

    #
    # Add *new* member to project group; starts out with trust=none.
    #
    function AddNewMember($user) {
	$group = $this->LoadGroup();

	return $group->AddNewMember($user);
    }

    #
    # Check if user is a member of this project (well, group)
    #
    function IsMember($user, &$approved) {
	$group = $this->LoadGroup();

	return $group->IsMember($user, $approved);
    }

    #
    # Member list for a group.
    #
    function MemberList() {
	$pid_idx = $this->pid_idx();
	$result  = array();

	$query_result =
	    DBQueryFatal("select uid_idx from group_membership ".
			 "where pid_idx='$pid_idx' and gid_idx=pid_idx");

	while ($row = mysql_fetch_array($query_result)) {
	    $uid_idx = $row["uid_idx"];

	    if (! ($user =& User::Lookup($uid_idx))) {
		TBERROR("Project::MemberList: ".
			"Could not load user $uid_idx!", 1);
	    }
	    $result[] =& $user;
	}
	return $result;
    }

    #
    # List of subgroups for a project member (not including default group).
    #
    function GroupList($user) {
	$pid_idx = $this->pid_idx();
	$uid_idx = $user->uid_idx();
	$result  = array();

	$query_result =
	    DBQueryFatal("select gid_idx from group_membership ".
			 "where pid_idx='$pid_idx' and pid_idx!=gid_idx and ".
			 "      uid_idx='$uid_idx'");

	while ($row = mysql_fetch_array($query_result)) {
	    $gid_idx = $row["gid_idx"];

	    if (! ($group = Group::Lookup($gid_idx))) {
		TBERROR("Project::GroupList: ".
			"Could not load group $gid_idx!", 1);
	    }
	    $result[] = $group;
	}
	return $result;
    }

    #
    # Change the leader for a project. Done *only* before project is
    # approved.
    #
    function ChangeLeader($leader) {
	$group = $this->LoadGroup();
	$idx   = $this->pid_idx();
	$uid   = $leader->uid();

	DBQueryFatal("update projects set head_uid='$uid' ".
		     "where pid_idx='$idx'");

	$this->project["head_uid"] = $uid;
	return $group->ChangeLeader($leader);
    }
    
    #
    # Change various fields.
    #
    function SetApproved($approved) {
	$idx   = $this->pid_idx();

	if ($approved)
	    $approved = 1;
	else
	    $approved = 0;
	
	DBQueryFatal("update projects set approved='$approved' ".
		     "where pid_idx='$idx'");

	$this->project["approved"] = $approved;
	return 0;
    }
    function SetRemoteOK($ok) {
	$idx    = $this->pid_idx();
	$safeok = addslashes($ok);

	DBQueryFatal("update projects set pcremote_ok='$safeok' ".
		     "where pid_idx='$idx'");

	$this->project["pcremote_ok"] = $ok;
	return 0;
	

    }

}
