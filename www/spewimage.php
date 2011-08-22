<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2003-2011 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("imageid_defs.php");

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
$reqargs = RequiredPageArguments("image",	PAGEARG_IMAGE,
				 "access_key",	PAGEARG_STRING);
$optargs = OptionalPageArguments("stamp",       PAGEARG_INTEGER);

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
set_time_limit(0);
register_shutdown_function("SPEWCLEANUP");

#
# Invoke backend script to do it all.
#
$imageid    = $image->imageid();
$access_key = escapeshellarg($access_key);
$arg        = (isset($stamp) ? "-t " . escapeshellarg($stamp) : "");
$group      = $image->Group();
$pid        = $group->pid();
$unix_gid   = $group->unix_gid();
$project    = $image->Project();
$unix_pid   = $project->unix_gid();

#
# We want to support HEAD requests to avoid send the file.
#
$ishead  = 0;
$headarg = "";
if ($_SERVER['REQUEST_METHOD'] == "HEAD") {
    $ishead  = 1;
    $headarg = "-h";
}

if ($fp = popen("$TBSUEXEC_PATH nobody $unix_pid,$unix_gid ".
		"webspewimage $arg $headarg -k $access_key $imageid", "r")) {
    header("Content-Type: application/octet-stream");
    header("Cache-Control: no-cache, must-revalidate");
    header("Pragma: no-cache");

    #
    # If this is a head request, then the output needs to be sent as
    # headers. Then exit.
    #
    if ($ishead) {
	while (!feof($fp) && connection_status() == 0) {
	    $string = fgets($fp);
	    if ($string) {
		$string = rtrim($string);
		header($string);
	    }
	}
    }
    else {
        #
        # The point of this is to allow the backend script to return status
        # that the file has not been modified and does not need to be sent.
        # The first read will come back with no output, which means nothing is
        # going to be sent except the headers.
        #
	$string = fread($fp, 1024);
	if ($string) {
	    print($string);
	    while (!feof($fp) && connection_status() == 0) {
		print(fread($fp, 1024*32));
		flush();
	    }
	}
    }
    $retval = pclose($fp);
    $fp = 0;

    if ($retval) {
	if ($retval == 2) {
	    SPITERROR(304, "File has not changed");
	}
	else {
	    SPITERROR(404, "Could not verify file: $retval!");
	}
    }
    flush();
}
else {
    SPITERROR(404, "Could not find $file!");
}

?>
