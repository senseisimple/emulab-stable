<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2009 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("geni_defs.php");
include("table_defs.php");
include_once('JSON.php');

#
#
# Only known and logged in users allowed.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify Page Arguments.
#
$optargs = OptionalPageArguments("showtype",   PAGEARG_STRING,
				 "index",      PAGEARG_INTEGER);
$showtypes = array();

if (!isset($showtype)) {
    $showtypes["aggregates"] = "aggregates";
    $showtypes["tickets"]    = "tickets";
    $index = 0;
}
else {
    if (! ($showtype == "aggregates"|| $showtype == "tickets")) {
	USERERROR("Improper argument: showtype=$showtype", 1);
    }
    $showtypes[$showtype] = $showtype;
}
if (!isset($index)) {
    $index = 0;
}

#
# Standard Testbed Header
#
PAGEHEADER("Geni History");

if (! $isadmin) {
    USERERROR("You do not have permission to view Geni slice list!", 1);
}

if (isset($showtypes["aggregates"])) {
    $myindex = $index;
    $dblink  = GetDBLink("cm");
    $clause  = ($myindex ? "and idx<$myindex " : "");

    $query_result =
	DBQueryFatal("select * from aggregate_history ".
		     "where `type`='Aggregate' $clause ".
		     "order by idx desc limit 20",
		     $dblink);

    $table = array('#id'	   => 'aggregate',
		   '#title'        => "Aggregate History",
		   '#headings'     => array("idx"          => "ID",
					    "slice_hrn"    => "Slice HRN",
					    "creator_hrn"  => "Creator HRN",
					    "created"      => "Created",
					    "Destroyed"    => "Destroyed",
					    "Manifest"     => "Manifest"));
    $rows = array();

    if (mysql_num_rows($query_result)) {
	while ($row = mysql_fetch_array($query_result)) {
	    $idx         = $row["idx"];
	    $uuid        = $row["uuid"];
	    $slice_hrn   = $row["slice_hrn"];
	    $creator_hrn = $row["creator_hrn"];
	    $created     = $row["created"];
	    $destroyed   = $row["destroyed"];

	    $tablerow = array("idx"       => $idx,
			      "hrn"       => $slice_hrn,
			      "creator"   => $creator_hrn,
			      "created"   => $created,
			      "destroyed" => $destroyed);

	    $manifest_result =
		DBQueryFatal("select * from manifest_history ".
			     "where aggregate_uuid='$uuid' ".
			     "order by idx desc limit 1", $dblink);

	    if (mysql_num_rows($manifest_result)) {
		$mrow = mysql_fetch_array($manifest_result);
		$manifest = $mrow["manifest"];
		$manifest = $json->encode($manifest);
	    
		$tablerow["manifest"] =
		    "<a href='#' title='' ".
		    "onclick='PopUpWindow($manifest);'>".
		    "manifest</a>";
	    }
	    else {
		$tablerow["Manifest"] = "Unknown";
	    }
	    $rows[]  = $tablerow;
	    $myindex = $idx;
	}
	list ($html, $button) = TableRender($table, $rows);
	echo $html;

	$query_result =
	    DBQueryFatal("select count(*) from aggregate_history ".
			 "where `type`='Aggregate' and idx<$myindex",
			 $dblink);

	$row = mysql_fetch_array($query_result);
	$remaining = $row[0];

	if ($remaining) {
	    echo "<center>".
	      "<a href='genihistory.php?index=$myindex&showtype=aggregates'>".
	      "More Entries</a></center><br>\n";
	}
    }
}

if (isset($showtypes["tickets"])) {
    $myindex = $index;
    $dblink  = GetDBLink("cm");
    $clause  = ($myindex ? "where idx<$myindex " : "");

    $query_result =
	DBQueryFatal("select * from ticket_history ".
		     "$clause ".
		     "order by idx desc limit 20",
		     $dblink);

    $table = array('#id'	   => 'tickets',
		   '#title'        => "Ticket History",
		   '#headings'     => array("idx"          => "ID",
					    "slice_hrn"    => "Slice HRN",
					    "owner_hrn"    => "Owner HRN",
					    "created"      => "Created",
					    "redeemed"     => "Redeemed",
					    "expired"      => "Expired",
					    "released"     => "Released",
					    "rspec"        => "Rspec"));
    $rows = array();

    if (mysql_num_rows($query_result)) {
	$json = new Services_JSON();
	
	while ($row = mysql_fetch_array($query_result)) {
	    $idx         = $row["idx"];
	    $uuid        = $row["uuid"];
	    $slice_hrn   = $row["slice_hrn"];
	    $owner_hrn   = $row["owner_hrn"];
	    $created     = $row["created"];
	    $redeemed    = $row["redeemed"];
	    $expired     = $row["expired"];
	    $released    = $row["released"];
	    $rspec       = $row["rspec_string"];

	    $tablerow = array("idx"       => $idx,
			 "slice"     => $slice_hrn,
			 "owner"     => $owner_hrn,
			 "created"   => $created,
			 "redeemed"  => $redeemed,
			 "expired"   => $expired,
			 "released"  => $released);

	    if ($rspec) {
		$rspec = $json->encode($rspec);
	    
		$tablerow["rspec"] =
		    "<a href='#' title='' ".
		    "onclick='PopUpWindow($rspec);'>".
		    "rspec</a>";
	    }
	    else {
		$tablerow["rspec"] = "Unknown";
	    }

	    $rows[]  = $tablerow;
	    $myindex = $idx;
	}
	list ($html, $button) = TableRender($table, $rows);
	echo $html;

	$query_result =
	    DBQueryFatal("select count(*) from ticket_history ".
			 "where idx<$myindex",
			 $dblink);

	$row = mysql_fetch_array($query_result);
	$remaining = $row[0];

	if ($remaining) {
	    echo "<center>".
		"<a href='genihistory.php?index=$myindex&showtype=tickets'>".
		"More Entries</a></center><br>\n";
	}
    }
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

