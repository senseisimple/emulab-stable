<?PHP
#
# EMULAB-COPYRIGHT
# Copyright (c) 2003, 2004, 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

#
# List the nodes that have checked in and are awaint being added the the real
# testbed
#

#
# Standard Testbed Header
#
PAGEHEADER("New Testbed Node");

#
# Only admins can see this page
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

if (! $isadmin) {
    USERERROR("You do not have admin privileges!", 1);
}

$reqargs = RequiredPageArguments("id",         PAGEARG_STRING);
$optargs = OptionalPageArguments("node_id",    PAGEARG_STRING,
				 "type",       PAGEARG_STRING,
				 "IP",         PAGEARG_STRING,
				 "identifier", PAGEARG_STRING);

#
# If we had any update information passed to us, do the update now
#
if (isset($node_id)) {
    DBQueryFatal("UPDATE new_nodes SET node_id='$node_id', type='$type', " .
    	"IP='$IP', identifier='$identifier' WHERE new_node_id='$id'");
}

#
# Same for interface update information
#
foreach ($HTTP_GET_VARS as $key => $value) {
    if (preg_match("/iface(\d+)_mac/",$key,$matches)) {
    	$card        = $matches[1];
    	$mac         = $HTTP_GET_VARS["iface${card}_mac"];
    	$type        = $HTTP_GET_VARS["iface${card}_type"];
    	$switch_id   = $HTTP_GET_VARS["iface${card}_switch_id"];
    	$switch_card = $HTTP_GET_VARS["iface${card}_switch_card"];
    	$switch_port = $HTTP_GET_VARS["iface${card}_switch_port"];
    	$cable       = $HTTP_GET_VARS["iface${card}_cable"];
    	$len         = $HTTP_GET_VARS["iface${card}_len"];
    	DBQueryFatal("UPDATE new_interfaces SET mac='$mac', " .
	    "interface_type='$type', switch_id='$switch_id', " .
	    "switch_card='$switch_card', switch_port='$switch_port', " .
	    "cable='$cable', len='$len' " .
	    "WHERE new_node_id=$id AND card='$card'");
    }
}

#
# Get the information about the node they asked for
#
$query_result = DBQueryFatal("SELECT new_node_id, node_id, type, IP, " .
	"DATE_FORMAT(created,'%M %e %H:%i:%s') as created, dmesg, " .
	"identifier, building " .
	"FROM new_nodes WHERE new_node_id='$id'");

if (mysql_num_rows($query_result) != 1) {
    USERERROR("Error getting information for node ID $id",1);
}

$row = mysql_fetch_array($query_result)

?>

<h4><a href="newnodes_list.php3">Back to the new node list</a></h4>

<form action="newnode_edit.php3" method="get">

<input type="hidden" name="id" value="<?=$id?>">

<h3 align="center">Node</h3>

<table align="center">
<tr>
    <th>ID</th>
    <td><?= $row['new_node_id'] ?></td>
</tr>
<tr>
    <th>Node ID</th>
    <td>
    <input type="text" width=10 name="node_id" value="<?=$row['node_id']?>">
    </td>
</tr>
<tr>
    <th>Identifier</th>
    <td>
    <input type="text" width=10 name="identifier" value="<?=$row['identifier']?>">
    </td>
</tr>
<tr>
    <th>Type</th>
    <td>
    <input type="text" width=10 name="type" value="<?=$row['type']?>">
    </td>
</tr>
<tr>
    <th>IP</th>
    <td>
    <input type="text" width=10 name="IP" value="<?=$row['IP']?>">
    </td>
</tr>
<tr>
    <th>Created</th>
    <td><?= $row['created'] ?></td>
</tr>
<tr>
    <th>Location</th>
    <td>
    <? if ($row['building']) {
	echo "Set (<a href=setnodeloc.php3?node_id=$id&isnewid=1>Change</a>)\n";
       } else {
	echo "Unset (<a href=setnodeloc.php3?node_id=$id&isnewid=1>Set</a>)\n";
       }
    ?>
    </td>
</tr>
<tr>
    <th>dmesg Output</th>
    <td><?= $row['dmesg'] ?></td>
</tr>
</table>

<h3 align="center">Interfaces</h3>

<em>Note:</em>Cable and Length are for informational use only, and are optional.

<table align="center">
<tr>
    <th>Interface</th>
    <th>MAC</th>
    <th>Type</th>
    <th>Switch</th>
    <th>Card</th>
    <th>Port</th>
    <th>Cable</th>
    <th>Length</th>
</tr>

<?

$query_result = DBQueryFatal("SELECT card, mac, interface_type, switch_id, " .
	"switch_card, switch_port, cable, len FROM new_interfaces " .
	"where new_node_id=$id");
while ($row = mysql_fetch_array($query_result)) {
    $card        = $row['card'];
    $mac         = $row['mac'];
    $type        = $row['interface_type'];
    $switch_id   = $row['switch_id'];
    $switch_card = $row['switch_card'];
    $switch_port = $row['switch_port'];
    $cable       = $row['cable'];
    $len         = $row['len'];
    echo "<tr>\n";
    echo "<td>$card</td>\n";
    echo "<td><input type='text' name='iface${card}_mac' size=12 " .
	"value='$mac'></td>\n";
    echo "<td><input type='text' name='iface${card}_type' size=5 " .
	"value='$type'></td>\n";
    echo "<td><input type='text' name='iface${card}_switch_id' size=10 " .
	"value='$switch_id'></td>\n";
    echo "<td><input type='text' name='iface${card}_switch_card' size=3 " .
	"value='$switch_card'></td>\n";
    echo "<td><input type='text' name='iface${card}_switch_port' size=3 " .
	"value='$switch_port'></td>\n";
    echo "<td><input type='text' name='iface${card}_cable' size=5 " .
	"value='$cable'></td>\n";
    echo "<td><input type='text' name='iface${card}_len' size=3 " .
	"value='$len'></td>\n";
    echo "</tr>\n";
}

?>

</table>

<br>

<center>
<input type="submit" name="submit" value="Update node">
</center>
<?


#
# Standard Testbed Footer
# 
PAGEFOOTER();

?>
