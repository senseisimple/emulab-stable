<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("template_defs.php");

#
# Only known and logged in users can end experiments.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

# This will not return if its a sajax request.
include("showlogfile_sup.php3");

#
# Verify Page Arguments.
#
$reqargs = RequiredPageArguments("experiment", PAGEARG_EXPERIMENT);
$optargs = OptionalPageArguments("canceled",   PAGEARG_STRING,
				 "confirmed",  PAGEARG_STRING);
				 
# Canceled operation redirects back to showexp page. See below.
if (isset($canceled) && $canceled) {
    header("Location: ". CreateURL("showexp", $experiment));
    return;
}

#
# Standard Testbed Header, after checking for cancel above.
#
PAGEHEADER("Terminate Experiment");

# Need these below.
$lockdown = $experiment->lockdown();
$exptidx  = $experiment->idx();
$pid      = $experiment->pid();
$eid      = $experiment->eid();

#
# Verify permissions.
#
if (! $experiment->AccessCheck($this_user, $TB_EXPT_DESTROY)) {
    USERERROR("You do not have permission to end experiment $pid/$eid!", 1);
}

# Template Instance Experiments get special treatment in this page.
$instance = TemplateInstance::LookupByExptidx($exptidx);
if ($instance && ($experiment->state() != $TB_EXPTSTATE_SWAPPED)) {
    PAGEARGERROR("Invalid action for template instance");
}

# Spit experiment pid/eid at top of page.
echo $experiment->PageHeader();
echo "<br>\n";

# A locked down experiment means just that!
if ($lockdown) {
    echo "<br><br>\n";
    USERERROR("Cannot proceed; the experiment is locked down!", 1);
}
   
#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case redirect the
# browser back up a level.
#
if (!isset($confirmed)) {
    echo "<center><br><font size=+2>Are you <b>REALLY</b> sure ";
    echo "you want to terminate " . ($instance ? "Instance " : "Experiment ");
    echo "'$eid?' </font>\n";
    echo "<br>(This will <b>completely</b> destroy all trace)<br><br>\n";

    $experiment->Show(1);

    $url = CreateURL("endexp", $experiment);
    
    echo "<form action='$url' method=post>";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}
flush();

#
# We need the unix gid for the project for running the scripts below.
#
$unix_gid = $experiment->UnixGID();

if ($instance) {
    $guid = $instance->guid();
    $vers = $instance->vers();
    
    $command = "webtemplate_swapout -e $eid $guid/$vers";
}
else {
    $command = "webendexp $pid $eid";
}

STARTBUSY("Terminating " . ($instance ? "Instance." : "Experiment."));

#
# Run the backend script.
#
$retval = SUEXEC($uid, "$pid,$unix_gid", $command, SUEXEC_ACTION_IGNORE);

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
# Exit status >0 means the operation could not proceed.
# Exit status =0 means the experiment is terminating in the background.
#
echo "<br>\n";
if ($retval) {
    echo "<h3>Experiment termination could not proceed</h3>";
    echo "<blockquote><pre>$suexec_output<pre></blockquote>";
}
else {
    echo "<b>Your experiment is terminating!</b> 
          You will be notified via email when the experiment has been torn
	  down, and you can reuse the experiment name.
          This typically takes less than two minutes, depending on the
          number of nodes in the experiment.
          If you do not receive email notification within a reasonable amount
          of time, please contact $TBMAILADDR.\n";
    echo "<br><br>\n";
    STARTLOG($experiment);
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
