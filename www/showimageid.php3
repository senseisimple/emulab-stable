<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Image Descriptor");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify form arguments.
# 
if (!isset($imageid) ||
    strcmp($imageid, "") == 0) {
    USERERROR("You must provide an ImageID.", 1);
}

if (! TBValidImageID($imageid)) {
    USERERROR("ImageID '$imageid' is not a valid ImageID!", 1);
}

#
# Verify permission.
#
if (!TBImageIDAccessCheck($uid, $imageid, $TB_IMAGEID_READINFO)) {
    USERERROR("You do not have permission to access ImageID $imageid.", 1);
}

SUBPAGESTART();
SUBMENUSTART("More Options");
$fooid = rawurlencode($imageid);
WRITESUBMENUBUTTON("Edit this Image Descriptor",
		   "editimageid.php3?imageid=$fooid");
WRITESUBMENUBUTTON("Snapshot Node Disk into Image",
		   "loadimage.php3?imageid=$fooid");
WRITESUBMENUBUTTON("Delete this Image Descriptor",
		   "deleteimageid.php3?imageid=$fooid");
WRITESUBMENUBUTTON("Create a New Image Descriptor",
		   "newimageid_ez.php3");
if ($isadmin) {
    WRITESUBMENUBUTTON("Create a new OS Descriptor",
		       "newosid_form.php3");
}
WRITESUBMENUBUTTON("Image Descriptor list",
		   "showimageid_list.php3");
WRITESUBMENUBUTTON("OS Descriptor list",
		   "showosid_list.php3");
SUBMENUEND();

#
# Dump record.
# 
SHOWIMAGEID($imageid, 0);

echo "<br>\n";

#
# Show experiments using this image - we have to handle all four partitions.
# Also we don't put OSIDs directly into the virt_nodes table, so we have to
# get the pid and osname for the image, and use that to look into the virt_nodes
# table.
#
$query_result = DBQueryFatal("select part1_osid, part2_osid, " .
	"part3_osid, part4_osid from images where imageid='$imageid'");
if (mysql_num_rows($query_result) != 1) {
    USERERROR("Error getting partition information for $imageid.", 1);
}

$row = mysql_fetch_array($query_result);

$parts  = array($row["part1_osid"], $row["part2_osid"],
		$row["part3_osid"], $row["part4_osid"]);

foreach ($parts as $osid) {
    if ($osid) {
	echo "<h3 align='center'>Experiments using OS ";
	SPITOSINFOLINK($osid);
	echo "</h3>\n";
	$query_result =
	    DBQueryFatal("select pid, osname from os_info where osid='$osid'");
	if (mysql_num_rows($query_result) != 1) {
	    echo "<h4>Error getting os_info for $osid!</h4>\n";
	    continue;
	}
	$row = mysql_fetch_array($query_result);
	SHOWOSIDEXPTS($row["pid"],$row["osname"],$uid);
    }
}


SUBPAGEEND();

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
