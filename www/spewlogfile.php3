<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("logfile", PAGEARG_LOGFILE);

if (! isset($logfile)) {
    PAGEARGERROR("Must provide either a logfile ID");
}

# Check permission in the backend. The user is logged in, so its safe enough
# to pass it through.
$logfileid = $logfile->logid();

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
ignore_user_abort(1);
register_shutdown_function("SPEWCLEANUP");

if ($fp = popen("$TBSUEXEC_PATH $uid nobody ".
		"spewlogfile -w -i " . escapeshellarg($logfileid), "r")) {
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
    USERERROR("Logfile $logfileid is no longer valid!", 1);
}

?>
