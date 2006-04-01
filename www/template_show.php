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
if (!isset($guid) ||
    strcmp($guid, "") == 0) {
    USERERROR("You must provide a template GUID.", 1);
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

#
# Standard Testbed Header after argument checking.
#
PAGEHEADER("Experiment Template: $guid/$version");

#
# Check to make sure this is a valid template.
#
if (! TBValidExperimentTemplate($guid, $version)) {
    USERERROR("The experiment template $guid/$version is not a valid ".
              "experiment template!", 1);
}
TBTemplatePidEid($guid, $version, $pid, $eid);

#
# Verify Permission.
#
if (! TBExptTemplateAccessCheck($uid, $guid, $TB_EXPT_READINFO)) {
    USERERROR("You do not have permission to view experiment template ".
	      "$guid/$version!", 1);
}

echo "<font size=+2>Experiment Template <b>" .
         MakeLink("template",
		  "guid=$guid&version=$version", "$guid/$version") . 
      "</b></font>\n";
echo "<br><br>\n";

SUBPAGESTART();
SUBMENUSTART("Template Options");

WRITESUBMENUBUTTON("Show NS File &nbsp &nbsp",
		   "spitnsdata.php3?guid=$guid&version=$version");

WRITESUBMENUBUTTON("Modify Template",
		   "template_modify.php?guid=$guid&version=$version");

WRITESUBMENUBUTTON("Instantiate Template",
		   "template_swapin.php?guid=$guid&version=$version");

WRITESUBMENUBUTTON("Add Metadata",
		   "template_metadata.php?action=add&".
		   "guid=$guid&version=$version");

WRITESUBMENUBUTTON("Template Archive",
		   "archive_view.php3?pid=$pid&eid=$eid");

WRITESUBMENUBUTTON("Template History",
		   "template_history.php?guid=$guid&version=$version");

SUBMENUEND_2A();

#
# Ick.
#
$rsrcidx = TBrsrcIndex($pid, $eid);

echo "<br>
      <img border=1 alt='template visualization'
           src='showthumb.php3?idx=$rsrcidx'>";

SUBMENUEND_2B();

SHOWTEMPLATE($guid, $version);
SHOWTEMPLATEMETADATA($guid, $version);
SUBPAGEEND();

if (SHOWTEMPLATEPARAMETERS($guid, $version)) {
    echo "<br>\n";
}
SHOWTEMPLATEINSTANCES($guid, $version);

SHOWTEMPLATEGRAPH($guid);

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
