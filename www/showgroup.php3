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
PAGEHEADER("Show Group Information");


#
# Note the difference with which this page gets it arguments!
# I invoke it using GET arguments, so uid and pid are are defined
# without having to find them in URI (like most of the other pages
# find the uid).
#

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

$isadmin = ISADMIN($uid);

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

SHOWGROUP($pid, $gid);
SHOWGROUPMEMBERS($pid, $gid, $prived);

if (TBProjAccessCheck($uid, $pid, $gid, $TB_PROJECT_EDITGROUP) ||
    TBProjAccessCheck($uid, $pid, $pid, $TB_PROJECT_DELGROUP)) {
    SUBPAGEEND();
}

#
# A list of Group experiments.
#
$query_result =
    DBQueryFatal("select e.*,count(r.node_id) as nodes from experiments as e ".
		 "left join reserved as r on e.pid=r.pid and e.eid=r.eid ".
		 "where e.gid='$gid' ".
		 "group by e.eid order by e.state,e.eid");

if (mysql_num_rows($query_result)) {
    echo "<center>
          <h3>Group Experiments</h3>
          </center>
          <table align=center border=1>\n";

    echo "<tr>
              <th>EID</th>
              <th>State</th>
              <th align=center>Nodes</th>
              <th align=center>Hours Idle</th>
              <th>Description</th>
          </tr>\n";

    while ($row = mysql_fetch_array($query_result)) {
        $eid  = $row[eid];
        $name = stripslashes($row[expt_name]);
	$state= $row[state];
	$nodes= $row[nodes];
	$idlehours = TBGetExptIdleTime($pid,$eid);
	if ($idlehours == -1) { $idlehours = "&nbsp;"; }
	if (!$name)
	    $name = "--";
	if ($row[swap_requests] > 0) {
	  $state .= "&nbsp;(idle)";
	}
        echo "<tr>
                 <td><A href='showexp.php3?pid=$pid&eid=$eid'>$eid</a></td>
		 <td>$state</td>
                 <td align=center>$nodes</td>
                 <td align=center>$idlehours</td>
                 <td>$name</td>
              </tr>\n";
    }
    echo "</table>\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
