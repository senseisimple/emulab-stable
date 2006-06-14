<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2005 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

$parser   = "$TB/libexec/ns2ir/parse-ns";
$TMPDIR   = "/tmp/";

#
# Only known and logged in users can modify experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

if (isset($formfields['exp_localnsfile'])) {
    $exp_localnsfile = $formfields['exp_localnsfile'];
}

# This will not return if its a sajax request.
include("showlogfile_sup.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Modify Experiment");

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
# Check to make sure this is a valid experiment.
#
if (! TBValidExperiment($pid, $eid)) {
    USERERROR("The experiment $eid is not a valid experiment ".
	      "in project $pid.", 1);
}

if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_MODIFY)) {
    USERERROR("You do not have permission to modify this experiment.", 1);
}

if (TBExptLockedDown($pid, $eid)) {
    USERERROR("Cannot proceed; experiment is locked down!", 1);
}

$expstate = TBExptState($pid, $eid);

if (strcmp($expstate, $TB_EXPTSTATE_ACTIVE) &&
    strcmp($expstate, $TB_EXPTSTATE_SWAPPED)) {
    USERERROR("You cannot modify an experiment in transition.", 1);
}

# Okay, start.
echo "<font size=+2>Experiment <b>".
     "<a href='showproject.php3?pid=$pid'>$pid</a>/".
     "<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a></b></font>\n";
echo "<br>\n";
flush();

#
# Put up the modify form on first load.
# 
if (! isset($go)) {
    echo "<a href='faq.php3#swapmod'>".
	 "Modify Experiment Documentation (FAQ)</a></h3>";

    echo "<script language=JavaScript>
          <!--
          function NormalSubmit() {
              document.form1.target='_self';
              document.form1.submit();
          }
          function SyntaxCheck() {
              window.open('','nscheck','width=650,height=400,toolbar=no,".
                              "resizeable=yes,scrollbars=yes,status=yes,".
                              "menubar=yes');
              var action = document.form1.action;
              var target = document.form1.target;

              document.form1.action='nscheck.php3';
              document.form1.target='nscheck';
              document.form1.submit();

              document.form1.action=action;
              document.form1.target=target;
          }
          //-->
          </script>\n";

    echo "<table align=center border=1>\n";
    if (STUDLY()) {
	echo "<tr><th colspan=2><font size='+1'>".
	    "<a href='clientui.php3?pid=$pid&eid=$eid'>GUI Editor</a>".
	    " - Edit the topology using a Java applet.</font>";
	echo "</th></tr>";
    }

    echo "<form name='form1' action='modifyexp.php3?pid=$pid&eid=$eid&go=1' method='post' onsubmit=\"return false;\" enctype=multipart/form-data>";
    echo "<tr><th>Upload new NS file: </th>";
    echo "<td><input type=hidden name=MAX_FILE_SIZE value=512000>";
    echo "<input type=file name=exp_nsfile size=30 onchange=\"this.form.syntax.disabled=(this.value=='')\" /></td></tr>\n";
    echo "<tr><th><em>or</em> NS file on server: </th> ";
    echo "<td><input type=text name=\"formfields[exp_localnsfile]\" size=40 onchange=\"this.form.syntax.disabled=(this.value=='')\" /> </td</tr>\n";
    
    echo "<tr><td colspan=2><b><em>or</em> Edit:</b><br>\n";
    echo "<textarea cols='100' rows='40' name='nsdata' onchange=\"this.form.syntax.disabled=(this.value=='')\">";

    $query_result =
	DBQueryFatal("SELECT nsfile from nsfiles ".
		     "where pid='$pid' and eid='$eid'");
    if (mysql_num_rows($query_result)) {
	$row    = mysql_fetch_array($query_result);
	$nsfile = $row[nsfile];
	    
	echo "$nsfile";
    }
    else {
	echo "# There was no stored NS file for $pid/$eid.\n";
    }
	
    echo "</textarea>";
    echo "</td></tr>\n";
    if (!strcmp($expstate, $TB_EXPTSTATE_ACTIVE)) {
	echo "<tr><td colspan=2><p><b>Note!</b> It is recommended that you 
	      reboot all nodes in your experiment by checking the box below.
	      This is especially important if changing your experiment
              topology (adding or removing nodes, links, and LANs).
	      If adding/removing a delay to/from an existing link, or
              replacing a lost node <i>without modifying the experiment
              topology</i>, this won't be necessary. Restarting the
	      event system is also highly recommended since the same nodes
	      in your virtual topology may get mapped to different physical
	      nodes.</p>";
	echo "<input type='checkbox' name='reboot' value='1' checked='1'>
	      Reboot nodes in experiment (Highly Recommended)</input>";
	echo "<br><input type='checkbox' name='eventrestart' value='1' checked='1'>
	      Restart Event System in experiment (Highly Recommended)</input>";
      echo "</td></tr>";
    }
    echo "<tr><th colspan=2><center>";
    echo "<input type=submit disabled id=syntax name=syntax value='Syntax Check' onclick=\"SyntaxCheck();\"> ";
    echo "<input type=button name='go' value='Modify' onclick='NormalSubmit();'>\n";
    echo "<input type='reset'>";
    echo "</center></th></tr>\n";
    echo "</table>\n";
    echo "</form>\n";
    PAGEFOOTER();
    exit();
}

#
# Okay, form has been submitted.
#
$speclocal  = 0;
$specupload = 0;
$specform   = 0;
$nsfile     = "";
$tmpfile    = 0;

if (isset($exp_localnsfile) && strcmp($exp_localnsfile, "")) {
    $speclocal = 1;
}
if (isset($exp_nsfile) && strcmp($exp_nsfile, "") &&
    strcmp($exp_nsfile, "none")) {
    $specupload = 1;
}
if (!$speclocal && !$specupload && isset($nsdata))  {
    $specform = 1;
}

if ($speclocal + $specupload + $specform > 1) {
    USERERROR("You may not specify both an uploaded NS file and an ".
	      "NS file that is located on the Emulab server", 1);
}
if (!$specupload && strcmp($exp_nsfile_name, "")) {
    #
    # Catch an invalid filename.
    #
    USERERROR("The NS file '$exp_nsfile_name' does not appear to be a ".
	      "valid filename. Please go back and try again.", 1);
}

#
# Gotta be one of them!
#
if (!$speclocal && !$specupload && !$specform) {
    USERERROR("You must supply an NS file!", 1);
}

if ($speclocal) {
    #
    # No way to tell from here if this file actually exists, since
    # the web server runs as user nobody. The startexp script checks
    # for the file before going to ground, so the user will get immediate
    # feedback if the filename is bogus.
    #
    # Do not allow anything outside of /users or /proj. I do not think there
    # is a security worry, but good to enforce it anyway.
    #
    if (!preg_match("/^([-\@\w\.\/]+)$/", $exp_localnsfile)) {
	USERERROR("NS File", "Pathname includes illegal characters", 1);
    }
    if (! ereg("^$TBPROJ_DIR/.*" ,$exp_localnsfile) &&
        ! ereg("^$TBUSER_DIR/.*" ,$exp_localnsfile) &&
        ! ereg("^$TBGROUP_DIR/.*" ,$exp_localnsfile)) {
	USERERROR("NS File: You must specify a server resident file in either ".
                  "$TBUSER_DIR/ or $TBPROJ_DIR/", 1);
    }
    
    $nsfile = $exp_localnsfile;
    $nonsfile = 0;
}
elseif ($specupload) {
    #
    # XXX
    # Set the permissions on the NS file so that the scripts can get to it.
    # It is owned by nobody, and most likely protected. This leaves the
    # script open for a short time. A potential security hazard we should
    # deal with at some point.
    #
    chmod($exp_nsfile, 0666);
    $nsfile = $exp_nsfile;
    $nonsfile = 0;
} else # $specform
if ($specform) {
    #
    # Take the NS file passed in from the form and write it out to a file
    #
    $tmpfile = 1;

    #
    # Generate a hopefully unique filename that is hard to guess.
    # See backend scripts.
    # 
    list($usec, $sec) = explode(' ', microtime());
    srand((float) $sec + ((float) $usec * 100000));
    $foo = rand();

    $nsfile = "/tmp/$uid-$foo.nsfile";
    $handle = fopen($nsfile,"w");
    fwrite($handle,$nsdata);
    fclose($handle);
    chmod($nsfile, 0666);
}

#
# Get exp group so we can get the unix_gid.
#
TBExptGroup($pid, $eid, $gid);
TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);

