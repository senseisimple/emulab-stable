<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2003, 2004, 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

# a few constants
$DEF_LIMIT = 20;
$DEF_NUMPAGENO = 10;

#
# Standard Testbed Header
#
# cheat and use the page args to influence the view.
$view = array();
if (isset($_REQUEST['pagelayout']) && $_REQUEST['pagelayout'] == "minimal") {
    $view = array ('hide_banner' => 1, 'hide_sidebar' => 1 );
}

PAGEHEADER("PlanetLab Metrics",$view);

#
# Only known and logged in users can get plab data.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

# make sure to add any new args here to pm_buildurl() as well
$optargs = OptionalPageArguments( "cols",   PAGEARG_ALPHALIST,
				  "offset", PAGEARG_INTEGER,
				  "limit",  PAGEARG_INTEGER,
				  "upnodefilter", PAGEARG_STRING,
				  "pagelayout", PAGEARG_BOOLEAN,
				  "experiment", PAGEARG_EXPERIMENT,
				  "sortcols", PAGEARG_ALPHALIST,
				  "sortdir", PAGEARG_STRING );

# link to show/hide sidebar

# basic options
#   which cols (including ordering somehow, probably javascript)
#   upnodefilter (emulab|comon|none) (also show brief totals here ---
#     emulab=330, comon=400, none=880)
#   hostname filter (basically, text to a like "%<text>%" sql expr)
#   enable/disable selection; show current selection and be able to remove 
#     nodes from it!
#   drop-down proj/exp list; only show nodes in that pid/eid (and only include
#     exps with planetlab nodes; admins see all exps with planetlab nodes)

# advanced options -- hideable div
#   constrain columns
#     some sort of query language, sigh -- maybe can find a php lib for it
#   build-your-own rank (based off column weights, normalization to max or 
#     user-chosen ceil; each col should be able to be minimized/maximized in 
#     its contribution to the rank)

# should provide cotop links (one node-centric, one slice-centric)
# paint comon columns red, emulab green, or something like that.


# A map of colnames the user can refer to, to their name in queries we issue.
$colmap = array( 'node_id' => 'pm.node_id',
		 'hostname' => 'pm.hostname',
		 'plab_id' => 'pm.hostname',
                 # comon columns
		 'resptime' => 'pcd.resptime','uptime' => 'pcd.uptime',
		 'lastcotop' => 'pcd.lastcotop','date' => 'pcd.date',
		 'drift' => 'pcd.drift','cpuspeed' => 'pcd.cpuspeed',
		 'busycpu' => 'pcd.busycpu','syscpu' => 'pcd.syscpu',
		 'freecpu' => 'pcd.freecpu','1minload' => 'pcd.1minload',
		 '5minload' => 'pcd.5minload','numslices' => 'pcd.numslices',
		 'liveslices' => 'pcd.liveslices','connmax' => 'pcd.connmax',
		 'connavg' => 'pcd.connavg','timermax' => 'pcd.timermax',
		 'timeravg' => 'pcd.timeravg','memsize' => 'pcd.memsize',
		 'memact' => 'pcd.memact','freemem' => 'pcd.freemem',
		 'swapin' => 'pcd.swapin','swapout' => 'pcd.swapout',
		 'diskin' => 'pcd.diskin','diskout' => 'pcd.diskout',
		 'gbfree' => 'pcd.gbfree','swapused' => 'pcd.swapused',
		 'bwlimit' => 'pcd.bwlimit','txrate' => 'pcd.txrate',
		 'rxrate' => 'pcd.rxrate',
                 # emulab columns
		 'unavail' => 'pnhs.unavail',
		 'jitdeduction' => 'pnhs.jitdeduct',
		 'successes' => 'pnhs.succnum',
		 'failures' => 'pnhs.failnum',
		 'nodestatus' => 'ns.status',
		 'nodestatustime' => 'ns.status_timestamp',
                 # what about what kind of link the node contains 
		 # (i.e., inet,inet2,dsl,...) ?
	       );

#
# First, deal with args:
#

# access control -- everybody can see stats for all nodes, but only members
# of a pid can see which nodes an eid in that pid has.
if (isset($experiment)) {
    # need these later
    $eid = $experiment->eid();
    $pid = $experiment->pid();

    if (!$experiment->AccessCheck($this_user, $TB_EXPT_READINFO)) {
	USERERROR("You do not have permission to view experiment $eid!",1);
    }
}

# we have to fix which node id we select based on if we're querying the 
# reserved table (we need phys node id, but have to join reserved to nodes
# to get it)
if (isset($experiment)) {
    $colmap['node_id'] = 'n.phys_nodeid';
}

# strip off the table prefixes; mysql driver doesn't seem to include them
$colmap_mysqlnames = array();
foreach ($colmap as $k => $v) {
    $sa = explode('.',$v);
    $nv = $sa[0];
    if (count($sa) == 2) {
	$nv = $sa[1];
    }
    elseif (count($sa) > 2) {
        $nv = implode('.',array_slice($sa,1));
    }

    $colmap_mysqlnames[$k] = $nv;
}

