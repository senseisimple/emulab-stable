<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2010 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("imageid_defs.php");

#
# Anyone can access this info, its a PUBLIC PAGE!
# Get current user if there is one.
#
$this_user = CheckLogin($check_status);
$reqargs   = RequiredPageArguments("node_type", PAGEARG_STRING);

# Sanitize.
if (!preg_match("/^[-\w]+$/", $node_type)) {
    PAGEARGERROR("Invalid characters in arguments.");
}

#
# Standard Testbed Header
#
PAGEHEADER("Node Type Information");

$query_result =
    DBQueryFatal("select * from node_types ".
		 "where type='$node_type'");

if (! mysql_num_rows($query_result) != 0) {
    USERERROR("No such node_type $node_type!", 1);
}
$noderow = mysql_fetch_array($query_result);

if ($this_user && ISADMIN()) {
    SUBPAGESTART();
    SUBMENUSTART("More Options");
    WRITESUBMENUBUTTON("Edit this type",
		       "editnodetype.php3?node_type=$node_type");
    WRITESUBMENUBUTTON("Create a PC type",
		       "editnodetype.php3?new_type=1&node_class=pc");
    WRITESUBMENUBUTTON("Create a Switch type",
		       "editnodetype.php3?new_type=1&node_class=switch");
    SUBMENUEND();
    SUBPAGEEND();
}

echo "<table border=2 cellpadding=0 cellspacing=2
             align=center>\n";

# Stuff from the node types table.
$class	 = $noderow["class"];
$options = array("isvirtnode",
		 "isdynamic",
		 "isjailed",
		 "isremotenode",
		 "issubnode",
		 "isplabdslice",
		 "isgeninode",
		 "isfednode",
		 "isswitch");

echo "<tr>
      <td>Type:</td>
      <td class=left>$node_type</td>
          </tr>\n";

echo "<tr>
      <td>Class:</td>
      <td class=left>$class</td>
          </tr>\n";

foreach ($options as $option) {
    $value = $noderow[$option];

    if ($value) {
	echo "<tr>
               <td>$option:</td>
               <td class=left>Yes</td>
              </tr>\n";
    }
}

#
# And now all of the attributes ...
#
# Grab the attributes for the type.
$query_result = DBQueryFatal("select * from node_type_attributes ".
			     "where type='$node_type' ".
			     "order by attrkey");
if (mysql_num_rows($query_result)) {
    echo "<tr></tr>\n";

    while ($row = mysql_fetch_array($query_result)) {
	$key      = $row["attrkey"];
	$val      = $row["attrvalue"];
	$attrtype = $row["attrtype"];
	
	if ($key == "default_osid" ||
	    $key == "jail_osid" ||
	    $key == "delay_osid") {
	    if ($osinfo = OSinfo::Lookup($val)) {
		$val = $osinfo->osname();
	    }
	}
	elseif ($key == "default_imageid") {
	    if ($image = Image::Lookup($val)) {
		$val = $image->imagename();
	    }
	}
	echo "<tr>\n";
	echo "<td>$key:</td>\n";
	echo "<td class=left>$val</td>\n";
	echo "</tr>\n";
    }
    echo "</table>\n";
}

#
# Suck out info for all the nodes of this type. We are going to show
# just a list of dots, in two color mode.  Note, we also check that the
# physical node is free, see note in nodecontrol_list.php3 for why.
#
$query_result =
    DBQueryFatal("select n.node_id,n.eventstate,ifnull(r.pid,rp.pid) as pid ".
		 "from nodes as n ".
		 "left join node_types as nt on n.type=nt.type ".
		 "left join reserved as r on n.node_id=r.node_id ".
		 "left join reserved as rp on n.phys_nodeid=rp.node_id ".
		 "where nt.type='$node_type' and ".
		 "      (role='testnode' or role='virtnode') ".
		 "ORDER BY priority");


if (mysql_num_rows($query_result)) {
    echo "<br>
          <center>
	  Nodes (<a href=nodecontrol_list.php3?showtype=$node_type>Show details</a>)
	  <br>
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




