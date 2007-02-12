<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# We let anyone access this page. No details are leaked out, hopefully.
#
$optargs = OptionalPageArguments("keymaster", PAGEARG_STRING);

# Locals
$thumbCount = 0;
$thumbMax   = 0;

function GENPLIST ($which, $query_result)
{
    global $thumbCount, $thumbMax, $TBOPSPID, $TB_EXPTSTATE_ACTIVE;

    echo "<center><h3>$which</h3></center>\n";
    echo "<table border=2 cols=4 cellpadding=2
                 cellspacing=2 align=center><tr>";

    while (($row = mysql_fetch_array($query_result)) &&
	   ($thumbMax == 0 || $thumbCount < $thumbMax)) {
	$pid        = $row["pid"];
	$eid        = $row["eid"];
	$pnodes     = $row["pnodes"];
	$swapdate   = $row["swapdate"];
	$state      = $row["state"];
	$rsrcidx    = $row["rsrcidx"];

	if ($pid == $TBOPSPID || $pnodes == 0) {
	    continue;
	}
	if ($state == $TB_EXPTSTATE_ACTIVE) {
	    $swapdate = "";
	}
	else {
	    $swapdate = "<font size=-1><br>$swapdate</font>";
	}

	echo "<td align=center>";
	echo "<img border=1 class='stealth' ".
	    " src='showthumb.php3?idx=$rsrcidx'>".
	    " $swapdate" .
	    "</td>";

	$thumbCount++;
	if (($thumbCount % 4) == 0) {
	    echo "</tr><tr>\n";
	}
    }
    echo "</tr></table>";
}

#
# Standard Testbed Header
#
if (isset($keymaster) &&
    strcmp($keymaster, "Clortho") == 0) {
    PAGEHEADER("All Experiments");

    $query_result =
	DBQueryFatal("select s.pid,s.eid,'terminated',s.rsrcidx,rs.pnodes, ".
		     "       s.swapin_last as swapdate ".
		     " from experiment_stats as s ".
		     "left join experiment_resources as rs on ".
		     "     rs.idx=s.rsrcidx ".
		     "where rs.pnodes>0 and rs.thumbnail is not null and ".
		     "      s.pid!='emulab-ops' ".
		     "order by s.swapin_last desc ");

    if (mysql_num_rows($query_result)) {
	GENPLIST("Experiments Run on Emulab", $query_result);
    }

    PAGEFOOTER();
    return;
}
else {
    PAGEHEADER("Active and Recently Swapped-out Experiments");
}

#
# Active Experiments.
# 
$query_result =
    DBQueryFatal("select e.pid,e.eid,e.state,s.rsrcidx,rs.pnodes, ".
		 "       s.swapin_last as swapdate ".
		 " from experiments as e ".
		 "left join experiment_stats as s on s.exptidx=e.idx ".
		 "left join experiment_resources as rs on rs.idx=s.rsrcidx ".
		 "where rs.pnodes>0 and rs.thumbnail is not null and ".
		 "      e.state='" . $TB_EXPTSTATE_ACTIVE . "' and " .
		 "      e.pid!='emulab-ops' and ".
		 "      not (e.pid='ron' and e.eid='all') ".
		 "order by s.swapin_last desc ");

echo "<a NAME=active></a>\n";
if (mysql_num_rows($query_result)) {
    GENPLIST("Active Experiments", $query_result);
}

$query_result =
    DBQueryFatal("select e.pid,e.eid,e.state,s.rsrcidx,rs.pnodes, ".
		 "       s.swapout_last as swapdate ".
		 " from experiments as e ".
		 "left join experiment_stats as s on s.exptidx=e.idx ".
		 "left join experiment_resources as rs on rs.idx=s.rsrcidx ".
		 "where rs.pnodes-rs.delaynodes>2 and ".
		 "      rs.thumbnail is not null and ".
		 "      e.state='" . $TB_EXPTSTATE_SWAPPED . "' " .
		 "order by s.swapout_last desc ".
		 "limit 200");

echo "<a NAME=swapped></a>\n";
if (mysql_num_rows($query_result)) {
    $thumbCount = 0;
    $thumbMax   = 80;
    GENPLIST("Recently Swapped-out Experiments", $query_result);
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
