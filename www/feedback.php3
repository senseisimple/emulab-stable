<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# No PAGEHEADER since we spit out a Location header later. See below.
# 

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify Page arguments
#
$reqargs = RequiredPageArguments("experiment", PAGEARG_EXPERIMENT,
				 "mode",       PAGEARG_STRING);
$optargs = OptionalPageArguments("duration",   PAGEARG_INTEGER,
				 "canceled",   PAGEARG_STRING,
				 "confirmed",  PAGEARG_STRING,
				 "clear_last", PAGEARG_BOOLEAN,
				 "clear_bootstrap", PAGEARG_BOOLEAN);

# Need these below.
$pid = $experiment->pid();
$eid = $experiment->eid();
$gid = $experiment->gid();
$expstate = $experiment->state();
$unix_gid = $experiment->UnixGID();

if (strcmp($expstate, $TB_EXPTSTATE_ACTIVE) &&
    strcmp($expstate, $TB_EXPTSTATE_SWAPPED)) {
    USERERROR("You cannot modify an experiment in transition.", 1);
}

if (!strcmp($expstate, $TB_EXPTSTATE_ACTIVE)) {
	$reboot = 1;
	$eventrestart = 1;
}
else {
	$reboot = 0;
	$eventrestart = 0;
}

if (strcmp($mode, "record") && strcmp($mode, "clear")) {
    PAGEARGERROR("Mode value, $mode, is not 'record' or 'clear'!");
}

#
# Get the duration of the feedback run, default to 30 seconds if nothing was
# given.
#
if (!isset($duration)) {
     $duration = 30; # seconds
}
elseif ($duration < 3) {
    PAGEARGERROR("Duration must be an integer >= 3");
}

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we should
# probably redirect the browser back up a level.
#
if (isset($canceled) && $canceled) {
    PAGEHEADER("$mode Feedback");
	
    echo "<center><h3><br>
          Operation canceled!
          </h3></center>\n";
    
    PAGEFOOTER();
    return;
}

if (!isset($confirmed)) {
    PAGEHEADER("$mode feedback");

    echo $experiment->PageHeader();

    if(strcmp($mode, "record") == 0) {
	echo "<center><font size=+2><br>
                      How much feedback data should be recorded?<br>
                      Must be atleast 3 seconds.
                      </font>\n";
    }
    else if(strcmp($mode, "clear") == 0) {
	echo "<center><font size=+2><br>
                      Really clear feedback data?
                      </font>\n";
    }
    
    $experiment->Show(1);

    $url = CreateURL("feedback", $experiment, "mode", $mode);
    
    echo "<form action='$url' method=post>";

    if(strcmp($mode, "record") == 0) {
	echo "<table align=center border=1>\n";
	echo "<tr>
                  <td>
                  <input type='text' name='duration' value='$duration'> seconds
                  </td>
              </tr>
              </table><br>\n";
    }
    else if(strcmp($mode, "clear") == 0) {
	echo "<br>Data to clear:</br>";
	echo "<table align=center border=1>\n";
	echo "<tr>
                  <td>
                  <input type='checkbox' name='clear_last' value='1' checked='1'> Last recorded trace.
                  </td>
              </tr>
              <tr>
                  <td>
                  <input type='checkbox' name='clear_bootstrap' value='1'> Bootstrap data.
                  </td>
              </tr>
              </table><br>\n";
    }

    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";
    
    PAGEFOOTER();
    return;
}

function SPEWCLEANUP()
{
    global $fp;

    if (connection_aborted() && $fp) {
	pclose($fp);
    }
    exit();
}

if (strcmp($mode, "record") == 0) {
    #
    # A cleanup function to keep the child from becoming a zombie, since
    # the script is terminated, but the children are left to roam.
    #
    $fp = 0;
    
    register_shutdown_function("SPEWCLEANUP");
    ignore_user_abort(1);
    
    # Start the script and pipe its output to the user.
    $fp = popen("$TBSUEXEC_PATH $uid $pid,$unix_gid webfeedback ".
		"-d $duration $pid $gid $eid",	"r");
    if (! $fp) {
	USERERROR("Feedback failed!", 1);
    }

    header("Content-Type: text/plain");
    header("Expires: Mon, 26 Jul 1997 05:00:00 GMT");
    header("Cache-Control: no-cache, must-revalidate");
    header("Pragma: no-cache");
    flush();
    
    echo date("D M d G:i:s T");
    echo "\n";
    echo "Recording feedback for $duration seconds\n";
    flush();
    while (!feof($fp)) {
	$string = fgets($fp, 1024);
	echo "$string";
	flush();
    }
    $retval = pclose($fp);
    $fp = 0;
    if ($retval == 0)
	echo "Feedback run was successful!\n";
    echo date("D M d G:i:s T");
    echo "\n";
    return;
}
else if (strcmp($mode, "clear") == 0) {
    PAGEHEADER("Clearing feedback");

    $options = "";
    if (isset($clear_last) && !strcmp($clear_last, "1"))
	$options .= " -c";
    if (isset($clear_bootstrap) && !strcmp($clear_bootstrap, "1"))
	$options .= " -b";
    if ($options != "") {
	$retval = SUEXEC($uid, "$pid,$unix_gid",
			 "webfeedback $options $pid $gid $eid",
			 SUEXEC_ACTION_USERERROR);
	
	#
	# Avoid	SIGPROF in child.
	# 
	set_time_limit(0);
	
	$query_result = DBQueryFatal("SELECT nsfile from nsfiles ".
				     "where pid='$pid' and eid='$eid'");
	if (mysql_num_rows($query_result)) {
	    $row    = mysql_fetch_array($query_result);
	    $nsdata = $row["nsfile"];
	}
	else {
	    $nsdata = ""; # XXX what to do...
	}
	
	list($usec, $sec) = explode(' ', microtime());
	srand((float) $sec + ((float) $usec * 100000));
	$foo = rand();
	$nsfile = "/tmp/$uid-$foo.nsfile";
	
	if (! ($fp = fopen($nsfile, "w"))) {
	    TBERROR("Could not create temporary file $nsfile", 1);
	}
	fwrite($fp, $nsdata);
	fclose($fp);
	chmod($nsfile, 0666);
	
	$retval = SUEXEC($uid, "$pid,$unix_gid",
			 "webswapexp " . ($reboot ? "-r " : "") .
			 ($eventrestart ? "-e " : "") .
			 "-s modify $pid $eid $nsfile",
			 SUEXEC_ACTION_IGNORE);
	
	unlink($nsfile);
	
	#
	# Fatal Error. Report to the user, even though there is not much he can
	# do with the error. Also reports to tbops.
	# 
	if ($retval < 0) {
	    SUEXECERROR(SUEXEC_ACTION_DIE);
	#
	# Never returns ...
	#
	    die("");
	}
    }
    
    echo "<center><h3><br>Done!</h3></center>\n";

    PAGEFOOTER();
    return;
}

?>
