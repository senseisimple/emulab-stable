<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Delay Control");

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

$isadmin = ISADMIN($uid);
if (!$isadmin) {
	USERERROR("This feature not available to non-admins yet.", 1);
}

if ($check == "1") {
    #
    # Must provide the PID/EID!
    # 
    if (!isset($pid) ||
	strcmp($pid, "") == 0) {
	USERERROR("The project ID was not provided!", 1);
    }

    if (!isset($eid) ||
	strcmp($eid, "") == 0) {
	USERERROR("The experiment ID was not provided!", 1);
    }
    if (!TBValidExperiment($pid, $eid)) {
	USERERROR("No such experiment $eid in project $eid!", 1);
    }
    
    echo "<pre>\n";

    while (list ($header, $value) = each ($HTTP_POST_VARS)) {
	$changestring = strstr($header, "DC::");
	if (! $changestring) {
	    continue;
	}

	# Too bad sscanf is broken ...
	$temp      = substr($header, 4);
	$param     = substr($temp, 0, strpos($temp, "::"));
	$temp      = substr($temp, strpos($temp, "::") + 2);
	$lan       = substr($temp, 0, strpos($temp, "::"));
	$vnode     = substr($temp, strpos($temp, "::") + 2);

	#echo "$param $lan $vnode $value<br>\n";

	#
	# Must taint check! This stuff is going to a shell program. 
	# 
	if (!ereg("^[0-9a-zA-Z\-\_]*$", $param) ||
	    !ereg("^[0-9a-zA-Z\-\_]*$", $lan) ||
	    !ereg("^[0-9a-zA-Z\-\_]*$", $vnode) ||
	    !ereg("^[0-9\.]*$", "$value") ||
	    !is_numeric($value)) {
	    continue;
	}

        #
        # XXX Running one command per change is terrible! Better to build
        # up a command string for each lan/vnode! 
        # 
	$cmd = "webdelay_config -s $vnode $pid $eid $lan ${param}=${value} ";
	#echo "Running '$cmd':\n";
	if ($fp = popen("$TBSUEXEC_PATH $uid $pid $cmd", "r")) {
	    fpassthru($fp);
	}
    }
    echo "</pre>\n";
}

$result_delays =
    DBQueryFatal("SELECT * FROM delays WHERE eid='$eid' AND pid='$pid'");
$result_linkdelays =
    DBQueryFatal("SELECT * FROM linkdelays WHERE eid='$eid' AND pid='$pid'");

if (mysql_num_rows($result_delays) == 0 &&
    mysql_num_rows($result_linkdelays) == 0) {
    USERERROR("No running delay nodes with eid='$eid' and pid='$pid'!", 1);
}

print "<form action='delaycontrol.php3' method=post>\n" .
      "<input type=hidden name=check value=1 />\n" .
      "<input type=hidden name=eid value='$eid' />\n" .
      "<input type=hidden name=pid value='$pid' />\n" .
      "<table>\n" .
      "<tr>" .
      "<th>Link Name</th>".
      "<th>Node</th>".
      "<th>Delay (msec)</th>".
      "<th>Bandwidth (kb/s)</th>".
      "<th>Loss (ratio)</th>".
      "</tr>";
           
$num = mysql_num_rows( $result_delays );
for( $i = 0; $i < $num; $i++ ) {
   $field = mysql_fetch_array( $result_delays );

   $vlan   = $field["vname"];
   $vnode0 = $field["vnode0"];
   $vnode1 = $field["vnode1"];

   echo "<tr>\n";
   echo "  <td>$vlan</td>\n";
   echo "  <td>$vnode0</td>\n";
   echo "  <td> " . $field["delay0"] . 
       "<br><input type=text name=\"DC::delay::$vlan::$vnode0\" size=6/>" .
       "</td>\n";
   echo "  <td> " . $field["bandwidth0"] . 
       "<br><input type=text name=DC::bandwidth:$vlan:$vnode0 size=6/>" .
       "</td>\n";
   echo "  <td> " . $field["lossrate0"] . 
       "<br><input type=text name=DC::plr:$vlan:$vnode0 size=6/>" .
       "</td>\n";
   echo "</tr>\n";

   #
   # If vnode0 eq vnode1, its a lan node. Print differently.
   # Note, we do not allow them to change lan nodes asymmetrically.
   #
   echo "<tr>\n";
   if (strcmp($field["vnode0"], $field["vnode1"])) {
       echo "  <td>$vlan</td>\n";
       echo "  <td>$vnode1</td>\n";
       echo "  <td> " . $field["delay1"] . 
	   "<br><input type=text name=DC::delay:$vlan:$vnode1 size=6/>" .
	   "</td>\n";
       echo "  <td> " . $field["bandwidth1"] . 
	   "<br><input type=text name=DC::bandwidth:$vlan:$vnode1 size=6/>" .
	   "</td>\n";
       echo "  <td> " . $field["lossrate1"] . 
	   "<br><input type=text name=DC::plr:$vlan:$vnode1 size=6/>" .
	   "</td>\n";
   }
   else {
       if (0) {
	   echo "  <td>&nbsp</td>\n";
	   echo "  <td>&nbsp</td>\n";
	   echo "  <td> " . $field["delay1"] . "</td>\n";
	   echo "  <td> " . $field["bandwidth1"] . "</td>\n";
	   echo "  <td> " . $field["lossrate1"] . "</td>\n";
       }
   }
   echo "</tr>\n";
}

$num = mysql_num_rows( $result_linkdelays );
for( $i = 0; $i < $num; $i++ ) {
   $field = mysql_fetch_array( $result_linkdelays );

   $vlan  = $field["vlan"];
   $vnode = $field["vnode"];

   echo "<tr>\n";
   echo "  <td>$vlan</td>\n";
   echo "  <td>$vnode</td>\n";
   echo "  <td> " . $field["delay"] . 
       "<br><input type=text name=DC::delay:$vlan:$vnode size=6/>" .
       "</td>\n";
   echo "  <td> " . $field["bandwidth"] . 
       "<br><input type=text name=DC::bandwidth:$vlan:$vnode size=6/>" .
       "</td>\n";
   echo "  <td> " . $field["lossrate"] . 
       "<br><input type=text name=DC::plr:$vlan:$vnode size=6/>" .
       "</td>\n";
   echo "</tr>\n";

   #
   # If duplex, its a lan node. Print reverse params.
   # Note, we do not allow them to change lan nodes asymmetrically.
   #
   if (0 && !strcmp($field{"type"}, "duplex")) {
       echo "<tr>\n";
       echo "  <td>&nbsp</td>\n";
       echo "  <td>&nbsp</td>\n";
       echo "  <td> " . $field["rdelay"] . "</td>\n";
       echo "  <td> " . $field["rbandwidth"] . "</td>\n";
       echo "  <td> " . $field["rlossrate"] . "</td>\n";
       echo "</tr>\n";
   }
}

print "</table>\n" .
      "<input type=submit value='Change' />\n" .
      "</form>\n";

PAGEFOOTER();
?>

