<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Utah Testbed Machine Statu");

echo "<center>
     <h1>Utah Testbed Machine Status</h1>
     </center>\n";

$query_result = mysql_db_query($TBDBNAME,
	"SELECT n.node_id, n.type, j.eid from nodes ".
	"as n left join reserved AS j ON n.node_id = j.node_id");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error getting node reservation status: $err\n", 1);
}

echo "<table border=1 padding=1>\n";
echo "<tr>
          <td><b>ID</b></td>
          <td><b>Type</b></td>
          <td><b>Reservation Status</b></td>
      </tr>\n";

while ($r = mysql_fetch_array($query_result)) {
    $id = $r["node_id"];  $type = $r["type"]; 
    $res = $r["eid"];
    if (!$res || $res == "NULL") {
	$res = "--";
    }
    echo "<tr><td>$id</td> <td>$type</td> <td>$res</td></tr>\n";
}
echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
