<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users can end experiments.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("experiment", PAGEARG_EXPERIMENT);
$optargs = OptionalPageArguments("canceled",   PAGEARG_STRING,
				 "confirmed",  PAGEARG_STRING);

# Canceled operation redirects back to showexp page. See below.
if (isset($canceled) && $canceled) {
    header("Location: " . CreateURL("showexp", $experiment));
    return;
}

#
# Standard Testbed Header, after cancel above.
#
PAGEHEADER("Replay Control");

#
# Need these below
#
$pid = $experiment->pid();
$eid = $experiment->eid();
$unix_gid = $experiment->UnixGID();

#
# Verify permissions.
#
if (!$experiment->AccessCheck($this_user, $TB_EXPT_MODIFY)) {
    USERERROR("You do not have permission for $eid!", 1);
}

echo $experiment->PageHeader();
echo "<br>\n";

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we 
# redirect the browser back to the experiment page (see above).
#
if (!isset($confirmed)) {
    echo "<center><h2><br>
          Are you sure you want to replay all events in experiment '$eid?'
          </h2>\n";
    
    $experiment->Show(1);

    $url = CreateURL("replayexp", $experiment);
    
    echo "<form action='$url' method=post>";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";

    PAGEFOOTER();
    return;
}

#
# Avoid SIGPROF in child.
# 
set_time_limit(0);

STARTBUSY("Starting event replay");
$retval = SUEXEC($uid, "$pid,$unix_gid",
		 "webeventsys_control replay $pid,$eid",
		 SUEXEC_ACTION_DIE);
STOPBUSY();

echo "Events for your experiment are now being replayed.\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
