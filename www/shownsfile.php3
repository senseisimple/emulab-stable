<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Visualization, NS File, and Details");

#
# Only known and logged in users can end experiments.
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

# if they dont exist, or are non-numeric, use defaults.
# note: one can use is_numeric in php4 instead of ereg.
if (!isset($zoom) || !ereg("^[0-9]{1,50}.?[0-9]{0,50}$", $zoom)) { $zoom = 1; }
if (!isset($detail) || !ereg("^[0-9]{1,50}$", $detail)) { $detail = 0; }
 
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

#
# Verify that this uid is a member of the project for the experiment
# being displayed.
#
if (!$isadmin) {
    $query_result =
	DBQueryFatal("SELECT pid FROM group_membership ".
		     "WHERE uid='$uid' and pid='$pid'");
    if (mysql_num_rows($query_result) == 0) {
        USERERROR("You are not a member of Project $pid!", 1);
    }
}

if ($showevents) {
    echo "<b><a href='shownsfile.php3?pid=$pid&eid=$eid'>
                Hide Event List</a>
          </b><br><br>\n";
}
elseif (TBEventCount($pid, $eid)) {
    echo "<b><a href='shownsfile.php3?pid=$pid&eid=$eid&showevents=1'>
                Show Event List</a>
          </b><br><br>\n";
}

#
# Spit out an image that refers to a php script. That script will run and
# send back the GIF image contents.
#
if (strcmp($expstate, $TB_EXPTSTATE_NEW) &&
    strcmp($expstate, $TB_EXPTSTATE_PRERUN)) {
    echo "<br>
          <blockquote><blockquote><blockquote><blockquote>
            <img src='top2image.php3?pid=$pid&eid=$eid&zoom=$zoom&detail=$detail' align=center>
	    <h5>
	      zoom:
	      <a href='shownsfile.php3?pid=$pid&eid=$eid&zoom=1.00&detail=$detail'>100%</a>
	      <a href='shownsfile.php3?pid=$pid&eid=$eid&zoom=1.12&detail=$detail'>112%</a>
	      <a href='shownsfile.php3?pid=$pid&eid=$eid&zoom=1.25&detail=$detail'>125%</a>
	      <a href='shownsfile.php3?pid=$pid&eid=$eid&zoom=1.50&detail=$detail'>150%</a>
	      <a href='shownsfile.php3?pid=$pid&eid=$eid&zoom=1.75&detail=$detail'>175%</a>
	      <a href='shownsfile.php3?pid=$pid&eid=$eid&zoom=2.00&detail=$detail'>200%</a>
	      <a href='shownsfile.php3?pid=$pid&eid=$eid&zoom=2.50&detail=$detail'>250%</a>
	      <a href='shownsfile.php3?pid=$pid&eid=$eid&zoom=3.00&detail=$detail'>300%</a>
	      <a href='shownsfile.php3?pid=$pid&eid=$eid&zoom=4.00&detail=$detail'>400%</a>
	      <br>";
    if ($detail == 0) {
	if ($zoom < 1.75) {
      	    echo "<a href='shownsfile.php3?pid=$pid&eid=$eid&zoom=1.75&detail=1'>More detail (and zoom)</a>";
	} else {
    	    echo "<a href='shownsfile.php3?pid=$pid&eid=$eid&zoom=$zoom&detail=1'>More detail</a>";
	}
    } else {
    	echo "<a href='shownsfile.php3?pid=$pid&eid=$eid&zoom=$zoom&detail=0'>Less detail</a>";
    }
    echo "  </h5>
          </blockquote></blockquote></blockquote></blockquote>\n";
}

echo "<br>
      <h3>NS File:</h3>\n";

$query_result =
    DBQueryFatal("SELECT nsfile from nsfiles where pid='$pid' and eid='$eid'");
if (mysql_num_rows($query_result)) {
    $row    = mysql_fetch_array($query_result);
    $nsfile = stripslashes($row[nsfile]);

    echo "<XMP>$nsfile</XMP>\n";
    flush();
}
else {
    echo "There is no stored NS file for $pid/$eid\n";
}

echo "<br>
      <h3>
         Experiment Details:
      </h3>\n";

$output = array();
$retval = 0;

if ($showevents) {
    $flags = "-v";
}
else {
    $flags = "-b";
}

$result = exec("$TBSUEXEC_PATH $uid $TBADMINGROUP webreport $flags $pid $eid",
 	       $output, $retval);

echo "<XMP>\n";
for ($i = 0; $i < count($output); $i++) {
    echo "$output[$i]\n";
}
echo "</XMP>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
