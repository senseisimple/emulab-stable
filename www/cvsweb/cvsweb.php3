<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#

#
# Wrapper script for cvsweb.cgi
#
chdir("../");
require("defs.php3");
include_once("template_defs.php");

unset($uid);
unset($repodir);

#
# We look for anon access, and if so, redirect to ops web server.
# WARNING: See the LOGGEDINORDIE() calls below.
#
if (($this_user = CheckLogin($check_status))) {
     $uid = $this_user->uid();
}

# Tell system we do not want any headers drawn on errors.
$noheaders = 1;

# Use cvsweb or viewvc.
$use_viewvc = 0;

#
# Verify form arguments.
#
$optargs = OptionalPageArguments("template",   PAGEARG_TEMPLATE,
				 "project",    PAGEARG_PROJECT,
				 "embedded",   PAGEARG_BOOLEAN);
if (!isset($embedded)) {
    $embedded = 0;
}

if (isset($project)) {
    if (!$CVSSUPPORT) {
	USERERROR("Project CVS support is not enabled!", 1);
    }
    $pid = $project->pid();
    
    # Redirect now, to avoid phishing.
    if ($this_user) {
	CheckLoginOrDie();
    }
    else {
	$url = $OPSCVSURL . "?cvsroot=$pid";
	
	header("Location: $url");
	return;
    }
    #
    # Authenticated access to the project repo.
    #
    if (! ISADMIN() &&
	! $project->AccessCheck($this_user, $TB_PROJECT_READINFO)) {
        # Then check to see if the project cvs repo is public.
	$query_result =
	    DBQueryFatal("select cvsrepo_public from projects ".
			 "where pid='$pid'");
	if (!mysql_num_rows($query_result)) {
	    TBERROR("Error getting cvsrepo_public bit", 1);
	}
	$row = mysql_fetch_array($query_result);
	if ($row[0] == 0) {
	    USERERROR("You are not a member of Project $pid.", 1);
	}
    }
    $repodir = "$TBCVSREPO_DIR/$pid";
}
elseif (isset($template)) {
    if (!$CVSSUPPORT) {
	USERERROR("Project CVS support is not enabled!", 1);
    }

    # Must be logged in for this!
    $this_user = CheckLoginOrDie();

    $guid = $template->guid();
    $vers = $template->vers();
    $pid  = $template->pid();
    $project = $template->GetProject();

    if (! ISADMIN() &&
	! $project->AccessCheck($this_user, $TB_PROJECT_READINFO)) {
	USERERROR("Not enough permission to view cvs repo for template", 1);
    }
    # Repo for the entire template stored here per-template.
    $repodir = "$TBPROJ_DIR/$pid/templates/$guid/cvsrepo/$guid";
}
else {
    $this_user = CheckLoginOrDie();
    if (! $this_user->cvsweb()) {
        USERERROR("You do not have permission to use cvsweb!", 1);
    }
    unset($pid);
}

$script = "cvsweb.cgi";

#
# Sine PHP helpfully scrubs out environment variables that we _want_, we
# have to pass them to env.....
#
$query = escapeshellcmd($_SERVER["QUERY_STRING"]);
$name = escapeshellcmd($_SERVER["SCRIPT_NAME"]);
$agent = escapeshellcmd($_SERVER["HTTP_USER_AGENT"]);
if (isset($_SERVER["HTTP_ACCEPT_ENCODING"])
    && $_SERVER["HTTP_ACCEPT_ENCODING"] != "") {
    $encoding = escapeshellcmd($_SERVER["HTTP_ACCEPT_ENCODING"]);
}

# This is special ...
if (isset($_SERVER["PATH_INFO"]) && $_SERVER["PATH_INFO"] != "") {
    $path = escapeshellcmd($_SERVER["PATH_INFO"]);
}
else {
    $path = '/';
}

#
# Helpfully enough, escapeshellcmd does not escape spaces. Sigh.
#
$script = preg_replace("/ /","\\ ",$script);
$query = preg_replace("/ /","\\ ",$query);
$name = preg_replace("/ /","\\ ",$name);
$agent = preg_replace("/ /","\\ ",$agent);
if (isset($encoding)) {
    $encoding = preg_replace("/ /","\\ ",$encoding);
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
    while ($line = fgets($fp)) {
        # Suck it all up.
        ;
    }
    pclose($fp);
    exit();
}
set_time_limit(0);
register_shutdown_function("SPEWCLEANUP");

$shellcmd = "env PATH=./cvsweb/ QUERY_STRING=$query PATH_INFO=$path " .
	    "SCRIPT_NAME=$name HTTP_USER_AGENT=$agent ";
if (isset($encoding)) {
    $shellcmd .= "HTTP_ACCEPT_ENCODING=$encoding ";
}

if (isset($repodir)) {
    $prog  = ($use_viewvc ? "webviewvc" : "webcvsweb");
    $embed = ($embedded ? "--embedded" : "");
    
    # I know, I added an argument to a script that is not supposed to
    # take any. So be it; it was easy.
    $shellcmd .= "$TBSUEXEC_PATH $uid $pid $prog $embed --repo $repodir";
}
else {
    $shellcmd .= "$script";
}

#TBERROR(print_r($_SERVER, true), 0);
#TBERROR($shellcmd, 0);

$fp = popen($shellcmd, 'r');

#
# Yuck. Since we can't tell php to shut up and not print headers, we have to
# 'merge' headers from cvsweb with PHP's.
#
while ($line = fgets($fp)) {
    # This indicates the end of headers
    if ($line == "\r\n") { break; }
    header(rtrim($line));
}

#
# Just pass through the rest of cvsweb.cgi's output
#
fpassthru($fp);

fclose($fp);

?>
