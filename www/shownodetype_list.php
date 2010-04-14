<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2010 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("table_defs.php");

#
#
# Only known and logged in users allowed.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Standard Testbed Header
#
PAGEHEADER("Image List");

#
# Get the list.
#
$query_result =
    DBQueryFatal("select * from node_types order by class,type");

if ($isadmin) {
    SUBPAGESTART();
    SUBMENUSTART("More Options");
    WRITESUBMENUBUTTON("Create a PC type",
		       "editnodetype.php3?new_type=1&node_class=pc");
    WRITESUBMENUBUTTON("Create a Switch type",
		       "editnodetype.php3?new_type=1&node_class=switch");
    SUBMENUEND();
}
echo "<br>";
echo "Note that many types in this list are not accessible to most users";
if ($isadmin) {
    SUBPAGEEND();
}

if (mysql_num_rows($query_result)) {
    $table = array('#id'	   => 'nodetypelist',
		   '#title'        => "Node Type List",
		   '#sortable'     => "yes",
		   '#headings'     => array("class"        => "Class",
					    "type"         => "Type",
					    "isvirt"       => "IsVirt",
					    "isrem"        => "IsRem",
					    "isdyn"        => "IsDyn",
					    "isfed"        => "IsFed",
					    "issw"         => "IsSwch",
					    "isjail"       => "IsJail"));
    $rows = array();
    
    while ($row = mysql_fetch_array($query_result)) {
	$type = $row["type"];
	$url  = "<a href='shownodetype.php3?node_type=$type'>$type</a>";
	
	$tablerow = array("class"        => $row["class"],
			  "type"         => $url,
			  "isvirt"       => $row["isvirtnode"],
			  "isrem"        => $row["isremotenode"],
			  "isdyn"        => $row["isdynamic"],
			  "isfed"        => $row["isfednode"],
			  "issw"         => $row["isswitch"],
			  "isjail"       => $row["isjailed"]);

	$rows[]  = $tablerow;
    }
    list ($html, $button) = TableRender($table, $rows);
    echo $html;
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
