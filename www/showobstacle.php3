<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005, 2006, 2007 University of Utah and the Flux Group.
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
# Verify page arguments.
#
$reqargs = RequiredPageArguments("id",  PAGEARG_INTEGER);

#
# Standard Testbed Header
#
PAGEHEADER("Obstacle Information");

$query_result =
    DBQueryFatal("select o.*,f.pixels_per_meter from obstacles as o ".
		 "left join floorimages as f on f.floor=o.floor and ".
		 "     f.building=o.building and f.scale=1 ".
		 "where o.obstacle_id='$id'");

if (!mysql_num_rows($query_result)) {
    USERERROR("There is no such obstacle!", 1);
}

echo "<font size=+2>".
     "Obstacle <b>$id</b>".
     "</font>\n";

echo "<table border=2 cellpadding=0 cellspacing=2
             align=center>\n";

echo "<table width=\"100%\" border=2 cellpadding=1 cellspacing=2
       align='center'>\n";

echo "<tr>
          <th rowspan=2>ID</th>
          <th>X1</th>
          <th>Y1</th>
          <th>Z1</th>
          <th align=center rowspan=2>Description</th>
      </tr>
      <tr>
          <th>X2</th>
          <th>Y2</th>
          <th>Z2</th>
      </tr>\n";

while ($row = mysql_fetch_array($query_result)) {
    $x1       = $row["x1"];
    $x2       = $row["x2"];
    $y1       = $row["y1"];
    $y2       = $row["y2"];
    $z1       = $row["z1"];
    $z2       = $row["z2"];
    $desc     = $row["description"];
    $ppm      = $row["pixels_per_meter"];

    if (isset($ppm) && $ppm != 0.0) {
	$x1 = sprintf("%.3f", $x1 / $ppm);
	$x2 = sprintf("%.3f", $x2 / $ppm);
	$y1 = sprintf("%.3f", $y1 / $ppm);
	$y2 = sprintf("%.3f", $y2 / $ppm);
	$z1 = sprintf("%.3f", $z1 / $ppm);
	$z2 = sprintf("%.3f", $z2 / $ppm);
    }

    echo "<tr>
            <td rowspan=2>$id</td>
	    <td>$x1</td>
	    <td>$y1</td>
	    <td>$z1</td>
            <td rowspan=2>$desc</td>
          </tr>
          <tr>
	    <td>$x2</td>
	    <td>$y2</td>
	    <td>$z2</td>
          </tr>\n";
}
echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
