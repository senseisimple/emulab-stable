<?php
require("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Node Up/Down Status");

?>
<?php
# This is kinda a pain!
$query_result = mysql_db_query($TBDBNAME,
		"SELECT count(*) FROM nodes ".
		"WHERE status = 'up'");

if (! $query_result) {
	$err = mysql_error();
	TBERROR("Database Error getting node status: $err\n", 1);
}

$r = mysql_fetch_array($query_result);
$num_up = $r[0];

# If you get an answer the first time, you're probably not gonna get an error the
# second time...
$query_result = mysql_db_query($TBDBNAME,
		"SELECT count(*) FROM nodes ".
		"WHERE status = 'possibly down'");
$r = mysql_fetch_array($query_result);
$num_pd = $r[0];

$query_result = mysql_db_query($TBDBNAME,
		"SELECT count(*) FROM nodes ".
		"WHERE status = 'down'");
$r = mysql_fetch_array($query_result);
$num_down = $r[0];

$query_result = mysql_db_query($TBDBNAME,
		"SELECT count(*) FROM nodes ".
		"WHERE status = 'unpingable'");
$r = mysql_fetch_array($query_result);
$num_unpingable = $r[0];

$num_total = ($num_up + $num_unpingable + $num_down + $num_pd);
?>

<table>
<tr><td align="right"><b>Up</b></td><td align="left"><? echo $num_up ?></td></tr>
<tr><td align="right"><b>Unpingable</b></td><td align="left"><? echo $num_unpingable ?></td></tr>
<tr><td align="right"><b>Possibly Down</b></td><td align="left"><? echo $num_pd ?></td></tr>
<tr><td align="right"><b>Down</b></td><td align="left"><?
		if ($num_down > 0) {
			echo "<font color=\"red\">$num_down</font>";
		} else {
			echo "$num_down";
		} ?> </td></tr>
<tr><td align="right"><b>Total</b></td><td align="left"><? echo $num_total ?></td></tr>
</table>


<b>
<?php
# Is there a better way to get 2 digit precision?
printf("%.2f",(($num_up + $num_unpingable)/$num_total) *100);
?>
% up or unpingable</b>

<h3>Key:</h3>
<p>
<img src="/autostatus-icons/up.png" alt="up"> - Up<br>
<img src="/autostatus-icons/maybe.png" alt="possibly down"> - Possibly down<br>
<img src="/autostatus-icons/down.png" alt="down"> - Down<br>
<img src="/autostatus-icons/dep.png" alt="unpingable"> - Unpingable: Running an operating system that does not respond to pings<br>
</p>

<?php
$query_result = mysql_db_query($TBDBNAME,
		"SELECT node_id, type, status FROM nodes ".
#		"WHERE type='pc' OR type='shark' " .
		"ORDER BY type,priority");

if (! $query_result) {
	$err = mysql_error();
	TBERROR("Database Error getting node status: $err\n", 1);
}


$nodecount = 0;
$maxcolumns = 4;

$column = 0; # First pass will increment to 1
$lasttype = "";

while ($r = mysql_fetch_array($query_result)) {
	$node_id = $r["node_id"];
	$type = $r["type"];
	$status = $r["status"];

	if ($type != $lasttype) {
		if ($lasttype != "") { # Doesn't  need to happen the first time
			print("</table>\n\n");
		}

		print("<h3>$type</h3>\n");
		print("<table cellspacing=0 cellpadding=5>\n");

		$lasttype = $type;
		$column = 0;
	}

	if ($column == $maxcolumns) {
		$column = 1;
		# End the old row
		print "</tr>\n";
	} else {
		$column++;
	}

	if ($column %2) {
		$tdbg = ""; # Make odd #'d rows have a white background
	} else {
		$tdbg = "bgcolor=\"#ffffff\"";
	}

	# Only start a new <tr> if back in column 0
	if ($column == 0) {
		print("<tr>");
	}

	print("<td $tdbg><nobr>");
	switch ($status) {
		case "up":
			print("<img src=\"/autostatus-icons/up.png\" alt=\"up\">");
			break;
		case "possibly down":
			print("<img src=\"/autostatus-icons/maybe.png\" alt=\"possibly down\">");
			break;
		case "down":
			print("<img src=\"/autostatus-icons/down.png\" alt=\"down\">");
			break;
		case "unpingable":
			print("<img src=\"/autostatus-icons/dep.png\" alt=\"unpingable\">");
			break;
		default:
			print("<b>Unknown</b>");
			break;
	}
	print("&nbsp;$node_id</nobr></td>");
}

# End the table printed on the last pass
print("</tr>\n</table>\n\n");

?>

<?php
#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

