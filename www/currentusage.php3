<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");
#
# This script uses Sajax ... BEWARE!
#
require("Sajax.php");

#
# Get current user.
#
$this_user = CheckLogin($check_status);

#
# For anonymous users, show experiment stats.
#
function SHOWSTATS()
{
    $query_result =
	DBQueryFatal("select count(*) from experiments as e " .
	     "left join experiment_stats as s on s.exptidx=e.idx " .
	     "left join experiment_resources as rs on rs.idx=s.rsrcidx ".
	     "where state='active' and rs.pnodes>0 and " .
		     "      e.pid!='emulab-ops' and ".
		     "      not (e.pid='ron' and e.eid='all')");
    
    if (mysql_num_rows($query_result) != 1) {
	$active_expts = "ERR";
    }
    else {
	$row = mysql_fetch_array($query_result);
	$active_expts = $row[0];
    }

    $query_result =
	DBQueryFatal("select count(*) from experiments where ".
		     "state='swapped' and pid!='emulab-ops' and ".
		     "pid!='testbed'");
    if (mysql_num_rows($query_result) != 1) {
	$swapped_expts = "ERR";
    }
    else {
	$row = mysql_fetch_array($query_result);
	$swapped_expts = $row[0];
    }

    $query_result =
	DBQueryFatal("select count(*) from experiments where ".
		     "state='active' and swap_requests > 0 and idle_ignore=0 ".
		     "and pid!='emulab-ops' and pid!='testbed'");
    if (mysql_num_rows($query_result) != 1) {
	$idle_expts = "ERR";
    }
    else {
	$row = mysql_fetch_array($query_result);
	$idle_expts = $row[0];
    }
    $freepcs = TBFreePCs();

    $output = "<table valign=top align=center width=100% height=100% border=1>
                <tr><th nowrap colspan=2 class='usagetitle'>
	            Current Experiments</th></tr>
                <tr><td class=menuoptusage align=right>$active_expts</td>
                    <td align=left class=menuoptusage>
                        <a target=_parent href=explist.php3#active>Active</a>
                    </td></tr>
                <tr><td align=right class=menuoptusage>$idle_expts</td>
                    <td align=left  class=menuoptusage>Idle</td></tr>
                <tr><td align=right class=menuoptusage>$swapped_expts</td>
                    <td align=left  class=menuoptusage>
                        <a target=_parent href=explist.php3#swapped>Swapped</a>
                    </td></tr>
                <tr><td align=right class=menuoptusage>
                        <font size=+1>$freepcs</font></td>
                    <td align=left  class=menuoptusage>
                        <font size=+1>Free PCs</font></td>
                </tr>
               </table>\n";
    return $output;
}

