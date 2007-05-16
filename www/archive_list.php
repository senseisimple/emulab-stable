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
$optargs = OptionalPageArguments("index",      PAGEARG_INTEGER,
				 "experiment", PAGEARG_EXPERIMENT,
				 "instance",   PAGEARG_INSTANCE,
				 "template",   PAGEARG_TEMPLATE,
				 "file",       PAGEARG_INTEGER,
				 "tag",        PAGEARG_STRING);

if (! (isset($experiment) || isset($instance) ||
       isset($template)   || isset($index))) {
    PAGEARGERROR("Must provide one of experiment, instance or template");
}

#
# An instance might be a current or historical. If its a template, use
# the underlying experiment of the template.
#
if (isset($instance)) {
    if (($foo = $instance->GetExperiment()))
	$experiment = $foo;
    else
	$index = $instance->exptidx();
}
elseif (isset($template)) {
    $experiment = $template->GetExperiment();
}
elseif (isset($index)) {
    $experiment = Experiment::Lookup($index);
}

#
# If we got a current experiment, great. Otherwise we have to lookup
# data for a historical experiment.
#
if (isset($experiment) && $experiment) {
    # Need these below.
    $pid = $experiment->pid();
    $gid = $experiment->gid();
    $eid = $experiment->eid();
    $idx = $experiment->idx();

    $stats = $experiment->GetStats();
    if (!$stats) {
	TBERROR("Could not load stats object for experiment $pid/$eid", 1);
    }
    $archive_idx = $stats->archive_idx();
    
    # Permission
    if (!$isadmin &&
	!$experiment->AccessCheck($this_user, $TB_EXPT_READINFO)) {
	USERERROR("You do not have permission to view this archive!", 1);
    }
}
elseif (isset($index)) {
    $stats = ExperimentStats::Lookup($index);
    if (!$stats) {
	PAGEARGERROR("Invalid experiment index: $index");
    }

    # Need these below.
    $pid = $stats->pid();
    $eid = $stats->eid();
    $gid = $stats->gid();
    $idx = $index;
    $archive_idx = $stats->archive_idx();

    # Permission
    if (!$isadmin &&
	!$stats->AccessCheck($this_user, $TB_PROJECT_READINFO)) {
	USERERROR("You do not have permission to view tags for ".
		  "archive in $pid/$eid!", 1);
    }
}

# This gets scrubbed ...
$query = escapeshellcmd($_SERVER["QUERY_STRING"]);
    
#
# A cleanup function to keep the child from becoming a zombie.
#
$fp = 0;

function SPEWCLEANUP()
{
    global $fp;

    if (connection_aborted() && $fp) {
	pclose($fp);
    }
    exit();
}
register_shutdown_function("SPEWCLEANUP");
ignore_user_abort(1);

# Pass the tag through.
$options  = (isset($tag) ? "-t " . escapeshellarg($tag) : "");
$options .= " -q $query ";
$options .= (isset($file) ? " -i " . escapeshellarg($file) : "");

$fp = popen("$TBSUEXEC_PATH $uid ".
	    "   $pid,$gid webarchive_list $options $archive_idx $idx",
	    "r");

if (! $fp) {
    USERERROR("Archive listing failed!", 1);
}

#
# Yuck. Since we cannot tell php to shut up and not print headers, we have to
# 'merge' headers from the backend with PHPs.
#
while ($line = fgets($fp)) {
    # This indicates the end of headers
    if ($line == "\n") { break; }
    header(rtrim($line));
}
flush();
fpassthru($fp);
$fp = 0;

?>
