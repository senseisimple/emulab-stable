<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2005, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Show Group Information");


#
# Note the difference with which this page gets it arguments!
# I invoke it using GET arguments, so uid and pid are are defined
# without having to find them in URI (like most of the other pages
# find the uid).
#

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify form arguments.
# 
if (!isset($pid) ||
    strcmp($pid, "") == 0) {
    USERERROR("You must provide a project ID.", 1);
}

if (!isset($gid) ||
    strcmp($gid, "") == 0) {
    USERERROR("You must provide a group ID.", 1);
}

#
# Check to make sure thats this is a valid PID/GID.
#
$query_result = 
    DBQueryFatal("SELECT * FROM groups WHERE pid='$pid' and gid='$gid'");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("The group $pid/$gid is not a valid group", 1);
}

#
# Verify permission to look at the group. This is a little different,
# since the standard test would look for permission in just the group,
# but we also want to allow user from the project with appropriate
# privs to look at the group.
#
if (! TBProjAccessCheck($uid, $pid, $gid, $TB_PROJECT_READINFO) &&
    ! TBMinTrust(TBGrpTrust($uid, $pid, $pid), $TBDB_TRUST_GROUPROOT)) {
    USERERROR("You are not a member of group $gid in project $pid!", 1);
}

#
# See if user is privledged for deletion.
#
$prived = 0;
if ($isadmin || TBProjAccessCheck($uid, $pid, $pid, $TB_PROJECT_DELUSER)) {
    $prived = 1;
}

#
# This menu only makes sense for people with privs to use them.
#
if (TBProjAccessCheck($uid, $pid, $gid, $TB_PROJECT_EDITGROUP) ||
    (strcmp($gid, $pid) && 
     TBProjAccessCheck($uid, $pid, $pid, $TB_PROJECT_DELGROUP))) {

    SUBPAGESTART();
    SUBMENUSTART("Group Options");

    if (TBProjAccessCheck($uid, $pid, $gid, $TB_PROJECT_EDITGROUP)) {
	WRITESUBMENUBUTTON("Edit this Group",
			   "editgroup_form.php3?pid=$pid&gid=$gid");
    }

    #
    # A delete option, but not for the default group!
    #
    if (strcmp($gid, $pid) &&
	TBProjAccessCheck($uid, $pid, $pid, $TB_PROJECT_DELGROUP)) {
	WRITESUBMENUBUTTON("Delete this Group",
			   "deletegroup.php3?pid=$pid&gid=$gid");
    }
    SUBMENUEND();
}

SHOWGROUP($pid, $gid, $uid);
SHOWGROUPMEMBERS($pid, $gid, $prived);

if (TBProjAccessCheck($uid, $pid, $gid, $TB_PROJECT_EDITGROUP) ||
    TBProjAccessCheck($uid, $pid, $pid, $TB_PROJECT_DELGROUP)) {
    SUBPAGEEND();
}

# Project wide Templates.
if ($EXPOSETEMPLATES) {
    SHOWTEMPLATELIST("GROUP", 0, $uid, $pid, $gid);
}

#
# A list of Group experiments.
#
SHOWEXPLIST("GROUP", $uid, $pid, $gid);

if ($isadmin) {
    echo "<center>
          <h3>Group Stats</h3>
         </center>\n";

    SHOWGROUPSTATS($pid, $gid);
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
