<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Swap/Restart an Experiment");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

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

if (!isset($inout) ||
    (strcmp($inout, "in") && strcmp($inout, "out") &&
     strcmp($inout, "restart"))) {
    USERERROR("The argument must be either in, out, or restart!", 1);
}

#
# Only admins can issue a force swapout
# 
if (isset($force) && $force == 1) {
	if (! ISADMIN($uid)) {
		USERERROR("Only testbed administrators can forcibly swap ".
			  "an experiment out!", 1);
	}
}
else {
	$force = 0;
}

$exp_eid = $eid;
$exp_pid = $pid;

if (!strcmp($inout, "in")) {
    $action = "swapin";
}
elseif (!strcmp($inout, "out")) {
    $action = "swapout";
}
elseif (!strcmp($inout, "restart")) {
    $action = "restart";
}

#
# Check to make sure thats this is a valid PID/EID tuple.
#
$query_result =
    DBQueryFatal("SELECT * FROM experiments WHERE ".
		 "eid='$exp_eid' and pid='$exp_pid'");
if (mysql_num_rows($query_result) == 0) {
    USERERROR("The experiment $exp_eid is not a valid experiment ".
	      "in project $exp_pid.", 1);
}
$row = mysql_fetch_array($query_result);
$exp_gid = $row[gid];
$batch   = $row[batchmode];

#
# Look for transition in progress and exit with error. 
#
$expt_locked = $row[expt_locked];
if ($expt_locked) {
    USERERROR("It appears that experiment $exp_eid went into transition at ".
	      "$expt_locked.<br>".
	      "You must wait until the experiment is no longer in transition.".
	      "<br><br>".
	      "When the transition has completed, a notification will be ".
	      "sent via email to the user that initiated it.", 1);
}

#
# Verify permissions.
#
if (! TBExptAccessCheck($uid, $exp_pid, $exp_eid, $TB_EXPT_MODIFY)) {
    USERERROR("You do not have permission for $exp_eid!", 1);
}

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we should
# probably redirect the browser back up a level.
#
if ($canceled) {
    echo "<center><h2><br>
          Experiment $action canceled!
          </h2></center>\n";
    
    PAGEFOOTER();
    return;
}

if (!$confirmed) {
    echo "<center><h2><br>
          Are you sure you want to ";
    if ($force) {
	    echo "<font color=red><br>forcibly</br></font> ";
    }
    echo "$action experiment '$exp_eid?'
          </h2>\n";
    
    echo "<form action='swapexp.php3?inout=$inout&pid=$exp_pid&eid=$exp_eid'
                method=post>";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";

    if ($force) {
	    echo "<input type=hidden name=force value=$force>\n";
    }
    
    echo "</form>\n";

    if (!strcmp($inout, "restart")) {
	echo "<p>
              <a href='$TBDOCBASE/faq.php3#UTT-Restart'>
                 (Information on experiment restart)</a>\n";
    }
    else {
	echo "<p>
              <a href='$TBDOCBASE/faq.php3#UTT-Swapping'>
                 (Information on experiment swapping)</a>\n";
    }
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

#
# We need the unix gid for the project for running the scripts below.
# Note usage of default group in project.
#
TBGroupUnixInfo($exp_pid, $exp_gid, $unix_gid, $unix_name);

#
# We run a wrapper script that does all the work of terminating the
# experiment. 
#
#   tbstopit <pid> <eid>
#
echo "<center><br>";
echo "<h2>Starting experiment $action. Please wait a moment ...
      </h2></center>";

flush();

#
# Run the scripts. We use a script wrapper to deal with changing
# to the proper directory and to keep some of these details out
# of this. 
#
# Avoid SIGPROF in child.
# 
set_time_limit(0);

$output = array();
$retval = 0;
$result = exec("$TBSUEXEC_PATH $uid $unix_gid ".
	       ($force ?
		"webidleswap $exp_pid $exp_eid" :
		"webswapexp -s $inout $exp_pid $exp_eid"),
 	       $output, $retval);

if ($retval) {
    echo "<br><br><h2>
          $action failure($retval): Output as follows:
          </h2>
          <br>
          <XMP>\n";
          for ($i = 0; $i < count($output); $i++) {
              echo "$output[$i]\n";
          }
    echo "</XMP>\n";

    PAGEFOOTER();
    die("");
}

#
# Exit status 0 means the experiment is swapping, or will be.
#
echo "<br><br><h3>\n";
if ($retval == 0) {
    if (strcmp($inout, "in") == 0)
	$howlong = "two to ten";
    else
	$howlong = "less than two";
    
    echo "Experiment
	  <a href='showexp.php3?pid=$exp_pid&eid=$exp_eid'>$exp_eid</a>
          in project <A href='showproject.php3?pid=$exp_pid'>$exp_pid</A>
          has started its $action.
          <br><br>
          You will be notified via email when the operation is complete.
          This typically takes $howlong minutes, depending on the
          number of nodes in the experiment. 
          If you do not receive email notification within a reasonable amount
          of time, please contact $TBMAILADDR.
          <br><br>
          While you are waiting, you can watch the log
          in <a target=_blank href=spewlogfile.php3?pid=$exp_pid&eid=$exp_eid>
          realtime</a>.\n";
}
echo "</h3>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
