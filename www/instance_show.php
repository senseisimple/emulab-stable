<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("template_defs.php");

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

#
# Standard Testbed Header after argument checking.
#
PAGEHEADER("Template Instance");

#
# Check to make sure this is a valid template.
#
if (! TBValidExperimentTemplate($guid, $version)) {
    USERERROR("The experiment template $guid/$version is not a valid ".
              "experiment template!", 1);
}
if (! TBIsTemplateInstanceExperiment($exptidx)) {
    USERERROR("The instance $exptidx is not a valid instance in ".
              "template $guid/$version!", 1);
}

#
# Verify Permission.
#
if (! TBExptTemplateAccessCheck($uid, $guid, $TB_EXPT_READINFO)) {
    USERERROR("You do not have permission to view experiment template ".
	      "$guid/$version!", 1);
}

echo "<font size=+2>Template Instance <b>" .
         MakeLink("template",
		  "guid=$guid&version=$version", "$guid/$version") . 
      "</b></font>\n";
echo "<br><br>\n";

SHOWTEMPLATEINSTANCE($guid, $version, $exptidx, 1);

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
