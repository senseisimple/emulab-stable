<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Node Up/Down Status");

$query_result =
    DBQueryFatal("SELECT node_id, nodes.type, status FROM nodes ".
	"left join node_types on node_types.type=nodes.type ".
	"WHERE node_types.class!='shark' and node_types.isvirtnode=0 ".
	"ORDER BY nodes.type,priority");

$num_up    = 0;
$num_pd    = 0;
$num_down  = 0;
$num_unpingable = 0;

while ($r = mysql_fetch_array($query_result)) {
	$status = $r["status"];

	switch ($status) {
	case "up":
	    $num_up++;
	    break;
	case "possibly down":
	    $num_pd++;
	    break;
	case "down":
	    $num_down++;
	    break;
	case "unpingable":
	    $num_unpingable++;
	    break;
	default:
	    break;
	}
}
$num_total = ($num_up + $num_unpingable + $num_down + $num_pd);
mysql_data_seek($query_result, 0);

?>

<table>
<tr><td align="right">
    <b>Up</b></td><td align="left"><? echo $num_up ?></td></tr>
<tr><td align="right">
    <b>Unpingable</b></td><td align="left"><? echo $num_unpingable ?></td></tr>
<tr><td align="right">
    <b>Possibly Down</b></td><td align="left"><? echo $num_pd ?></td></tr>
<tr><td align="right"><b>Down</b></td><td align="left">
<?
if ($num_down > 0) {
    echo "<font color=\"red\">$num_down</font>";
}
else {
    echo "$num_down";
}
?>
</td></tr>
<tr><td align="right">
    <b>Total</b></td><td align="left"><? echo $num_total ?></td></tr>
</table>


<b>
<?php
# Is there a better way to get 2 digit precision?
printf("%.2f",(($num_up + $num_unpingable)/$num_total) *100);
?>
% up or unpingable</b>

<h3>Key:</h3>
<p>
<img src="/autostatus-icons/greenball.gif" alt="up"> - Up<br>
<img src="/autostatus-icons/yellowball.gif" alt="possibly down"> - Possibly down<br>
<img src="/autostatus-icons/redball.gif" alt="down"> - Down<br>
<img src="/autostatus-icons/blueball.gif" alt="unpingable"> - Unpingable: Running an operating system that does not respond to pings<br>
</p>

<?php
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
		print("<table cellspacing=0 border=0 cellpadding=5>\n");

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
			print("<img src=\"/autostatus-icons/greenball.gif\" alt=\"up\">");
			break;
		case "possibly down":
			print("<img src=\"/autostatus-icons/yellowball.gif\" alt=\"possibly down\">");
			break;
		case "down":
			print("<img src=\"/autostatus-icons/redball.gif\" alt=\"down\">");
			break;
		case "unpingable":
			print("<img src=\"/autostatus-icons/blueball.gif\" alt=\"unpingable\">");
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

