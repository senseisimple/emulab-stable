<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Image List");

#
#
# Only known and logged in users allowed.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Admin users can see all ImageIDs, while normal users can only see
# ones in their projects or ones that are globally available.
#
$isadmin = ISADMIN($uid);

if (! isset($sortby))
    $sortby = "normal";

if (! strcmp($sortby, "normal") ||
    ! strcmp($sortby, "name"))
    $order = "i.imagename";
elseif (! strcmp($sortby, "pid"))
    $order = "i.pid";
elseif (! strcmp($sortby, "desc"))
    $order = "i.description";
else 
    $order = "i.imagename";

#
# Get the list.
#
if ($isadmin) {
    $query_result = DBQueryFatal("SELECT * FROM images as i order by $order");
}
else {
    #
    # User is allowed to view the list of all global images, and all images
    # in his project. Include images in the subgroups too, since its okay
    # for the all project members to see the descriptors. They need proper 
    # permission to use/modify the image/descriptor of course, but that is
    # checked in the pages that do that stuff. In other words, ignore the
    # shared flag in the descriptors.
    # 
    $query_result =
	DBQueryFatal("select distinct i.* from images as i ".
		     "left join group_membership as g on g.pid=i.pid ".
		     "where g.uid='$uid' or i.global order by $order");
}

SUBPAGESTART();
SUBMENUSTART("More Options");
WRITESUBMENUBUTTON("Create an Image Descriptor",
		   "newimageid_ez.php3");
WRITESUBMENUBUTTON("Create an OS Descriptor",
		   "newosid_form.php3");
WRITESUBMENUBUTTON("OS Descriptor list",
		   "showosid_list.php3");
SUBMENUEND();
SUBPAGEEND();

if (mysql_num_rows($query_result)) {
    echo "<table border=2 cellpadding=0 cellspacing=2
           align='center'>\n";

    echo "<tr>
              <td><a href='showimageid_list.php3?&sortby=name'>
                  Image</td>
              <td><a href='showimageid_list.php3?&sortby=pid'>
                  PID</td>
              <td><a href='showimageid_list.php3?&sortby=desc'>
                  Description</td>
          </tr>\n";

    while ($row = mysql_fetch_array($query_result)) {
	$imageid    = $row[imageid];
        # Must encode the imageid since Rob started using plus signs in
	# the names.
	$url        = rawurlencode($imageid);
	$descrip    = stripslashes($row[description]);
	$imagename  = $row[imagename];
	$pid        = $row[pid];

	echo "<tr>
                  <td><A href='showimageid.php3?imageid=$url'>
                         $imagename</A></td>
                  <td>$pid</td>
                  <td>$descrip</td>\n";
        echo "</tr>\n";
    }
    echo "</table>\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
