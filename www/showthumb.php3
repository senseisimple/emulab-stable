<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# This script generates the contents of an image. No headers or footers,
# just spit back an image. The thumbs are public, so no checking is done.
# To obfuscate, do not use pid/eid, but rather use the resource index. 
#
$reqargs = RequiredPageArguments("idx", PAGEARG_INTEGER);

#
# Get the thumb from the DB. 
#
$query_result =
    DBQueryFatal("select thumbnail from experiment_resources ".
		 "where idx='$idx'");

if ($query_result && mysql_num_rows($query_result)) {
    $row  = mysql_fetch_array($query_result);
    $data = $row["thumbnail"];

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