# default columns displayed, in this order.
# note that they can't get rid of node_id.
$defcols = array( 'hostname','lastcotop','5minload','freemem',
	          'txrate','rxrate','unavail','nodestatus','nodestatustime' );

if (!isset($cols) || count($cols) == 0) {
    $cols = $defcols;
}

#
# Display search options and selection:
#


#
# Next, build query:
#

# Grab query parts
$qbits = pm_buildqueryinfo();
$select_s = $qbits[0]; $src_s = $qbits[1]; $filter_s = $qbits[2]; 
$sort_s = $qbits[3]; $pag_s = $qbits[4];

# query that counts num data rows
$qcount = "select count(" . $colmap['node_id'] . ") as num" . 
    " from $src_s $filter_s";

# query that gets data
$q = "select $select_s from $src_s $filter_s $sort_s $pag_s";

$qres = DBQueryFatal($qcount);
if (mysql_num_rows($qres) != 1) {
    TBERROR("Unexpected number of rows in count query!");
}
$row = mysql_fetch_array($qres);
$totalrows = $row['num'];

#
# Finally, display data:
#

pm_showtable($totalrows,$q);

#
# Standard Testbed Footer
#
PAGEFOOTER();

#
# Any page arguments that you want preserved (i.e., 
# (Optional|Required)PageArguments ones) should be put in here.
# This is where we build urls that the various clickable links have.
# If you need to override any of the defaults, or add more, pass an array 
# with key/value params.
#
function pm_buildurl() {
    global $offset,$limit,$upnodefilter,$pagelayout,$pid,$eid;
    global $sortcols,$sortdir;

    $pageargs = array( 'pid','eid','cols','sortcols','sortdir',
                       'upnodefilter','pagelayout','limit','offset' );

    $params = array();
    foreach ($pageargs as $pa) {
        if (isset($GLOBALS[$pa])) {
            $params[$pa] = $GLOBALS[$pa];
        }
    }

    if (func_num_args() > 0) {
        $aa = func_get_args();
        foreach ($aa[0] as $ok => $ov) {
            $params[$ok] = $ov;
        }
    }

    $retval = "plabmetrics.php3?";
    foreach ($params as $k => $v) {
        if (is_array($v)) {
            $v = implode($v,',');
        }
        $retval .= "$k=" . urlencode($v) . "&";
    }
    $retval = rtrim($retval,'&');

    return $retval;
}

function pm_getpaginationlinks($totalrows) {
    global $offset,$limit,$DEF_NUMPAGENO;

    $retval = '';

    $pagecount = ceil($totalrows / $limit);
    $pageno = floor($offset / $limit) + 1;

    $retval .= "Page $pageno of $pagecount &mdash; ";
    if ($pageno > 1) {
        $url = pm_buildurl(array('offset' => 0));
        $retval .= "<a href='$url'>&lt;&lt;</a> ";
        $url = pm_buildurl(array('offset' => ($offset - $limit)));
        $retval .= "<a href='$url'>&lt;</a> ";
    }
    else {
        $retval .= "<< < ";
    }
    # print DEF_NUMPAGENO max, centered around the current page if possible
    if ($DEF_NUMPAGENO > $pagecount) {
        $startpn = 1;
        $stoppn = $pagecount;
    }
    else {
        $startpn = $pageno - floor(($DEF_NUMPAGENO - 1) / 2);
        $stoppn = $startpn + ($DEF_NUMPAGENO - 1);
        if ($startpn < 1) {
            $startpn = 1;
            $stoppn = $startpn + ($DEF_NUMPAGENO - 1);
            $stoppn = ($stoppn > $pagecount)?$pagecount:$stoppn;
        }
        elseif ($stoppn > $pagecount) {
            $stoppn = $pagecount;
            $startpn = $stoppn - ($DEF_NUMPAGENO - 1);
        }
    }
    for ($i = $startpn; $i <= $stoppn; ++$i) {
        $url = pm_buildurl(array('offset' => ($i - 1) * $limit));
        $retval .= "<a href='$url'>$i</a> ";
    }

    if ($pageno < $pagecount) {
        $url = pm_buildurl(array('offset' => ($offset + $limit)));
        $retval .= "<a href='$url'>&gt;</a> ";
        $url = pm_buildurl(array('offset' => ($pagecount-1)*$limit));
        $retval .= "<a href='$url'>&gt;&gt;</a> ";
    }
    else {
        $retval .= "> >> ";
    }

    return $retval;
}

