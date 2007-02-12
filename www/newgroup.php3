<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Create a Project Group");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments
#
$reqargs = RequiredPageArguments("project",           PAGEARG_PROJECT,
				 "group_id",          PAGEARG_STRING,
				 "group_description", PAGEARG_ANYTHING,
				 "group_leader",      PAGEARG_STRING);

# Need these below
$group_pid = $project->pid();
$unix_gid  = $project->unix_gid();
$safe_id   = escapeshellarg($group_id);

#
# Check ID for sillyness.
#
if ($group_id == "") {
    USERERROR("Must provide a group name!", 1);
}
elseif (! TBvalid_gid($group_id)) {
    USERERROR("Invalid group name: " . TBFieldErrorString(), 1);
}
if ($group_leader == "") {
    USERERROR("Must provide a group leader!", 1);
}

#
# Certain of these values must be escaped or otherwise sanitized.
# 
$group_description = addslashes($group_description);

#
# Verify permission.
#
if (!$project->AccessCheck($this_user, $TB_PROJECT_MAKEGROUP)) {
    USERERROR("You do not have permission to create groups in project ".
	      "$group_pid!", 1);
}

# Need the user object for creating the group.
if (! ($leader = User::Lookup($group_leader))) {
    USERERROR("User '$group_leader' is an unknown user!", 1);
}

#
# Verify leader. Any user can lead a group, but they must be a member of
# the project, and we have to avoid an ISADMIN() check in AccessCheck().
#
if (!$project->IsMember($leader, $isapproved) ||
    !$project->AccessCheck($leader, $TB_PROJECT_LEADGROUP)) {
    USERERROR("$group_leader does not have enough permission to lead a group ".
	      "in project $group_pid!", 1);
}

#
# Make sure the GID is not already there.
#
if (($oldgroup = Group::LookupByPidGid($group_pid, $group_id))) {
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

STARTBUSY("Creating project group $group_id.");

#
# Run the script. This will make the group directory, set the perms, etc.
#
SUEXEC($uid, $unix_gid,
       "webmkgroup $group_pid $safe_id", SUEXEC_ACTION_DIE);

#
# Now add the group leader to the group.
# 
SUEXEC($uid, $unix_gid,
       "webmodgroups -a $group_pid:$safe_id:group_root $group_leader",
       SUEXEC_ACTION_DIE);

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
PAGEREPLACE(CreateURL("showgroup", $newgroup));

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
