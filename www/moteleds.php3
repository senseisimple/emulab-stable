<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2004 University of Utah and the Flux Group.
# All rights reserved.
#

include("defs.php3");
include("showstuff.php3");

#
# Make sure they are logged in
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Verify page arguments.
# 
if (!isset($pid) ||
    strcmp($pid, "") == 0) {
    USERERROR("You must provide a Project ID.", 1);
}

if (!isset($eid) ||
    strcmp($eid, "") == 0) {
    USERERROR("You must provide an Experiment ID.", 1);
}

#
# Define a stripped-down view of the web interface - less clutter
#
$view = array(
    'hide_banner' => 1,
    'hide_sidebar' => 1,
    'hide_copyright' => 1
);

#
# Standard Testbed Header now that we have the pid/eid okay.
#
PAGEHEADER("Mote LEDs ($pid/$eid)",$view);

#
# Make sure they have permission to view this experiment
#
if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_READINFO)) {
    USERERROR("You do not have permission to view experiment $exp_eid!", 1);
}

#
# Get a list of all nodes in this experiment of type 'garcia' or 'stargate'
#
$query_result =
    DBQueryFatal("select r.vname, r.node_id,t.type,t.class from reserved as r ".
		     "left join nodes as n on ".
		     "     r.node_id=n.node_id ".
		     "left join node_types as t on ".
		     "     n.type=t.type ".
		     "where r.pid='$pid' and r.eid='$eid' ".
		     "order by r.vname");
		     
if (mysql_num_rows($query_result) == 0) {
   echo "<h3>No nodes to display in this experiment!</h3>\n";
} else {
    echo "<table align=center cellpadding=2 border=1>
	  <tr><th>Mote</th><th>Phys Name</th><th>LEDs</th>\n";
    while ($row = mysql_fetch_array($query_result)) {
	if ($row['type'] != "garcia" && $row['class'] != "sg") {
	    # Only the LEDs, mam
	    continue;
	}
	echo "<tr><th>$row[vname]</th>";
	echo "<td>$row[node_id]</td><td>";
	SHOWBLINKENLICHTEN($uid,
	    $HTTP_COOKIE_VARS[$TBAUTHCOOKIE],
	"ledpipe.php3?node=$row[node_id]");
    }
    echo "</table>\n";

}

#
# Standard Testbed Footer
# 
PAGEFOOTER($view);

?>
