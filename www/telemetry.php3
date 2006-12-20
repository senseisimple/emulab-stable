<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005, 2006 University of Utah and the Flux Group.
# All rights reserved.
#

include("defs.php3");
include("showstuff.php3");

#
# Make sure they are logged in
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
if (!isset($node) || strcmp($node, "") == 0) {
    USERERROR("You must provide a node ID.", 1);
}

if (!TBvalid_node_id($node)) {
    USERERROR("Invalid node ID.", 1);
}

if (!TBValidNodeName($node)) {
    USERERROR("Invalid node ID.", 1);
}

if (!TBNodeAccessCheck($uid, $node, $TB_NODEACCESS_READINFO)) {
    USERERROR("Not enough permission.", 1);
}

#
# Define a stripped-down view of the web interface - less clutter
#
$view = array(
    'hide_banner' => 1,
    'hide_sidebar' => 1,
    'hide_copyright' => 1
);

#
# Standard Testbed Header now that we have the pid/eid okay.
#
PAGEHEADER("Telemetry for $node", $view);

if (TBNodeStatus($node) == "up") {
    echo "
    <applet code='thinlet.AppletLauncher.class'
            archive='thinlet.jar,oncrpc.jar,mtp.jar,garcia-telemetry.jar'
            width='300' height='400'
            alt='You need java to run this applet'>
        <param name='class' value='GarciaTelemetry'>
        <param name='pipeurl' value='servicepipe.php3?node=$node'>
        <param name='uid' value='$uid'>
        <param name='auth' value='$HTTP_COOKIE_VARS[$TBAUTHCOOKIE]'>
    </applet>\n";
}
else {
    USERERROR("Robot is not alive. ". TBNodeStatus($node), 1);
}

#
# Standard Testbed Footer
# 
PAGEFOOTER($view);

?>
