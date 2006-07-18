<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2005 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Node Type Information");

#
# Anyone can access this info, its a PUBLIC PAGE!
#

#
# Verify form arguments.
# 
if (!isset($node_type) ||
    strcmp($node_type, "") == 0) {
    USERERROR("You must provide a node type.", 1);
}
# Sanitize.
if (!preg_match("/^[-\w]+$/", $node_type)) {
    PAGEARGERROR("Invalid characters in arguments.");
}

#
# Check to make sure that this is a valid nodeid
#
if (! TBValidNodeType($node_type)) {
    USERERROR("$node_type is not a valid node type!", 1);
}

$query_result =
    DBQueryFatal("select * from node_types ".
		 "where type='$node_type'");

if (! mysql_num_rows($query_result) != 0) {
    TBERROR("No entry for node_type $node_type!", 1);
}
$noderow = mysql_fetch_assoc($query_result);

echo "<font size=+2>".
     "Node Type <b>$node_type</b>".
     "</font>\n";

echo "<table border=2 cellpadding=0 cellspacing=2
             align=center>\n";

# Stuff from the node types table.
$class		= $noderow["class"];
$isvirtnode	= $noderow["isvirtnode"];
$isremotenode	= $noderow["isremotenode"];

# Grab the attributes for the type.
$query_result = DBQueryFatal("select * from node_type_attributes ".
			     "where type='$node_type' ".
			     "order by attrkey");

echo "<tr>
      <td>Type:</td>
      <td class=left>$node_type</td>
          </tr>\n";

echo "<tr>
      <td>Class:</td>
      <td class=left>$class</td>
          </tr>\n";

if ($isremotenode) {
    echo "<tr>
          <td>Remote:</td>
          <td class=left>Yes</td>
              </tr>\n";
}

if ($isvirtnode) {
    echo "<tr>
          <td>Virtual:</td>
          <td class=left>Yes</td>
              </tr>\n";
}

#
# And now all of the attributes ...
#
while ($row = mysql_fetch_array($query_result)) {
    $key      = $row["attrkey"];
    $val      = $row["attrvalue"];
    $attrtype = $row["attrtype"];

    if ($key == "default_osid" ||
	$key == "jail_osid" ||
	$key == "delay_osid") {
	TBOSInfo($val, $osname, $pid);
	$val = $osname;
    }
    elseif ($key == "default_imageid") {
	TBImageInfo($val, $imagename, $pid);
	$val = $imagename;
    }
    echo "<tr>\n";
    echo "<td>$key:</td>\n";
    echo "<td class=left>$val</td>\n";
    echo "</tr>\n";
}
echo "</table>\n";

#
# Suck out info for all the nodes of this type. We are going to show
# just a list of dots, in two color mode.
# 
$query_result =
    DBQueryFatal("select n.node_id,n.eventstate,r.pid ".
		 "from nodes as n ".
		 "left join node_types as nt on n.type=nt.type ".
		 "left join reserved as r on n.node_id=r.node_id ".
		 "where nt.type='$node_type' and ".
		 "      (role='testnode' or role='virtnode') ".
		 "ORDER BY priority");


if (mysql_num_rows($query_result)) {
    echo "<br>
          <center>
          <table class=nogrid cellspacing=0 border=0 cellpadding=5>\n";

    $maxcolumns = 4;
    $column     = 0;
    
    while ($row = mysql_fetch_array($query_result)) {
	$node_id = $row["node_id"];
	$es      = $row["eventstate"];
	$pid     = $row["pid"];

	if ($column == 0) {
	    echo "<tr>\n";
	}
	$column++;

	echo "<td align=left><nobr>\n";

	if (!$pid) {
	    if (($es == TBDB_NODESTATE_ISUP) ||
		($es == TBDB_NODESTATE_ALWAYSUP) ||
		($es == TBDB_NODESTATE_POWEROFF) ||
		($es == TBDB_NODESTATE_PXEWAIT)) {
		echo "<img src=\"/autostatus-icons/greenball.gif\" alt=free>\n";
	    }
	    else {
		echo "<img src=\"/autostatus-icons/yellowball.gif\" alt='unusable free'>\n";
	    }
	}
	else {
	    echo "<img src=\"/autostatus-icons/redball.gif\" alt=reserved>\n";
	}
	echo "&nbsp;";
#	echo "<a href=shownode.php3?node_id=$node_id>";
	echo "$node_id";
#	echo "</a>";
	echo "</nobr>
              </td>\n";
	
	if ($column == $maxcolumns) {
	    echo "</tr>\n";
	    $column = 0;
	}
    }
    echo "</table>\n";
    echo "<br>
          <img src=\"/autostatus-icons/greenball.gif\" alt=free>&nbsp;Free
          &nbsp; &nbsp; &nbsp;
          <img src=\"/autostatus-icons/redball.gif\" alt=free>&nbsp;Reserved
          </center>\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>




