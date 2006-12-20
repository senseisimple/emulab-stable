<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Standard Header
#
#
# Standard Testbed Header
#
PAGEHEADER("Create a Project Group");

ignore_user_abort(1);

#
# First off, sanity check the form to make sure all the required fields
# were provided. I do this on a per field basis so that we can be
# informative. Be sure to correlate these checks with any changes made to
# the project form. 
#
if (!isset($group_pid) ||
    strcmp($group_pid, "") == 0) {
  FORMERROR("Select Project");
}
if (!isset($group_id) ||
    strcmp($group_id, "") == 0) {
  FORMERROR("Group Name");
}
if (!isset($group_description) ||
    strcmp($group_description, "") == 0) {
  FORMERROR("Group Description");
}
if (!isset($group_leader) ||
    strcmp($group_leader, "") == 0) {
  FORMERROR("Group leader");
}

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Check ID for sillyness.
#
if (! ereg("^[-_a-zA-Z0-9]+$", $group_id)) {
    USERERROR("The group name must be alphanumeric characters only!", 1);
}

#
# Database limits
#
if (strlen($group_id) > $TBDB_GIDLEN) {
    USERERROR("The Group name is too long! Please select another.", 1);
}

#
# Certain of these values must be escaped or otherwise sanitized.
# 
$group_description = addslashes($group_description);

#
# Verify permission.
#
if (! TBProjAccessCheck($uid, $group_pid, 0, $TB_PROJECT_MAKEGROUP)) {
    USERERROR("You do not have permission to create groups in project ".
	      "$group_pid!", 1);
}

#
# Verify project and leader. Any user can lead a group.
#
if (! TBProjAccessCheck($group_leader, $group_pid, 0, $TB_PROJECT_LEADGROUP)) {
    USERERROR("$group_leader does not have enough permission to lead a group ".
	      "in project $group_pid!", 1);
}
	       
#
# Make sure the GID is not already there.
#
if (TBValidGroup($group_pid, $group_id)) {
    USERERROR("The group $group_id already exists! Please select another.", 1);
}

#
# The unix group name must be globally unique. Form a name and check it.
#
$unix_gname = substr($group_pid, 0, 3) . "-" . $group_id;
$maxtries   = 99;
$count      = 0;

while ($count < $maxtries) {
    if (strlen($unix_gname) > $TBDB_UNIXGLEN) {
	TBERROR("Unix group name $unix_gname is too long!", 1);
    }
    
    $query_result =
	DBQueryFatal("select gid from groups where unix_name='$unix_gname'");

    if (mysql_num_rows($query_result) == 0) {
	break;
    }
    $count++;

    $unix_gname = substr($group_pid, 0, 3) . "-" .
	substr($group_id,  0, strlen($group_id) - 2) . "$count";
}
if ($count == $maxtries) {
    TBERROR("Could not form a unique Unix group name!", 1);
}

# Need the user object for creating the group.
if (! ($leader = User::Lookup($group_leader))) {
    TBERROR("Could not lookup user '$group_leader'!", 1);
}
# and the project.
if (! ($project = Project::Lookup($group_pid))) {
    TBERROR("Could not lookup project '$group_pid'!", 1);
}

#
# Create the new group and set up the initial membership for the leader.
#
# Note, if the project leader wants to be in the subgroup, he/she has to
# add themself via the edit page. 
#
if (! ($newgroup = Group::NewGroup($project, $group_id, $leader,
				   $group_description, $unix_gname))) {
    TBERROR("Could not create new group $group_pid/$group_id!", 1);
}

#
# Grab the unix GID for running scripts.
#
TBGroupUnixInfo($group_pid, $group_pid, $unix_gid, $unix_name);

STARTBUSY("Creating project group $group_id.");

#
# Run the script. This will make the group directory, set the perms, etc.
#
SUEXEC($uid, $unix_gid, "webmkgroup $group_pid $group_id", 1);

#
# Now add the group leader to the group.
# 
SUEXEC($uid, $unix_gid,
       "webmodgroups -a $group_pid:$group_id:group_root $group_leader", 1);

STOPBUSY();

#
# Send an email message with a join link.
#
$group_leader_name  = $leader->name();
$group_leader_email = $leader->email();
$user_name          = $this_user->name();
$user_email         = $this_user->email();

TBMAIL("$group_leader_name '$group_leader' <$group_leader_email>",
       "New Group '$group_pid/$group_id' Created",
       "\n".
       "This message is to notify you that group '$group_id' in project\n".
       "'$group_pid' has been created. Please save this link so that you\n".
       "send it to people you wish to have join this group:\n".
       "\n".
       "    ${TBBASE}/joinproject.php3".
                             "?target_pid=$group_pid&target_gid=$group_id\n".
       "\n",
       "From: $user_name '$uid' <$user_email>\n".
       "CC: $user_name '$uid' <$user_email>\n".
       "Errors-To: $TBMAIL_WWW");

#
# Redirect back to project page.
#
PAGEREPLACE("showgroup.php3?pid=$group_pid&gid=$group_id");

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
