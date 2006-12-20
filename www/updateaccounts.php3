<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Must provide the EID!
# 
if (!isset($pid) ||
    strcmp($pid, "") == 0) {
  USERERROR("The project ID was not provided!", 1);
}

if (!isset($eid) ||
    strcmp($eid, "") == 0) {
  USERERROR("The experiment ID was not provided!", 1);
}

if (isset($nodeid) && strcmp($nodeid, "")) {
    $nodeid = addslashes($nodeid);
}
else {
    unset($nodeid);
}

# Canceled operation redirects back to showexp page. See below.
if ($canceled) {
    header("Location: showexp.php3?pid=$pid&eid=$eid");
    return;
}

#
# Standard Testbed Header
#
PAGEHEADER("Update Experimental Nodes");

#
# Check to make sure thats this is a valid PID/EID tuple.
#
$query_result =
    DBQueryFatal("SELECT * FROM experiments WHERE ".
		 "eid='$eid' and pid='$pid'");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("The experiment $eid is not a valid experiment ".
            "in project $pid.", 1);
}
$row = mysql_fetch_array($query_result);
$gid = $row[gid];

#
# Verify permissions.
#
if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_UPDATE)) {
    USERERROR("You do not have permission to initiate node updates!", 1);
}
if (isset($nodeid) && !TBValidNodeName($nodeid)) {
    USERERROR("Node $nodeid is not a valid nodeid!", 1);
}

echo "<font size=+2>Experiment <b>".
     "<a href='showproject.php3?pid=$pid'>$pid</a>/".
     "<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a></b></font>\n";

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button (see above).
#
if (!$confirmed) {
    echo "<br><br><b>
          Confirming this operation will initiate various updates to be
          performed, including updating the password and group files,
          adding new mounts, and installing (or updating if modified) tarballs
          and rpms.
          This is sometimes quicker and easier than rebooting nodes.</b>\n";

    echo "<center><h2><br>
          Are you sure you want to perform an update on ";

    if (isset($nodeid)) {
	echo "node $nodeid in experiment '$eid?'\n";
    }
    else {
	echo "all of the nodes in experiment '$eid?'\n";
    }
    echo "</h2>\n";

    if (isset($nodeid)) {
	SHOWNODE($nodeid, SHOWNODE_SHORT);
    }
    else {
	SHOWEXP($pid, $eid, 1);
    }
    
    echo "<form action='updateaccounts.php3?pid=$pid&eid=$eid' method=post>";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    if (isset($nodeid)) {
	echo "<input type=hidden name=nodeid value=$nodeid>\n";
    }
    echo "</form>\n";
    echo "</center>\n";

    echo "<br>
          <b>NOTE</b> that this operation will consume a small number of CPU
          cycles on
          each node. If this would disturb an experiment in progress, you
          should cancel this operation until later.
          Please note that accounts are automatically updated whenever a node
          reboots, or when an experiment is swapped back in.\n";

    PAGEFOOTER();
    return;
}

#
# We need the unix gid for the project for running the scripts below.
# Note usage of default group in project.
#
TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);

#
# Start up a script to do this.
#
echo "<center><br>";
echo "<h2>Starting node update. Please wait a moment ...
      </h2></center>";

flush();

$retval = SUEXEC($uid, "$pid,$unix_gid",
		 "webnodeupdate -b $pid $eid" .
		 (isset($nodeid) ? " $nodeid" : ""),
		 SUEXEC_ACTION_IGNORE);

#
# Fatal Error. Report to the user, even though there is not much he can
# do with the error. Also reports to tbops.
# 
if ($retval < 0) {
    SUEXECERROR(SUEXEC_ACTION_DIE);
    #
    # Never returns ...
    #
    die("");
}

#
# Exit status 0 means the operation is proceeding in the background.
#
echo "<br>\n";
if ($retval) {
    echo "<h3>Node update could not proceed</h3>";
    echo "<blockquote><pre>$suexec_output<pre></blockquote>";
}
else {
    echo "You will be notified via email when the update has completed on
          all of the nodes in your experiment. This might take a just a
          few minutes (if updating only accounts) or it might take longer
          if new tar/rpm files need to be installed. In the meantime, the
          experiment is locked it may not be swapped or modified.\n";

    echo "<br><br>
         If you do not receive
         email notification within a reasonable amount of time,
         please contact $TBMAILADDR.\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
