<?PHP
#
# EMULAB-COPYRIGHT
# Copyright (c) 2003 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");
require("newnode-defs.php3");

#
# List the nodes that have checked in and are awaint being added the the real
# testbed
#

#
# Standard Testbed Header
#
PAGEHEADER("New Testbed Nodes");

#
# Only admins can see this page
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);
if (! $isadmin) {
    USERERROR("You do not have admin privileges!", 1);
}

#
# XXX - a hack
#
$gid = "nobody";

#
# Start out by performing any operations they may have asked for
#
if ($selected) {
    $selected_nodes = $selected;
} else {
    $selected_nodes = array();
}

#
# Build up a handy little clause for these nodes to be used in WHEREs
#
if (count($selected_nodes)) {
    $equal_clauses = array();
    foreach ($selected_nodes as $node) {
	$equal_clauses[] = "new_node_id=$node";
	$equal_clauses_qualified[] = "n.new_node_id=$node";
    }
    $whereclause = implode(" OR ",$equal_clauses);
    $whereclause_qualified = implode(" OR ",$equal_clauses_qualified);
} else {
    if ($delete || $calc || $create || $research || $swap || $newtype ||
	    $newprefix || $addnumber || $renumber) {
	USERERROR("At least one node must be selected!",1);
    }
}

#
# Delete nodes
#
if ($delete) {
    DBQueryFatal("DELETE FROM new_nodes WHERE $whereclause");
    DBQueryFatal("DELETE FROM new_interfaces WHERE $whereclause");
}

#
# Recalculate IPs
#
if ($calc) {
   $query_result = DBQueryFatal("SELECT new_node_id, node_id FROM new_nodes " .
   	"WHERE $whereclause ORDER BY new_node_id");
    while ($row = mysql_fetch_array($query_result)) {
        $id   = $row["new_node_id"];
        $name = $row["node_id"];
	if (preg_match("/^(.*\D)(\d+)$/",$name,$matches) ||
	    preg_match("/^(.*-)([a-zA-Z])$/",$name,$matches)) {
	    $prefix = $matches[1];
	    $number = $matches[2];
	    $newIP = guess_IP($prefix,$number);
	    if (!$newIP) {
	       echo "<h4>Failed to guess IP for $name!</h4>";
	    } else {
		DBQueryFatal("UPDATE new_nodes SET IP='$newIP' WHERE " .
		    "new_node_id='$id'");
	    }
	} else {
	    echo "Unable to understand the node name $name";
	}
    }
}

#
# Create the nodes in the real database
#
if ($create) {
    $nodenames = array();
    $query_result = DBQueryFatal("SELECT node_id FROM new_nodes " .
	"WHERE $whereclause");
    while ($row = mysql_fetch_array($query_result)) {
        $nodenames[] = $row["node_id"];
    }
    $nodelist = implode(" ",$nodenames);
    echo "<h3>Creating nodes - this could take a while, please wait</h3>\n";
    echo "<hr>\n";
    echo "<pre>\n";
    passthru("$TBSUEXEC_PATH $uid $gid newnode $nodelist 2>&1");
    echo "</pre>\n";
    echo "<hr>\n";
}

#
# Look for the nodes on the switch again
#
if ($research) {
    #
    # Get the MACs we're supposed to be looking for
    #
    $query_result = DBQueryFatal("SELECT i.mac, i.new_node_id, n.node_id, " .
	"i.card, i.card=t.control_net AS is_control " .
	"FROM new_interfaces as i LEFT JOIN new_nodes as n " .
	"ON i.new_node_id = n.new_node_id " .
	"LEFT JOIN node_types as t on n.type = t.type " .
	"WHERE $whereclause_qualified");
    $mac_list = array();
    while ($row = mysql_fetch_array($query_result)) {
        if ($row["is_control"]) {
	    $class = "control";
	} else {
	    $class = "experimental";
	}
        $mac_list[$row["mac"]] = array( "new_node_id" => $row["new_node_id"],
	    "node_id" => $row["node_id"], "card" => $row["card"],
	    "class" => $class);
    }

    echo "<h3>Looking for MACs, this could take a while...</h3>";
    find_switch_macs($mac_list);
    foreach ($mac_list as $mac => $switchport) {
        if ($switchport["switch"]) {
	    DBQueryFatal("UPDATE new_interfaces SET " .
		"switch_id='$switchport[switch]', " .
		"switch_card='$switchport[switch_card]', " .
		"switch_port='$switchport[switch_port]' ".
		"WHERE new_node_id='$switchport[new_node_id]' " .
		"AND card=$switchport[card]");
	} else {
	    echo "<h4>Unable to find $switchport[node_id]:$switchport[card] " .
		"on switches, not updating</h4>\n";
	}
    }
}