#
# Assemble query chunks.  Returns an array of query subparts.  Each element is
# intended to go into the query like this:
#   select [0] from [1] [2] [3] [4] 
# where [1] is a table source with joins; [2] is a where filter; 
# [3] is a sort; and [4] is a limit/offset thing for pagination.
#
function pm_buildqueryinfo() {
    global $cols,$colmap,$experiment,$upnodefilter,$sortcols,$sortdir;
    global $limit,$offset;
    global $DEF_LIMIT;

    $q_colstr = $colmap['node_id'];
    foreach ($cols as $c) {
        $q_colstr .= "," . $colmap[$c];
    }

    # setup the main select/join string
    if (isset($experiment)) {
        $q_joinstr = " reserved as r" . 
            " left join nodes as n on r.node_id=n.node_id" .
            " left join plab_mapping as pm on n.phys_nodeid=pm.node_id";
    }
    else {
        $q_joinstr = " plab_mapping as pm";
    }
    # for now, just join all potential tables, even if we're not selecting data
    $q_joinstr .= " left join node_status as ns on pm.node_id=ns.node_id";
    $q_joinstr .= " left join plab_comondata as pcd on pm.node_id=pcd.node_id";
    $q_joinstr .= " left join plab_nodehiststats as pnhs" . 
	" on pm.node_id=pnhs.node_id";

    # setup the quick filter string (note that all of these get and'd)
    $q_quickfs = '';
    if (isset($experiment)) {
        if (strlen($q_quickfs) > 0) {
	    $q_quickfs .= " and ";
        }
        $q_quickfs .= " r.pid='$pid' and r.eid='$eid'";
    }
    if (isset($upnodefilter)) {
        if ($upnodefilter == "emulab") {
            if (strlen($q_quickfs) > 0) {
                $q_quickfs .= " and ";
            }
            $q_quickfs .= " ns.status='up'";
        }
        elseif ($upnodefilter == "comon") {
            if (strlen($q_quickfs) > 0) {
                $q_quickfs .= " and ";
            }
            $q_quickfs .= " (pcd.lastcotop - now()) < 600";
        }
    }

    # setup the user filter string (can be very complex; is and'd to the quick
    # filter string if it exists)
    $q_userfs = '';

    # finalize the filter string
    $q_finalfs = '';
    if (strlen($q_quickfs) > 0) {
        if (strlen($q_finalfs) > 0) {
	    $q_finalfs .= " and ";
        }
        $q_finalfs .= " $q_quickfs ";
    }
    if (strlen($q_userfs) > 0) {
        if (strlen($q_finalfs) > 0) {
            $q_finalfs .= " and ";
        }
        $q_finalfs .= " $q_userfs ";
    }
    if (strlen($q_finalfs) > 0) {
        $q_finalfs = "where $q_finalfs";
    }

    # setup sorting
    if (!isset($sortcols) || count($sortcols) == 0) {
        $sortcols = array('node_id');
    }
    $q_sortstr = 'order by ';
    foreach ($sortcols as $sc) {
        $q_sortstr .= $colmap[$sc] . ",";
    }
    $q_sortstr = rtrim($q_sortstr,',');
    if (isset($sortdir)) {
        if ($sortdir != "asc" && $sortdir != "desc") {
            # XXX - bad sort param!
	    $sortdir = "asc";
        }
    }
    else {
        # we set one by default so that users don't have a hard time in the
        # default case
        $sortdir = "asc";
    }
    $q_sortstr .= " $sortdir";

    # setup pagination
    $q_pagstr = '';
    if (!isset($limit)) {
        $limit = $DEF_LIMIT;
    }
    if (isset($limit) && !isset($offset)) {
        $offset = 0;
    }
    $q_pagstr .= " limit $limit offset $offset";

    return array($q_colstr,$q_joinstr,$q_finalfs,$q_sortstr,$q_pagstr);
}

function pm_showtable($totalrows,$query) {
    global $sortcols,$sortdir,$colmap,$colmap_mysqlnames,$cols;

    echo "<center><div style=''>\n";
    echo pm_getpaginationlinks($totalrows);
    echo "<br>\n";

    # dump table header
    echo "<table>\n";
    echo "  <tr>\n";

    # reverse the sort direction if necessary
    if (isset($sortcols) && count($sortcols) == 1 
        && $sortcols[0] == 'node_id') {
        $nsortdir = ($sortdir == "asc")?"desc":"asc";
    }
    else {
        $nsortdir = $sortdir;
    }
    $url = pm_buildurl(array('sortcols' => 'node_id','sortdir' => $nsortdir));
    echo "    <th><a href='$url'>node_id</a></th>\n";
    foreach ($cols as $c) {
        # reverse the sort direction if necessary
        if (isset($sortcols) && count($sortcols) == 1 
            && $sortcols[0] == $c) {
            $nsortdir = ($sortdir == "asc")?"desc":"asc";
        }
        else {
            $nsortdir = $sortdir;
        }
        $url = pm_buildurl(array('sortcols' => $c,'sortdir' => $nsortdir));
        echo "    <th><a href='$url'>$c</a></th>\n";
    }

    # now run the real query
    $qres = DBQueryFatal($query);

    while ($row = mysql_fetch_array($qres)) {
        echo "  <tr>\n";
        echo "    <td>" . $row[$colmap_mysqlnames['node_id']] . "</td>\n";
        foreach ($cols as $c) {
            $dbval = $row[$colmap_mysqlnames[$c]];
            # stupid mysql driver
            if (is_numeric($dbval) && is_float($dbval+0)) {
                $dbval = sprintf("%.4f",$dbval);
            }
            echo "      <td>$dbval</td>\n";
        }
    }

    echo "</table>\n";
    echo pm_getpaginationlinks($totalrows);
    echo "</div></center>\n";
}

?>
