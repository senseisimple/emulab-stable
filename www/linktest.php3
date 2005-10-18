<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2005 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# This script uses Sajax ... BEWARE!
#
require("Sajax.php");
sajax_init();
sajax_export("start_linktest", "stop_linktest");

#
# We need all errors to come back to us so that we can report the error
# to the user even when its from within an exported sajax function.
# 
function handle_error($message, $death)
{
    echo "failed:$message";
    # Always exit; ignore $death.
    exit(1);
}
$session_errorhandler = 'handle_error';

# If this call is to client request function, then turn off interactive mode.
# All errors will go to above function and get reported back through the
# Sajax interface.
if (sajax_client_request()) {
    $session_interactive = 0;
}

# Now check login status.
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Need this argument checking in a function so it can called from the
# request handlers.
#
function CHECKPAGEARGS() {
    global $uid, $TB_EXPTSTATE_ACTIVE, $TB_EXPT_MODIFY;
    
    #
    # Check to make sure a valid experiment.
    #
    $pid = $_GET['pid'];
    $eid = $_GET['eid'];
    
    if (isset($pid) && strcmp($pid, "") &&
	isset($eid) && strcmp($eid, "")) {
	if (! TBvalid_eid($eid)) {
	    PAGEARGERROR("$eid is contains invalid characters!");
	}
	if (! TBvalid_pid($pid)) {
	    PAGEARGERROR("$pid is contains invalid characters!");
	}
	if (! TBValidExperiment($pid, $eid)) {
	    USERERROR("$pid/$eid is not a valid experiment!", 1);
	}
	if (TBExptState($pid, $eid) != $TB_EXPTSTATE_ACTIVE) {
	    USERERROR("$pid/$eid is not currently swapped in!", 1);
	}
	if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_MODIFY)) {
	    USERERROR("You do not have permission to run linktest on ".
		      "$pid/$eid!", 1);
	}
    }
    else {
	PAGEARGERROR("Must specify pid and eid!");
    }
}

#
# Grab DB data we need.
#
function GRABDBDATA() {
    global $pid, $eid;
    global $gid, $linktest_level, $linktest_pid;
    
    # Need the GID, plus current level and the pid.
    $query_result =
	DBQueryFatal("select gid,linktest_level,linktest_pid ".
		     "  from experiments ".
		     "where pid='$pid' and eid='$eid'");
    $row = mysql_fetch_array($query_result);
    $gid            = $row[0];
    $linktest_level = $row[1];
    $linktest_pid   = $row[2];
}

#
# Stop a running linktest.
# 
function stop_linktest() {
    global $linktest_pid;
    global $uid, $pid, $gid, $eid, $suexec_output;

    # Must do this!
    CHECKPAGEARGS();
    GRABDBDATA();

    if (! $linktest_pid) {
	return "stopped:Linktest is not running on experiment $pid/$eid!";
    }
    # For backend script call.
    TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);
    
    $retval = SUEXEC($uid, "$pid,$unix_gid", "weblinktest -k $pid $eid",
		     SUEXEC_ACTION_IGNORE);

    if ($retval < 0) {
	return "failed:$suexec_output";
    }
    return "stopped:Linktest has been stopped.";
}

#
# If user hits stop button in the output side, stop linktest.
#
$process = 0;

function SPEWCLEANUP()
{
    global $pid, $gid;
    $process = $GLOBALS["process"];

    TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);

    if (connection_aborted() && $process) {
	SUEXEC($uid, "$pid,$unix_gid", "weblinktest -k $pid $eid",
	       SUEXEC_ACTION_IGNORE);
	proc_close($process);
    }
}

#
# Start linktest running.
# 
function start_linktest($level) {
    global $linktest_pid;
    global $uid, $pid, $gid, $eid, $suexec_output, $TBSUEXEC_PATH;

    # Must do this!
    CHECKPAGEARGS();
    GRABDBDATA();

    if ($linktest_pid) {
	return "failed:Linktest is already running on experiment $pid/$eid!";
    }
    if (! TBvalid_tinyint($level) ||
	$level < 0 || $level > TBDB_LINKTEST_MAX) {
	return "failed:Linktest level ($level) must be an integer ".
	    "1 <= level <= ". TBDB_LINKTEST_MAX;
    }
    
    # For backend script call.
    TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);

    # Make sure we shutdown if client goes away.
    register_shutdown_function("SPEWCLEANUP");
    ignore_user_abort(1);
    set_time_limit(0);

    #
    # Having problems with popen reporting the proper exit status,
    # so switched to proc_open instead. Obtuse, I know.
    #
    $descriptorspec = array(0 => array("pipe", "r"),
			    1 => array("pipe", "w"));

    $process = proc_open("$TBSUEXEC_PATH ".
		 "$uid $pid,$unix_gid weblinktest -l $level $pid $eid",
			 $descriptorspec, $pipes);

    if (! is_resource($process)) {
	return "failed:Linktest failed!";
    }
    $GLOBALS["process"] = $process;
    fclose($pipes[0]);

    # Build up an output string to return to caller.
    $output = "Starting linktest run at level $level on " .
	date("D M d G:i:s T") . "\n";

    while (!feof($pipes[1])) {
	$string  = fgets($pipes[1], 1024);
	$output .= $string;
    }
    fclose($pipes[1]);
    $retval = proc_close($process);
    $fp = 0;
    if ($retval > 0) {
	$output .= "Linktest reported errors! Stopped at " .
	    date("D M d G:i:s T") . "\n";
	return "error:$output";
    }
    elseif ($retval < 0) {
	return "failed:$output";
    }
    # Success.
    $output .= "Linktest run was successful! Stopped at " .
	date("D M d G:i:s T") . "\n";
    return "stopped:$output";
}

