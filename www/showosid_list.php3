<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004, 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

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
$optargs = OptionalPageArguments("creator",  PAGEARG_USER);

#
# Standard Testbed Header
#
PAGEHEADER("OS Descriptor List");

#
# Allow for creator restriction
#
$extraclause = "";
if (isset($creator)) {
    $creator_idx = $creator->uid_idx();

    if ($isadmin) 
	$extraclause = "where o.creator_idx='$creator_idx' ";
    else
	$extraclause = "and o.creator_idx='$creator_idx' ";
}

#
# Get the project list.
#
if ($isadmin) {
    $query_result =
	DBQueryFatal("SELECT * FROM os_info as o ".
		     "$extraclause ".
		     "order by o.osname");
}
else {
    $uid_idx = $this_user->uid_idx();
    
    $query_result =
	DBQueryFatal("select distinct o.* from os_info as o ".
		     "left join group_membership as g on g.pid=o.pid ".
		     "where (g.uid_idx='$uid_idx' or o.shared=1) ".
		     "$extraclause ".
		     "order by o.osname");
}

SUBPAGESTART();
SUBMENUSTART("More Options");
WRITESUBMENUBUTTON("Create an Image Descriptor",
		   "newimageid_ez.php3");
if ($isadmin) {
    WRITESUBMENUBUTTON("Create an OS Descriptor",
		       "newosid.php3");
}
WRITESUBMENUBUTTON("Image Descriptor list",
		   "showimageid_list.php3");
SUBMENUEND();

echo "Listed below are the OS Descriptors that you may use in your NS file
      with the <a href='$WIKIDOCURL/nscommands#OS'>
      <tt>tb-set-node-os</tt></a> directive. If the OS you have selected for
      a node is not loaded on that node when the experiment is swapped in,
      the Testbed system will automatically reload that node's disk with the
      appropriate image. You might notice that it takes a few minutes longer
      to start your experiment when selecting an OS that is not
      already resident. Please be patient.
      <br>
      More information on how to create your own Images is in the
      <a href='$WIKIDOCURL/Tutorial#CustomOS'>Custom OS</a> section of
      the <a href='$WIKIDOCURL/Tutorial'>Emulab Tutorial.</a>
      <br>\n";

SUBPAGEEND();

if (mysql_num_rows($query_result)) {
    echo "<br>
          <table border=2 cellpadding=0 cellspacing=2
                 align='center' id='showosidlist'>\n";
    
    echo "<thead class='sort'>
           <tr>
              <th>Name</th>
              <th>PID</th>
              <th>Description</th>
           </tr>
          </thead>\n";
    
    while ($row = mysql_fetch_array($query_result)) {
        $osname  = $row["osname"];
        $osid    = $row["osid"];
        $descrip = $row["description"];
        $pid     = $row["pid"];
	$url     = CreateURL("showosinfo", URLARG_OSID, $osid);
    
        echo "<tr>
                  <td><A href='$url'>$osname</A></td>
                  <td>$pid</td>
                  <td>$descrip</td>\n";
        echo "</tr>\n";
    }
    echo "</table>\n";
}
echo "<script type='text/javascript' language='javascript'>
	sorttable.makeSortable(getObjbyName('showosidlist'));
      </script>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
