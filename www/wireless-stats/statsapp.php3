<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
chdir("..");
include("defs.php3");

$uid = GETLOGIN();
LOGGEDINORDIE($uid);

PAGEHEADER("Wireless/WSN Testbed Connectivity Statistics");

## now do some text

?>

<h3>Instructions</h3>

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

##
## need to find out what datasets exist
##
$dbq = DBQueryFatal("select name from wireless_stats");
$count = 0;
$strlist = "";

while ($row = mysql_fetch_array($dbq)) {
    $name = $row["name"];
    if ($count > 0) {
        $strlist = $strlist . ",";
    }
    ++$count;

    $strlist = $strlist . $name;
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


PAGEFOOTER();
?>