# See if this request is to one of the above functions. Does not return
# if it is. Otherwise return and continue on.
sajax_handle_client_request();

#
# In plain kill mode, just use the stop_linktest function and then
# redirect back to the showexp page.
#
if (isset($kill) && $kill == 1) {
    stop_linktest();
    
    header("Location: showexp.php3?pid=$pid&eid=$eid");
    return;
}

#
# Okay, this is the initial page.
# 
PAGEHEADER("Run Linktest");

# Must do this!
CHECKPAGEARGS();
GRABDBDATA();

echo "<script>\n";
sajax_show_javascript();
if ($linktest_pid) {
    echo "var curstate    = 'running';";
}
else {
    echo "var curstate    = 'stopped';";
}
?>
function do_start_cb(msg) {
    if (msg == '') {
	return;
    }

    //
    // The first part of the message is an indicator whether linktest
    // really started. The rest of it (after the ":" is the text itself).
    //
    var offset = msg.indexOf(":");
    if (offset == -1) {
	return;
    }
    var status = msg.substring(0, offset);
    var output = msg.substring(offset + 1);

    curstate = 'stopped';
    getObjbyName('action').value = 'Start';

    // Display the data, even if error.
    var stuff = "";

    if (status == 'error') {
	stuff = "<font size=+1 color=red><blink>" +
	    "Linktest has reported errors! Please examine log below." +
	    "</blink></font>";
    }

    stuff = stuff + 
	'<textarea id=outputarea rows=15 cols=90 readonly>' +
	output + '</textarea>';

    getObjbyName('output').innerHTML = stuff;

    // If we got an error; disable the button. 
    if (status == 'failed') {
	getObjbyName('action').disabled = true;    
    }
}

// Linktest stopped. No need to do anything since if a request was running,
// it will have returned to do_start_cb() above, prematurely. 
function do_stop_cb(msg) {
    if (msg == '') {
	return;
    }

    //
    // The first part of the message is an indicator whether command
    // was sucessful. The rest of it (after the ":" is the text itself).
    //
    var offset = msg.indexOf(":");
    if (offset == -1) {
	return;
    }
    var status = msg.substring(0, offset);
    var output = msg.substring(offset + 1);

    // If we got an error; throw up something useful.
    if (status != 'stopped') {
	alert("Linktest Stop: " + output);
    }
}

function heartbeat() {
    var today=new Date();
    var h=today.getHours();
    var m=today.getMinutes();
    var s=today.getSeconds();
    
    // add a zero in front of numbers<10
    if (m < 10)
	m = "0" + 1;
    if (s < 10)
	s = "0" + 1;

    if (curstate == 'running') {
	getObjbyName('outputarea').value =
	    "Linktest starting up ... please be patient: " + h+":"+m+":"+s;
	setTimeout('heartbeat()', 500);
    }
}

function doaction(theform) {
    var levelx = theform['level'].selectedIndex;
    var level  = theform['level'].options[levelx].value;
    var action = theform['action'].value;

    if (curstate == 'stopped') {
	if (level == '0')
	    return;

	// Display something so that users activity.
	getObjbyName('output').innerHTML =
	    '<textarea id=outputarea rows=15 cols=90 readonly>' +
	    "Linktest starting up ... please be patient. " + '</textarea>';
	
	curstate = "running";
	theform['action'].value = "Stop Linktest";
	heartbeat();
	x_start_linktest(level, do_start_cb);
    }
    else {
	x_stop_linktest(do_stop_cb);
    }
}
<?php
echo "</script>\n";

echo "<font size=+2>Experiment <b>".
	"<a href='showproject.php3?pid=$pid'>$pid</a>/".
	"<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a></b></font>\n";

echo "<center><font size=+2><br>
         Are you <b>sure</b> you want to run linktest?
         </font><br><br>\n";

SHOWEXP($pid, $eid, 1);

echo "<br>\n";
echo "<form action=linktest.php3 method=post name=myform
                target=Linktest_${pid}_${eid}>";
echo "<input type=hidden name=pid value=$pid>\n";
echo "<input type=hidden name=eid value=$eid>\n";

echo "<table align=center border=1>\n";
echo "<tr>
          <td><a href='$TBDOCBASE/doc/docwrapper.php3?".
                 "docname=linktest.html'>Linktest</a> Option:</td>
          <td><select name=level>
                 <option value=0>Skip Linktest </option>\n";

for ($i = 1; $i <= TBDB_LINKTEST_MAX; $i++) {
    $selected = "";

    if (strcmp("$level", "$i") == 0)
	$selected = "selected";
	
    echo "        <option $selected value=$i>Level $i - " .
	$linktest_levels[$i] . "</option>\n";
}
echo "       </select>";
echo "    </td>
          </tr>
          </table><br>\n";

if ($linktest_pid) {
    echo "<input type=button name=action id=action value='Stop Linktest' ".
	         "onclick=\"doaction(myform); return false;\">\n";
}
else {
    echo "<input type=button name=action id=action value=Start ".
	         "onclick=\"doaction(myform); return false;\">\n";
}

echo "</form>\n";
echo "<div id=output></div>\n";
echo "</center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
