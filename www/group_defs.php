<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
#
# A cache of groups to avoid lookups. Indexed by gid_idx;
#
$group_cache = array();

class Group
{
    var	$group;
    var $project;

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
	$this->group   = mysql_fetch_array($query_result);
	$this->project =  null;
    }

    # Hmm, how does one cause an error in a php constructor?
    function IsValid() {
	return !is_null($this->group);
    }

    # Lookup by gid_idx.
    function Lookup($gid_idx) {
	global $group_cache;

        # Look in cache first
	if (array_key_exists("$gid_idx", $group_cache))
	    return $group_cache["$gid_idx"];
	
	$foo = new Group($gid_idx);

	if (! $foo->IsValid())
	    return null;

	# Insert into cache.
	$group_cache["$gid_idx"] = $foo;
	return $foo;
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

	return Group::Lookup($idx);	
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

    #
    # Class function to create a new Project Group.
    #
    function Create($project, $uid, $args, &$errors) {
	global $suexec_output, $suexec_output_array;

        #
        # Generate a temporary file and write in the XML goo.
        #
	$xmlname = tempnam("/tmp", "newgroup");
	if (! $xmlname) {
	    TBERROR("Could not create temporary filename", 0);
	    $errors[] = "Transient error; please try again later.";
	    return null;
	}
	if (! ($fp = fopen($xmlname, "w"))) {
	    TBERROR("Could not open temp file $xmlname", 0);
	    $errors[] = "Transient error; please try again later.";
	    return null;
	}

	# Add these. Maybe caller should do this?
	$args["project"]  = $project->pid();

	fwrite($fp, "<group>\n");
	foreach ($args as $name => $value) {
	    fwrite($fp, "<attribute name=\"$name\">");
	    fwrite($fp, "  <value>" . htmlspecialchars($value) . "</value>");
	    fwrite($fp, "</attribute>\n");
	}
	fwrite($fp, "</group>\n");
	fclose($fp);
	chmod($xmlname, 0666);

	# Note: running as the user for mkgroup and modgroups underneath.
	$unix_gid  = $project->unix_gid();
	$retval = SUEXEC($uid, $unix_gid, "webnewgroup $xmlname",
			 SUEXEC_ACTION_IGNORE);

	if ($retval) {
	    if ($retval < 0) {
		$errors[] = "Transient error; please try again later.";
		SUEXECERROR(SUEXEC_ACTION_CONTINUE);
	    }
	    else {
		# unlink($xmlname);
		if (count($suexec_output_array)) {
		    for ($i = 0; $i < count($suexec_output_array); $i++) {
			$line = $suexec_output_array[$i];
			if (preg_match("/^([-\w]+):\s*(.*)$/",
				       $line, $matches)) {
			    $errors[$matches[1]] = $matches[2];
			}
			else
			    $errors[] = $line;
		    }
		}
		else
		    $errors[] = "Transient error; please try again later.";
	    }
	    return null;
	}

        #
        # Parse the last line of output. Ick.
        #
	unset($matches);
	
	if (!preg_match("/^GROUP\s+([^\/]+)\/(\d+)\s+/",
			$suexec_output_array[count($suexec_output_array)-1],
			$matches)) {
	    $errors[] = "Transient error; please try again later.";
	    SUEXECERROR(SUEXEC_ACTION_CONTINUE);
	    return null;
	}
	$group = $matches[2];
	$newgroup = Group::Lookup($group);
	if (! $newgroup) {
	    $errors[] = "Transient error; please try again later.";
	    TBERROR("Could not lookup new group $group", 0);
	    return null;
	}
	# Unlink this here, so that the file is left behind in case of error.
	# We can then create the group by hand from the xmlfile, if desired.
	unlink($xmlname);
	return $newgroup; 
    }

    #
    # Class function to edit group membership.
    #
    function EditGroup($group, $uid, $args, &$errors) {
	global $suexec_output, $suexec_output_array;

        #
        # Generate a temporary file and write in the XML goo.
        #
	$xmlname = tempnam("/tmp", "editgroup");
	if (! $xmlname) {
	    TBERROR("Could not create temporary filename", 0);
	    $errors[] = "Transient error; please try again later.";
	    return null;
	}
	if (! ($fp = fopen($xmlname, "w"))) {
	    TBERROR("Could not open temp file $xmlname", 0);
	    $errors[] = "Transient error; please try again later.";
	    return null;
	}

	# Add these. Maybe caller should do this?
	$args["group"]  = $group->gid_idx();

	fwrite($fp, "<group>\n");
	foreach ($args as $name => $value) {
	    fwrite($fp, "<attribute name=\"$name\">");
	    fwrite($fp, "  <value>" . htmlspecialchars($value) . "</value>");
	    fwrite($fp, "</attribute>\n");
	}
	fwrite($fp, "</group>\n");
	fclose($fp);
	chmod($xmlname, 0666);

	# Grab the unix GID for running scripts.
	$unix_gid = $group->unix_gid();

	$retval = SUEXEC($uid, $unix_gid, "webeditgroup $xmlname",
			 SUEXEC_ACTION_IGNORE);

	if ($retval) {
	    if ($retval < 0) {
		$errors[] = "Transient error; please try again later.";
		SUEXECERROR(SUEXEC_ACTION_CONTINUE);
	    }
	    else {
		# unlink($xmlname);
		if (count($suexec_output_array)) {
		    for ($i = 0; $i < count($suexec_output_array); $i++) {
			$line = $suexec_output_array[$i];
			if (preg_match("/^([-\w]+):\s*(.*)$/",
				       $line, $matches)) {
			    $errors[$matches[1]] = $matches[2];
			}
			else
			    $errors[] = $line;
		    }
		}
		else
		    $errors[] = "Transient error; please try again later.";
	    }
	    return null;
	}
	# There are no return value(s) to parse at the end of the output.

	# Unlink this here, so that the file is left behind in case of error.
	# We can then edit the group by hand from the xmlfile, if desired.
	unlink($xmlname);
	return true;
    }

    #
    # Load the project for a group lazily.
    #
    function LoadProject() {
	$pid_idx = $this->pid_idx();

	if (! ($project = Project::Lookup($pid_idx))) {
	    TBERROR("Group::LoadProject: Could not load project $pid_idx!", 1);
	}
	$this->project = $project;
	return 0;
    }
    function Project() {
	if (! $this->project) {
	    $this->LoadProject();
	}
	return $this->project;
    }
    
    #
    # Return user object for leader.
    #
    function GetLeader() {
	$leader_idx = $this->leader_idx();

	if (! ($leader = User::Lookup($leader_idx))) {
	    TBERROR("Could not find user object for $leader_idx", 1);
	}
	return $leader;
    }

    #
    # Is this group the default project group. Returns boolean.
    #
    function IsProjectGroup() {
	return $this->pid_idx() == $this->gid_idx();
    }

    #
    # Project permission checks.
    #
    #	returns 0 if not allowed.
    #   returns 1 if allowed.
    # 
    function AccessCheck($user, $access_type) {
	global $TB_PROJECT_READINFO;
	global $TB_PROJECT_MAKEGROUP;
	global $TB_PROJECT_EDITGROUP;
	global $TB_PROJECT_GROUPGRABUSERS;
	global $TB_PROJECT_BESTOWGROUPROOT;
	global $TB_PROJECT_DELGROUP;
	global $TB_PROJECT_LEADGROUP;
	global $TB_PROJECT_ADDUSER;
	global $TB_PROJECT_DELUSER;
	global $TB_PROJECT_MAKEOSID;
	global $TB_PROJECT_DELOSID;
	global $TB_PROJECT_MAKEIMAGEID;
	global $TB_PROJECT_DELIMAGEID;
	global $TB_PROJECT_CREATEEXPT;
	global $TB_PROJECT_MIN;
	global $TB_PROJECT_MAX;
	global $TBDB_TRUST_USER;
	global $TBDB_TRUST_LOCALROOT;
	global $TBDB_TRUST_GROUPROOT;
	global $TBDB_TRUST_PROJROOT;
        # Min declaration.
	$mintrust = $TB_PROJECT_READINFO;

	$pid = $this->pid();
	$gid = $this->gid();
	$uid = $user->uid();

	if ($access_type < $TB_PROJECT_MIN ||
	    $access_type > $TB_PROJECT_MAX) {
	    TBERROR("Invalid access type: $access_type!", 1);
	}
 
        #
        # Admins do whatever they want!
        # 
	if (ISADMIN()) {
	    return 1;
	}

	if ($access_type == $TB_PROJECT_READINFO) {
	    $mintrust = $TBDB_TRUST_USER;
	}
	elseif ($access_type == $TB_PROJECT_MAKEGROUP ||
		$access_type == $TB_PROJECT_DELGROUP) {
	    $mintrust = $TBDB_TRUST_GROUPROOT;
	}
	elseif ($access_type == $TB_PROJECT_LEADGROUP) {
            #
            # Allow mere user (in default group) to lead a subgroup.
            # 
	    $mintrust = $TBDB_TRUST_USER;
	}
	elseif ($access_type == $TB_PROJECT_MAKEOSID ||
		$access_type == $TB_PROJECT_MAKEIMAGEID ||
		$access_type == $TB_PROJECT_CREATEEXPT) {
	    $mintrust = $TBDB_TRUST_LOCALROOT;
	}
	elseif ($access_type == $TB_PROJECT_ADDUSER ||
		$access_type == $TB_PROJECT_EDITGROUP) {
	    #
	    # If user is project_root or group_root in default group, 
	    # allow them to add/edit/remove users in any group.
	    #
	    if (TBMinTrust(TBGrpTrust($uid, $pid, $pid),
			   $TBDB_TRUST_GROUPROOT)) {
		return 1;
	    }
	    #
	    # Otherwise, editing a group requires group_root 
	    # in that group.
	    #	
	    $mintrust = $TBDB_TRUST_GROUPROOT;
	}
	elseif ($access_type == $TB_PROJECT_BESTOWGROUPROOT) {
            #
	    # If user is project_root, 
	    # allow them to bestow group_root in any group.
 	    #
	    if (TBMinTrust(TBGrpTrust($uid, $pid, $pid),
			   $TBDB_TRUST_PROJROOT)) {
		return 1;
	    }
	    
	    if ($gid == $pid)  {
	        #
	        # Only project_root can bestow group_root in default group, 
	        # and we already established that they are not project_root,
		# so fail.
	        #
		return 0;
	    }
	    else {
	        #
	        # Non-default group.
	        # group_root in default group may bestow group_root.
	        #
	        if (TBMinTrust(TBGrpTrust($uid, $pid, $pid),
			       $TBDB_TRUST_GROUPROOT)) {
		    return 1;
		}

                #
	        # group_root in the group in question may also bestow
		# group_root.
	        #
		$mintrust = $TBDB_TRUST_GROUPROOT;
	    }
	}
	elseif ($access_type == $TB_PROJECT_GROUPGRABUSERS) {
	    #
	    # Only project_root or group_root in default group
	    # may grab (involuntarily add) users into groups.
	    #
	    $gid = $pid;
	    $mintrust = $TBDB_TRUST_GROUPROOT;
	}
	elseif ($access_type == $TB_PROJECT_DELUSER) {
	    $mintrust = $TBDB_TRUST_PROJROOT;
	}
	else {
	    TBERROR("Unexpected access type: $access_type!", 1);
	}

	return TBMinTrust(TBGrpTrust($uid, $pid, $gid), $mintrust);
    }

    #
    # Return a users trust within the group.
    #
    function UserTrust($user) {
	global $TBDB_TRUST_NONE;
	
	$uid_idx = $user->uid_idx();
	$pid_idx = $this->pid_idx();
	$gid_idx = $this->gid_idx();

	$query_result =
	    DBQueryFatal("select trust from group_membership ".
			 "where uid_idx='$uid_idx' and ".
			 "      pid_idx='$pid_idx' and gid_idx='$gid_idx'");

        #
        # No membership is the same as no trust. True? Maybe an error instead?
        # 
	if (mysql_num_rows($query_result) == 0) {
	    return $TBDB_TRUST_NONE;
	}
	$row = mysql_fetch_array($query_result);
	$trust_string = $row["trust"];

	# Convert string to number.      
	return TBTrustConvert($trust_string);
    }

    # accessors
    function field($name) {
	return (is_null($this->group) ? -1 : $this->group[$name]);
    }
    function pid()	        { return $this->field("pid"); }
    function gid()	        { return $this->field("gid"); }
    function pid_idx()          { return $this->field("pid_idx"); }
    function gid_idx()          { return $this->field("gid_idx"); }
    function uuid()             { return $this->field("gid_uuid"); }
    function leader()           { return $this->field("leader"); }
    function leader_idx()       { return $this->field("leader_idx"); }
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

	$starting_gid = $MIN_UNIX_GID;
	$ending_gid   = 50000;

        #
        # Check that we can guarantee uniqueness of the unix group name.
        # 
	$query_result =
	    DBQueryFatal("select gid from groups ".
			 "where unix_name='$unix_name'");

	if (mysql_num_rows($query_result)) {
	    TBERROR("Could not form a unique Unix group name: $unix_name!", 1);
	}

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
	$uuid = NewUUID();
	
	# Get me an unused unix id. Nice query, eh? Basically, find 
	# unused numbers by looking at existing numbers plus one, and check
	# to see if that number is taken. 
	$query_result =
	    DBQueryWarn("select g.unix_gid + 1 as start from groups as g ".
			"left outer join groups as r on ".
			"  g.unix_gid + 1 = r.unix_gid ".
			"where g.unix_gid>=$starting_gid and ".
			"      g.unix_gid<$ending_gid and ".
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
			 " leader_idx='" . $leader->uid_idx() . "'," .
			 " created=now(), ".
			 " description='$description', ".
			 " unix_name='$unix_name', ".
			 " gid_uuid='$uuid', ".
			 " gid_idx=$gid_idx, ".
			 " pid_idx=$pid_idx, ".
			 " unix_gid=$unix_gid")) {
	    return null;
	}

	if (! DBQueryWarn("insert into group_stats ".
			  "  (pid,gid,gid_idx,pid_idx,gid_uuid) ".
			  "values ('$pid', '$gid', $gid_idx, ".
			  "        $pid_idx, '$uuid')")) {
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
    function Initialize($uid) {
	global $TBOPSPID;
	
	$emulabgroup = Group::LookupByPidGid($TBOPSPID, $TBOPSPID);

	if (! $emulabgroup) {
	    TBERROR("Group::Initialize: Could not find $TBOPSPID!", 1);
	}

	$user = User::Lookup($uid);

	if (! $user) {
	    TBERROR("Group::Initialize: Could not find user $uid!", 1);
	}

	if (!$emulabgroup->IsMember($user, $ignore) &&
	    $emulabgroup->AddNewMember($user) < 0) {
	    TBERROR("Group::Initialize: Could not add $uid to $TBOPSPID!", 1);
	}

	# Inline approval; usually done with modgroups.
	DBQueryFatal("update group_membership set date_approved=now(), ".
		     "  trust='" . TBDB_TRUSTSTRING_GROUPROOT . "' ".
		     "where uid='$uid' and pid='$TBOPSPID'");
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
    # And a delete function.
    #
    function DeleteMember($user) {
	$uid     = $user->uid();
	$uid_idx = $user->uid_idx();
	$pid     = $this->pid();
	$pid_idx = $this->pid_idx();
	$gid     = $this->gid();
	$gid_idx = $this->gid_idx();

	DBQueryFatal("delete from group_membership ".
		     "where uid_idx='$uid_idx' and pid_idx='$pid_idx' and ".
		     "      gid_idx='$gid_idx'");
	
	return 0;
    }

    #
    # Notify leaders of new (and verified) group member.
    #
    function NewMemberNotify($user) {
	global $TBWWW, $TBMAIL_APPROVAL, $TBMAIL_AUDIT, $TBMAIL_WWW;
	
	if (! $this->project) {
	    $this->LoadProject();
	}
	$project	= $this->project;
	$pid            = $project->pid();
	$gid            = $this->gid();
	$leader		= $project->GetLeader();
	$leader_name	= $leader->name();
	$leader_email	= $leader->email();
	$leader_uid	= $leader->uid();
	$allleaders	= $this->LeaderMailList();
	$joining_uid    = $user->uid();
	$usr_title	= $user->title();
	$usr_name	= $user->name();
	$usr_affil	= $user->affil();
	$usr_email	= $user->email();
	$usr_addr	= $user->addr();
	$usr_addr2	= $user->addr2();
	$usr_city	= $user->city();
	$usr_state	= $user->state();
	$usr_zip	= $user->zip();
	$usr_country	= $user->country();
	$usr_phone	= $user->phone();
	$usr_URL	= $user->URL();
	
	TBMAIL("$leader_name '$leader_uid' <$leader_email>",
	   "$joining_uid $pid Project Join Request",
	   "$usr_name is trying to join your group $gid in project $pid.\n".
	   "\n".
	   "Contact Info:\n".
	   "Name:            $usr_name\n".
	   "Emulab ID:       $joining_uid\n".
	   "Email:           $usr_email\n".
	   "User URL:        $usr_URL\n".
	   "Job Title:       $usr_title\n".
	   "Affiliation:     $usr_affil\n".
	   "Address 1:       $usr_addr\n".
	   "Address 2:       $usr_addr2\n".
	   "City:            $usr_city\n".
	   "State:           $usr_state\n".
	   "ZIP/Postal Code: $usr_zip\n".
	   "Country:         $usr_country\n".
	   "Phone:           $usr_phone\n".
	   "\n".
	   "Please return to $TBWWW,\n".
	   "log in, and select the 'New User Approval' page to enter your\n".
	   "decision regarding $usr_name's membership in your project.\n\n".
	   "Thanks,\n".
	   "Testbed Operations\n",
	   "From: $TBMAIL_APPROVAL\n".
	   "Cc: $allleaders\n".
	   "Bcc: $TBMAIL_AUDIT\n".
	   "Errors-To: $TBMAIL_WWW");

	return 0;
    }

    #
    # Check if user is a member of this group.
    #
    function IsMember($user, &$approved) {
	global $TBDB_TRUST_USER;
	
	$uid     = $user->uid();
	$uid_idx = $user->uid_idx();
	$gid     = $this->gid();
	$gid_idx = $this->gid_idx();

	$query_result =
	    DBQueryFatal("select trust from group_membership ".
			 "where uid_idx='$uid_idx' and gid_idx='$gid_idx'");

	if (mysql_num_rows($query_result) == 0) {
	    $approved = 0;
	    return 0;
	}

	$row      = mysql_fetch_row($query_result);
	$trust    = $row[0];
	$approved = TBMinTrust($trust, $TBDB_TRUST_USER);

	return 1;
    }

    #
    # Return list of approved members in this group. 
    #
    function &MemberList($exclude_leader = 1) {
	$gid_idx    = $this->gid_idx();
	$pid_idx    = $this->pid_idx();
	$trust_none = TBDB_TRUSTSTRING_NONE;
	$leader     = $this->GetLeader();
	$result     = array();

	$query_result =
	    DBQueryFatal("select m.uid_idx,m.trust ".
			 "   from group_membership as m ".
			 "left join groups as g on ".
			 "     g.pid=m.pid and g.gid=m.gid ".
			 "where m.pid_idx='$pid_idx' and ".
			 "      m.gid_idx='$gid_idx' and ".
			 "      m.trust!='$trust_none'");

	while ($row = mysql_fetch_array($query_result)) {
	    $uid_idx = $row["uid_idx"];
	    $trust   = $row["trust"];

	    if ($exclude_leader && $leader->uid_idx() == $uid_idx)
		continue;

	    if (! ($user = User::Lookup($uid_idx))) {
		TBERROR("Group::MemberList: ".
			"Could not load user $uid_idx!", 1);
	    }
	    # So caller can get this.
	    $user->SetTempData($trust);
	    
	    $result["$uid_idx"] = $user;
	}
	return $result;
    }

    #
    # Grab the user list from the project. These are the people who can be
    # added. Do not include people in the above list, obviously! Do not
    # include members that have not been approved to main group either! This
    # will force them to go through the approval page first.
    #
    function NonMemberList() {
	$gid_idx    = $this->gid_idx();
	$pid_idx    = $this->pid_idx();
	$trust_none = TBDB_TRUSTSTRING_NONE;
	$result     = array();
	
	$query_result =
	    DBQueryFatal("select m.uid_idx from group_membership as m ".
			 "left join group_membership as a on ".
			 "     a.uid_idx=m.uid_idx and ".
			 "     a.pid_idx=m.pid_idx and a.gid_idx='$gid_idx' ".
			 "where m.pid_idx='$pid_idx' and ".
			 "      m.gid_idx=m.pid_idx and a.uid_idx is NULL ".
			 "      and m.trust!='$trust_none'");

	while ($row = mysql_fetch_array($query_result)) {
	    $uid_idx = $row["uid_idx"];

	    if (! ($user = User::Lookup($uid_idx))) {
		TBERROR("Group::NonMemberList: ".
			"Could not load user $uid_idx!", 1);
	    }
	    $result[] = $user;
	}
	return $result;
    }

    #
    # Change the leader for a group.
    #
    function ChangeLeader($leader) {
	$idx     = $this->gid_idx();
	$uid     = $leader->uid();
	$uid_idx = $leader->uid_idx();

	DBQueryFatal("update groups set leader='$uid',leader_idx='$uid_idx' ".
		     "where gid_idx='$idx'");

	$this->group["leader"] = $uid;
	$this->group["leader_idx"] = $uid_idx;
	return 0;
    }

    #
    # Trust consistency.
    #
    function CheckTrustConsistency($user, $newtrust, $fail) {
	global $TBDB_TRUST_USER;
	
	$uid = $user->uid();
	$pid = $this->pid();
	$gid = $this->gid();
	$uid_idx = $user->uid_idx();
	$pid_idx = $this->pid_idx();
	$gid_idx = $this->gid_idx();
	$trust_none = TBDB_TRUSTSTRING_NONE;
	$project = $this->Project();
	
        # 
        # set $newtrustisroot to 1 if attempting to set a rootful trust,
        # 0 otherwise.
        #
	$newtrustisroot = TBTrustConvert($newtrust) > $TBDB_TRUST_USER ? 1 : 0;

        #
        # If changing subgroup trust level, then compare levels.
        # A user may not have root privs in the project and user privs
        # in the subgroup; it makes no sense to do that and can violate trust.
        #
	if ($pid_idx != $gid_idx) {
            #
            # Setting non-default "sub"group.
	    # Verify that if user has root in project,
	    # we are setting a rootful trust for him in 
	    # the subgroup as well.
	    #
	    $projtrustisroot =
		($project->UserTrust($user) > $TBDB_TRUST_USER ? 1 : 0);

	    if ($projtrustisroot > $newtrustisroot) {
		if (!$fail)
		    return 0;
		
		TBERROR("User $uid may not have a root trust level in ".
			"the default group of $pid, ".
			"yet be non-root in subgroup $gid!", 1);
	    }
	}
	else {
            #
	    # Setting default group.
	    # Do not verify anything (yet.)
	    #
	    $projtrustisroot = $newtrustisroot;
	}

        #
        # Get all the subgroups not equal to the subgroup being changed.
        # 
	$query_result =
	    DBQueryFatal("select trust,gid from group_membership ".
			 "where uid_idx='$uid_idx' and ".
			 "      pid_idx='$pid_idx' and ".
			 "      gid_idx!=pid_idx and ".
			 "      gid_idx!='$gid_idx' and ".
			 "      trust!='$trust_none'");

	while ($row = mysql_fetch_array($query_result)) {
	    $grptrust = $row[0];
	    $ogid     = $row[1];
	
  	    # 
	    # Get what the users trust level is in the 
	    # current subgroup we are looking at.
	    #
	    $grptrustisroot = 
		TBTrustConvert($grptrust) > $TBDB_TRUST_USER ? 1 : 0;

 	    #
	    # If users trust level is higher in the default group than in the
	    # subgroup we are looking at, this is wrong.
 	    #
	    if ($projtrustisroot > $grptrustisroot) {
	        if (!$fail)
		    return 0;

		TBERROR("User $uid may not have a root trust level in ".
			"the default group of $pid, ".
			"yet be non-root in subgroup $ogid!", 1);
	    }

	    if ($pid_idx != $gid_idx) {
                #
	        # Iff we are modifying a subgroup, 
	        # Make sure that the trust we are setting is as
	        # rootful as the trust we already have set in
	        # every other subgroup.
	        # 
		if ($newtrustisroot != $grptrustisroot) { 
		    if (!$fail)
			return 0;
		    
		    TBERROR("User $uid may not mix root and ".
			    "non-root trust levels in ".
			    "different subgroups of $pid!", 1);
		}
	    }
	}
	return 1;
    }

    #
    # Hmm, this is really a grooup_membership query. Needs different treatment.
    #
    function MemberShipInfo($user, &$trust, &$date_applied, &$date_approved) {
	$uid_idx = $user->uid_idx();
	$gid_idx = $this->gid_idx();

	$query_result =
	    DBQueryFatal("select trust,date_applied,date_approved ".
			 "  from group_membership ".
			 "where uid_idx='$uid_idx' and gid_idx='$gid_idx'");

	if (! mysql_num_rows($query_result)) {
	    TBERROR("Group::MemberShipInfo: ".
		    "Lookup failed for $uid_idx/$gid_idx", 1);
	}
	$row      = mysql_fetch_row($query_result);
	$trust    = $row[0];
	$date_applied  = $row[1];
	$date_approved = $row[2];
	
	return 0;
    }

    #
    # Return mail addresses for project_root and group_root people.
    #
    function LeaderMailList() {
	$gid_idx = $this->gid_idx();
	$pid_idx = $this->pid_idx();

	# Constants.
	$trust_group  = TBDB_TRUSTSTRING_GROUPROOT;
	$trust_project= TBDB_TRUSTSTRING_PROJROOT;

	$query_result =
	    DBQueryFatal("select distinct usr_name,u.uid,usr_email ".
			 "   from users as u ".
			 "left join group_membership as gm on ".
			 "     gm.uid_idx=u.uid_idx ".
			 "where (trust='$trust_project' and ".
			 "       pid_idx='$pid_idx') or ".
			 "      (trust='$trust_group' and ".
			 "       pid_idx='$pid_idx' and gid_idx='$gid_idx') ".
			 "order by trust DESC, usr_name");
  
	if (mysql_num_rows($query_result) == 0) {
	    return "";
	}
	
	$mailstr="";
	while ($row = mysql_fetch_array($query_result)) {
	    if ($mailstr != "")
		$mailstr .= ", ";
	    
	    $mailstr .= '"' . $row["usr_name"] . " (". $row["uid"] . ")\" <" .
		$row["usr_email"] . ">";
	}
	return $mailstr;
    }

    function Show() {
	global $OURDOMAIN;
	global $MAILMANSUPPORT;

	echo "<center>
               <h3>Group Profile</h3>
              </center>
              <table align=center border=1>\n";

	$pid         = $this->pid();
	$gid         = $this->gid();
	$gid_idx     = $this->gid_idx();
	$created     = $this->created();
	$leader	     = $this->leader();
	$description = $this->description();
	$expt_count  = $this->expt_count();
	$expt_last   = $this->expt_last();
	$unix_gid    = $this->unix_gid();
	$unix_name   = $this->unix_name();
	$uuid        = $this->uuid();

	if ($this->IsProjectGroup()) 
	    $mail = "$pid" . "-users@" . $OURDOMAIN;
	else
	    $mail = "$pid-$gid" . "-users@" . $OURDOMAIN;

	if (!$expt_last) {
	    $expt_last = "&nbsp;";
	}

	if (! ($leader_user = $this->GetLeader())) {
	    TBERROR("Could not lookup object for user $leader", 1);
	}
	$showuser_url = CreateURL("showuser", $leader_user);

	echo "<tr>
                  <td>GID: </td>
                  <td class=\"left\">
                   <a href='showgroup.php3?pid=$pid&gid=$gid'>$gid ($gid_idx)".
	          "</a></td>
              </tr>\n";
    
	echo "<tr>
                  <td>PID: </td>
                  <td class=\"left\">
                    <a href='showproject.php3?pid=$pid'>$pid</a></td>
              </tr>\n";
    
	echo "<tr>
                  <td>Description: </td>
                  <td class=\"left\">$description</td>
              </tr>\n";
    
	echo "<tr>
                  <td>Unix GID: </td>
                  <td class=\"left\">$unix_gid</td>
              </tr>\n";
    
	echo "<tr>
                  <td>Unix Group Name: </td>
                  <td class=\"left\">$unix_name</td>
              </tr>\n";
    
	echo "<tr>
                  <td>Group Leader: </td>
                  <td class=\"left\">
                     <a href='$showuser_url'>$leader</a></td>
              </tr>\n";
    
	if ($MAILMANSUPPORT) {
	    $mmurl   = "gotommlist.php3?pid=$pid&gid=$gid";

	    echo "<tr>
                      <td>Email List:</td>
                      <td class=\"left\">
                          <a href='$mmurl'>$mail</a> ";

	    if (ISADMIN()) {
		$mmurl .= "&wantadmin=1";
		echo "<a href='$mmurl'>(admin page)</a>";
	    }
	    echo "    </td>
                  </tr>\n";
	}
	else {
	    echo "<tr>
                      <td>Email List: </td>
                      <td class=\"left\">$mail</td>
                  </tr>\n";
	}

	if (ISADMIN()) {
	    echo "<tr>
                    <td>UUID: </td>
                    <td class=left>$uuid</td>
                  </tr>\n";
	}

	echo "<tr>
                  <td>Created: </td>
                  <td class=\"left\">$created</td>
              </tr>\n";
    
	echo "<tr>
                  <td>Experiments Created:</td>
                  <td class=\"left\">$expt_count</td>
              </tr>\n";
    
	echo "<tr>
                  <td>Date of last experiment:</td>
                  <td class=\"left\">$expt_last</td>
              </tr>\n";
    
	echo "</table>\n";
    }

    #
    # Pretty display of group members.
    #
    function ShowMembers($prived = 0) {
	$gid_idx = $this->gid_idx();
	$pid_idx = $this->pid_idx();
	$project = $this->Project();

	$query_result =
	    DBQueryFatal("select uid_idx,trust from group_membership ".
			 "where pid_idx='$pid_idx' and gid_idx='$gid_idx'");
    
	if (! mysql_num_rows($query_result)) {
	    return;
	}
	$showdel  = (($prived && $pid_idx == $gid_idx) ? 1 : 0);
	$projgrp  = $this->IsProjectGroup();

	echo "<center>\n";
	if ($projgrp)
	    echo "<h3>Project Members</h3>\n";
	else
	    echo "<h3>Group Members</h3>\n";

	echo "</center>
              <table align=center border=1 cellpadding=1
                     id='userlist' cellspacing=2>\n";

	echo "<thead class='sort'>";
	echo "<tr>
                  <th>Name</th>\n";
	if (! $projgrp) {
	    echo "<th>Email</th>\n";
	}
	echo "    <th>UID</th>
                  <th>Privs</th>\n";
	if ($showdel) {
	    echo "<th>Remove</th>\n";
	}
	echo "</tr></thead>\n";

	while ($row = mysql_fetch_array($query_result)) {
	    $uid_idx = $row["uid_idx"];
	    $trust   = $row["trust"];

	    if (! ($target_user = User::Lookup($uid_idx))) {
		TBERROR("Could not lookup object for user $uid_idx", 1);
	    }
	    $usr_name     = $target_user->name();
	    $usr_email    = $target_user->email();
	    $usr_uid      = $target_user->uid();
	    $showuser_url = CreateURL("showuser", $target_user);
	    $deluser_url  = CreateURL("deleteuser", $target_user, $project);

	    echo "<tr>
                      <td>$usr_name</td>\n";

	    if (! $projgrp) {
		echo "<td>$usr_email</td>\n";
	    }
	    echo "    <td>
                        <a href='$showuser_url'>$usr_uid</a>
                      </td>\n";

	    if ($trust == TBDB_TRUSTSTRING_NONE) 
		echo "<td><font color=red>$trust</font></td>\n";
	    else
		echo "<td>$trust</td>\n";

	    if ($showdel) {
		echo "<td align=center>
		          <a href='$deluser_url'>
                             <img alt='Delete User' src=redball.gif></td>\n";
	    }
	    echo "</tr>\n";
	}
	echo "</table>\n";
        echo "<script type='text/javascript' language='javascript'>
	       sorttable.makeSortable(getObjbyName('userlist'));
             </script>\n";
    }

    function ShowStats() {
	$gid_idx = $this->gid_idx();

	$query_result =
	    DBQueryFatal("SELECT * from group_stats ".
			 "where gid_idx='$gid_idx'");

	if (! mysql_num_rows($query_result)) {
	    return;
	}
	$row = mysql_fetch_assoc($query_result);

        #
        # Not pretty printed yet.
        #
	echo "<table align=center border=1>\n";
    
	foreach($row as $key => $value) {
	    echo "<tr>
                      <td>$key:</td>
                      <td>$value</td>
                  </tr>\n";
	}
	echo "</table>\n";
    }

    #
    # Return list of experiments for a group, or just a count.
    #
    function ExperimentList($listify = 1) {
	$pid = $this->pid();
	$gid = $this->gid();

	$query_result =
	    DBQueryFatal("select idx from experiments ".
			 "where pid='$pid' and gid='$gid'");

	if (! $listify) {
	    return mysql_num_rows($query_result);
	}

	# Else, create a list of the groups.
	$result  = array();

	while ($row = mysql_fetch_array($query_result)) {
	    $idx = $row["idx"];

	    if (! ($experiment = Experiment::Lookup($idx))) {
		TBERROR("Group::ExperimentList: ".
			"Could not load experiment $idx!", 1);
	    }
	    $result[] = $experiment;
	}
	return $result;
    }
}

#
# Until all the code is fixed, this is a global function that loads the
# group object and passes the call off.
#
# Determine the trust level for a uid/pid/gid. That is, each uid will have
# a different trust level depending on the project/group in question.
# Return that trust level as one of the numeric values above. 
# 
function TBGrpTrust($uid, $pid, $gid)
{
    #
    # No group, then use the default group.
    #
    if (! $gid) {
	$gid = $pid;
    }

    if (! ($group = Group::LookupByPidGid($pid, $gid))) {
        TBERROR("TBGrpTrust: Could not look up group object for $pid/$eid!",1);
    }
    
    if (! ($user = User::Lookup($uid))) {
        TBERROR("TBGrpTrust: Could not look up user object for $uid!", 1);
    }
    return $group->UserTrust($user);
}

