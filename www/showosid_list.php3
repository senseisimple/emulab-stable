<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
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
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Admin users can see all OSIDs, while normal users can only see
# ones in their projects or ones that are globally available.
#
$isadmin = ISADMIN($uid);

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
# Get the project list.
#
if ($isadmin) {
    $query_result =
	DBQueryFatal("SELECT * FROM os_info as o order by $order");
}
else {
    $query_result =
	DBQueryFatal("select distinct o.* from os_info as o ".
		     "left join group_membership as g on g.pid=o.pid ".
		     "where g.uid='$uid' or o.shared=1 ".
		     "order by $order");
}

if (mysql_num_rows($query_result) == 0) {
	if ($isadmin) {
	    USERERROR("There are no OSIDs!", 1);
	}
	else {
	    USERERROR("There are no OSIDs in any of your projects!", 1);
	}
}

SUBPAGESTART();
SUBMENUSTART("More Options");
WRITESUBMENUBUTTON("Create an Image Descriptor",
		   "newimageid_explain.php3");
WRITESUBMENUBUTTON("Create an OS Descriptor",
		   "newosid_form.php3");
WRITESUBMENUBUTTON("Image Descriptor list",
		   "showimageid_list.php3");
SUBMENUEND();

echo "<p>
      Listed below are the OS Descriptors that you may use in your NS file
      with the <a href='tutorial/docwrapper.php3?docname=nscommands.html#OS'>
      <tt>tb-set-node-os</tt></a> directive. If the OS you have selected for
      a node is not loaded on that node when the experiment is swapped in,
      the Testbed system will automatically reload that node's disk with the
      appropriate image. You might notice that it takes a few minutes longer
      to start start your experiment when selecting an OS that is not
      already resident. Please be patient.
      </p><br />\n";

SUBPAGEEND();

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
    $osid    = $row[osid];
    $descrip = stripslashes($row[description]);
    $pid     = $row[pid];

    echo "<tr>
              <td><A href='showosinfo.php3?osid=$osid'>$osname</A></td>
              <td>$pid</td>
              <td>$descrip</td>\n";
    echo "</tr>\n";
}
echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
