<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

require("Sajax.php");
sajax_init();
sajax_export("GetPNodes");

# If this call is to client request function, then turn off interactive mode.
# All errors will go to above function and get reported back through the
# Sajax interface.
if (sajax_client_request()) {
    $session_interactive = 0;
}

#
# Only known and logged in users can look at experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

function CHECKPAGEARGS($pid, $eid) {
    global $uid, $TB_EXPT_READINFO;
    
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
    # Check to make sure this is a valid PID/EID tuple.
    #
    if (! TBValidExperiment($pid, $eid)) {
	USERERROR("The experiment $pid/$eid is not a valid experiment!", 1);
    }
    
    #
    # Verify permission.
    #
    if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_READINFO)) {
	USERERROR("You do not have permission to view the log for $pid/$eid!", 1);
    }
}

function GetPNodes($pid, $eid) {
    CHECKPAGEARGS($pid, $eid);
    
    $retval = array();

    $query_result = DBQueryFatal(
	"select r.node_id from reserved as r ".
	"where r.eid='$eid' and r.pid='$pid' order by LENGTH(node_id) desc");

    while ($row = mysql_fetch_array($query_result)) {
      $retval[] = $row[node_id];
    }

    return $retval;
}

# See if this request is to one of the above functions. Does not return
# if it is. Otherwise return and continue on.
sajax_handle_client_request();

CHECKPAGEARGS($pid, $eid);

#
# Standard Testbed Header
#
PAGEHEADER("Experiment Activity Log");

#
# Check for a logfile. This file is transient, so it could be gone by
# the time we get to reading it.
#
if (! TBExptLogFile($pid, $eid)) {
    USERERROR("Experiment $pid/$eid is no longer in transition!", 1);
}

echo "<font size=+2>Experiment <b>".
     "<a href='showproject.php3?pid=$pid'>$pid</a>/".
     "<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a></b></font>\n";
echo "<br /><br />\n";

?>

<pre id='outputarea'>
</pre>
<br>
<img id='busy' src='busy.gif'><span id='loading'> Loading...</span>

<?php

echo "
<script type='text/javascript' language='javascript' src='json.js'></script>
<script type='text/javascript' language='javascript' src='mungelog.js'></script>\n";
echo "
<script type='text/javascript' language='javascript'>\n";

    sajax_show_javascript();

echo "
exp_pid = \"$pid\";
exp_eid = \"$eid\";
</script>
<iframe id='downloader' name='downloader' width=0 height=0 src='spewlogfile.php3?pid=$pid&eid=$eid' onload='ml_handleReadyState(LOG_STATE_LOADED);' border=0 style='width:0px; height:0px; border: 0px'>
</iframe>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
