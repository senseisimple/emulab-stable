<?php
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Node Log");

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

#
# Verify form arguments.
# 
if (!isset($node_id) ||
    strcmp($node_id, "") == 0) {
    USERERROR("You must provide a node ID.", 1);
}

#
# Check to make sure that this is a valid nodeid
#
$query_result =
    DBQueryFatal("SELECT node_id FROM nodes WHERE node_id='$node_id'");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("The node $node_id is not a valid nodeid!", 1);
}

#
# Admin users can look at any node, but normal users can only control
# nodes in their own experiments.
#
if (! $isadmin) {
    USERERROR("You do not have permission to view log for node $node_id!", 1);
}

$query_result =
    DBQueryFatal("select * from nodelog where node_id='$node_id'".
		 "order by reported");

if (! mysql_num_rows($query_result)) {
    echo "<br>
          <center>
            There are no entries in the log for node $node_id.
          </center>\n";
}
else {
    echo "<br>
          <center>
            Log for node $node_id.
          </center><br>\n";

    echo "<table border=1 cellpadding=2 cellspacing=2 align='center'>\n";

    echo "<tr>
             <td align=center>Date</td>
             <td align=center>ID</td>
             <td align=center>Type</td>
             <td align=center>Reporter</td>
             <td align=center>Entry</td>
          </tr>\n";

    while ($row = mysql_fetch_array($query_result)) {
	$type       = $row[type];
	$log_id     = $row[log_id];
	$reporter   = $row[reporting_uid];
	$date       = $row[reported];
	$entry      = $row[entry];

	echo "<tr>
                 <td>$date</td>
                 <td>$log_id</td>
                 <td>$type</td>
                 <td>$reporter</td>
                 <td>$entry</td>
              </tr>\n";
    }
    echo "</table>\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