#
# Do an initial parse test.
#
$retval = SUEXEC($uid, "$pid,$unix_gid", "webnscheck $nsfile",
		 SUEXEC_ACTION_IGNORE);

if ($retval != 0) {
    if ($tmpfile) {
        unlink($nsfile);
    }
    
    # Send error to tbops.
    if ($retval < 0) {
	SUEXECERROR(SUEXEC_ACTION_CONTINUE);
    }
    echo "<br>";
    echo "<h3>Modified NS file contains syntax errors</h3>";
    echo "<blockquote><pre>$suexec_output<pre></blockquote>";

    PAGEFOOTER();
    exit();
}

#	
# Avoid SIGPROF in child.
# 
set_time_limit(0);

# Run the script.
$retval = SUEXEC($uid, "$pid,$unix_gid",
		 "webswapexp $rebootswitch " . ($reboot ? "-r " : "") .
		 ($eventrestart ? "-e " : "") .
		 "-s modify $pid $eid $nsfile",
		 SUEXEC_ACTION_IGNORE);
		 
# It has been copied out by the program!
if ($tmpfile) {
    unlink($nsfile);
}

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

#
# Exit status 0 means the experiment is swapping, or will be.
#
echo "<br>\n";
if ($retval) {
    echo "<h3>Experiment modify could not proceed</h3>";
    echo "<blockquote><pre>$suexec_output<pre></blockquote>";
}
else {
    #
    # Exit status 0 means the experiment is modifying.
    #
    echo "<b>Your experiment is being modified!</b> ";
    echo "You will be notified via email when the experiment has ".
	"finished modifying and you are able to proceed. This ".
	"typically takes less than 10 minutes, depending on the ".
	"number of nodes in the experiment. ".
	"If you do not receive email notification within a ".
	"reasonable amount time, please contact $TBMAILADDR. ".
	"<br><br>".
	"While you are waiting, you can watch the log of experiment ".
	"modification in realtime:<br><br>\n";
    STARTLOG($pid, $eid);    
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>


