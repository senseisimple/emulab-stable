<?php
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

#
# Get the project list.
#
if ($isadmin) {
    $query_result =
	DBQueryFatal("SELECT * FROM os_info order by pid,osname");
}
else {
    $query_result =
	DBQueryFatal("select distinct o.* from os_info as o ".
		     "left join group_membership as g on g.pid=o.pid ".
		     "where g.uid='$uid' or o.shared=1 ".
		     "order by o.pid,o.osname");
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
WRITESUBMENUBUTTON("Back to Image Descriptor list",
		   "showimageid_list.php3");
SUBMENUEND();
SUBPAGEEND();

echo "<table border=2 cellpadding=0 cellspacing=2
       align='center'>\n";

echo "<tr>
          <td>Name</td>
          <td>PID</td>
          <td>Description</td>
      </tr>\n";

while ($row = mysql_fetch_array($query_result)) {
    $osname  = $row[osname];
    $osid    = $row[osid];
    $descrip = $row[description];
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
