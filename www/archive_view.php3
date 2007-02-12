<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("template_defs.php");

#
# Only known and logged in users can look at experiments.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("experiment", PAGEARG_EXPERIMENT,
				 "template",   PAGEARG_TEMPLATE,
				 "instance",   PAGEARG_INSTANCE,
				 "exptidx",    PAGEARG_INTEGER,
				 "records",    PAGEARG_INTEGER);

#
# Standard Testbed Header now that we know what to say.
#
PAGEHEADER((isset($instance) || isset($template) ?
	    "Template Datastore" : "Experiment Archive"));

#
# An instance might be a current or historical. If its a template, use
# the underlying experiment of the template.
#
if (isset($instance)) {
    if (($foo = $instance->GetExperiment()))
	$experiment = $foo;
    else
	$exptidx = $instance->exptidx();
    $template = $instance->GetTemplate();
}
elseif (isset($exptidx)) {
    #
    # Just in case we get here via a current experiment link.
    #
    if (($foo = Experiment::Lookup($exptidx))) {
	$experiment = $foo;
    }
}
elseif (isset($template)) {
    $experiment = $template->GetExperiment();
}

#
# If we got a current experiment, great. Otherwise we have to lookup
# data for a historical experiment.
#
if (isset($experiment)) {
    # Need these below.
    $pid = $experiment->pid();
    $eid = $experiment->eid();
    $exptidx = $experiment->idx();

    # Permission
    if (!$isadmin &&
	!$experiment->AccessCheck($this_user, $TB_EXPT_READINFO)) {
	USERERROR("You do not have permission to view tags for ".
		  "archive in $pid/$eid!", 1);
    }
}
elseif (isset($exptidx)) {
    $stats = ExperimentStats::Lookup($exptidx);
    if (!$stats) {
	PAGEARGERROR("Invalid experiment index: $exptidx");
    }

    # Need these below.
    $pid = $stats->pid();
    $eid = $stats->eid();

    # Permission
    if (!$isadmin &&
	!$stats->AccessCheck($this_user, $TB_PROJECT_READINFO)) {
	USERERROR("You do not have permission to view tags for ".
		  "archive in $pid/$eid!", 1);
    }
}
else {
    PAGEARGERROR("Must provide a current or former experiment index");
}

$url = preg_replace("/archive_view/", "cvsweb/cvsweb",
		    $_SERVER['REQUEST_URI']);

# This is how you get forms to align side by side across the page.
if (isset($experiment)) 
    $style = 'style="float:left; width:50%;"';
else
    $style = "";

echo "<center>\n";
echo "<font size=+1>";

if (isset($template)) {
    $path = null;
    
    if (isset($instance)) {
	$id = $instance->exptidx();
	if (isset($experiment))
	    $path = $experiment->path();

	echo "Subversion datastore for Instance $id";
    }
    else {
	$guid = $template->guid();
	$vers = $template->vers();
	$path = $template->path();

	echo "Subversion datastore for Template $guid/$vers";
    }

    if ($path) {
	echo "<br>(located at $USERNODE:$path)\n";
    }
}
elseif ($experiment) {
    echo "Subversion archive for experiment $pid/$eid";
}
else {
    echo "Subversion archive for experiment $exptidx.";
}

echo "<br><br></font>";

#
# Only current experiments can be tagged.
#
if (isset($experiment)) {
    echo "<form action='${TBBASE}/archive_tag.php3' $style method=get>\n";
    echo "<b><input type=hidden name=experiment value='$exptidx'></b>";
    echo "<b><input type=submit name=tag value='Tag Archive'></b>";
    echo "</form>";
}

echo "<form action='${TBBASE}/archive_tags.php3' $style method=get>";
echo "<b><input type=hidden name=experiment value='$exptidx'></b>";
echo "<b><input type=submit name=tag value='Show Tags'></b>";
echo "</form>";

echo "<iframe width=100% height=800 scrolling=yes src='$url' border=2>".
     "</iframe>\n";
echo "</center><br>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
