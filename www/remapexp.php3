<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

PAGEHEADER("Remap Virtual Nodes");

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Check to make sure a valid experiment.
#
if (isset($pid) && strcmp($pid, "") &&
    isset($eid) && strcmp($eid, "")) {
    if (! TBvalid_eid($eid)) {
	PAGEARGERROR("$eid is contains invalid characters!");
    }
    if (! TBvalid_pid($pid)) {
	PAGEARGERROR("$pid is contains invalid characters!");
    }
    if (! TBValidExperiment($pid, $eid)) {
	USERERROR("$pid/$eid is not a valid experiment!", 1);
    }
    if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_MODIFY)) {
	USERERROR("You do not have permission to run remap on $pid/$eid!",
		  1);
    }
}
else {
    PAGEARGERROR("Must specify pid and eid!");
}

$expstate = TBExptState($pid, $eid);

if (strcmp($expstate, $TB_EXPTSTATE_ACTIVE) &&
    strcmp($expstate, $TB_EXPTSTATE_SWAPPED)) {
    USERERROR("You cannot remap an experiment in transition.", 1);
}

if (!strcmp($expstate, $TB_EXPTSTATE_ACTIVE)) {
	$reboot = 1;
	$eventrestart = 1;
}
else {
	$reboot = 0;
	$eventrestart = 0;
}

if ($canceled) {
	
    echo "<center><h3><br>
          Operation canceled!
          </h3></center>\n";
    
    PAGEFOOTER();
    return;
}

if (!$confirmed) {

    echo "<font size=+2>Experiment <b>".
	"<a href='showproject.php3?pid=$pid'>$pid</a>/".
	"<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a></b></font>\n";

    echo "<center><font size=+2><br>
              Are you sure you want to remap your experiment?
              </font>\n";

    SHOWEXP($pid, $eid, 1);

    echo "<form action=remapexp.php3 method=post>";
    echo "<input type=hidden name=pid value=$pid>\n";
    echo "<input type=hidden name=eid value=$eid>\n";

    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

# For backend.
TBExptGroup($pid, $eid, $gid);
TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);

echo "<center>";
echo "<h2>Starting experiment remap. Please wait a moment ...
      </h2></center>";

flush();

#	
# Avoid SIGPROF in child.
# 
set_time_limit(0);

$query_result = DBQueryFatal("SELECT nsfile from nsfiles ".
			     "where pid='$pid' and eid='$eid'");
if (mysql_num_rows($query_result)) {
    $row    = mysql_fetch_array($query_result);
    $nsdata = $row[nsfile];
}
else {
    $nsdata = ""; # XXX what to do...
}

list($usec, $sec) = explode(' ', microtime());
srand((float) $sec + ((float) $usec * 100000));
$foo = rand();
$nsfile = "/tmp/$uid-$foo.nsfile";

if (! ($fp = fopen($nsfile, "w"))) {
    TBERROR("Could not create temporary file $nsfile", 1);
}
fwrite($fp, $nsdata);
fclose($fp);
chmod($nsfile, 0666);

$retval = SUEXEC($uid, "$pid,$unix_gid",
		 "webswapexp " . ($reboot ? "-r " : "") .
		 ($eventrestart ? "-e " : "") .
  		 "-s modify $pid $eid $nsfile",
		 SUEXEC_ACTION_IGNORE);

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
    echo "<h3>Experiment remap could not proceed: $retval</h3>";
    echo "<blockquote><pre>$suexec_output<pre></blockquote>";
}
else {
    #
    # Exit status 0 means the experiment is modifying.
    #
    echo "<br>";
    echo "Your experiment is being remapped!<br><br>";

    echo "You will be notified via email when the experiment has ".
	"finished remapping and you are able to proceed. This ".
	"typically takes less than 10 minutes, depending on the ".
	"number of nodes in the experiment. ".
	"If you do not receive email notification within a ".
	"reasonable amount time, please contact $TBMAILADDR. ".
	"<br><br>".
	"While you are waiting, you can watch the log of experiment ".
	"modification in ".
	"<a href=showlogfile.php3?pid=$pid&eid=$eid> ".
	"realtime</a>.\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
