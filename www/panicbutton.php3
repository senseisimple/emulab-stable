<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2011 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

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
$optargs = OptionalPageArguments("canceled",   PAGEARG_BOOLEAN,
				 "confirmed",  PAGEARG_BOOLEAN);

# Need these below.
$pid = $experiment->pid();
$eid = $experiment->eid();
$unix_gid = $experiment->UnixGID();
$project  = $experiment->Project();
$unix_pid = $project->unix_gid();

# Canceled operation redirects back to showexp page. See below.
if (isset($canceled) && $canceled) {
    header("Location: " . CreateURL("showexp", $experiment));
    return;
}

#
# Standard Testbed Header, after checking for cancel above.
#
PAGEHEADER("Press the Panic Button!");

#
# Verify permissions.
#
if (!$experiment->AccessCheck($this_user, $TB_EXPT_MODIFY)) {
    USERERROR("You do not have permission to press the panic button for ".
	      "experiment $eid!", 1);
}

echo $experiment->PageHeader();
echo "<br>\n";
    
#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case redirect the
# browser back up a level.
#
if (!isset($confirmed)) {
    echo "<center><h3><br>
          Are you <b>REALLY</b>
          sure you want to press the panic button for Experiment '$eid?'
          </h3>\n";

    $experiment->Show(1);

    $url = CreateURL("panicbutton", $experiment);
    
    echo "<form action='$url' method=post>";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

#
# We run a wrapper script that does all the work.
#
STARTBUSY("Pressing the panic button");
$retval = SUEXEC($uid, "$unix_pid,$unix_gid", "webpanic $pid $eid",
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
STOPBUSY();

#
# Exit status >0 means the operation could not proceed.
# Exit status =0 means the experiment is terminating in the background.
#
echo "<br>\n";
if ($retval) {
    echo "<h3>Panic Button failure</h3>";
    echo "<blockquote><pre>$suexec_output<pre></blockquote>";
}
else {
    echo "<h3>The panic button has been pressed!</h3><br>
              You will need to contact testbed operations to continue.\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
