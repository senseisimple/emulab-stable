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
$reqargs = RequiredPageArguments("template", PAGEARG_TEMPLATE);
$optargs = OptionalPageArguments("expand",   PAGEARG_INTEGER);

if (!isset($expand)) {
    $expand = 0;
}

#
# Standard Testbed Header after argument checking.
#
PAGEHEADER("Experiment Template History");

#
# Check permission.
#
if (! $template->AccessCheck($this_user, $TB_EXPT_READINFO)) {
    USERERROR("You do not have permission to view experiment template ".
	      "$guid/$version!", 1);
}

echo $template->PageHeader();
echo "<br><br>\n";
$template->ShowHistory($expand);

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
