<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004, 2006 University of Utah and the Flux Group.
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
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Admin users can see all ImageIDs, while normal users can only see
# ones in their projects or ones that are globally available.
#

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
$extraclause = "";

#
# Allow for creator restriction
#
if (isset($creator) && $creator != "") {
    if (! User::ValidWebID($creator)) {
	PAGEARGERROR("Invalid characters in creator");
    }
    if ($isadmin) 
	$extraclause = "where i.creator='$creator' ";
    else
	$extraclause = "and i.creator='$creator' ";
}

#
# Get the list.
#
if ($isadmin) {
    $query_result = DBQueryFatal("SELECT * FROM images as i ".
				 "$extraclause ".
				 "order by $order");
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
		     "where (g.uid='$uid' or i.global) ".
		     "$extraclause ".
		     "order by $order");
}

SUBPAGESTART();
SUBMENUSTART("More Options");
WRITESUBMENUBUTTON("Create an Image Descriptor",
		   "newimageid_ez.php3");
if ($isadmin) {
    WRITESUBMENUBUTTON("Create an OS Descriptor",
		       "newosid_form.php3");
}
WRITESUBMENUBUTTON("OS Descriptor list",
		   "showosid_list.php3");
SUBMENUEND();

echo "Listed below are the Images that you can load on your nodes with the
      <a href='tutorial/docwrapper.php3?docname=nscommands.html#OS'>
      <tt>tb-set-node-os</tt></a> directive. If the OS you have selected for
      a node is not loaded on that node when the experiment is swapped in,
      the Testbed system will automatically reload that node's disk with the
      appropriate image. You might notice that it takes a few minutes longer
      to start your experiment when selecting an OS that is not
      already resident. Please be patient.
      <br>
      More information on how to create your own Images is in the
      <a href='tutorial/tutorial.php3#CustomOS'>Custom OS</a> section of
      the <a href='tutorial/tutorial.php3'>Emulab Tutorial.</a>
      <br>\n";

SUBPAGEEND();

if (mysql_num_rows($query_result)) {
    echo "<table border=2 cellpadding=0 cellspacing=2
           align='center'>\n";

    echo "<tr>
              <th><a href='showimageid_list.php3?&sortby=name'>
                  Image</th>
              <th><a href='showimageid_list.php3?&sortby=pid'>
                  PID</th>
              <th><a href='showimageid_list.php3?&sortby=desc'>
                  Description</th>
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
