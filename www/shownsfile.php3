<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
if (!isset($justns)) {
    $justns = 0;
}

if (!$justns) {
    PAGEHEADER("Visualization, NS File, and Details");
}

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

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

#
# if it is netbuild wanting an NS to modify, send that along
# (For now, as a disgusting hack, send node positioning along, too.)
#
if ($justns) {
    $query_result =
	DBQueryFatal("SELECT nsfile from nsfiles where pid='$pid' and eid='$eid'");
    if (mysql_num_rows($query_result)) {
	$row    = mysql_fetch_array($query_result);
	$nsfile = $row[nsfile];
	
	echo "$nsfile\n";
	# flush();

	$query_result = 
	    DBQueryFatal("SELECT vname, x, y FROM vis_nodes where pid='$pid' and eid='$eid'");

	while ($row = mysql_fetch_array($query_result)) {
	    $name = $row[vname];
	    $x = $row[x];
	    $y = $row[y];
	    echo "tb-set-vis-position \$$name $x $y\n";
	}
	flush();
    }
    else {
	echo "No stored NS file for $pid/$eid\n";
    }    

    return;
}

echo "<font size=+2><b>".
     "<a href='showproject.php3?pid=$pid'>$pid</a>/".
     "<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a>".
     "</b></font>\n";
echo "<br />\n";
#echo "<br />\n";

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
    echo "<table cellpadding='0' cellspacing='0' border='0' class='stealth'>
	    <tr><td class='stealth' width='32'>&nbsp;</td>
            <td class='stealth'><center>
		<img src='top2image.php3?pid=$pid&eid=$eid&zoom=$zoom&detail=$detail' align=center>
	    <h5>
	      zoom: ";
    if ($zoom == 1.00) { 
	echo "<b>100%</b> "; 
    } else { 
	echo "<a href='shownsfile.php3?pid=$pid&eid=$eid&zoom=1.00&detail=$detail'>100%</a> ";
    }
    if ($zoom == 1.12) { 
	echo "<b>112%</b> "; 
    } else { 
	echo "<a href='shownsfile.php3?pid=$pid&eid=$eid&zoom=1.12&detail=$detail'>112%</a> ";
    }
    if ($zoom == 1.25) { 
	echo "<b>125%</b> "; 
    } else { 
	echo "<a href='shownsfile.php3?pid=$pid&eid=$eid&zoom=1.25&detail=$detail'>125%</a> ";
    }
    if ($zoom == 1.50) { 
	echo "<b>150%</b> "; 
    } else { 
	echo "<a href='shownsfile.php3?pid=$pid&eid=$eid&zoom=1.50&detail=$detail'>150%</a> ";
    }
    if ($zoom == 1.75) { 
	echo "<b>175%</b> "; 
    } else { 
	echo "<a href='shownsfile.php3?pid=$pid&eid=$eid&zoom=1.75&detail=$detail'>175%</a> ";
    }
    if ($zoom == 2.00) { 
	echo "<b>200%</b> "; 
    } else { 
	echo "<a href='shownsfile.php3?pid=$pid&eid=$eid&zoom=2.00&detail=$detail'>200%</a> ";
    }
    if ($zoom == 2.50) { 
	echo "<b>250%</b> "; 
    } else { 
	echo "<a href='shownsfile.php3?pid=$pid&eid=$eid&zoom=2.50&detail=$detail'>250%</a> ";
    }
    if ($zoom == 3.00) { 
	echo "<b>300%</b> "; 
    } else { 
	echo "<a href='shownsfile.php3?pid=$pid&eid=$eid&zoom=3.00&detail=$detail'>300%</a> ";
    }
    if ($zoom == 4.00) { 
	echo "<b>400%</b> "; 
    } else { 
	echo "<a href='shownsfile.php3?pid=$pid&eid=$eid&zoom=4.00&detail=$detail'>400%</a> ";
    }
    echo "<br>detail: ";
    if ($detail == 0) {
	if ($zoom < 1.75) {
      	    echo "<a href='shownsfile.php3?pid=$pid&eid=$eid&zoom=1.75&detail=1'>high</a> <b>low</b>";
	} else {
    	    echo "<a href='shownsfile.php3?pid=$pid&eid=$eid&zoom=$zoom&detail=1'>high</a> <b>low</b>";
	}
    } else {
    	echo "<b>high</b> <a href='shownsfile.php3?pid=$pid&eid=$eid&zoom=$zoom&detail=0'>low</a>";
    }
    echo "  </h5></center></td></tr></table>";
}

echo "<br>
      <a href=spitnsdata.php3?pid=$pid&eid=$eid><h3>NS File:</h3></a>\n";

$query_result =
    DBQueryFatal("SELECT nsfile from nsfiles where pid='$pid' and eid='$eid'");
if (mysql_num_rows($query_result)) {
    $row    = mysql_fetch_array($query_result);
    $nsfile = $row[nsfile];

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
    # Show event summary and firewall info.
    $flags = "-b -e -f";
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
