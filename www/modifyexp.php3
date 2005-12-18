<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2005 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

$parser   = "$TB/libexec/ns2ir/parse-ns";

#
# Only known and logged in users can modify experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

# This will not return if its a sajax request.
include("showlogfile_sup.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Modify Experiment");

#
# Verify page arguments.
# 
if (!isset($pid) ||
    strcmp($pid, "") == 0) {
    USERERROR("You must provide a Project ID.", 1);
}
if (!isset($eid) ||
    strcmp($eid, "") == 0) {
    USERERROR("You must provide an Experiment ID.", 1);
}

#
# Be paranoid.
#
$pid = addslashes($pid);
$eid = addslashes($eid);

#
# Check to make sure this is a valid experiment.
#
if (! TBValidExperiment($pid, $eid)) {
    USERERROR("The experiment $eid is not a valid experiment ".
	      "in project $pid.", 1);
}

if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_MODIFY)) {
    USERERROR("You do not have permission to modify this experiment.", 1);
}

if (TBExptLockedDown($pid, $eid)) {
    USERERROR("Cannot proceed; experiment is locked down!", 1);
}

$expstate = TBExptState($pid, $eid);

if (strcmp($expstate, $TB_EXPTSTATE_ACTIVE) &&
    strcmp($expstate, $TB_EXPTSTATE_SWAPPED)) {
    USERERROR("You cannot modify an experiment in transition.", 1);
}

# Okay, start.
echo "<font size=+2>Experiment <b>".
     "<a href='showproject.php3?pid=$pid'>$pid</a>/".
     "<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a></b></font>\n";
echo "<br>\n";
flush();

#
# Put up the modify form on first load.
# 
if (! isset($go)) {
    echo "<a href='faq.php3#swapmod'>".
	 "Modify Experiment Documentation (FAQ)</a></h3>";
    echo "<br>";

    if (STUDLY()) {
	echo "<font size='+1'>".
	    "<a href='clientui.php3?pid=$pid&eid=$eid'>GUI Editor</a>".
	    " - Edit the topology using a Java applet.</font>";
	echo "<br>";
    }

    echo "<form action='modifyexp.php3?pid=$pid&eid=$eid' method='post'>";
    echo "<textarea cols='100' rows='40' name='nsdata'>";

    $query_result =
	DBQueryFatal("SELECT nsfile from nsfiles ".
		     "where pid='$pid' and eid='$eid'");
    if (mysql_num_rows($query_result)) {
	$row    = mysql_fetch_array($query_result);
	$nsfile = $row[nsfile];
	    
	echo "$nsfile";
    }
    else {
	echo "# There was no stored NS file for $pid/$eid.\n";
    }
	
    echo "</textarea>";
    echo "<br>";
    if (!strcmp($expstate, $TB_EXPTSTATE_ACTIVE)) {
	echo "<p><b>Note!</b> It is recommended that you 
	      reboot all nodes in your experiment by checking the box below.
	      This is especially important if changing your experiment
              topology (adding or removing nodes, links, and LANs).
	      If adding/removing a delay to/from an existing link, or
              replacing a lost node <i>without modifying the experiment
              topology</i>, this won't be necessary. Restarting the
	      event system is also highly recommended since the same nodes
	      in your virtual topology may get mapped to different physical
	      nodes.</p>";
	echo "<input type='checkbox' name='reboot' value='1' checked='1'>
	      Reboot nodes in experiment (Highly Recommended)</input>";
	echo "<br><input type='checkbox' name='eventrestart' value='1' checked='1'>
	      Restart Event System in experiment (Highly Recommended)</input>";
    }
    echo "<br>";
    echo "<input type='submit' name='go' value='Modify'>";
    echo "</form>\n";
    PAGEFOOTER();
    exit();
}

#
# Okay, form has been submitted.
#
if (! isset($nsdata)) {
    USERERROR("NSdata CGI variable missing (How did that happen?)",1);
}

#
# Generate a hopefully unique filename that is hard to guess.
# See backend scripts.
# 
list($usec, $sec) = explode(' ', microtime());
srand((float) $sec + ((float) $usec * 100000));
$foo = rand();
    
$nsfile = "/tmp/$uid-$foo.nsfile";

if (! ($fp = fopen($nsfile, "w"))) {
    TBERROR("Could not create temporary file $nsfile", 1);
}
$nsdata_string = $nsdata;
fwrite($fp, $nsdata_string);
fclose($fp);
chmod($nsfile, 0666);

#
# Get exp group so we can get the unix_gid.
#
TBExptGroup($pid, $eid, $gid);
TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);

#
# Do an initial parse test.
#
$retval = SUEXEC($uid, "$pid,$unix_gid", "webnscheck $nsfile",
		 SUEXEC_ACTION_IGNORE);

if ($retval != 0) {
    unlink($nsfile);
    
    # Send error to tbops.
    if ($retval < 0) {
	SUEXECERROR(SUEXEC_ACTION_CONTINUE);
    }
    echo "<br>";
    echo "<h3>Modified NS file contains syntax errors</h3>";
    echo "<blockquote><pre>$suexec_output<pre></blockquote>";

    PAGEFOOTER();
    exit();
}

#	
# Avoid SIGPROF in child.
# 
set_time_limit(0);

# Run the script.
$retval = SUEXEC($uid, "$pid,$unix_gid",
		 "webswapexp $rebootswitch " . ($reboot ? "-r " : "") .
		 ($eventrestart ? "-e " : "") .
		 "-s modify $pid $eid $nsfile",
		 SUEXEC_ACTION_IGNORE);
		 
# It has been copied out by the program!
unlink($nsfile);

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
# Exit status 0 means the experiment is swapping, or will be.
#
echo "<br>\n";
if ($retval) {
    echo "<h3>Experiment modify could not proceed</h3>";
    echo "<blockquote><pre>$suexec_output<pre></blockquote>";
}
else {
    #
    # Exit status 0 means the experiment is modifying.
    #
    echo "<b>Your experiment is being modified!</b> ";
    echo "You will be notified via email when the experiment has ".
	"finished modifying and you are able to proceed. This ".
	"typically takes less than 10 minutes, depending on the ".
	"number of nodes in the experiment. ".
	"If you do not receive email notification within a ".
	"reasonable amount time, please contact $TBMAILADDR. ".
	"<br><br>".
	"While you are waiting, you can watch the log of experiment ".
	"modification in realtime:<br>\n";
    STARTLOG($pid, $eid);    
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>


