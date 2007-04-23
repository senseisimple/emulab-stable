<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2004, 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users can see this information
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("node",       PAGEARG_NODE,
				 "formfields", PAGEARG_ARRAY);

if (isset($node)) {
    $node_id = $node->node_id();
    
    PAGEHEADER("Wide Area Link Characteristics For $node_id");
} else {
    PAGEHEADER("Wide Area Link Characteristics");
}

#
#
#
function SPITDATA($table, $title, $formfields)
{
    #
    # Grab a couple of the formfields that we are gonna use a lot
    #
    $node_id     = $formfields["node_id"];
    $dst_node_id = $formfields["dst_node_id"];
    $sortby      = $formfields["sortby"];

    #
    # Provide a default sort order
    #
    if (!$sortby) {
	$sortby = "node";
    }

    #
    # Figure out what order to sort in
    #
    if (!strcmp($sortby,"bw")) {
	$sortclause = "t.bandwidth desc";
    } elseif (!strcmp($sortby,"delay")) {
	$sortclause = "t.time";
    } elseif (!strcmp($sortby,"plr")) {
	$sortclause = "t.lossrate";
    } else {
	$sortclause = "n.priority";
    }

    #
    # Do we have a destination node, or are we grabbing all destinations for
    # the source node?
    #
    if ($node_id) {
	$whereclause = "t.node_id1='$node_id'";
    } else {
	$whereclause = "1";
    }
    if ($dst_node_id) {
	$whereclause .= " and t.node_id2='$dst_node_id'";
    }

    #
    # Handle constraints on the bandwidth, latency, and PLR
    #
    if ($formfields["min_bw"]) {
	$min_bw = $formfields["min_bw"];
	$whereclause .= " and t.bandwidth >= $min_bw";
    }
    if ($formfields["max_bw"]) {
	$max_bw = $formfields["max_bw"];
	$whereclause .= " and t.bandwidth <= $max_bw";
    }

    # Convert from percent to fractional
    if ($formfields["min_plr"]) {
	$min_plr = $formfields["min_plr"] / 100.0;
	$whereclause .= " and t.lossrate >= $min_plr";
    }
    if ($formfields["max_plr"]) {
	$max_plr = $formfields["max_plr"] / 100.0;
	$whereclause .= " and t.lossrate <= $max_plr";
    }

    # Convert from ms to seconds
    if ($formfields["min_latency"]) {
	$min_latency = $formfields["min_latency"] / 1000.0;
	$whereclause .= " and t.time >= $min_latency";
    }
    if ($formfields["max_latency"]) {
	$max_latency = $formfields["max_latency"] / 1000.0;
	$whereclause .= " and t.time <= $max_latency";
    }

    #
    # Start the table and explain its contents
    #
    echo "<center>
	   <b>$title</b><br>\n";

    echo "</center><br>\n";

    echo "<center>
    Delay in milliseconds, Bandwidth in KB/s, PLR rounded to two
    decimal places (0-100%).
    </center>\n";

    echo "<table border=2 
		 cellpadding=1 cellspacing=2 align=center>
	   <tr>";

    #
    # Make the column headers links to change the sort order, unless we are
    # aready sorting by the column - also, if we have a desitnation node (and
    # thus are only displaying one row), sorting is pointless,
    #
    if ($dst_node_id || !strcmp($sortby,"node")) {
	echo "   <th>Node</th>\n";
    } else {
	echo "   <th>" . makeURL("Node",$formfields,$node_id,"node")
	     . "</th>\n";
    }
    if (!$node_id) {
	if ($dst_node_id || !strcmp($sortby,"node")) {
	    echo "   <th>Node</th>\n";
	} else {
	    echo "   <th>" . makeURL("Node",$formfields,$node_id,"node")
	         . "</th>\n";
	}
    }
    if ($dst_node_id || !strcmp($sortby,"delay")) {
	echo "   <th>Delay</th>\n";
    } else {
	echo "   <th>" . makeURL("Delay",$formfields,$node_id,"delay")
	     . "</th>\n";
    }
    if ($dst_node_id || !strcmp($sortby,"bw")) {
	echo "   <th>BW</th>\n";
    } else {
	echo "   <th>" . makeURL("BW",$formfields,$node_id,"bw") . "</th>\n";
    }
    if ($dst_node_id || !strcmp($sortby,"plr")) {
	echo "   <th>PLR</th>\n";
    } else {
	echo "   <th>" . makeURL("PLR",$formfields,$node_id,"plr")
	     . "</th>\n";
    }
    echo "   <th>Time Span</th>\n";
    if ($node_id) {
	echo "   <th>Hostname</th>\n";
    }
    echo " </tr>\n";

    $query_result =
	DBQueryFatal("select t.node_id1, t.node_id2, t.time, t.bandwidth, " .
	             "   t.lossrate, t.start_time, t.end_time, i.hostname " .
		     "from $table as t ".
		     "left join nodes as n on n.node_id=t.node_id1 ".
		     "left join widearea_nodeinfo as i on t.node_id2=i.node_id".
		     " where $whereclause order by $sortclause");

    while ($row = mysql_fetch_array($query_result)) {
	$node_id1 = $row["node_id1"];
	$node_id2 = $row["node_id2"];
	$time     = $row["time"];
	$bw       = $row["bandwidth"];
	$plr      = $row["lossrate"];
	$start    = $row["start_time"];
	$end      = $row["end_time"];
	$hostname = $row["hostname"];
    
	#
	# Convert a few columns to the right formats
	#
	$speed = $time * 1000;
	$bws   = $bw;
	$plrs  = $plr * 100.0;
	$dates = strftime("%m/%d-%H:%M", $start) . " to " .
		 strftime("%m/%d-%H:%M", $end);
	
	if (strcmp($node_id1, $node_id2)) {
	    echo "<tr>\n";
	    #
	    # Provide a link the equivalent page for each node
	    #
	    if (!$node_id) {
		echo "<td> " . makeURL($node_id1,$formfields,$node_id1,$sortby)
		           . "</td>\n";
	    }
	    echo "<td> " . makeURL($node_id2,$formfields,$node_id2,$sortby)
	                 . "</td>\n";

	    echo "<td>$speed</td>\n";
	    echo "<td>$bws</td>\n";
	    echo "<td>$plrs</td>\n";
	    echo "<td><nobr>$dates</nobr></td>\n";
	    if ($node_id) {
		echo "<td><code>$hostname</code></td>\n";
	    }
	    echo "</tr>\n";
	} else {
	    #
	    # Skip any data from a node to itself
	    #
	    continue;
	}
    }
    echo "</table>\n";
    echo "<br>\n";
}

