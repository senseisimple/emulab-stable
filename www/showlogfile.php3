<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005, 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

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

$reqargs = RequiredPageArguments("experiment", PAGEARG_EXPERIMENT);

# Need these below.
$pid = $experiment->pid();
$eid = $experiment->eid();

if (! $experiment->AccessCheck($this_user, $TB_EXPT_READINFO)) {
    USERERROR("You do not have permission to view the log for $pid/$eid!", 1);
}

#
# Check for a logfile. This file is transient, so it could be gone by
# the time we get to reading it.
#
if (! $experiment->logfile()) {
    USERERROR("Experiment $pid/$eid is no longer in transition!", 1);
}

echo $experiment->PageHeader();
echo "<br /><br />\n";

# This spits out the frame.
STARTLOG($experiment);

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
