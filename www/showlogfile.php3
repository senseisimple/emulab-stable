<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005, 2006 University of Utah and the Flux Group.
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

# This will not return if its a sajax request.
include("showlogfile_sup.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Experiment Activity Log");

# Otherwise, check page arguments.
CHECKPAGEARGS($pid, $eid);

#
# Check for a logfile. This file is transient, so it could be gone by
# the time we get to reading it.
#
if (! TBExptLogFile($pid, $eid)) {
    USERERROR("Experiment $pid/$eid is no longer in transition!", 1);
}

echo "<font size=+2>Experiment <b>".
     "<a href='showproject.php3?pid=$pid'>$pid</a>/".
     "<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a></b></font>\n";
echo "<br /><br />\n";

# This spits out the frame.
STARTLOG($pid, $eid);

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
