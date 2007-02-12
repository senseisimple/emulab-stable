<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("template_defs.php");
require("Sajax.php");
sajax_init();
sajax_export("GraphShow");

#
# Only known and logged in users ...
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("instance",  PAGEARG_INSTANCE,
				 "runidx",    PAGEARG_INTEGER);

$template = $instance->GetTemplate();
# Need these below.
$guid = $template->guid();
$vers = $template->vers();
$pid  = $template->pid();
$eid  = $instance->eid();

if (! $template->AccessCheck($this_user, $TB_EXPT_READINFO)) {
    USERERROR("You do not have permission to view experiment template ".
	      "$guid/$version!", 1);
}
if (! $instance->ValidRun($runidx)) {
    USERERROR("The run $runidx is not a valid experiment run!", 1);
}

#
# For the Sajax Interface
#
function GraphShow($which, $arg0, $arg1)
{
    global $template, $instance, $runidx;

    ob_start();
    $instance->ShowGraphArea($which, $runidx, $arg0, $arg1);
    $html = ob_get_contents();
    ob_end_clean();
    return $html;
}

#
# See if this request is to the above function. Does not return
# if it is. Otherwise return and continue on.
#
sajax_handle_client_request();

#
# Standard Testbed Header after argument checking.
#
PAGEHEADER("Experiment Run");
echo $instance->RunPageHeader($runidx);
echo "<br><br>\n";
$instance->ShowRun($runidx);

echo "<script type='text/javascript' language='javascript'>\n";
sajax_show_javascript();
echo "</script>\n";

#
# Throw up graph stuff.
#
if (0) {
echo "<center>\n";
echo "<br>";
$instance->ShowGraphArea("pps", $runidx);
echo "</center>\n";
} 

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
