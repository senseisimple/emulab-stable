<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

PAGEHEADER("Remap Virtual Nodes");

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("experiment",   PAGEARG_EXPERIMENT);
$optargs = OptionalPageArguments("canceled",     PAGEARG_STRING,
				 "reboot",       PAGEARG_BOOLEAN,
				 "eventrestart", PAGEARG_BOOLEAN,
				 "confirmed",    PAGEARG_STRING);

#
# Need these below
#
$pid = $experiment->pid();
$eid = $experiment->eid();
$unix_gid = $experiment->UnixGID();
$expstate = $experiment->state();

if (!$experiment->AccessCheck($this_user, $TB_EXPT_MODIFY)) {
    USERERROR("You do not have permission to run remap on $pid/$eid!", 1);
}

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

if (isset($canceled) && $canceled) {
    echo "<center><h3><br>
          Operation canceled!
          </h3></center>\n";
    
    PAGEFOOTER();
    return;
}

if (!isset($confirmed)) {
    echo $experiment->PageHeader();
    echo "<br>\n";

    echo "<center><font size=+2><br>
              Are you sure you want to remap your experiment?
              </font>\n";

    $experiment->Show(1);

    $url = CreateURL("remapexp", $experiment);

    echo "<form action='$url' method=post>";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

#
# Need to pass out the NS data.
#
if (! ($nsdata = $experiment->NSFile())) {
    # XXX what to do...
    $nsdata = "";
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

#	
# Avoid SIGPROF in child.
# 
set_time_limit(0);

# optargs.
$optargs = "";
if (isset($reboot) && $reboot) {
     $optargs .= " -r ";
}
if (isset($eventrestart) && $eventrestart) {
     $optargs .= " -e ";
}

STARTBUSY("Starting experiment remap");
$retval = SUEXEC($uid, "$pid,$unix_gid",
		 "webswapexp $optargs -s modify $pid $eid $nsfile",
		 SUEXEC_ACTION_IGNORE);
CLEARBUSY();
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

echo "<br>\n";
if ($retval) {
    echo "<h3>Experiment remap could not proceed: $retval</h3>";
    echo "<blockquote><pre>$suexec_output<pre></blockquote>";
}
else {
    $showlog_url = CreateURL("showlogfile", $experiment);
    
    #
    # Exit status 0 means the experiment is remapping.
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
	"<a href='$url'>realtime</a>.\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
