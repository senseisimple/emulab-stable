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
$optargs = OptionalPageArguments("index",      PAGEARG_INTEGER,
				 "experiment", PAGEARG_EXPERIMENT,
				 "template",   PAGEARG_TEMPLATE,
				 "instance",   PAGEARG_INSTANCE,
				 "records",    PAGEARG_INTEGER,
				 "tag",        PAGEARG_STRING);

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
	$index = $instance->exptidx();
    $template   = $instance->GetTemplate();
    $urlarg     = "instance";
    $urlval     = $instance->exptidx();
    echo $instance->PageHeader();    
}
elseif (isset($template)) {
    $experiment = $template->GetExperiment();
    $export_url = CreateURL("template_export", $template);
    $urlarg     = "template";
    $urlval     = $template->guid() . "/" . $template->vers();
    echo $template->PageHeader();
}
elseif (isset($index)) {
    $experiment = Experiment::Lookup($index);
    $urlarg     = "index";
    $urlval     = $index;
    echo $experiment->PageHeader();
}
else {
    $export_url = CreateURL("template_export", $experiment);
    $urlarg     = "experiment";
    $urlval     = $experiment->idx();
    echo $experiment->PageHeader();
}

#
# If we got a current experiment, great. Otherwise we have to lookup
# data for a historical experiment.
#
if (isset($experiment) && $experiment) {
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
elseif (isset($index)) {
    $stats = ExperimentStats::Lookup($index);
    if (!$stats) {
	PAGEARGERROR("Invalid experiment index: $index");
    }

    # Need these below.
    $pid = $stats->pid();
    $eid = $stats->eid();
    $exptidx = $index;

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

$url = preg_replace("/archive_view\.php3/", "archive_list.php",
		    $_SERVER['REQUEST_URI']);

# This is how you get forms to align side by side across the page.
$style = 'style="float:left; width:50%;"';

echo "<center>\n";

#
# Buttons to export and to view the tags.
#
echo "<form action='template_export.php' $style method=get>\n";
echo "<b><input type=hidden name=$urlarg value='$urlval'></b>";
if (isset($tag)) {
    echo "<b><input type=hidden name=tag value='$tag'></b>";
}
echo "<b><input type=submit name=submit value='Export'></b>";
echo "</form>";

echo "<form action='archive_tags.php3' $style method=get>";
echo "<b><input type=hidden name=$urlarg value='$urlval'></b>";
echo "<b><input type=submit name=submit value='Show Tags'></b>";
echo "</form>";

echo "<div><iframe src='$url' class='outputframe' ".
	"id='outputframe' name='outputframe'></iframe></div>\n";
echo "</center><br>\n";

echo "<script type='text/javascript' language='javascript'>\n";
echo "SetupOutputArea('outputframe', false);\n"; 
echo "</script>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
