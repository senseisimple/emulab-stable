<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Pretty Pictures of (Recently) Active Experiments");

#
# We let anyone access this page. No details are leaked out, hopefully.
#
$thumbCount = 0;

function GENPLIST ($which, $query_result)
{
    global $thumbCount, $TBOPSPID, $TB_EXPTSTATE_ACTIVE;

    echo "<center><h3>$which</h3></center>\n";
    echo "<table border=2 cols=4 cellpadding=2
                 cellspacing=2 align=center><tr>";

    while (($row = mysql_fetch_array($query_result)) && $thumbCount < 100) {
	$pid        = $row["pid"];
	$eid        = $row["eid"];
	$pnodes     = $row["pnodes"];
	$thumb_hash = $row["thumb_hash"];
	$swapdate   = $row["swapdate"];
	$state      = $row["state"];

	if ($pid == $TBOPSPID || $pnodes == 0 || !isset($thumb_hash) ||
	    !isset($swapdate)) {
	    continue;
	}
	if ($state == $TB_EXPTSTATE_ACTIVE) {
	    $swapdate = "";
	}
	else {
	    $swapdate = "<font size=-1><br>$swapdate</font>";
	}

	echo "<td align=center>";
	echo "<img border=1 width=128 height=128 class='stealth' ".
	    " src='thumbs/tn$thumb_hash.png'>".
	    " $swapdate" .
	    "</td>";

	$thumbcount++;
	if (($thumbcount % 4) == 0) {
	    echo "</tr><tr>\n";
	}
    }
    echo "</tr></table>";
}

#
# Active Experiments.
# 
$query_result =
    DBQueryFatal("select e.pid,e.eid,e.state,rs.pnodes,ve.thumb_hash, ".
		 "       s.swapin_last as swapdate ".
		 " from experiments as e ".
		 "left join vis_experiments as ve on ".
		 "     ve.pid=e.pid and ve.eid=e.eid ".
		 "left join experiment_stats as s on s.exptidx=e.idx ".
		 "left join experiment_resources as rs on rs.idx=s.rsrcidx ".
		 "where rs.pnodes!=0 and ".
		 "      e.state='" . $TB_EXPTSTATE_ACTIVE . "' " .
		 "order by s.swapin_last desc ");

if (mysql_num_rows($query_result)) {
    GENPLIST("Active Experiments", $query_result);
}

$query_result =
    DBQueryFatal("select e.pid,e.eid,e.state,rs.pnodes,ve.thumb_hash, ".
		 "       s.swapout_last as swapdate ".
		 " from experiments as e ".
		 "left join vis_experiments as ve on ".
		 "     ve.pid=e.pid and ve.eid=e.eid ".
		 "left join experiment_stats as s on s.exptidx=e.idx ".
		 "left join experiment_resources as rs on rs.idx=s.rsrcidx ".
		 "where rs.pnodes-rs.delaynodes>2 and ".
		 "      e.state='" . $TB_EXPTSTATE_SWAPPED . "' " .
		 "order by s.swapout_last desc ".
		 "limit 50");

if (mysql_num_rows($query_result)) {
    GENPLIST("Recently Active Experiments", $query_result);
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>


