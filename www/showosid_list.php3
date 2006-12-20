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
PAGEHEADER("OS Descriptor List");

#
#
# Only known and logged in users allowed.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Admin users can see all OSIDs, while normal users can only see
# ones in their projects or ones that are globally available.
#

if (! isset($sortby))
    $sortby = "normal";

if (! strcmp($sortby, "normal") ||
    ! strcmp($sortby, "name"))
    $order = "o.osname";
elseif (! strcmp($sortby, "pid"))
    $order = "o.pid";
elseif (! strcmp($sortby, "desc"))
    $order = "o.description";
else 
    $order = "o.osname";

#
# Allow for creator restriction
#
$extraclause = "";
if (isset($creator) && $creator != "") {
    if (! User::ValidWebID($creator)) {
	PAGEARGERROR("Invalid characters in creator");
    }
    if ($isadmin) 
	$extraclause = "where o.creator='$creator' ";
    else
	$extraclause = "and o.creator='$creator' ";
}

#
# Get the project list.
#
if ($isadmin) {
    $query_result =
	DBQueryFatal("SELECT * FROM os_info as o ".
		     "$extraclause ".
		     "order by $order");
}
else {
    $query_result =
	DBQueryFatal("select distinct o.* from os_info as o ".
		     "left join group_membership as g on g.pid=o.pid ".
		     "where (g.uid='$uid' or o.shared=1) ".
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
WRITESUBMENUBUTTON("Image Descriptor list",
		   "showimageid_list.php3");
SUBMENUEND();

echo "Listed below are the OS Descriptors that you may use in your NS file
      with the <a href='tutorial/docwrapper.php3?docname=nscommands.html#OS'>
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
    echo "<br>
          <table border=2 cellpadding=0 cellspacing=2 align='center'>\n";
    
    echo "<tr>
              <th><a href='showosid_list.php3?&sortby=name'>
                  Name</th>
              <th><a href='showosid_list.php3?&sortby=pid'>
                  PID</th>
              <th><a href='showosid_list.php3?&sortby=desc'>
                  Description</th>
          </tr>\n";
    
    while ($row = mysql_fetch_array($query_result)) {
        $osname  = $row[osname];
        $osid    = urlencode($row[osid]);
        $descrip = stripslashes($row[description]);
        $pid     = $row[pid];
    
        echo "<tr>
                  <td><A href='showosinfo.php3?osid=$osid'>$osname</A></td>
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
