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
$optargs = OptionalPageArguments("experiment", PAGEARG_EXPERIMENT,
				 "instance",   PAGEARG_INSTANCE,
				 "template",   PAGEARG_TEMPLATE,
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
    if (isset($experiment)) {
	#
	# Wants access to the experiment archive, which is really a repo.
	#
	$pid = $experiment->pid();
	$eid = $experiment->eid();
	
	if (! ISADMIN() &&
	    ! $experiment->AccessCheck($this_user, $TB_EXPT_READINFO)) {
	    USERERROR("Not enough permission to view '$pid/$eid'", 1);
	}
	# Get the repo index for the experiment.
	$query_result =
	    DBQueryFatal("select s.archive_idx from experiments as e ".
			 "left join experiment_stats as s on s.exptidx=e.idx ".
			 "where e.pid='$pid' and e.eid='$eid'");
	
	if (!mysql_num_rows($query_result)) {
	    TBERROR("Error getting repo index for '$pid/$eid'", 1);
	}
	$row = mysql_fetch_array($query_result);
	if (!isset($row[0])) {
	    TBERROR("Error getting repo index for '$pid/$eid'", 1);
	}
	$repoidx = $row[0];
	$repodir = "/usr/testbed/exparchive/$repoidx/repo/";
	$use_viewvc = 1;
    }
    else {
	#
	# Wants access to the project repo.
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
}
elseif (isset($experiment) || isset($instance) || isset($template)) {
    if (!$CVSSUPPORT) {
	USERERROR("Project CVS support is not enabled!", 1);
    }

    # Must be logged in for this!
    if ($this_user) {
	CheckLoginOrDie();
    }

    if (isset($template)) {
	$experiment = $template->GetExperiment();
    }
    
    if (isset($instance)) {
	$pid = $instance->pid();
	$eid = $instance->eid();
	$idx = $instance->exptidx();
	$project = $instance->Project();
    }
    else {
	$pid = $experiment->pid();
	$eid = $experiment->eid();
	$idx = $experiment->idx();
	$project = $experiment->Project();
    }
    
    # Need the pid/eid/gid. Access the stats table since we want to provide
    # cvs access to terminated experiments via the archive.
    $query_result =
	DBQueryFatal("select s.archive_idx,a.archived ".
		     "   from experiment_stats as s ".
		     "left join archives as a on a.idx=s.archive_idx ".
		     "where s.exptidx='$idx'");
    if (!mysql_num_rows($query_result)) {
	USERERROR("Experiment '$idx' is not a valid experiment", 1);
    }
    $row = mysql_fetch_array($query_result);
    $repoidx  = $row[0];
    $archived = $row[1];

    # Lets do group level check since it might not be a current experiment.
    if (!$archived) {
	if (! ISADMIN() &&
	    ! $project->AccessCheck($this_user, $TB_PROJECT_READINFO)) {
	    USERERROR("Not enough permission to view archive", 1);
	}
	$repodir = "/usr/testbed/exparchive/$repoidx/repo/";
    }
    else {
	if (! ISADMIN()) {
	    USERERROR("Must be administrator to view historical archives!", 1);
	}
	$repodir = "/usr/testbed/exparchive/Archive/$repoidx/repo/";
    }
    $use_viewvc = 1;
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
$encoding = escapeshellcmd($_SERVER["HTTP_ACCEPT_ENCODING"]);

# This is special ...
if (isset($_SERVER["PATH_INFO"])) {
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
$encoding = preg_replace("/ /","\\ ",$encoding);

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
            "SCRIPT_NAME=$name HTTP_USER_AGENT=$agent " .
            "HTTP_ACCEPT_ENCODING=$encoding ";

if (isset($repodir)) {
    $prog  = ($use_viewvc ? "webviewvc" : "webcvsweb");
    $embed = ($embedded ? "--embedded" : "");
    
    # I know, I added an argument to a script that is not supposed to
    # take any. So be it; it was easy.
    $shellcmd .= "$TBSUEXEC_PATH $uid $pid $prog $embed --repo=$repodir";
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
