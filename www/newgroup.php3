<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Create a Project Group");

$mydebug = 0;

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
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

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
# Verify project and leader. That is, the leader choosen has to be a member
# of the default group for the project and must already possess a
# minimum level of trust since it would make no sense to make "user" a
# group leader.
#
if (! TBProjAccessCheck($group_leader, $group_pid, 0, $TB_PROJECT_LEADGROUP)) {
    USERERROR("$group_leader is not a trusted (local root or better) member ".
	      "of the project $group_pid!", 1);
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

#
# Create the new group and set up the initial membership for the leader
# (and the project leader if not the same).
# 
DBQueryFatal("INSERT INTO groups ".
	     "(pid, gid, leader, created, description, unix_gid, unix_name) ".
	     "VALUES ('$group_pid', '$group_id', '$group_leader', now(), ".
	     "        '$group_description', NULL, '$unix_gname')");

DBQueryFatal("insert into group_membership ".
	     "(uid, pid, gid, trust, date_applied, date_approved) ".
	     "values ('$group_leader','$group_pid','$group_id', ".
	     "        'group_root', now(), now())");

#
# Grab the project head uid for the project. If its different than the
# the leader for the group, insert the project head also. This is polite.
#
$query_result =
    DBQueryFatal("select head_uid from projects where pid='$group_pid'");
if (($row = mysql_fetch_row($query_result)) == 0) {
    DBFatal("Getting head_uid for project $group_pid.");
}
$head_uid = $row[0];

if (strcmp($head_uid, $group_leader)) {
    DBQueryFatal("insert into group_membership ".
		 "(uid, pid, gid, trust, date_applied, date_approved) ".
		 "values ('$head_uid','$group_pid','$group_id', ".
		 "        'group_root', now(), now())");
}

#
# Grab the unix GID for running scripts.
#
TBGroupUnixInfo($group_pid, $group_pid, $unix_gid, $unix_name);

echo "<br>
      Group '$group_id' in project '$group_pid' is being created!<br><br>
      This will take a minute or two. <b>Please</b> do not click the Stop
      button during this time. If you do not receive notification within
      a reasonable amount of time, please contact $TBMAILADDR.\n";
flush();

#
# Run the script. This will make the group directory, set the perms,
# and do the account stuff for all of the people in the group. This
# is the same script that gets run when the group membership changes.
#
SUEXEC($uid, $unix_gid, "webmkgroup $group_pid $group_id", 1);
SUEXEC($uid, $unix_gid, "websetgroups $head_uid $group_leader", 1);

echo "<br><br>
      <b>Done!</b>
      <br>\n";

#
# Show it!
#
SHOWGROUP($group_pid, $group_id);

#
# Back to ...
# 
echo "<br>
       <A href='showproject.php3?pid=$group_pid'>Back to Project page</a>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
