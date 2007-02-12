<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

$currentusage  = 0;

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

PAGEBEGINNING("Experiment State Change", 0, 1);

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("experiment", PAGEARG_EXPERIMENT,
				 "state",      PAGEARG_STRING);

# Need these below.
$pid = $experiment->pid();
$eid = $experiment->eid();

echo "<div class=contentbody id=statechange>\n";
echo "<br><br><font size=+1>
          Emulab experiment $pid/$eid is now $state.</font><br>\n";
echo "<br><br>\n";
echo "<center><button name=close type=button onClick='window.close();'>";
echo "Close Window</button></center>\n";
echo "</div>\n";
echo "</body></html>";

?>