#
# Form to let the user specify which node(s) and link characteristics they are
# interested in
#
function SPITFORM($formfields) {

    echo "<form action='widearea_nodeinfo.php3' method='get'>
	  <table align='center'>
	    <tr>
		<td colspan=2>
		<b>Search for link characteristics</b><br>
		All search terms are optional, but you must supply at least one
		</td>
	    </tr>
	    <tr>
		<td>Source Node:</td>
		<td><input name='formfields[node_id]'
		     value='$formfields[node_id]' size=10></td>
	    </tr>
	    <tr>
		<td>Destination Node: <i>(requires source node)<i></td>
		<td><input name='formfields[dst_node_id]'
		     value='$formfields[dst_node_id]' size=10></td>
	    </tr>
	    <tr>
		<td>Min bandwidth: <i>(KBps)</i></td>
		<td><input name='formfields[min_bw]'
		     value='$formfields[min_bw]' size=4></td>
	    </tr>
	    <tr>
		<td>Max bandwidth:</td>
		<td><input name='formfields[max_bw]'
		     value='$formfields[max_bw]' size=4></td>
	    </tr>
	    <tr>
		<td>Min latency: <i>(ms)</i></td>
		<td><input name='formfields[min_latency]'
		     value='$formfields[min_latency]' size=4></td>
	    </tr>
	    <tr>
		<td>Max latency:</td>
		<td><input name='formfields[max_latency]'
		     value='$formfields[max_latency]' size=4></td>
	    </tr>
	    <tr>
		<td>Min packet loss: <i>(%)</i></td>
		<td><input name='formfields[min_plr]'
		     value='$formfields[min_plr]' size=4></td>
	    </tr>
	    <tr>
		<td>Max packet loss:</td>
		<td><input name='formfields[max_plr]'
		     value='$formfields[max_plr]' size=4></td>
	    </tr>
            <input type=hidden name='formfields[sortby]'
                      value='$formfields[sortby]'>
	    <tr>
		<td colspan=2 align='center'>
		    <input type='submit' name='Submit' value='Submit'>
		</td>
	    </tr>
	  </table>
	  </form>
	  <br>\n";
}

