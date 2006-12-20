<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
chdir("..");
include("defs.php3");

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
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Check to make sure a valid nodeid.
#
if (isset($node_id) && strcmp($node_id, "")) {
    if (!TBvalid_node_id($node_id)) {
	USERERROR("Illegal characters in node ID.", 1);
    }

    if (!isset($x) || !isset($y) || !isset($o)) {
	USERERROR("Must specify x,y,o coordinates!", 1);
    }

    # Create one entry array for code below.
    $nodeidlist = array();
    $nodeidlist[$node_id] = "$x, $y, $o";
}
elseif (!isset($nodeidlist)) {
    #
    # We expect an array of node ids otherwise. 
    # 
    USERERROR("Must specify a nodeid or a nodeidlist!", 1);
}

#TBERROR(print_r($nodeidlist, TRUE) . "\n\n", 0);

#
# Go through the list and check the args. Bail if any are bad.
# 
while (list ($node_id, $value) = each ($nodeidlist)) {
    if (!TBvalid_node_id($node_id)) {
	USERERROR("Illegal characters in node ID.", 1);
    }
    if (! TBValidNodeName($node_id)) {
	USERERROR("$node_id is not a valid node name!", 1);
    }
    if (! TBNodeAccessCheck($uid, $node_id, $TB_NODEACCESS_MODIFYINFO)) {
        USERERROR("You do not have permission to move $node_id!", 1);
    }

    #
    # Split up the argument into x,y,o and check them.
    #
    unset($matches);
    
    if (!preg_match("/^\"(.*),(.*),(.*)\"$/", $value, $matches)) {
	USERERROR("Must specify x,y,o coordinates for $node_id!", 1);
    }
    $x = $matches[1];
    $y = $matches[2];
    $o = $matches[3];

    if (! (TBvalid_float($x) && TBvalid_float($y) && TBvalid_float($o))) {
	USERERROR("Must specify proper x,y,o coordinates for $node_id!", 1);
    }
}

#
# Okay, now do it for real ...
#
reset($nodeidlist);

while (list ($node_id, $value) = each ($nodeidlist)) {
    #
    # Split up the argument into x,y,o 
    #
    unset($matches);
    
    if (!preg_match("/^\"(.*),(.*),(.*)\"$/", $value, $matches)) {
	USERERROR("Must specify x,y,o coordinates for $node_id!", 1);
    }
    $x = $matches[1];
    $y = $matches[2];
    $o = $matches[3];

    if (! TBNodeIDtoExpt($node_id, $pid, $eid, $gid)) {
	USERERROR("$node_id is not reserved to an experiment!", 1);
    }

    $retval = SUEXEC($uid, "$pid,$gid",
		     "websetdest -d -x $x -y $y -o $o $node_id",
		     SUEXEC_ACTION_IGNORE);

#    SUEXECERROR(SUEXEC_ACTION_CONTINUE);

    #
    # Report fatal errors.
    # 
    if ($retval) {
	USERERROR($suexec_output, 1);
    }
}

#
# Standard testbed footer
#
if (!isset($fromapplet)) {
    PAGEFOOTER();
}
?>
