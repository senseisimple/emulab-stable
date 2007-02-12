<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2003-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("node_defs.php");

function SPITERROR($code, $msg)
{
    header("HTTP/1.0 $code $msg");
    exit();
}

#
# Capture script errors and report back to user.
#
$session_interactive  = 0;
$session_errorhandler = 'handle_error';

function handle_error($message, $death)
{
    SPITERROR(400, $message);
}

#
# Must be SSL, even though we do not require an account login.
#
if (!isset($SSL_PROTOCOL)) {
    SPITERROR(400, "Must use https:// to access this page!");
}

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("node",    PAGEARG_NODE,
				 "key",     PAGEARG_STRING);
$optargs = OptionalPageArguments("elabinelab_source", PAGEARG_STRING,
				 "file",              PAGEARG_STRING,
				 "stamp",             PAGEARG_INTEGER,
				 "md5",               PAGEARG_STRING,
				 "cvstag",            PAGEARG_STRING);
$node_id = $node->node_id();

#
# A variant allows us to pass the Emulab source code back to an ElabInElab
# experiment. 
#
if (!isset($elabinelab_source)) {
    if (!isset($file) ||
	strcmp($file, "") == 0) {
	SPITERROR(400, "You must provide an filename.");
    }
    if (!isset($stamp) || !strcmp($stamp, "")) {
	unset($stamp);
    }
    # We ignore MD5 for now. 
    if (!isset($md5) || !strcmp($md5, "")) {
	unset($md5);
    }
}

#
# Make sure a reserved node.
#
if (! ($experiment = $node->Reservation())) {
    SPITERROR(400, "$node_id is not reserved to an experiment!");
}
if (! ($creator = $experiment->GetCreator())) {
    SPITERROR(400, "Could not map experiment creator to object!");
}
$pid = $experiment->pid();
$eid = $experiment->eid();
$unix_gid = $experiment->UnixGID();
$creator_uid = $creator->uid();

#
# We need the secret key to match
#
if (!$experiment->keyhash() || $experiment->keyhash() == "") {
    SPITERROR(403, "No key defined for this experiment!");
}
if ($experiment->keyhash() != $key) {
    SPITERROR(403, "Wrong Key!");
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

#
# Special case. If requesting elab source code, the experiment must
# be an elabinelab experiment. 
#
if (isset($elabinelab_source)) {
    #
    # Make sure the IP really matches. 
    #
    if (! ($realnode = Node::LookupByIP($REMOTE_ADDR))) {
	SPITERROR(403, "Could not map $REMOTE_ADDR to node object!");
    }
    if ($realnode->node_id() != $node_id) {
	SPITERROR(403, "Not an elabinelab experiment!");
    }
    
    #
    # Must be an elabinelab experiment of course.
    #
    if (! $experiment->elabinelab()) {
	SPITERROR(403, "Not an elabinelab experiment!");
    }

    #
    # If a specific tag is requested, call out to the spewsource program.
    # Otherwise send it the usual file.
    #
    if (isset($cvstag)) {
	if (! preg_match("/^[-\w\@\/\.]+$/", $cvstag)) {
	    SPITERROR(400, "Invalid characters in cvstag!");
	}

	# Do it anyway.
	$cvstag = escapeshellarg($cvstag);

	if ($fp = popen("$TBSUEXEC_PATH $creator_uid $pid,$unix_gid ".
			"spewsource -t $cvstag", "r")) {
	    header("Content-Type: application/x-gzip");
	    header("Expires: Mon, 26 Jul 1997 05:00:00 GMT");
	    header("Cache-Control: no-cache, must-revalidate");
	    header("Pragma: no-cache");

	    flush();
	    fpassthru($fp);
	    $fp = 0;
	    flush();
	    return;
	}
	else {
	    SPITERROR(404, "Could not find $file!");
	}
    }
    else {
	if (!is_readable("/usr/testbed/src/emulab-src.tar.gz")) {
	    SPITERROR(404, "Could not find $file!");
	}
	header("Content-Type: application/octet-stream");
	header("Expires: Mon, 26 Jul 1997 05:00:00 GMT");
	header("Cache-Control: no-cache, must-revalidate");
	header("Pragma: no-cache");
	flush();
	readfile("/usr/testbed/src/emulab-src.tar.gz");
	exit(0);
    }
}

#
# MUST DO THIS!
#
$file   = escapeshellarg($file);
$arg    = (isset($stamp) ? "-t " . escapeshellarg($stamp) : "");

#
# Run once with just the verify option to see if the file exists.
# Then do it for real, spitting out the data. Sure, the user could
# delete the file in the meantime, but thats his problem. 
#
$retval = SUEXEC($creator_uid, "$pid,$unix_gid",
		 "spewrpmtar -v $arg $node_id $file",
		 SUEXEC_ACTION_IGNORE);

if ($retval < 0) {
    SUEXECERROR(SUEXEC_ACTION_CONTINUE);
    SPITERROR(500, "Could not verify file!");
}

#
# An expected error.
# 
if ($retval) {
    if ($retval == 2) {
	SPITERROR(304, "File has not changed");
    }
    SPITERROR(404, "Could not verify file: $retval!");
}

#
# Okay, now do it for real. 
# 
if ($fp = popen("$TBSUEXEC_PATH $creator_uid $pid,$unix_gid ".
		"spewrpmtar $node_id $file", "r")) {
    header("Content-Type: application/octet-stream");
    header("Expires: Mon, 26 Jul 1997 05:00:00 GMT");
    header("Cache-Control: no-cache, must-revalidate");
    header("Pragma: no-cache");

    fpassthru($fp);
    $fp = 0;
    flush();
}
else {
    SPITERROR(404, "Could not find $file!");
}

?>
