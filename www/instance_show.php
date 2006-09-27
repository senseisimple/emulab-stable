<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
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

#
# For the Sajax Interface
#
function GraphShow($which, $arg0, $arg1)
{
    global $template, $instance;

    ob_start();
    $instance->ShowGraphArea($which, -1, $arg0, $arg1);
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
PAGEHEADER("Template Instance");

echo "<font size=+2>Template Instance <b>" .
         MakeLink("template",
		  "guid=$guid&version=$version", "$guid/$version") . 
      "</b></font>\n";
echo "<br><br>\n";

$instance->Show(1);

echo "<script type='text/javascript' language='javascript'>\n";
sajax_show_javascript();
echo "</script>\n";

#
# Throw up graph stuff.
#
if (0) {
echo "<center>\n";
echo "<br>";
$instance->ShowGraphArea("pps");
echo "</center>\n";
} 

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