#
# Swap the IDs and IPs of two nodes
#
if ($swap) {
    if (count($selected_nodes) != 2) {
       USERERROR("Exactly two nodes must be selected for swapping",0);
    } else {
	$id1 = $selected_nodes[0];
	$id2 = $selected_nodes[1];
	$query_result = DBQueryFatal("SELECT node_id, IP FROM new_nodes " .
		"WHERE new_node_id=$id1");
	$row = mysql_fetch_array($query_result);
	$node_id1 = $row["node_id"];
	$IP1 = $row["IP"];
	$query_result = DBQueryFatal("SELECT node_id, IP FROM new_nodes " .
		"WHERE new_node_id=$id2");
	$row = mysql_fetch_array($query_result);
	$node_id2 = $row["node_id"];
	$IP2 = $row["IP"];

	DBQueryFatal("UPDATE new_nodes SET node_id='$node_id2', IP='$IP2' " .
		"WHERE new_node_id=$id1");
	DBQueryFatal("UPDATE new_nodes SET node_id='$node_id1', IP='$IP1' " .
		"WHERE new_node_id=$id2");
    }
}

#
# Change node types
#
if ($newtype) {
    DBQueryFatal("UPDATE new_nodes SET type='$newtype' WHERE $whereclause");
}

#
# Change node name prefix
#
if ($newprefix || $addnumber) {
   $query_result = DBQueryFatal("SELECT new_node_id, node_id FROM new_nodes " .
   	"WHERE $whereclause");
    while ($row = mysql_fetch_array($query_result)) {
    	$id    = $row['new_node_id'];
	$name  = $row['node_id'];
	if (preg_match("/^(.*\D)(\d+)$/",$name,$matches) ||
	    preg_match("/^(.*-)([a-zA-Z])$/",$name,$matches)) {
	    $prefix = $matches[1];
	    $number = $matches[2];
	    if ($addnumber) {
	        if (is_string($number)) {
		    $number = chr(ord($number) + $addnumber);
		} else {
		    $number = $number + $addnumber;
		}
	    }
	    if ($newprefix) {
	        $prefix = $newprefix;
	    }
	    $newname = $prefix . $number;
	    DBQueryFatal("UPDATE new_nodes SET node_id='$newname' " .
		"WHERE new_node_id='$id'");
	} else {
	    echo "Unable to understand the node name $name";
	}
    }
}

#
# Re-number interfaces
#
if ($renumber) {
    #
    # Move them out of the way
    #
    foreach ($remap as $index => $value) {
        if ($value != "") {
	    DBQueryFatal("UPDATE new_interfaces SET card = card + 100 WHERE " .
	        "card = $index AND ($whereclause)");
	}
    }

    #
    # Move them back to the correct location
    #
    foreach ($remap as $index => $value) {
        if ($value != "") {
	    DBQueryFatal("UPDATE new_interfaces SET card = $value WHERE " .
	        "card = " . ($index + 100) . " AND ($whereclause)");
	}
    }

}

#
# Okay, now get the node information and display the form
#
$query_result = DBQueryFatal("SELECT n.new_node_id, node_id, n.type, IP, " .
	"DATE_FORMAT(created,'%M %e %H:%i:%s') as created, i.MAC, " .
	"i.switch_id, i.switch_card, i.switch_port, n.temporary_IP, n.dmesg " .
	"FROM new_nodes AS n " .
	"LEFT JOIN node_types AS t on n.type=t.type " .
	"LEFT JOIN new_interfaces AS i ON n.new_node_id=i.new_node_id " .
	"    AND t.control_net = i.card " .
	"ORDER BY n.new_node_id");

?>

<h3><a href="newnodes_list.php3">Refresh this page</a></h3>

<form action="newnodes_list.php3" method="get" name="nodeform">

<script language="JavaScript">
<!--
function selectAll(form) {
    for (i = 0; i < form.length; i++)
	form[i].checked = true ;
}

function deselectAll(form) {
    for (i = 0; i < form.length; i++)
	form[i].checked = false ;
}
-->
</script>

