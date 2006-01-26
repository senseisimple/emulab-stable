<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Show Project Information");


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

#
# Check to make sure thats this is a valid PID.
#
if (! TBValidProject($pid)) {
    USERERROR("The project '$pid' is not a valid project.", 1);
}

#
# Verify that this uid is a member of the project being displayed.
#
if (! TBProjAccessCheck($uid, $pid, $pid, $TB_PROJECT_READINFO)) {
    USERERROR("You are not a member of Project $pid.", 1);
}

SUBPAGESTART();
SUBMENUSTART("Project Options");
WRITESUBMENUBUTTON("Create Subgroup",
		   "newgroup_form.php3?pid=$pid");
WRITESUBMENUBUTTON("Edit User Privs",
		   "editgroup_form.php3?pid=$pid&gid=$pid");
WRITESUBMENUBUTTON("Remove Users",
		   "showgroup.php3?pid=$pid&gid=$pid");
WRITESUBMENUBUTTON("Show Project History",
		   "showstats.php3?showby=project&which=$pid");
WRITESUBMENUBUTTON("Free Node Summary",
		   "nodecontrol_list.php3?showtype=summary&bypid=$pid");
if ($isadmin) {
    WRITESUBMENUDIVIDER();
    WRITESUBMENUBUTTON("Delete this project",
		       "deleteproject.php3?pid=$pid");
}

SUBMENUEND();
SHOWPROJECT($pid, $uid);
SUBPAGEEND();

echo "<center>\n";
echo "<table border=0 bgcolor=#000 color=#000 class=stealth>\n";
echo "<tr valign=top><td class=stealth align=center>\n";

#
# A list of project members (from the default group).
#
SHOWGROUPMEMBERS($pid, $pid, 0);

echo "</td><td align=center class=stealth>\n";

#
# A list of project Groups
#
echo "<h3>Project Groups</h3>\n";

$query_result =
    DBQueryFatal("SELECT * FROM groups WHERE pid='$pid'");
echo "<table align=center border=1>\n";
echo "<tr>
          <th>GID</th>
          <th>Description</th>
          <th>Leader</th>
      </tr>\n";

while ($row = mysql_fetch_array($query_result)) {
    $gid      = $row[gid];
    $desc     = stripslashes($row[description]);
    $leader   = $row[leader];

    echo "<tr>
              <td><A href='showgroup.php3?pid=$pid&gid=$gid'>$gid</a></td>
              <td>$desc</td>
              <td><A href='showuser.php3?target_uid=$leader'>$leader</A></td>
          </tr>\n";
}
echo "</table>\n";
echo "</td></table>\n";
echo "</center>\n";

#
# A list of project experiments.
#
SHOWEXPLIST("PROJ", $uid, $pid);

if ($isadmin) {
    echo "<center>
          <h3>Project Stats</h3>
         </center>\n";

    SHOWPROJSTATS($pid);
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
