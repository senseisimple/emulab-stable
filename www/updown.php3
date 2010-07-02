<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

if (isset($_REQUEST["output"]) &&
    $_REQUEST["output"] == "xml") {
    header("Content-type: text/xml");

    $query_result =
	DBQueryFatal("SELECT n.node_id, n.type, ns.status FROM nodes as n ".
		     "left join node_types as nt on nt.type=n.type ".
		     "left join node_status as ns on ns.node_id=n.node_id ".
		     "WHERE nt.class!='shark' and role='testnode'".
		     "ORDER BY n.type,n.priority");

    print "<?xml version=\"1.0\" encoding=\"ISO-8859_1\" standalone=\"yes\"?>\n";

    print "<!DOCTYPE testbed \n[\n".
          "  <!ELEMENT testbed (node*)>\n".
	  "  <!ATTLIST testbed name CDATA #REQUIRED>\n".
          "  <!ELEMENT node EMPTY>\n".
	  "  <!ATTLIST node id     ID    #REQUIRED\n".
	  "                 type   CDATA #IMPLIED\n".
          "                 status CDATA \"unknown\"\n".
          "  >\n". 
	  "]>\n";

    print "<testbed name=\"$OURDOMAIN\">\n";
    while ($r = mysql_fetch_array($query_result)) {
	$node_id = $r["node_id"];
	$type = $r["type"];
	$status = $r["status"];
	
	print "  <node id=\"$node_id\" type=\"$type\" ";
	if ($status != "") {
	    print " status=\"$status\" ";
	}
	print "/>\n";
    }
    print "</testbed>\n";

    return;
}

#
# Standard Testbed Header
#
PAGEHEADER("Node Up/Down Status");

$query_result =
    DBQueryFatal("SELECT n.node_id,n.type,ns.status,nt.class FROM nodes as n ".
	"left join node_types as nt on nt.type=n.type ".
	"left join node_status as ns on ns.node_id=n.node_id ".
	"WHERE nt.class!='shark' and role='testnode'".
	"ORDER BY n.type,n.priority");

$num_up    = 0;
$num_pd    = 0;
$num_down  = 0;
$num_unk   = 0;

while ($r = mysql_fetch_array($query_result)) {
	$status = $r["status"];

	switch ($status) {
	case "up":
	    $num_up++;
	    break;
	case "possibly down":
	case "unpingable":
	    $num_pd++;
	    break;
	case "down":
	    $num_down++;
	    break;
	default:
	    $num_unk++;
	    break;
	}
}
$num_total = ($num_up + $num_down + $num_pd + $num_unk);
mysql_data_seek($query_result, 0);

?>

<table>
<tr><td align="right">
    <b>Up</b></td><td align="left"><? echo $num_up ?></td></tr>
<tr><td align="right">
    <b>Possibly Down</b></td><td align="left"><? echo $num_pd ?></td></tr>
<tr><td align="right">
    <b>Unknown</b></td><td align="left"><? echo $num_unk ?></td></tr>
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
printf("%.2f",(($num_up)/$num_total) *100);
?>
% up </b>

<h3>Key:</h3>
<ul>
<li>
<img src="/autostatus-icons/greenball.gif" alt="up"> - Up<br>
</li><li>
<img src="/autostatus-icons/yellowball.gif" alt="Possibly Down"> -
     Possibly Down<br>
</li><li>
<img src="/autostatus-icons/redball.gif" alt="down"> - Down<br>
</li><li>
<img src="/autostatus-icons/blueball.gif" alt="Unknown"> - Unknown<br>
</li>
</ul>


<?php
$nodecount = 0;
$maxcolumns = 4;

$column = 0; # First pass will increment to 1
$lasttype = "";

while ($r = mysql_fetch_array($query_result)) {
	$node_id = $r["node_id"];
	$type = $r["type"];
	$status = $r["status"];
	$class = $r["class"];

	if ($type != $lasttype) {
		if ($lasttype != "") { # Doesn't  need to happen the first time
			print("</table>\n\n");
		}

                if ($class == "pc") {
  		    print("<a href=shownodetype.php3?node_type=$type>".
                          "<h4>$type</h4></a>\n");
                }
                else {
		    print("<h4>$type</h4>\n");
                }
		print("<table class=\"nogrid\" ".
                      "cellspacing=0 border=0 cellpadding=5>\n");

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
		case "unpingable":
		case "possibly down":
			print("<img src=\"/autostatus-icons/yellowball.gif\" alt=\"possibly down\">");
			break;
		case "down":
			print("<img src=\"/autostatus-icons/redball.gif\" alt=\"down\">");
			break;
		default:
			print("<img src=\"/autostatus-icons/blueball.gif\" alt=\"Unknown\">");
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

