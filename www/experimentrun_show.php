<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("template_defs.php");

#
# Only known and logged in users ...
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

#
# Verify page arguments.
# 
if (!isset($exptidx) ||
    strcmp($exptidx, "") == 0) {
    USERERROR("You must provide an instance ID", 1);
}
if (!isset($runidx) ||
    strcmp($runidx, "") == 0) {
    USERERROR("You must provide an run ID", 1);
}
if (!isset($guid) ||
    strcmp($guid, "") == 0) {
    USERERROR("You must provide a template ID", 1);
}
if (!isset($version) ||
    strcmp($version, "") == 0) {
    USERERROR("You must provide a template version number", 1);
}
if (!TBvalid_guid($guid)) {
    PAGEARGERROR("Invalid characters in GUID!");
}
if (!TBvalid_integer($version)) {
    PAGEARGERROR("Invalid characters in version!");
}
if (!TBvalid_integer($exptidx)) {
    PAGEARGERROR("Invalid characters in instance ID!");
}
if (!TBvalid_integer($runidx)) {
    PAGEARGERROR("Invalid characters in run ID!");
}

#
# Standard Testbed Header after argument checking.
#
PAGEHEADER("Experiment Run");

#
# Check to make sure this is a valid template and user has permission.
#
$template = Template::Lookup($guid, $version);
if (!$template) {
    USERERROR("The experiment template $guid/$version is not a valid ".
              "experiment template!", 1);
}
if (! $template->AccessCheck($uid, $TB_EXPT_READINFO)) {
    USERERROR("You do not have permission to view experiment template ".
	      "$guid/$version!", 1);
}
$instance = TemplateInstance::LookupByExptidx($exptidx);
if (!$instance) {
    USERERROR("The instance $exptidx is not a valid instance in ".
              "template $guid/$version!", 1);
}
if (! $instance->ValidRun($runidx)) {
    USERERROR("The run $runidx is not a valid experiment run!", 1);
}

echo "<font size=+2>Experiment Run<b> " .
         MakeLink("instance",
		  "guid=$guid&version=$version&exptidx=$exptidx",
		  "$guid/$version/$exptidx") . 
      "</b></font>\n";
echo "<br><br>\n";


$instance->ShowRun($runidx);

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
