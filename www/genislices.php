<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2011 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("geni_defs.php");
include("table_defs.php");

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
$optargs = OptionalPageArguments("showtype",   PAGEARG_STRING);
$showtypes = array();

if (!isset($showtype)) {
    $showtypes[] = "cm";
    $showtypes[] = "sa";
    $showtypes[] = "ch";
}
else {
    if (! ($showtype == "sa"|| $showtype == "cm" || $showtype == "ch")) {
	USERERROR("Improper argument: showtype=$showtype", 1);
    }
    $showtypes[] = $showtype;
}

#
# Standard Testbed Header
#
PAGEHEADER("Geni Slice List");

if (! $isadmin) {
    USERERROR("You do not have permission to view Geni slice list!", 1);
}

foreach ($showtypes as $type) {
    $slicelist = GeniSlice::AllSlices($type);
    $which = ($type == "cm" ? "Component Manager" :
	      ($type == "sa" ? "Slice Authority" : "Clearing House"));

    if (!$slicelist || !count($slicelist))
	continue;

    # The form attributes:
    $table = array('#id'       => $type,
		   '#title'    => $which,
		   '#sortable' => 1,
		   '#headings' => array("idx"          => "ID",
					"hrn"          => "HRN",
					"created"      => "Created",
					"expires"      => "Expires"));

    $rows = array();

    foreach ($slicelist as $slice) {
	$slice_idx  = $slice->idx();
	$slice_hrn  = $slice->hrn();
	$created    = $slice->created();
	$expires    = $slice->expires();

	$url = CreateURL("showslice", "showtype", $type,
			 "slice_idx", $slice_idx);
	$href = "<a href='$url'>$slice_hrn</a>";

	$experiment = Experiment::LookupByUUID($slice->uuid());
	if ($experiment) {
	    $eid     = $experiment->eid();
	    $expurl  = CreateURL("showexp", $experiment);
	    $href    = "$href (<a href='$expurl'>$eid</a>)";
	}
	$rows[$slice_idx] = array("idx"       => $slice_idx,
				  "hrn"       => $href,
				  "created"   => $created,
				  "expires"   => $expires);
    }
    list ($html, $button) = TableRender($table, $rows);
    echo $html;
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

