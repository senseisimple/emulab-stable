<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("OSID List");

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
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT * FROM os_info order by osid");
}
else {
    $query_result = mysql_db_query($TBDBNAME,
	"select distinct o.* from os_info as o ".
	"left join proj_memb as p on o.pid IS NULL or p.pid=o.pid ".
	"where p.uid='$uid' order by o.osid");
}
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error getting user list: $err\n", 1);
}

if (mysql_num_rows($query_result) == 0) {
	if ($isadmin) {
	    USERERROR("There are no OSIDs!", 1);
	}
	else {
	    USERERROR("There are no OSIDs in any of your projects!", 1);
	}
}

echo "<table border=2 cellpadding=0 cellspacing=2
       align='center'>\n";

echo "<tr>
          <td>OSID</td>
          <td>PID</td>
          <td>Description</td>
      </tr>\n";

while ($row = mysql_fetch_array($query_result)) {
    $osid    = $row[osid];
    $descrip = $row[description];
    $pid     = $row[pid];

    if (! $pid) {
	$pid = "&nbsp";
    }

    echo "<tr>
              <td><A href='showosinfo.php3?osid=$osid'>$osid</A></td>
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
