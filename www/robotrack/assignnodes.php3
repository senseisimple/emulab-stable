<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
chdir("..");
include("defs.php3");

#
# Only known and logged in users can watch LEDs
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# When called from the applet, the variable "fromapplet" will be set.
# In that case, spit back simple text based errors that can go into
# a dialog box.
#
if (isset($fromapplet)) {
    $session_interactive  = 0;
    $session_errorhandler = 'handle_error';
}
else {
    PAGEHEADER("Set Robot Destination");
}

#
# Capture script errors in non-interactive case.
#
function handle_error($message, $death)
{
    header("Content-Type: text/plain");
    echo "$message";
    if ($death)
	exit(1);
}

#
# Verify page arguments. Allow user to optionally specify building/floor.
#
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

if (!preg_match("/^[-\w]+$/", $pid)) {
    PAGEARGERROR("Invalid pid argument.");
}
if (!preg_match("/^[-\w]+$/", $eid)) {
    PAGEARGERROR("Invalid eid argument.");
}

#
# Check to make sure this is a valid PID/EID tuple.
#
if (! TBValidExperiment($pid, $eid)) {
  USERERROR("Experiment $pid/$eid is not a valid experiment.", 1);
}

#
# Verify Permission.
#
if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_READINFO)) {
    USERERROR("You do not have permission to access experiment $pid/$eid!", 1);
}

if (!isset($nodelist)) {
    #
    # We expect an array of node ids otherwise. 
    # 
    USERERROR("Must specify a nodelist!", 1);
}

#
# Go through the list and check the args. Bail if any are bad.
# 
while (list ($vname, $pname) = each ($nodelist)) {
    if (!TBvalid_node_id($pname)) {
	USERERROR("Illegal characters in node ID.", 1);
    }
    if (!TBvalid_node_id($vname)) {
	USERERROR("Illegal characters in vname.", 1);
    }
    if (!TBValidNodeName($pname)) {
	USERERROR("$node_id is not a valid node name!", 1);
    }
}

#
# Okay, now do it for real ...
#
reset($nodelist);

while (list ($vname, $pname) = each ($nodelist)) {
    DBQueryFatal("update virt_nodes set fixed='$pname' ".
		 "where pid='$pid' and eid='$eid' and vname='$vname'");
}

#
# Standard testbed footer
#
if (!isset($fromapplet)) {
    PAGEFOOTER();
}

?>
