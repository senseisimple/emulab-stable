<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
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
$reqargs  = RequiredPageArguments("experiment", PAGEARG_EXPERIMENT);
$optargs  = OptionalPageArguments("action", PAGEARG_STRING);

if (!isset($action) || $action == "tag" || $action == "commit") {
    $action    = "commit";
}

#
# Verify Permission.
#
if (! $experiment->AccessCheck($this_user, $TB_EXPT_MODIFY)) {
    USERERROR("You do not have permission to view experiment $eid!", 1);
}

# Group to suexc as.
$pid = $experiment->pid();
$gid = $experiment->gid();

if ($action == "commit") {
    SUEXEC($uid, "$pid,$gid",
	   "webarchive_control commit $pid $eid",
	   SUEXEC_ACTION_DIE);

    $newurl = preg_replace("/archive_control/",
			   "archive_view", $_SERVER['REQUEST_URI']);

    header("Location: $newurl");
    exit(0);
}

?>
