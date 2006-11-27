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

	if ($foo->IsValid())
	    return $foo;
	return null;
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
    # Add *new* member to project group; starts out with trust=none.
    #
    function AddNewMember($user) {
	$group = $this->LoadGroup();

	return $group->AddNewMember($user);
    }

    #
    # Check if user is a member of this project (well, group)
    #
    function IsMember($user) {
	$group = $this->LoadGroup();

	return $group->IsMember($user);
    }

    #
    # Return user object for the leader.
    #
    function Leader() {
	$head_uid = $this->head_uid();

	if (! ($leader = User::LookupByUid($head_uid))) {
	    TBERROR("Project::Leader: Could not lookup leader $head_uid!", 1);
	}
	return $leader;
    }
    
}