<table>
	<tr>
	    <th></th>
	    <th>ID</th>
	    <th>Node ID</th>
	    <th>Type</th>
	    <th>IP</th>
	    <th>Control MAC</th>
	    <th>Control Port</th>
	    <th>Interfaces</th>
	    <th>Temporary IP</th>
	    <th>Created</th>
	    <th>Warnings</th>
	</tr>

<?

while ($row = mysql_fetch_array($query_result)) {
	$id         = $row["new_node_id"];
	$node_id    = $row["node_id"];
	$type       = $row["type"];
	$IP         = $row["IP"];
	$created    = $row["created"];
	$mac        = $row["MAC"];
	$tempIP     = $row["temporary_IP"];
	$dmesg      = $row["dmesg"];
	if ($row["switch_id"]) {
	    $port = "$row[switch_id].$row[switch_card]/$row[switch_port]";
	} else {
	    $port = "unknown";
	}
	$checked = in_array($id,$selected_nodes) ? "checked" : "";

        $iface_query_result = DBQueryFatal("SELECT COUNT(*) AS count " .
	    "FROM new_interfaces WHERE new_node_id=$id");
        $iface_row = mysql_fetch_array($iface_query_result);
	$interfaces = $iface_row["count"];

	echo "	<tr>\n";
	echo "		<td><input type=\"checkbox\" name=\"selected[]\" " .
	    "value='$id' $checked></td>\n";
	echo "		<td><a href=\"newnode_edit.php3?id=$id\">$id</a></td>\n";
	echo "		<td>$node_id</td>\n";
	echo "		<td>$type</td>\n";
	echo "		<td>$IP</td>\n";
	echo "          <td>$mac</td>\n";
	echo "          <td>$port</td>\n";
	echo "		<td>$interfaces</td>\n";
	echo "		<td>$tempIP</td>\n";
	echo "		<td>$created</td>\n";
	if ($dmesg) {
	    echo "		<td><img src=\"redball.gif\"></td>\n";
	} else {
	    echo "		<td><img src=\"greenball.gif\"></td>\n";
	}
	echo "	</tr>\n";
}

?>

<tr>
    <td align="center" colspan=11>
    <input type="button" name="SelectAll" value="Select All"
	onClick="selectAll(document.nodeform.elements['selected[]'])">
    &nbsp;
    <input type="button" name="DeselectAll" value="Deselect All"
	onClick="deselectAll(document.nodeform.elements['selected[]'])">
    </td>
</tr>

</table>

<h3>Actions</h3>

<table>
<tr>
    <th>Set Type</th>
    <td><input type="text" width="10" name="newtype"></td>
</tr>

<tr>
    <th>Set Node ID Prefix (eg. 'pc')</th>
    <td><input type="text" width="10" name="newprefix"></td>
</tr>

<tr>
    <th>Add to Node ID Suffix (integer, can be negative)</th>
    <td><input type="text" width="10" name="addnumber"></td>
</tr>

<tr>
    <td colspan=2 align="center">
    <input type="submit" value="Update selected nodes" name="submit">
    </td>
</tr>
</table>

<br><br>

<table>
<tr>
    <th>0 -&gt;</th>
    <td><input type="text" size=2 name="remap[0]"></td>
</tr>
<tr>
    <th>1 -&gt;</th>
    <td><input type="text" size=2 name="remap[1]"></td>
</tr>
<tr>
    <th>2 -&gt;</th>
    <td><input type="text" size=2 name="remap[2]"></td>
</tr>
<tr>
    <th>3 -&gt;</th>
    <td><input type="text" size=2 name="remap[3]"></td>
</tr>
<tr>
    <th>4 -&gt;</th>
    <td><input type="text" size=2 name="remap[4]"></td>
</tr>
</table>
    <input type="submit" value="Re-number interfaces of selected nodes"
	name="renumber">

<br><br>

<input type="submit" value="Recalculate IPs for selected nodes" name="calc">
<br><br>
<input type="submit" value="Swap Node IDs and IPs for selected nodes" name="swap">
<br><br>
<input type="submit" value="Search switch ports for selected nodes" name="research">
<br><br>
<input type="submit" value="Create selected nodes" name="create">
<br><br>
<input type="submit" value="Delete selected nodes" name="delete">

</form>

<?

#
# Standard Testbed Footer
# 
PAGEFOOTER();

?>
