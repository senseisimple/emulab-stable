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
sajax_export("ModifyAnno");

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
function ModifyAnno($newtext)
{
    global $this_user, $template, $instance, $runidx;

    $instance->SetRunAnnotation($this_user, $runidx, $newtext);
    return 0;
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

echo "<script type='text/javascript' language='javascript'>\n";
sajax_show_javascript();
echo "</script>\n";

echo $instance->RunPageHeader($runidx);
echo "<br><br>\n";
$instance->ShowRun($runidx);

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
