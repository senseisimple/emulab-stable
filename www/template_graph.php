<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("template_defs.php");

#
# This script generates the contents of an image. No headers or footers,
# just spit back an image. The thumbs are public, so no checking is done.
# To obfuscate, do not use pid/eid, but rather use the resource index. 
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

function SPITGRAPH($guid)
{
    #
    # Get the data from the DB.
    #
    $query_result =
	DBQueryWarn("select image from experiment_template_graphs ".
		    "where parent_guid='$guid'");

    if (!$query_result || !mysql_num_rows($query_result)) {
        # No Data. Spit back a stub image.
	SPITERROR();
	return;
    }
    $row  = mysql_fetch_array($query_result);
    $data = $row['image'];

    if ($data != "") {
	header("Content-type: image/png");
	echo "$data";
    }
    else {
	SPITERROR();
    }
}
    

#
# If the request did not specify a zoom, return whatever we have.
#
if (!isset($zoom)) {
    SPITGRAPH($guid);
    return;
}

#
# Otherwise regen the picture, zooming in or out.
#
$optarg = "-z " . ($zoom == "in" ? "in" : "out");

if (! TBGuid2PidGid($guid, $pid, $gid)) {
    TBERROR("Could not get template pid,gid for template $guid", 0);
    SPITERROR();
}
TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);

$retval = SUEXEC($uid, "$pid,$unix_gid", "webtemplate_graph $optarg $guid",
		 SUEXEC_ACTION_CONTINUE);

if ($retval) {
    SPITERROR();
}
else {
    SPITGRAPH();
}

#
# No Footer!
# 
?>
