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
    # Array of changes, indexed by [lan:node]
    # 
    $changes = array();
    
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
    $query_result =
	DBQueryFatal("SELECT gid FROM experiments WHERE ".
		     "eid='$eid' and pid='$pid'");
    $row = mysql_fetch_array($query_result);
    $gid = $row[gid];

    #
    # Grab the unix GID for running scripts.
    #
    TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);
    
    while (list ($header, $value) = each ($HTTP_POST_VARS)) {
	$changestring = strstr($header, "DC::");
	if (! $changestring) {
	    continue;
	}
	if (!isset($value) || !strcmp($value, "") ||
	    !ereg("^[0-9\.bs]*$", "$value")) {
	    continue;
	}

	# Too bad sscanf is broken ...
	$temp      = substr($header, 4);
	$param     = substr($temp, 0, strpos($temp, "::"));
	$temp      = substr($temp, strpos($temp, "::") + 2);
	$lan       = substr($temp, 0, strpos($temp, "::"));
	$vnode     = substr($temp, strpos($temp, "::") + 2);

	echo "$param $lan $vnode $value<br>\n";

	#
	# Must taint check! This stuff is going to a shell program. 
	# 
	if (!ereg("^[0-9a-zA-Z\-\_]*$", $param) ||
	    !ereg("^[0-9a-zA-Z\-\_]*$", $lan) ||
	    !ereg("^[0-9a-zA-Z\-\_]*$", $vnode)) {
	    continue;
	}

	#
	# Queue limit is special. Need to parse for "b" or "s" extension.
	#
	$qlimitarg = "";
	if (! strcmp($param, "limit")) {
	    $lastchr = $value{strlen($value)-1};

	    if (ctype_alpha($lastchr)) {
		if ($lastchr == "s") {
		    $qlimitarg = "QUEUE-IN-BYTES=0";
		}
		elseif ($lastchr == "b") {
		    $qlimitarg = "QUEUE-IN-BYTES=1";
		}
		$value = substr($value, 0, strlen($value) - 1);
	    }
	}

	if (!isset($vnode) || !strcmp($vnode, "")) {
	    $changes["$lan"]["allnodes"] .= "${param}=${value} $qlimitarg ";
	}
	else {
	    $changes["$lan"]["$vnode"] .= "${param}=${value} $qlimitarg ";
	}
    }
    foreach ($changes as $lan => $foo) {
	foreach ($foo as $vnode => $string) {
	    if (strcmp($vnode, "allnodes")) {
		$vnode = "-s $vnode";
	    }
	    else {
		$vnode = "";
	    }
	
	    $cmd = "webdelay_config $vnode $pid $eid $lan $string ";

	    echo "<pre>\n";
	    echo "Running '$cmd':<br>\n";

	    if ($fp = popen("$TBSUEXEC_PATH $uid $unix_gid $cmd", "r")) {
		fpassthru($fp);
	    }
	    echo "</pre>\n";
	}
    }
}

$result_delays =
    DBQueryFatal("SELECT * FROM delays WHERE eid='$eid' AND pid='$pid' " .
		 "order by vname,vnode0,vnode1");
$result_linkdelays =
    DBQueryFatal("SELECT * FROM linkdelays WHERE eid='$eid' AND pid='$pid' ".
		 "order by vlan,vnode");

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
      " <th rowspan=2>Link Name</th>".
      " <th rowspan=2>Node</th>".
      " <th rowspan=2>Delay (msec)</th>".
      " <th rowspan=2>Bandwidth<br>(kb/s)</th>".
      " <th rowspan=2>Loss (ratio)</th>".
      " <th rowspan=2>Queue Size</th>".
      " <th align=center colspan=4>
            RED/GRED<br>(only if link specified as RED)</th>".
      "</tr>".
      "<tr>".
      " <th>q_weight</th>".
      " <th>minthresh</th>".
      " <th>maxthresh</th>".
      " <th>linterm</th>".
      "</tr>";
           
