<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
chdir("..");
include("defs.php3");

$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

PAGEHEADER("Wireless/WSN Testbed Connectivity Statistics");

?>

<h2><a href="#instructions">Instructions</a></h2>

<?php

##
## need to find out what datasets exist
##
$dbq = DBQueryFatal("select ws.name,ws.floor,ws.building," .
		    "fi.scale,fi.pixels_per_meter " .
		    "from wireless_stats as ws left join floorimages as fi " .
		    "on fi.building=ws.building and ws.floor=fi.floor");

## now, here's the method:
## if there is more than one floor, only the primary should be listed.
## now, when the applet scans the position data, if it sees more floors than
## the primary, it calls up and downloads the maps it needs.  Thus, we want to
## send a bit for each dataset in the following format:
## "name,building,primary_floor,s1:s2:...:sN,f1:f2:...:fN,ppm1:ppm2:...:ppmN"
## each of these strings is then separated by a ';'

$count = -1;
$lpc = 0;
$strlist = "";

$names = array();
$floors = array();
$buildings = array();
$nameScales = array();
$nameScaleFactors = array();
$namePPMs = array();

while ($row = mysql_fetch_array($dbq)) {
    $name = $row["name"];

#    if ($name == 'SensorNet-MEB') {
#        continue;
#    }

    $floor = $row["floor"];
    $building = $row["building"];
    $scale = $row["scale"];
    $factor = 0;
    if ($scale == 1) {
        $factor = 1;
    }
    else if ($scale == 2) {
        $factor = 1.5;
    }
    else {
        $factor = $scale - 1;
    }
    $ppm = $row["pixels_per_meter"];

    # there should only be floor and building per name (this is determined
    # by the PRIMARY KEY  (building,floor,scale) in the floorimages table
    if (!in_array($name,$names)) {
        ++$count;
        
        $names[$count] = $name;
        $floors[$count] = $floor;
        $buildings[$count] = $building;

        $nameScales[$count] = array();
        $nameScaleFactors[$count] = array();
        $namePPMs[$count] = array();
        $lpc = 0;
    }

    # now, we *can* have more than one value for scale and ppm, but
    # each corresponds to a building-floor-name tuple:
    $nameScales[$count][$lpc] = $scale;
    $nameScaleFactors[$count][$lpc] = $factor;
    $namePPMs[$count][$lpc] = $ppm;
    ++$lpc;
}

## now we print out the big list:
$strlist = '';
for ($i = 0; $i <= $count; ++$i) {
    $n = $names[$i];
    $f = $floors[$i];
    $b = $buildings[$i];
    $sa = $nameScales[$i];
    $sf = $nameScaleFactors[$i];
    $ppma = $namePPMs[$i];

    if ($strlist != '') {
        $strlist .= ";";
    }

    $strlist .= "$n,$b,$f,";

    # now add the scales...
    for ($j = 0; $j < count($sa); ++$j) {
        if ($j != 0) {
            $strlist .= ":";
        }
        $strlist .= $sa[$j];
    }
    $strlist .= ",";
    # now add the scale factors...
    for ($j = 0; $j < count($sf); ++$j) {
        if ($j != 0) {
            $strlist .= ":";
        }
        $strlist .= $sf[$j];
    }
    $strlist .= ",";
    # now add the ppms...
    for ($j = 0; $j < count($ppma); ++$j) {
        if ($j != 0) {
            $strlist .= ":";
        }
        $strlist .= $ppma[$j];
    }

}     


$auth    = $HTTP_COOKIE_VARS[$TBAUTHCOOKIE];

## the requester will need to append '&dataset=...'
$mapurl = "getdata.php3?type=map";
$dataurl = "getdata.php3?type=data";
$positurl = "getdata.php3?type=posit";

echo "<applet name='wireless-stats' code='WirelessMapApplet.class'
              archive='wireless-stats.jar'
              width='1024' height='768'
              alt='You need java to run this applet'>
            <param name='uid' value='$uid'>
            <param name='auth' value='$auth'>
	    <param name='datasets' value='$strlist'>
	    <param name='mapurl' value='$mapurl'>
	    <param name='dataurl' value='$dataurl'>
            <param name='positurl' value='$positurl'>
     </applet>\n";

?>

<h3><a name="instructions">Instructions</a></h3>

This applet allows you to inspect the connectivity properties of
various Emulab wireless testbeds and areas.  At the moment, you
can only view statistics for the static WSN testbed, but soon 
you'll be able to inspect the various floors Emulab's wifi testbed
as well.  The display shows nodes, "links" between them, and the 
percentage of loss on each link.
<p>

To use this applet, simply select a dataset.  Then, if the dataset
offers multiple power levels, choose one that corresponds to your 
intended use.  By default, all wireless "links" that can transmit 
some data between any nodes are displayed.  You can select a different
mode if desirable.  "Select by source" means that links between nodes
you select in the list below and any other nodes are shown.  "Select
by receiver" means that all links between the receivers you selected 
and any senders are shown.  You can also modify the selected nodes by 
clicking on the map on the appropriate node circles.  To add a node to a
selection, hold down the shift key and left-click on the node.  To remove
a node from a selection, hold down the shift key and click the node.  To
remove all nodes from a selection, click anywhere on the map outside of any
nodes.  You can edit the current selection using the map, the list, or a
combination.
<p>

Since some maps may get cluttered with any links, we provide some 
limiting options to reduce the number of links shown in a sensible manner.  
You can limit by "k best neighbors", which filters out all but the 
N best links leaving or entering the selected nodes; N is the number 
you enter in the box just below this limit option.  You can also 
limit by setting a threshold; only nodes with loss rates above this 
threshold are shown.  To remove the limit, choose the "None" limit.

Finally, an additional option allows you to never see links with 0% 
connectivity.
<p>


<?php

PAGEFOOTER();

?>
