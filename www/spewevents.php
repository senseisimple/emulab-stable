<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006-2011 University of Utah and the Flux Group.
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

# Need these below.
$pid = $experiment->pid();
$eid = $experiment->eid();
$project  = $experiment->Project();
$unix_pid = $project->unix_gid();

#
# Verify permission.
#
if (!$experiment->AccessCheck($this_user, $TB_EXPT_READINFO)) {
    USERERROR("You do not have permission to view events for $pid/$eid!", 1);
}

#
# A cleanup function to keep the child from becoming a zombie, since
# the script is terminated, but the children are left to roam.
#
$fp = 0;

function SPEWCLEANUP()
{
    global $fp;

    if (!$fp || !connection_aborted()) {
	exit();
    }
    pclose($fp);
    exit();
}
register_shutdown_function("SPEWCLEANUP");

if ($fp = popen("$TBSUEXEC_PATH $uid $unix_pid webspewevents -w $pid $eid", "r")) {
    header("Content-Type: text/plain");
    header("Expires: Mon, 26 Jul 1997 05:00:00 GMT");
    header("Cache-Control: no-cache, must-revalidate");
    header("Pragma: no-cache");
    flush();

    while (!feof($fp)) {
	$string = fgets($fp, 1024);
	echo "$string";
	flush();
    }
    pclose($fp);
    $fp = 0;
}
else {
    USERERROR("Experiment $pid/$eid is no longer in transition!", 1);
}

?>
