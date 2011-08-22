<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2011 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("node_defs.php");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("experiment", PAGEARG_EXPERIMENT);
$optargs = OptionalPageArguments("node",       PAGEARG_NODE,
				 "canceled",   PAGEARG_BOOLEAN,
				 "confirmed",  PAGEARG_BOOLEAN);

# Canceled operation redirects back to showexp page. See below.
if (isset($canceled) && $canceled) {
    header("Location: " . CreateURL("showexp", $experiment));
    return;
}

# Need these below
$pid = $experiment->pid();
$eid = $experiment->eid();
$unix_gid = $experiment->UnixGID();
$node_id  = (isset($node) ? $node->node_id() : "");
$project  = $experiment->Project();
$unix_pid = $project->unix_gid();

#
# Standard Testbed Header
#
PAGEHEADER("Update Experimental Nodes");

#
# Verify permissions.
#
if (!$experiment->AccessCheck($this_user, $TB_EXPT_UPDATE)) {
    USERERROR("You do not have permission to initiate node updates!", 1);
}

echo $experiment->PageHeader();

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button (see above).
#
if (!isset($confirmed)) {
    echo "<br><br><b>
          Confirming this operation will initiate various updates to be
          performed, including updating the password and group files,
          adding new mounts, and installing (or updating if modified) tarballs
          and rpms.
          This is sometimes quicker and easier than rebooting nodes.</b>\n";

    echo "<center><h2><br>
          Are you sure you want to perform an update on ";

    if (isset($node)) {
	echo "node $node_id in experiment '$eid?'\n";
    }
    else {
	echo "all of the nodes in experiment '$eid?'\n";
    }
    echo "</h2>\n";

    if (isset($node)) {
	$node->Show(SHOWNODE_SHORT);
	$url = CreateURL("updateaccounts", $experiment, $node);
    }
    else {
	$experiment->Show(1);
	$url = CreateURL("updateaccounts", $experiment);
    }
    
    echo "<form action='$url' method=post>";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
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

STARTBUSY("Starting node update");
$retval = SUEXEC($uid, "$unix_pid,$unix_gid",
		 "webnode_update -b $pid $eid $node_id",
		 SUEXEC_ACTION_IGNORE);
CLEARBUSY();

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
