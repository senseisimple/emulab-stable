<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# This script uses Sajax ... BEWARE!
#
require("Sajax.php");
sajax_init();
sajax_export("stop_linktest");

#
# We need all errors to come back to us so that we can report the error
# to the user even when its from within an exported sajax function.
# 
function handle_error($message, $death)
{
    echo "failed:$message";
    TBERROR("failed:$message", 0);
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
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

# This stuff will be setup below in CHECKPAGEARGS()
$pid            = "";
$eid            = "";
$unix_gid       = "";
$linktest_level = 0;
$linktest_pid   = 0;

#
# Need this argument checking in a function so it can called from the
# request handlers.
#
function CHECKPAGEARGS() {
    global $this_user, $TB_EXPTSTATE_ACTIVE, $TB_EXPT_MODIFY;
    global $TB_EXPTSTATE_ACTIVATING, $TB_EXPTSTATE_MODIFY_RESWAP;
    global $pid, $eid, $experiment;
    global $unix_gid, $linktest_level, $linktest_pid;

    $reqargs = RequiredPageArguments("experiment", PAGEARG_EXPERIMENT);
    
    if (!$experiment->AccessCheck($this_user, $TB_EXPT_MODIFY)) {
	USERERROR("You do not have permission to start/stop linktest ".
		  "on $pid/$eid!", 1);
    }
    $expstate = $experiment->state();
    if ($expstate != $TB_EXPTSTATE_ACTIVE &&
	$expstate != $TB_EXPTSTATE_ACTIVATING &&
	$expstate != $TB_EXPTSTATE_MODIFY_RESWAP) {
	USERERROR("Experiment $eid must be active to start/stop linktest," . 
		  " not $expstate!", 1);
    }
    $pid            = $experiment->pid();
    $eid            = $experiment->eid();
    $unix_gid       = $experiment->UnixGID();
    $linktest_level = $experiment->linktest_level();
    $linktest_pid   = $experiment->linktest_pid();
}

#
# Stop a running linktest.
# 
function stop_linktest() {
    global $linktest_pid;
    global $uid, $pid, $unix_gid, $eid, $suexec_output, $session_interactive;

    # Must do this!
    CHECKPAGEARGS();

    if (! $linktest_pid) {
	if ($session_interactive) {
	    USERERROR("$pid/$eid is not running linktest!", 1);
	}
	return "stopped:Linktest is not running on experiment $pid/$eid!";
    }
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
$linktest_running = 0;

function SPEWCLEANUP()
{
    global $pid, $unix_gid, $uid, $eid, $linktest_running;

    if (connection_aborted() && $linktest_running) {
	SUEXEC($uid, "$pid,$unix_gid", "weblinktest -k $pid $eid",
	       SUEXEC_ACTION_IGNORE);
    }
}

#
# Start linktest running.
# 
function start_linktest($level) {
    global $linktest_pid, $linktest_running, $TBSUEXEC_PATH;
    global $uid, $pid, $unix_gid, $eid, $suexec_output;

    # Must do this!
    CHECKPAGEARGS();

    if ($linktest_pid) {
	return "failed:Linktest is already running on experiment $pid/$eid!";
    }
    if (! TBvalid_tinyint($level) ||
	$level < 0 || $level > TBDB_LINKTEST_MAX) {
	return "failed:Linktest level ($level) must be an integer ".
	    "1 <= level <= ". TBDB_LINKTEST_MAX;
    }
    
    # Make sure we shutdown if client goes away.
    register_shutdown_function("SPEWCLEANUP");
    set_time_limit(0);

    # XXX Hackish!
    $linktest_running = 1;
    $fp = popen("$TBSUEXEC_PATH $uid $pid,$unix_gid ".
		"weblinktest -l $level $pid $eid", "r");
    if (! $fp) {
	USERERROR("Could not start linktest!", 1);
    }

    header("Content-Type: text/plain");
    header("Expires: Mon, 26 Jul 1997 05:00:00 GMT");
    header("Cache-Control: no-cache, must-revalidate");
    header("Pragma: no-cache");

    echo "Starting linktest run at level $level on " .
	date("D M d G:i:s T");
    # Oh, this is soooooo bogus!
    for ($i = 0; $i < 1024; $i++) {
	echo " ";
    }
    echo "\n";
    
    flush();
    
    while (!feof($fp)) {
	$string = fgets($fp, 1024);
	echo "$string";
	flush();
    }
    $retval = pclose($fp);
    $linktest_running = 0;

    if ($retval > 0) {
	echo "Linktest reported errors! Stopped at " .
	    date("D M d G:i:s T") . "\n";
    }
    elseif ($retval < 0) {
	echo $suexec_output;
    }
    else {
        # Success.
	echo "Linktest run was successful! Stopped at " .
	    date("D M d G:i:s T") . "\n";
    }
    exit(0);
}

# See if this request is to one of the above functions. Does not return
# if it is. Otherwise return and continue on.
sajax_handle_client_request();

#
# In plain kill mode, just use the stop_linktest function and then
# redirect back to the showexp page.
#
if (isset($_REQUEST["kill"])) {
    stop_linktest();
    header("Location: showexp.php3?pid=$pid&eid=$eid");
    return;
}
if (isset($_REQUEST["start"]) && isset($_REQUEST["level"])) {
    start_linktest($_REQUEST["level"]);
    return;
}

#
# Okay, this is the initial page.
# 
PAGEHEADER("Run Linktest");

# Must do this!
CHECKPAGEARGS();

echo "<script>\n";
sajax_show_javascript();
if ($linktest_pid) {
    echo "var curstate    = 'busy';\n";
}
else {
    echo "var curstate    = 'stopped';\n";
}
?>

// Linktest stopped. No need to do anything since if a request was running,
// it will have returned prematurely.
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
	alert("Linktest could not be stopped: " + output);
	return;
    }

    // Strictly for the benefit of stopping a linktest started from
    // another window.
    if (curstate == 'busy') {
	curstate = 'stopped';	
	getObjbyName('action').value = 'Start';

	// This clears the message area.
	getObjbyName('message').innerHTML = "";
    }
}

