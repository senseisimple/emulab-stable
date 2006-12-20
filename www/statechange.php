<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
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
if (!isset($pid) ||
    strcmp($pid, "") == 0) {
    PAGEARGERROR("You must provide a Project ID.");
}

if (!isset($eid) ||
    strcmp($eid, "") == 0) {
    PAGEARGERROR("You must provide an Experiment ID.");
}

if (!isset($state) ||
    strcmp($state, "") == 0) {
    PAGEARGERROR("You must provide the new state.");
}

echo "<div class=contentbody id=statechange>\n";
echo "<br><br><font size=+1>
          Emulab experiment $pid/$eid is now $state.</font><br>\n";
echo "<br><br>\n";
echo "<center><button name=close type=button onClick='window.close();'>";
echo "Close Window</button></center>\n";
echo "</div>\n";
echo "</body></html>";

?>
