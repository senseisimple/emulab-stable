<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

$parser   = "$TB/libexec/ns2ir/parse-ns";

#
# Standard Testbed Header
#
PAGEHEADER("Modify Experiment");

#
# Only known and logged in users can modify experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

$isadmin = ISADMIN($uid);

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
# Be paranoid.
#
$pid = addslashes($pid);
$eid = addslashes($eid);

#
# Check to make sure this is a valid PID/EID tuple.
#
$query_result =
    DBQueryFatal("SELECT * FROM experiments WHERE ".
		 "eid='$eid' and pid='$pid'");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("The experiment $eid is not a valid experiment ".
            "in project $pid.", 1);
}

$expstate = TBExptState($pid, $eid);

if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_MODIFY ) ) {
  USERERROR("You do not have permission to modify this experiment.", 1);
}

if (strcmp($expstate, $TB_EXPTSTATE_ACTIVE) &&
    strcmp($expstate, $TB_EXPTSTATE_SWAPPED)) {
  USERERROR("You cannot modify an experiment in transition.", 1);
}


if (! isset($go)) {

	echo "<form action='modifyexp.php3' method='post'>";
	echo "<textarea cols='100' rows='40' name='nsdata'>";

	$query_result =
	    DBQueryFatal("SELECT nsfile from nsfiles where pid='$pid' and eid='$eid'");
	if (mysql_num_rows($query_result)) {
	    $row    = mysql_fetch_array($query_result);
	    $nsfile = stripslashes($row[nsfile]);
	    
	    echo "$nsfile";
	} else {
	    echo "# There was no stored NS file for $pid/$eid.\n";
	}
	
	echo "</textarea>";

	echo "<br />";
	if (0 == strcmp($expstate, $TB_EXPTSTATE_ACTIVE)) {
	    echo "<p><b>Note!</b> If changing your experiment topology 
              (adding or removing nodes, links, and LANs), you will most likely 
	      have to reboot all nodes in your experiment (check the box below.)
	      If adding/removing a delay to/from an existing link, or replacing 
	      a lost node <i>without modifying the experiment topology</i>,
	      this won't be necessary.</p>";
	    echo "<input type='checkbox' name='reboot' value='1' checked='1'>
	      Reboot nodes in experiment</input>";
	}
	echo "<br />";
	echo "<input type='hidden' name='pid' value='$pid' />";
	echo "<input type='hidden' name='eid' value='$eid' />";
	echo "<button name='go'>Modify</button>";
	echo "</form>\n";
    } else {
	if (! isset($nsdata)) {
	    USERERROR("NSdata CGI variable missing (How'd that happen?)",1);
	}

#	echo "<pre>$nsdata</pre>\n";
#	echo "<h1>2</h1>\n";

	$nsfile = tempnam("/tmp", "$pid-$eid.nsfile.");
	if (! ($fp = fopen($nsfile, "w"))) {
	    TBERROR("Could not create temporary file $nsfile", 1);
	}
	$nsdata_string = urldecode($nsdata);
	fwrite($fp, $nsdata_string);
	fclose($fp);
	chmod($nsfile, 0666);	

#	echo "<pre>$nsdata_string</pre>\n";

	if (system("$parser -n -a $nsfile") != 0) {
	    USERERROR("Modified NS file contains syntax errors; aborting.", 1);
	}

	#
	# Get exp group.
	#
	TBExptGroup($pid,$eid,$gid);

	#	
	# We need the unix gid for the project for running the scripts below.
	#
	TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);
#	echo "<pre>".
#	     "$TBSUEXEC_PATH $uid $unix_gid ".
#	     "webswapexp -s modify $pid $eid $nsfile".
#	     "</pre>\n";
 	#	
	# Avoid SIGPROF in child.
	# 
	set_time_limit(0);
	
	$output = array();
	$retval = 0;

	$rebootswitch = "";
	if ($reboot) { $rebootswitch = "-r"; }

	$result = exec("$TBSUEXEC_PATH $uid $unix_gid ".
		       "webswapexp $rebootswitch -s modify $pid $eid $nsfile",
		       $output, $retval);
	
	if ($retval) {
	    echo "<br /><br />\n".
		 "<h2>Modify failure($retval): Output as follows:</h2>\n".
		 "<br />".
		 "<xmp>\n";
	    for ($i = 0; $i < count($output); $i++) {
		echo "$output[$i]\n";
	    }
	    echo "</xmp>\n";

	    PAGEFOOTER();
	    die("");
	} else {      
	    #
	    # Exit status 0 means the experiment is swapping, or will be.
	    #
	    
	    echo "<br /><br />";
	    echo "<font size=+1>
	      <p>Experiment
              <a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a>
              in project <A href='showproject.php3?pid=$pid'>$pid</A>
              is being modified!</p><br />
	      <p>You will be notified via email when the operation is complete.
              This could take one to ten minutes, depending on
              whether nodes were added to the experiments, and whether
              disk images had to be loaded.</p>
              <p>While you are waiting, you can watch the log 
              in <a target=_blank href=spewlogfile.php3?pid=$pid&eid=$eid>
              realtime</a>.</p></font>\n";
	}
	
#	if ($delnsfile) {
#	    unlink($nsfile);
#	}
	
#	echo "<h1>If this did anything, It would have done it by now.</h1>\n";
    }
#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>