$num  = mysql_num_rows( $result_delays );
$last = "";
for( $i = 0; $i < $num; $i++ ) {
   $row = mysql_fetch_array($result_delays);

   $vlan   = $row["vname"];
   $vnode0 = $row["vnode0"];
   $vnode1 = $row["vnode1"];

   if (! $row["q0_red"]) {
       $row["q0_weight"] = 0;
       $row["q0_minthresh"] = 0;
       $row["q0_maxthresh"] = 0;
       $row["q0_linterm"] = 0;
   }
   if (! $row["q1_red"]) {
       $row["q1_weight"] = 0;
       $row["q1_minthresh"] = 0;
       $row["q1_maxthresh"] = 0;
       $row["q1_linterm"] = 0;
   }

   if (strcmp($last, $vlan)) {
     $last = $vlan;
     echo "<tr>\n";
     echo "  <td>$vlan</td>\n";
     echo "  <td><font color=red>All Nodes</font></td>\n";
     echo "  <td> " .
          "<input type=text name=\"DC::delay::$vlan::allnodes\" size=6/>" .
          "</td>\n";
     echo "  <td> " .
          "<input type=text name=DC::bandwidth::$vlan::allnodes size=6/>" .
          "</td>\n";
     echo "  <td> " .
          "<input type=text name=DC::plr::$vlan::allnodes size=6/>" .
          "</td>\n";
     echo "  <td> " . 
          "<input type=text name=DC::limit::$vlan::allnodes size=6/>" .
          "</td>\n";
     echo "  <td> " .
          "<input type=text name=DC::q_weight::$vlan::allnodes size=6/>" .
          "</td>\n";
     echo "  <td> " .
          "<input type=text name=DC::minthresh::$vlan::allnodes size=6/>" .
          "</td>\n";
     echo "  <td> " .
          "<input type=text name=DC::thresh::$vlan::allnodes size=6/>" .
          "</td>\n";
     echo "  <td> " .
          "<input type=text name=DC::linterm::$vlan::allnodes size=6/>" .
          "</td>\n";
     echo "</tr>\n";
   }

   echo "<tr>\n";
   echo "  <td>$vlan</td>\n";
   echo "  <td>$vnode0</td>\n";
   echo "  <td> " . $row["delay0"] . 
       "<br><input type=text name=\"DC::delay::$vlan::$vnode0\" size=6/>" .
       "</td>\n";
   echo "  <td> " . $row["bandwidth0"] . 
       "<br><input type=text name=DC::bandwidth::$vlan::$vnode0 size=6/>" .
       "</td>\n";
   echo "  <td> " . $row["lossrate0"] . 
       "<br><input type=text name=DC::plr::$vlan::$vnode0 size=6/>" .
       "</td>\n";
   echo "  <td> " . $row["q0_limit"] .
       ($row["q0_qinbytes"] ? "b" : "s") . 
       "<br><input type=text name=DC::limit::$vlan::$vnode0 size=6/>" .
       "</td>\n";
   echo "  <td> " . $row["q0_weight"] . 
       "<br><input type=text name=DC::q_weight::$vlan::$vnode0 size=6/>" .
       "</td>\n";
   echo "  <td> " . $row["q0_minthresh"] . 
       "<br><input type=text name=DC::minthresh::$vlan::$vnode0 size=6/>" .
       "</td>\n";
   echo "  <td> " . $row["q0_maxthresh"] . 
       "<br><input type=text name=DC::thresh::$vlan::$vnode0 size=6/>" .
       "</td>\n";
   echo "  <td> " . $row["q0_linterm"] . 
       "<br><input type=text name=DC::linterm::$vlan::$vnode0 size=6/>" .
       "</td>\n";
   echo "</tr>\n";

   #
   # If vnode0 and vnode1 are different, its a plain duplex link.
   # Otherwise its a lan node. At some point we will allow asymmetric
   # changes to lan nodes, but the backend script does not support that
   # so do not give them the option.
   #
   echo "<tr>\n";
   if (strcmp($row["vnode0"], $row["vnode1"])) {
       echo "  <td>$vlan</td>\n";
       echo "  <td>$vnode1</td>\n";
       echo "  <td> " . $row["delay1"] . 
	   "<br><input type=text name=DC::delay::$vlan::$vnode1 size=6/>" .
	   "</td>\n";
       echo "  <td> " . $row["bandwidth1"] . 
	   "<br><input type=text name=DC::bandwidth::$vlan::$vnode1 size=6/>" .
	   "</td>\n";
       echo "  <td> " . $row["lossrate1"] . 
	   "<br><input type=text name=DC::plr::$vlan::$vnode1 size=6/>" .
	   "</td>\n";
       echo "  <td> " . $row["q1_limit"] .
	   ($row["q1_qinbytes"] ? "b" : "s") . 
	   "<br><input type=text name=DC::limit::$vlan::$vnode1 size=6/>" .
	   "</td>\n";
       echo "  <td> " . $row["q1_weight"] . 
           "<br><input type=text name=DC::q_weight::$vlan::$vnode1 size=6/>" .
           "</td>\n";
       echo "  <td> " . $row["q1_minthresh"] . 
           "<br><input type=text name=DC::minthresh::$vlan::$vnode1 size=6/>" .
           "</td>\n";
       echo "  <td> " . $row["q1_maxthresh"] . 
           "<br><input type=text name=DC::thresh::$vlan::$vnode1 size=6/>" .
           "</td>\n";
       echo "  <td> " . $row["q1_linterm"] . 
           "<br><input type=text name=DC::linterm::$vlan::$vnode1 size=6/>" .
           "</td>\n";
   }
   else {
       if (0) {
	   echo "  <td>&nbsp</td>\n";
	   echo "  <td>&nbsp</td>\n";
	   echo "  <td> " . $row["delay1"] . "</td>\n";
	   echo "  <td> " . $row["bandwidth1"] . "</td>\n";
	   echo "  <td> " . $row["lossrate1"] . "</td>\n";
	   echo "  <td> " . $row["q1_limit"] . "</td>\n";
       }
   }
   echo "</tr>\n";
}

