<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

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
if (!TBvalid_pid($pid)) {
    PAGEARGERROR("Invalid project ID.");
}
if (!TBvalid_eid($eid)) {
    PAGEARGERROR("Invalid experiment ID.");
}
$exptidx = TBExptIndex($pid, $eid);
if ($exptidx < 0) {
    TBERROR("Could not map $pid/$eid to ID!", 1);
}
if (!TBExptGroup($pid, $eid, $gid)) {
    TBERROR("Could not get experiment gid for $pid/$eid!", 1);
}

if (!$isadmin) {
    if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_READINFO)) {
	USERERROR("You do not have permission to view missing files for ".
		  "experiment $pid/$eid!", 1);
    }
}

#
# Move files requested.
#
if (isset($movesome)) {
    #
    # Go through the post list and find all the filenames.
    #
    $fileargs = "";
    
    while (list ($var, $value) = each ($_POST)) {
	if (preg_match('/^fn[\d]+$/', $var) &&
	    preg_match('/^([-\w\/\.\+\@,~]+)$/', $value)) {
	    $fileargs = "$fileargs " . escapeshellarg($value);
	}
    }
    SUEXEC($uid, "$pid,$gid",
	   "webarchive_control addtoarchive $pid $eid $fileargs",
	   SUEXEC_ACTION_DUPDIE);
    
    header("Location: archive_missing.php3?pid=$pid&eid=$eid");
    return;
}

#
# Standard Testbed Header
#
PAGEHEADER("Add Missing Files");

echo "<script language=JavaScript>
      <!--
          function NormalSubmit() {
              document.form1.target='_self';
              document.form1.submit();
          }
          function SelectAll() {
              var i;
              var setval;

              if (document.form1.selectall.value == 'Select All') {
                   document.form1.selectall.value = 'Clear All'
                   setval = true;
              }
              else {
                   document.form1.selectall.value = 'Select All'
                   setval = false;
              }

              for (i = 0; i < document.form1.elements.length; i++) {
                  var element = document.form1.elements[i];

                  if (element.type == 'checkbox') {
                      element.checked = setval;
                  }
              }
          }
          //-->
          </script>\n";

echo "<font size=+2>".
     "Experiment <b>".
     "<a href='showproject.php3?pid=$pid'>$pid</a>/".
     "<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a> ".
     "</b></font>\n";
     "<br>";
echo "<br>\n";

#
# We ask an external script for the list of missing files. 
#
SUEXEC($uid, "$pid,$gid",
       "webarchive_control missing $pid $eid",
       SUEXEC_ACTION_DIE);

#
# Show the user the output.
#
if (count($suexec_output_array)) {
    echo "<br>".
	"<b>These files have been referenced by your experiment, but ".
	"are not contained within the experiments archive directory. ".
	"Selecting these files will move them into the experiment archive ".
	"directory, leaving symlinks behind.";
    echo "</b><br><br>";

    echo "<table border=1>\n";
    echo "<form action='archive_missing.php3?pid=$pid&eid=$eid'
                onsubmit=\"return false;\"
                name=form1 method=post>\n";
    echo "<input type=hidden name=movesome value=Submit>\n";    
    echo "<tr><td align=center colspan=2>\n";
    echo "<input type=button name=movesome value='Move Selected'
                 onclick=\"NormalSubmit();\"></b>";
    echo "&nbsp &nbsp &nbsp ";
    echo "<input type=button name=selectall value='Select All'
                 onclick=\"SelectAll();\"></b>";
    echo "</td></tr>\n";

    echo "<tr>
           <th>Pathname</th>
           <th>Move to Archive</th>
          </tr>\n";
    
    for ($i = 0; $i < count($suexec_output_array); $i++) {
	$fn = rtrim($suexec_output_array[$i]);
	$name = "fn$i";
	
	echo "<tr>\n";
	echo "<td><tt>" . $fn . "</tt></td>";
	echo "<td><input type=checkbox name=$name value='$fn'>&nbsp</td>\n";
	echo "</tr>\n";
    }
    echo "</form>\n";
    echo "</table>\n";
}

#
# Standard Testbed Footer
#
PAGEFOOTER();
?>
