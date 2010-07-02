<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("experiment",   PAGEARG_EXPERIMENT);
$optargs = OptionalPageArguments("zoom",         PAGEARG_NUMERIC,
				 "detail",       PAGEARG_BOOLEAN,
				 "showevents",   PAGEARG_BOOLEAN,
				 "justns",       PAGEARG_BOOLEAN);

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
# Need these below
#
$pid = $experiment->pid();
$eid = $experiment->eid();
$shownsfile_url = CreateURL("shownsfile", $experiment);
$top2image_url  = CreateURL("top2image", $experiment);
$spitnsdata_url = CreateURL("spitnsdata", $experiment);

# if they dont exist, or are non-numeric, use defaults.
if (!isset($zoom))       { $zoom   = 1; }
if (!isset($detail))     { $detail = 0; }
if (!isset($justns))     { $justns = 0; }
if (!isset($showevents)) { $showevents = 0; }
 
#
# Must have permission to view experiment details.
#
if (!$isadmin &&
    !$experiment->AccessCheck($this_user, $TB_EXPT_READINFO)) {
    USERERROR("You do not have permission to view experiment details!", 1);
}

#
# if it is netbuild wanting an NS to modify, send that along
# (For now, as a disgusting hack, send node positioning along, too.)
#
if ($justns) {
    if (($nsfile = $experiment->NSFile())) {
	echo "$nsfile\n";

	$query_result = 
	    DBQueryFatal("SELECT vname, x, y FROM vis_nodes ".
			 "where pid='$pid' and eid='$eid'");

	while ($row = mysql_fetch_array($query_result)) {
	    $name = $row["vname"];
	    $x = $row["x"];
	    $y = $row["y"];
	    echo "tb-set-vis-position \$$name $x $y\n";
	}
	flush();
    }
    else {
	echo "No stored NS file for $pid/$eid\n";
    }    
    return;
}

echo $experiment->PageHeader();
echo "<br />\n";

if ($showevents) {
    echo "<b><a href='$shownsfile_url'>Hide Event List</a>
          </b><br><br>\n";
}
elseif ($experiment->EventCount()) {
    echo "<b><a href='${shownsfile_url}&showevents=1'>Show Event List</a>
          </b><br><br>\n";
}

#
# Spit out an image that refers to a php script. That script will run and
# send back the GIF image contents.
#
if (strcmp($experiment->state(), $TB_EXPTSTATE_NEW) &&
    strcmp($experiment->state(), $TB_EXPTSTATE_PRERUN)) {
    echo "<table cellpadding='0' cellspacing='0' border='0' class='stealth'>
	    <tr><td class='stealth' width='32'>&nbsp;</td>
            <td class='stealth'><center>
		<img src='${top2image_url}&zoom=$zoom&detail=$detail'
                     align=center>
	    <h5>
	      zoom: ";
    if ($zoom == 1.00) { 
	echo "<b>100%</b> "; 
    } else { 
	echo "<a href='${shownsfile_url}&zoom=1.00&detail=$detail'>100%</a> ";
    }
    if ($zoom == 1.12) { 
	echo "<b>112%</b> "; 
    } else { 
	echo "<a href='${shownsfile_url}&zoom=1.12&detail=$detail'>112%</a> ";
    }
    if ($zoom == 1.25) { 
	echo "<b>125%</b> "; 
    } else { 
	echo "<a href='${shownsfile_url}&zoom=1.25&detail=$detail'>125%</a> ";
    }
    if ($zoom == 1.50) { 
	echo "<b>150%</b> "; 
    } else { 
	echo "<a href='${shownsfile_url}&zoom=1.50&detail=$detail'>150%</a> ";
    }
    if ($zoom == 1.75) { 
	echo "<b>175%</b> "; 
    } else { 
	echo "<a href='${shownsfile_url}&zoom=1.75&detail=$detail'>175%</a> ";
    }
    if ($zoom == 2.00) { 
	echo "<b>200%</b> "; 
    } else { 
	echo "<a href='${shownsfile_url}&zoom=2.00&detail=$detail'>200%</a> ";
    }
    if ($zoom == 2.50) { 
	echo "<b>250%</b> "; 
    } else { 
	echo "<a href='${shownsfile_url}&zoom=2.50&detail=$detail'>250%</a> ";
    }
    if ($zoom == 3.00) { 
	echo "<b>300%</b> "; 
    } else { 
	echo "<a href='${shownsfile_url}&zoom=3.00&detail=$detail'>300%</a> ";
    }
    if ($zoom == 4.00) { 
	echo "<b>400%</b> "; 
    } else { 
	echo "<a href='${shownsfile_url}&zoom=4.00&detail=$detail'>400%</a> ";
    }
    echo "<br>detail: ";
    if ($detail == 0) {
	if ($zoom < 1.75) {
      	    echo "<a href='${shownsfile_url}&zoom=1.75&detail=1'>high</a> <b>low</b>";
	} else {
    	    echo "<a href='${shownsfile_url}&zoom=$zoom&detail=1'>high</a> <b>low</b>";
	}
    } else {
    	echo "<b>high</b> <a href='${shownsfile_url}&zoom=$zoom&detail=0'>low</a>";
    }
    echo "  </h5></center></td></tr></table>";
}

echo "<br>
      <a href='$spitnsdata_url'><h3>NS File:</h3></a>\n";

# Dump the NS file.
if (($nsfile = $experiment->NSFile())) {
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

$result =
    exec("$TBSUEXEC_PATH $uid $TBADMINGROUP webtbreport $flags $pid $eid",
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
