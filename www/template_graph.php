<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("template_defs.php");

#
# This script generates the contents of an image. No headers or footers,
# just spit back an image. 
#

#
# Verify page arguments.
# 
if (!isset($guid) ||
    strcmp($guid, "") == 0) {
    USERERROR("You must provide a Template ID.", 1);
}
if (!TBvalid_guid($guid)) {
    PAGEARGERROR("Invalid characters in GUID!");
}

if (isset($zoom)) {
    if ($zoom != "in" && $zoom != "out") {
	PAGEARGERROR("Invalid characters in zoom factor!");
    }
}
else {
    unset($zoom);
}

function SPITERROR()
{
    header("Content-type: image/gif");
    readfile("coming-soon-thumb.gif");
}

function SPITGRAPH($template)
{
    $data = NULL;

    if ($template->GraphImage($data) != 0 || $data == NULL || $data == "") {
	SPITERROR();
    }
    else {
	header("Content-type: image/png");
	echo "$data";
    }
}

$template = Template::LookupRoot($guid);
if (!$template) {
# Bad template; spit back a stub image.
    SPITERROR();
    return;
}

#
# If the request did not specify a zoom, return whatever we have.
#
if (!isset($zoom)) {
    SPITGRAPH($template);
    return;
}

#
# Otherwise regen the picture, zooming in or out.
#
$optarg = "-z " . ($zoom == "in" ? "in" : "out");

$pid = $template->pid();
$gid = $template->gid();
TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);

$retval = SUEXEC($uid, "$pid,$unix_gid", "webtemplate_graph $optarg $guid",
		 SUEXEC_ACTION_CONTINUE);

if ($retval) {
    SPITERROR();
}
else {
    SPITGRAPH($template);
}

#
# No Footer!
# 
?>
