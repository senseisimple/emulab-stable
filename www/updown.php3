<?php
require("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Node Up/Down Status");

$query_result = mysql_db_query($TBDBNAME,
		"SELECT node_id, type, status FROM nodes ".
		"WHERE type='pc' OR type='shark' " .
		"ORDER BY type,priority");

if (! $query_result) {
	$err = mysql_error();
	TBERROR("Database Error getting node status: $err\n", 1);
}

# I want the summary at the top of the page, so I build an array that
# gets printed later, while couting the number of nodes with each status
$num_up = 0;
$num_pd = 0;
$num_down = 0;
$num_unpingable = 0;
$lines = array();

while ($r = mysql_fetch_array($query_result)) {
	$node_id = $r["node_id"];
	$type = $r["type"];
	$status = $r["status"];

	$line = "<tr><td>$node_id</td><td>$type</td>";

	switch ($status) {
		case "up":
			$num_up++;
			$line .= "<td><img src=\"/autostatus-icons/up.png\" alt=\"up\"></td>";
			break;
		case "possibly down":
			$num_pd++;
			$line .= "<td><img src=\"/autostatus-icons/maybe.png\" alt=\"possibly down\"></td>";
			break;
		case "down":
			$num_down++;
			$line .= "<td><img src=\"/autostatus-icons/down.png\" alt=\"down\"></td>";
			break;
		case "unpingable":
			$num_unpingable++;
			$line .= "<td><img src=\"/autostatus-icons/dep.png\" alt=\"unpingable\"></td>";
			break;
		default:
			$line .= "<td><b>Unknown</b></td>";
			break;
	}

	$line .= "</tr>\n";
	$lines[sizeof($lines)] = $line;
}
?>


<table align="center">
<tr><td align="right"><b>Up</b></td><td align="left"><? echo $num_up ?></td></tr>
<tr><td align="right"><b>Possibly Down</b></td><td align="left"><? echo $num_pd ?></td></tr>
<tr><td align="right"><b>Down</b></td><td align="left"><?
		if ($num_down > 0) {
			echo "<font color=\"red\">$num_down</font>";
		} else {
			echo "$num_down";
		} ?> </td></tr>
<tr><td align="right"><b>Unpingable</b></td><td align="left"><? echo $num_unpingable ?></td></tr>
</table>

<h3 align="center">
<?php
# Is there a better way to get 2 digit precision?
printf("%.2f",(($num_up + $num_unpingable)
			/($num_up + $num_unpingable + $num_down + $num_pd)) *100);
?>
% up or unpingable</h3>

<h3>Key:</h3>
<p>
<img src="/autostatus-icons/up.png" alt="up"> - Up<br>
<img src="/autostatus-icons/maybe.png" alt="possibly down"> - Possibly down<br>
<img src="/autostatus-icons/down.png" alt="down"> - Down<br>
<img src="/autostatus-icons/dep.png" alt="unpingable"> - Unpingable: Running an operating system that does not respond to pings<br>
</p>

<table align="center">
<tr>
	<th>Node</th><th>Type</th><th>Status</th>
</tr>

<?php
for ($i = 0; $i < sizeof($lines); $i++) {
	echo $lines[$i];
}
?>

</table>

<?php
#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