#
# Logged in users, show free node counts.
#
function SHOWFREENODES()
{
    # Get typelist and set freecounts to zero.
    $query_result =
	DBQueryFatal("select n.type from nodes as n ".
		     "left join node_types as nt on n.type=nt.type ".
		     "where (role='testnode') and class='pc' ");
    while ($row = mysql_fetch_array($query_result)) {
	$type              = $row[0];
	$freecounts[$type] = 0;
    }
    
    # Get free totals by type.
    $query_result =
	DBQueryFatal("select n.eventstate,n.type,count(*) from nodes as n ".
		     "left join node_types as nt on n.type=nt.type ".
		     "left join reserved as r on r.node_id=n.node_id ".
		     "where (role='testnode') and class='pc' ".
		     "      and r.pid is null and n.reserved_pid is null ".
		     "group BY n.eventstate,n.type");

    while ($row = mysql_fetch_array($query_result)) {
	$type  = $row[1];
	$count = $row[2];
        # XXX Yeah, I'm a doofus and can't figure out how to do this in SQL.
	if (($row[0] == TBDB_NODESTATE_ISUP) ||
	    ($row[0] == TBDB_NODESTATE_PXEWAIT) ||
	    ($row[0] == TBDB_NODESTATE_ALWAYSUP) ||
	    ($row[0] == TBDB_NODESTATE_POWEROFF)) {
	    $freecounts[$type] = $count;
	}
    }
    $output = "";
	
    $freepcs   = TBFreePCs();
    $reloading = TBReloadingPCs();

    $output .= "<table valign=top align=center width=100% height=100% border=1>
                 <tr><td nowrap colspan=6 class=menuoptusage align=center>
 	           <font size=+1>$freepcs Free PCs</font></td></tr>\n";

    $pccount = count($freecounts);
    $newrow  = 1;
    $maxcols = (int) ($pccount / 3);
    if ($pccount % 3)
	$maxcols++;
    $cols    = 0;
    foreach($freecounts as $key => $value) {
	$freecount = $freecounts[$key];

	if ($newrow) {
	    $output .= "<tr>\n";
	}
	
	$output .= "<td class=menuoptusage align=right>
                     <a target=_parent href=shownodetype.php3?node_type=$key>
                        $key</a></td>
                    <td class=menuoptusage align=left>${freecount}</td>\n";

	$cols++;
	$newrow = 0;
	if ($cols == $maxcols || $pccount <= 3) {
	    $cols   = 0;
	    $newrow = 1;
	}

	if ($newrow) {
	    $output .= "</tr>\n";
	}
    }
    if (! $newrow) {
        # Fill out to $maxcols
	for ($i = $cols + 1; $i <= $maxcols; $i++) {
	    $output .= "<td class=menuoptusage>&nbsp</td>";
	    $output .= "<td class=menuoptusage>&nbsp</td>";
	}
	$output .= "</tr>\n";
    }
    # Fill in up to 3 rows.
    if ($pccount < 3) {
	for ($i = $pccount + 1; $i <= 3; $i++) {
	    $output .= "<tr><td class=menuoptusage>&nbsp</td>
                            <td class=menuoptusage>&nbsp</td></tr>\n";
	}
    }

    $output .= "<tr>
                 <td class=menuoptusage colspan=6 align=center>
                    <font size=+1>$reloading PCs reloading</font></td>
               </tr>\n";
    $output .= "</table>";
    return $output;
}

#
# This is for the Sajax request.
#
function FreeNodeHtml() {
    global $this_user;

    if ($this_user) {
	return SHOWFREENODES();
    }
    else {
	return SHOWSTATS();
    }
}

#
# We need all errors to come back this function so that the Sajax request
# fails and the timer is terminated. See below.
# 
function handle_error($message, $death)
{
    echo "failed:$message";
    # Always exit; ignore $death.
    exit(1);
}
     
#
# If user is anonymous, show experiment stats, otherwise useful info.
# 
if ($this_user) {
    sajax_init();
    sajax_export("FreeNodeHtml");

    # If this call is to client request function, then turn off
    # interactive mode; errors will cause the Sajax request to fail
    # and the timer to stop.
    if (sajax_client_request()) {
	$session_interactive  = 0;
	$session_errorhandler = 'handle_error';
    }
    sajax_handle_client_request();

    PAGEBEGINNING("Free Node Summary", 1, 1);
    echo "<script>\n";
    sajax_show_javascript();

    ?>
    function FreeNodeHtml_CB(stuff) {
	getObjbyName('usage').innerHTML = stuff;
	setTimeout('GetFreeNodeHtml()', 60000);
    }
    function GetFreeNodeHtml() {
	x_FreeNodeHtml(FreeNodeHtml_CB);
    }
    setTimeout('GetFreeNodeHtml()', 60000);
    
    <?php
    echo "</script>\n";
	  
    echo "<div id=usage>\n";
    echo   SHOWFREENODES();
    echo "</div>\n";
    echo "</body></html>";
}
else {
    PAGEBEGINNING("Current Usage", 1, 1);
    echo "<div id=usage>\n";
    echo   SHOWSTATS();
    echo "</div>\n";
    echo "</body></html>";
}