#
# Make a URL for this page, going to the given node_id and using the given
# sorting scheme (either of which can be empty, to avoid overriding the current
# value), with $text as the anchor text
#
function makeURL($text,$formfields,$node_id,$sortby) {
    $fieldcopy = $formfields;
    if ($node_id) {
	$fieldcopy["node_id"] = $node_id;
    }
    if ($sortby) {
	$fieldcopy["sortby"] = $sortby;
    }
    $fieldstrings = array();
    foreach ($fieldcopy as $field => $value) {
	array_push($fieldstrings, "formfields[$field]=$value");
    }
    $args = implode("&",$fieldstrings);
    return "<a href=\"widearea_nodeinfo.php3?$args\">$text</a>";
}

$err = false;
if (!isset($formfields)) {
    $formfields = array();
    
    $formfields["node_id"]	= "";
    $formfields["dst_node_id"]	= "";
    $formfields["min_bw"]	= "";
    $formfields["max_bw"]	= "";
    $formfields["min_latency"]	= "";
    $formfields["max_latency"]	= "";
    $formfields["min_plr"]	= "";
    $formfields["max_plr"]	= "";
    $formfields["sortby"]	= "node";
    
    SPITFORM($formfields);
}
else {
    if ($formfields["node_id"] &&
	!TBvalid_node_id($formfields["node_id"])) {
	$err = true;
	USERERROR("Invalid characters in node_id");
    }
    if ($formfields["dst_node_id"] &&
	!TBvalid_node_id($formfields["dst_node_id"])) {
	$err = true;
	USERERROR("Invalid characters in dst_node_id");
    }
    if ($formfields["min_bw"] &&
	!TBvalid_integer($formfields["min_bw"])) {
	$err = true;
	USERERROR("Invalid characters in min_bw");
    }
    if ($formfields["max_bw"] &&
	!TBvalid_integer($formfields["max_bw"])) {
	$err = true;
	USERERROR("Invalid characters in max_bw");
    }
    if ($formfields["min_latency"] &&
	!TBvalid_integer($formfields["min_latency"])) {
	$err = true;
	USERERROR("Invalid characters in min_latency");
    }
    if ($formfields["max_latency"] &&
	!TBvalid_integer($formfields["max_latency"])) {
	$err = true;
	USERERROR("Invalid characters in max_latency");
    }
    if ($formfields["min_plr"] &&
	!TBvalid_float($formfields["min_plr"])) {
	$err = true;
	USERERROR("Invalid characters in min_plr");
    }
    if ($formfields["max_plr"] &&
	!TBvalid_float($formfields["max_plr"])) {
	$err = true;
	USERERROR("Invalid characters in max_plr");
    }
    
    SPITFORM($formfields);
    if (! $err) {
	SPITDATA("widearea_recent", "Most Recent Data", $formfields);
	SPITDATA("widearea_delays", "Aged Data", $formfields);
    }
}

#
# Standard Testbed Footer
#
PAGEFOOTER();
?>
