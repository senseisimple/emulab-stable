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
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("experiment", PAGEARG_EXPERIMENT,
				 "template",   PAGEARG_TEMPLATE,
				 "logfile",    PAGEARG_LOGFILE);

if (! (isset($experiment) || isset($template) || isset($logfile))) {
    PAGEARGERROR("Must provide either an experiment, template or ID");
}

#
# Verify permission and sure there is a logfile.
#
if (isset($experiment)) {
    $pid = $experiment->pid();
    $eid = $experiment->eid();
    
    if (!$experiment->AccessCheck($this_user, $TB_EXPT_READINFO)) {
	USERERROR("You do not have permission to view logs for $pid/$eid!", 1);
    }
    if (! $experiment->logfile()) {
	USERERROR("Experiment $pid/$eid is no longer in transition!", 1);
    }
}
elseif (isset($template)) {
    $pid  = $template->pid();
    $guid = $template->guid();
    $vers = $template->vers();

    if (!$template->AccessCheck($this_user, $TB_EXPT_READINFO)) {
	USERERROR("You do not have permission to view logs for ".
		  "$guid/$vers!", 1);
    }
    if (! $template->logfile()) {
	USERERROR("Template $guid/$vers is no longer in transition!", 1);
    }
}
else {
    # Permission is granted just by knowing the ID.
    $logfileid = $logfile->logid();
    # Not sure what to do about this yet ...
    $pid  = "nobody";
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
ignore_user_abort(1);
register_shutdown_function("SPEWCLEANUP");

if (isset($experiment)) {
    $args = "-e " . $experiment->pid() . "/" . $experiment->eid();
}
elseif (isset($template)) {
    $args = "-t " . $template->guid() . "/" . $template->vers();
}
else {
    $args = "-i " . escapeshellarg($logfile->logid());
}

if ($fp = popen("$TBSUEXEC_PATH $uid $pid spewlogfile -w $args", "r")) {
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
    if (isset($experiment))
	USERERROR("Experiment $pid/$eid is no longer in transition!", 1);
    elseif (isset($template))
	USERERROR("Template $guid/$vers is no longer in transition!", 1);
    else {
	USERERROR("Logfile $logfileid is no longer valid!", 1);
    }
}

?>
