<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Utah Testbed Machine Status");

$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

$query_result =
    DBQueryFatal("SELECT n.node_id, n.phys_nodeid, n.type, nt.class, ".
		 " nt.isvirtnode, j.pid, j.eid, j.vname from nodes AS n ".
		 "LEFT JOIN reserved AS j ON n.node_id = j.node_id ".
		 "LEFT JOIN node_types AS nt ON n.type=nt.type ".
		 "WHERE (role='testnode' or role='virtnode') and ".
		 "      nt.class!='shark' " .
		 "ORDER BY priority,type");

#
# Count the types up.
#
$numpcs    = 0;
$numsharks = 0;
$freepcs   = 0;
$freesharks= 0;

while ($r = mysql_fetch_array($query_result)) {
    $type = $r["class"];
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
          <td width=10%><b>ID</b></td>
          <td width=10%><b>Type</b></td>\n";
if ($isadmin) {
  echo "<td width=25%><b>Project</b></td>
          <td width=25%><b>Experiment</b></td>
          <td width=25%><b>Virtual Nickname</b></td>\n";
} else {
  echo "<td width=20%><b>Reservation Status</b></td>\n";
}
echo "</tr>\n";

mysql_data_seek($query_result, 0);
while ($r = mysql_fetch_array($query_result)) {
    $id = $r["node_id"];  $type = $r["type"]; $class = $r["class"];
    $res1 = $r["pid"];
    $res2 = $r["eid"];
    $res3 = $r["vname"];
    $isvirt = $r["isvirtnode"];
    $phys = $r["phys_nodeid"];
    if (!$res1 || $res1 == "NULL") {
      $res1 = "--";
    }
    if (!$res2 || $res2 == "NULL") {
      $res2 = "--";
    }
    if (!$res3 || $res3 == "NULL") {
      $res3 = "--";
    }
    if ($isvirt)
	echo "<tr><td>$id ($phys)</td> ";
    else
	echo "<tr><td>$id</td> ";
    echo "<td>$type ($class)</td> ";
    # If I'm an admin, I can see pid/eid/vname, but if not, I only see eid
    if ($isadmin) { echo "<td>$res1</td> "; }
    echo "<td>$res2</td> ";
    if ($isadmin) { echo "<td>$res3</td>"; }
    echo "</tr>\n";
}
echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