// onLoad callback for when linktest stops in the iframe.
function linktest_stopped() {
    // Avoid initial outer page load event.
    if (curstate != 'running' && curstate != 'stopping')
	return;
    
    curstate = 'stopped';	
    getObjbyName('action').value = 'Start';

    var Iframe = document.getElementById('outputarea');
    var html   = Iframe.contentWindow.document.documentElement.innerHTML;

    re = /reported errors/gi
    re.multiline = true;

    if (html.search(re) != -1) {
	getObjbyName('message').innerHTML =
	    '<font size=+1 color=red><blink>' +
	    'Linktest has reported errors! Please examine log below.' +
	    '</blink></font>';
    }
    else {
	getObjbyName('message').innerHTML =
	    '<font size=+1 color=black>' +
	    'Linktest has completed, no reported errors' + '</font>';
    }
}

function doaction(theform) {
    var levelx = theform['level'].selectedIndex;
    var level  = theform['level'].options[levelx].value;
    var action = theform['action'].value;

    if (curstate == 'stopped') {
	if (level == '0')
	    return;

	// This clears the message area.
	getObjbyName('message').innerHTML = "";

	Iframe = document.getElementById('outputarea');
	// This stuff clears the current contents of the iframe.
	Iframe.contentWindow.document.open();
	Iframe.contentWindow.document.write(" ");
	Iframe.contentWindow.document.close();
	// And this fires it up.
	Iframe.contentWindow.document.location =
	    '<?php echo $REQUEST_URI; ?>&start=1&level=' + level;

	curstate = "running";
	theform['action'].value = "Stop Linktest";
    }
    else if (curstate == 'running') {
	curstate = 'stopping';	
	x_stop_linktest(do_stop_cb);
    }
    else if (curstate == 'busy') {
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

$experiment->Show(1);

echo "<br>\n";
echo "<form action=linktest.php3 method=post name=myform id=myform>";
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

    if (strcmp("$linktest_level", "$i") == 0)
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
echo "<div id=message>\n";
if ($linktest_pid) {
    echo "<font size=+1 color=red>Linktest is already running</font>\n";
}
echo "</div>\n";
echo "<div id=output style='overflow:auto'>
      <iframe onload=\"linktest_stopped();\"
      width=80% height=400 scrolling=auto id=outputarea frameborder=1>
      </iframe></center>\n";
echo "</center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
