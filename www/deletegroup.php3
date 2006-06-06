<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Delete a Group");

#
# Only known and logged in users.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# First off, sanity check page args.
#
if (!isset($pid) ||
    strcmp($pid, "") == 0) {
    USERERROR("Must provide a Project ID!", 1);
}
if (!isset($gid) ||
    strcmp($gid, "") == 0) {
    USERERROR("Must privide a Group ID!", 1);
}

#
# We do not allow the default group to be deleted. Never ever!
#
if (strcmp($gid, $pid) == 0) {
    USERERROR("You are not allowed to delete a project's default group!", 1);
}

#
# Verify permission.
#
if (! TBProjAccessCheck($uid, $pid, $pid, $TB_PROJECT_DELGROUP)) {
    USERERROR("You do not have permission to delete groups in project $pid!",
	      1);
}

#
# Check to see if there are any active experiments. Abort if there are.
#
$query_result =
    DBQueryFatal("SELECT * FROM experiments where pid='$pid' and gid='$gid'");
if (mysql_num_rows($query_result)) {
    USERERROR("Project/Group '$pid/$gid' has active experiments. You must ".
	      "terminate ".
	      "those experiments before you can remove the project!", 1);
}

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we should
# probably redirect the browser back up a level.
#
if ($canceled) {
    echo "<center><h2>
          Group removal canceled!
          </h2></center>\n";
    
    PAGEFOOTER();
    return;
}

if (!$confirmed) {
    echo "<center><h2>
          Are you <b>REALLY</b> sure you want to remove Group $gid
          in Project $pid?
          </h2>\n";
    
    echo "<form action='deletegroup.php3?pid=$pid&gid=$gid' method=post>";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

#
# Grab the unix group info of the project for running scripts.
#
TBGroupUnixInfo($pid, $pid, $unix_gid, $unix_name);

STARTBUSY("Group '$gid' in project '$pid' is being removed!");

#
# Run the script. They will remove the group directory and the unix group,
# and the DB state.
#
SUEXEC($uid, $unix_gid, "webrmgroup $pid $gid", SUEXEC_ACTION_DIE);
STOPBUSY();

PAGEREPLACE("showproject.php3?pid=$pid");

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
