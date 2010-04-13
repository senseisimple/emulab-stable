<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2010 University of Utah and the Flux Group.
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
$reqargs = RequiredPageArguments("slice_idx",   PAGEARG_INTEGER);
$optargs = OptionalPageArguments("showtype",   PAGEARG_STRING);

if (!isset($showtype)) {
    $showtype='sa';
}

#
# Standard Testbed Header
#
PAGEHEADER("Geni Slice");

if (! $isadmin) {
    USERERROR("You do not have permission to view Geni slices!", 1);
}

if (! ($showtype == "sa"|| $showtype == "cm" || $showtype == "ch")) {
    USERERROR("Improper argument: showtype=$showtype", 1);
}

$slice = GeniSlice::Lookup($showtype, $slice_idx);
if (!$slice) {
    USERERROR("No such slice $slice_idx", 1);
}

# The table attributes:
$table = array('#id'	   => 'form1',
	       '#title'    => "Slice $slice_idx ($showtype)");
$rows = array();

$rows[] = array("idx"      => $slice->idx());
$rows[] = array("hrn"      => $slice->hrn());
$rows[] = array("uuid"     => $slice->uuid());
$rows[] = array("created"  => $slice->created());
$rows[] = array("expires"  => $slice->expires());
if ($slice->locked()) {
    $rows[] = array("locked"  => $slice->locked());
}
if (($manifest = $slice->GetManifest())) {
    $json = new Services_JSON();
    $manifest = $json->encode($manifest);
    $rows[] = array("manifest" =>
		    "<a href='#' title='' ".
		    "onclick='PopUpWindow($manifest);'>".
		    "Click to View</a>");
}

$experiment = Experiment::LookupByUUID($slice->uuid());
if ($experiment) {
    $eid = $experiment->eid();
    $exptidx = $experiment->idx();
    $url = CreateURL("showexp", $experiment);
    $rows[] = array("Experiment"  => "<a href='$url'>$eid ($exptidx)</a>");
}

$geniuser = GeniUser::Lookup($showtype, $slice->creator_uuid());
if ($geniuser) {
    $rows[] = array("Creator" => $geniuser->hrn());
}
else {
    $user = User::LookupByUUID($slice->creator_uuid());
    if ($user) {
	$rows[] = array("Creator" => $user->uid());
    }
}

if ($showtype != "sa") {
    $saslice = GeniSlice::Lookup("sa", $slice->uuid());
    if ($saslice) {
	$saidx = $saslice->idx();
	$url   = CreateURL("showslice", "slice_idx", $saidx, "showtype", "sa");

	$rows[] = array("SA Slice" => "<a href='$url'>$saidx</a>");
    }
}
if ($showtype != "cm") {
    $cmslice = GeniSlice::Lookup("cm", $slice->uuid());
    if ($cmslice) {
	$cmidx = $cmslice->idx();
	$url   = CreateURL("showslice", "slice_idx", $cmidx, "showtype", "cm");

	$rows[] = array("CM Slice" => "<a href='$url'>$cmidx</a>");
    }
}

list ($html, $button) = TableRender($table, $rows);
echo $html;

$clientslivers = ClientSliver::SliverList($slice);
if ($clientslivers && count($clientslivers)) {
    $table = array('#id'	   => 'clientslivers',
		   '#title'        => "Client Slivers",
		   '#headings'     => array("idx"      => "ID",
					    "urn"      => "URN",
					    "manager"  => "Manager URN",
					    "created"  => "Created",
					    "manifest" => "Manifest"));
    $rows = array();

    foreach ($clientslivers as $clientsliver) {
	$row = array("idx"      => $clientsliver->idx(),
		     "urn"      => $clientsliver->urn(),
		     "manager"  => $clientsliver->manager_urn(),
		     "created"  => $clientsliver->created());

	if ($clientsliver->manifest()) {
	    $json = new Services_JSON();
	    $manifest = $json->encode($clientsliver->manifest());
	    
	    $row["manifest"] =
		"<a href='#' title='' ".
		"onclick='PopUpWindow($manifest);'>".
		"Manifest</a>";
	}
	else {
	    $row["manifest"] = "Unknown";
	}
	$rows[] = $row;
    }

    list ($html, $button) = TableRender($table, $rows);
    echo $html;
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