$num = mysql_num_rows( $result_linkdelays );
for( $i = 0; $i < $num; $i++ ) {
   $row = mysql_fetch_array( $result_linkdelays );

   $vlan  = $row["vlan"];
   $vnode = $row["vnode"];

   if (! $row["q_red"]) {
       $row["q_weight"] = 0;
       $row["q_minthresh"] = 0;
       $row["q_maxthresh"] = 0;
       $row["q_linterm"] = 0;
   }

   if (strcmp($last, $vlan)) {
     $last = $vlan;
     echo "<tr>\n";
     echo "  <td>$vlan</td>\n";
     echo "  <td><font color=red>All Nodes</font></td>\n";
     echo "  <td> " .
          "<input type=text name=\"DC::delay::$vlan::allnodes\" size=6/>" .
          "</td>\n";
     echo "  <td> " .
          "<input type=text name=DC::bandwidth::$vlan::allnodes size=6/>" .
          "</td>\n";
     echo "  <td> " .
          "<input type=text name=DC::plr::$vlan::allnodes size=6/>" .
          "</td>\n";
     echo "  <td> " . 
          "<input type=text name=DC::limit::$vlan::allnodes size=6/>" .
          "</td>\n";
     echo "  <td> " .
          "<input type=text name=DC::q_weight::$vlan::allnodes size=6/>" .
          "</td>\n";
     echo "  <td> " .
          "<input type=text name=DC::minthresh::$vlan::allnodes size=6/>" .
          "</td>\n";
     echo "  <td> " .
          "<input type=text name=DC::thresh::$vlan::allnodes size=6/>" .
          "</td>\n";
     echo "  <td> " .
          "<input type=text name=DC::linterm::$vlan::allnodes size=6/>" .
          "</td>\n";
     echo "</tr>\n";
   }

   echo "<tr>\n";
   echo "  <td>$vlan</td>\n";
   echo "  <td>$vnode</td>\n";
   echo "  <td> " . $row["delay"] . 
       "<br><input type=text name=DC::delay::$vlan::$vnode size=6/>" .
       "</td>\n";
   echo "  <td> " . $row["bandwidth"] . 
       "<br><input type=text name=DC::bandwidth::$vlan::$vnode size=6/>" .
       "</td>\n";
   echo "  <td> " . $row["lossrate"] . 
       "<br><input type=text name=DC::plr::$vlan::$vnode size=6/>" .
       "</td>\n";
   echo "  <td> " . $row["q_limit"] .
       ($row["q_qinbytes"] ? "b" : "s") . 
       "<br><input type=text name=DC::limit::$vlan::$vnode size=6/>" .
       "</td>\n";
   echo "  <td> " . $row["q_weight"] . 
       "<br><input type=text name=DC::q_weight::$vlan::$vnode size=6/>" .
       "</td>\n";
   echo "  <td> " . $row["q_minthresh"] . 
       "<br><input type=text name=DC::minthresh::$vlan::$vnode size=6/>" .
       "</td>\n";
   echo "  <td> " . $row["q_maxthresh"] . 
       "<br><input type=text name=DC::thresh::$vlan::$vnode size=6/>" .
       "</td>\n";
   echo "  <td> " . $row["q_linterm"] . 
       "<br><input type=text name=DC::linterm::$vlan::$vnode size=6/>" .
       "</td>\n";
   echo "</tr>\n";

   #
   # If duplex, its a lan node. Print reverse params.
   # Note, we do not allow them to change lan nodes asymmetrically yet
   # since the backend script cannot handle that.
   #
   if (0 && !strcmp($row{"type"}, "duplex")) {
       echo "<tr>\n";
       echo "  <td>&nbsp</td>\n";
       echo "  <td>&nbsp</td>\n";
       echo "  <td> " . $row["rdelay"] . "</td>\n";
       echo "  <td> " . $row["rbandwidth"] . "</td>\n";
       echo "  <td> " . $row["rlossrate"] . "</td>\n";
       echo "  <td> " . $row["q_limit"] . "</td>\n";
       echo "</tr>\n";
   }
   
}

print "</table>\n" .
      "<input type=submit value='Change' />\n" .
      "</form>\n";

PAGEFOOTER();
?>

