<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Utah Testbed Machine Status");

$query_result = mysql_db_query($TBDBNAME,
	"SELECT n.node_id, n.type, j.eid FROM nodes ".
	"AS n LEFT JOIN reserved AS j ON n.node_id = j.node_id ".
	"WHERE type='pc' OR type='shark' ORDER BY type,node_id");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error getting node reservation status: $err\n", 1);
}

#
# Count the types up.
#
$numpcs    = 0;
$numsharks = 0;
$freepcs   = 0;
$freesharks= 0;

while ($r = mysql_fetch_array($query_result)) {
    $type = $r["type"];
    $eid  = $r["eid"];
    
    if ($type == "pc") {
	$numpcs++;
    }
    if ($type == "shark") {
	$numsharks++;
    }
    if ($eid && $eid != "NULL") {
	continue;
    }
    if ($type == "pc") {
	$freepcs++;
    }
    if ($type == "shark") {
	$freesharks++;
    }
}

echo "<center><h3>
      $freepcs of $numpcs PCs Free <br>
      $freesharks of $numsharks Sharks Free.
      </h3></center>\n";

echo "<table border=1 align=center padding=1>\n";
echo "<tr>
          <td><b>ID</b></td>
          <td><b>Type</b></td>
          <td><b>Reservation Status</b></td>
      </tr>\n";

mysql_data_seek($query_result, 0);
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
