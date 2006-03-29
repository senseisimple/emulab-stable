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

#
# Get the data from the DB.
#
$query_result =
    DBQueryFatal("select image from experiment_template_graphs ".
		 "where parent_guid='$guid'");

if ($query_result && mysql_num_rows($query_result)) {
    $row  = mysql_fetch_array($query_result);
    $data = $row['image'];

    if (strlen($data)) {
	header("Content-type: image/png");
	echo "$data";
	return;
    }
}

# No Data. Spit back a stub image.
header("Content-type: image/gif");
readfile("coming-soon-thumb.gif");

#
# No Footer!
# 
?>
