<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2004, 2005, 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#

include("defs.php3");
include("showstuff.php3");

#
# Make sure they are logged in
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("experiment", PAGEARG_EXPERIMENT);

# Need these below
$pid = $experiment->pid();
$eid = $experiment->eid();

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
if (!$experiment->AccessCheck($this_user, $TB_EXPT_READINFO)) {
    USERERROR("You do not have permission to view experiment $eid!", 1);
}

#
# Get a list of all nodes in this experiment of type 'garcia' or 'stargate'
#
$query_result =
    DBQueryFatal("select r.vname,n.phys_nodeid ".
		 "from reserved as r ".
		 "left join nodes as n on r.node_id=n.node_id ".
		 "left join node_types as t on n.type=t.type ".
		 "where r.pid='$pid' and r.eid='$eid' and t.class='mote' ".
		 "order by r.vname");

if (mysql_num_rows($query_result) == 0) {
   echo "<h3>No nodes to display in this experiment!</h3>\n";
}
else {
    echo "<table align=center cellpadding=2 border=1>
	  <tr><th>Mote</th><th>Phys Name</th><th>LEDs</th>\n";
    while ($row = mysql_fetch_array($query_result)) {
	$vname = $row["vname"];
	$pnode = $row["phys_nodeid"];
	
	echo "<tr><th>$vname</th>";
	echo "<td>$pnode</td><td>";
	SHOWBLINKENLICHTEN($uid,
			   $HTTP_COOKIE_VARS[$TBAUTHCOOKIE],
			   CreateURL("ledpipe", $pnode));
    }
    echo "</table>\n";

}

#
# Standard Testbed Footer
# 
PAGEFOOTER($view);

?>
