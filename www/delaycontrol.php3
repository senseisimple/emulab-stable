<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
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
    DBQueryFatal("SELECT gid,state FROM experiments WHERE ".
		 "eid='$eid' and pid='$pid'");
$row = mysql_fetch_array($query_result);
$gid         = $row[gid];
$state       = $row[state];

#
# Look for transition and exit with error.
#
if ($state != $TB_EXPTSTATE_ACTIVE &&
    $state != $TB_EXPTSTATE_SWAPPED) {
    USERERROR("Experiment $eid is not ACTIVE or SWAPPED.<br>".
	      "You must wait until the experiment is no longer in transition.".
	      "<br>", 1);
}

#
# Must be active. The backend can deal with changing the base experiment
# when the experiment is swapped out, but we need to generate a form based
# on virt_lans instead of delays/linkdelays. Thats harder to do. 
#
if (strcmp($state, $TB_EXPTSTATE_ACTIVE)) {
    USERERROR("Experiment $eid must be active to change its traffic ".
	      "shaping configuration!", 1);
}

#
# Verify permission.
#
if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_MODIFY)) {
    USERERROR("You do not have permission to modify experiment $eid!", 1);
}

if ($dochange == "1") {
    #
    # Array of changes, indexed by [lan:node]
    # 
    $changes = array();

    #
    # modbase 
    #
    $modbasearg = "";
    if (isset($modbase) && $modbase) {
	$modbasearg = "-m";
    }
    
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

	#echo "$param $lan $vnode $value<br>\n";

	#
	# Must taint check! This stuff is going to a shell program. 
	# 
	if (!preg_match("/^[-\w]*$/", $param) ||
	    !preg_match("/^[-\w]*$/", $lan) ||
	    !preg_match("/^[-\w]*$/", $vnode)) {
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
	    	
	    $cmd = "webdelay_config $modbasearg $vnode $pid $eid $lan $string";
	    #echo "$cmd<br>\n";

	    #
	    # Need proper auditing or logging. For now, run as normal and
	    # if something breaks we get the mail from the web interface.
	    # This might change depending on how often we get email!
	    #
	    $retval = SUEXEC($uid, "$pid,$unix_gid", $cmd,
			     SUEXEC_ACTION_IGNORE);
	    if ($retval) {
		# Ug, I know this hardwired return value is bad! 
		if ($retval == 2) {
		    # A usage error, reported to user. At some point the
		    # approach is to do all the argument checking first.
		    SUEXECERROR(SUEXEC_ACTION_USERERROR);
		}
		else {
		    # All other errors reported to tbops since they are bad!
		    SUEXECERROR(SUEXEC_ACTION_DIE);
		}
	    }
	}
    }
}

#
# Okay, even if changes were made, keep going and report again.
# 
$result_delays =
    DBQueryFatal("select * from delays ".
		 "where eid='$eid' and pid='$pid' and noshaping=0 " .
		 "order by vname,vnode0,vnode1");
$result_linkdelays =
    DBQueryFatal("select * from linkdelays ".
		 "where eid='$eid' and pid='$pid' " .
		 "order by vlan,vnode");

if (mysql_num_rows($result_delays) == 0 &&
    mysql_num_rows($result_linkdelays) == 0) {
    USERERROR("No running delay nodes with eid='$eid' and pid='$pid'!", 1);
}

echo "<font size=+1>Experiment <b>".
     "<a href='showproject.php3?pid=$pid'>$pid</a>/".
     "<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a></b></font>\n";
echo "<br><br>\n";

echo "Use this page to alter the traffic shaping parameters of your
      <em>swapped in</em> experiment. You can change as many values as you
      like at a time. The first line in each link or lan
      (labeled <font color=red>All Nodes</font>) allows you to set
      the parameters for the <em>entire</em> link or lan. If you want to
      change the values for indvidual nodes, then enter new values on the
      proper line instead. Anything you leave blank will be unaffected.<br> 
      When you are ready, click on the Execute button at the bottom of the
      form. If you want these changes to be saved across swapout, then
      check the Save box.<br><br>\n";

print "<form action='delaycontrol.php3' method=post>\n" .
      "<input type=hidden name=dochange value=1 />\n" .
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
      " <td align=center colspan=4>
            RED/GRED<br>(only if link specified as RED)</td>".
      "</tr>".
      "<tr>".
      " <th>q_weight</th>".
      " <th>minthresh</th>".
      " <th>maxthresh</th>".
      " <th>linterm</th>".
      "</tr>";
           
$num  = mysql_num_rows( $result_delays );
$last = "";
for ($i = 0; $i < $num; $i++) {
    $row = mysql_fetch_array($result_delays);

    $vlan   = $row["vname"];
    $vnode0 = $row["vnode0"];
    $vnode1 = $row["vnode1"];

    if (strcmp($last, $vlan)) {
	$last = $vlan;
	echo "<tr>\n";
	echo "  <td><font color=blue>$vlan</font></td>\n";
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

	if ($row["q0_red"]) {
	    echo "  <td> " .
		"<input type=text name=DC::q_weight::$vlan::allnodes size=6/>".
		"</td>\n";
	    echo "  <td> " .
	       "<input type=text name=DC::minthresh::$vlan::allnodes size=6/>".
		"</td>\n";
	    echo "  <td> " .
		"<input type=text name=DC::thresh::$vlan::allnodes size=6/>" .
		"</td>\n";
	    echo "  <td> " .
		"<input type=text name=DC::linterm::$vlan::allnodes size=6/>" .
		"</td>\n";
	}
	else {
	    echo "<td>n/a</td>\n";
	    echo "<td>n/a</td>\n";
	    echo "<td>n/a</td>\n";
	    echo "<td>n/a</td>\n";
	}
	echo "</tr>\n";
    }

    echo "<tr>\n";
    echo "  <td>&nbsp</td>\n";
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
    if ($row["q0_red"]) {
	echo "  <td> " . $row["q0_weight"] . 
	    "<br><input type=text name=DC::q_weight::$vlan::$vnode0 size=6/>".
	    "</td>\n";
	echo "  <td> " . $row["q0_minthresh"] . 
            "<br><input type=text name=DC::minthresh::$vlan::$vnode0 size=6/>".
	    "</td>\n";
	echo "  <td> " . $row["q0_maxthresh"] . 
	    "<br><input type=text name=DC::thresh::$vlan::$vnode0 size=6/>" .
	    "</td>\n";
	echo "  <td> " . $row["q0_linterm"] . 
	    "<br><input type=text name=DC::linterm::$vlan::$vnode0 size=6/>" .
	    "</td>\n";
    }
    else {
	echo "<td>n/a</td>\n";
	echo "<td>n/a</td>\n";
	echo "<td>n/a</td>\n";
	echo "<td>n/a</td>\n";
    }
    echo "</tr>\n";

    #
    # If vnode0 and vnode1 are different, its a plain duplex link.
    # Otherwise its a lan node. At some point we will allow asymmetric
    # changes to lan nodes, but the backend script does not support that
    # so do not give them the option.
    #
    echo "<tr>\n";
    if (strcmp($row["vnode0"], $row["vnode1"])) {
	echo "  <td>&nbsp</td>\n";
	echo "  <td>$vnode1</td>\n";
	echo "  <td> " . $row["delay1"] . 
	    "<br><input type=text name=DC::delay::$vlan::$vnode1 size=6/>" .
	    "</td>\n";
	echo "  <td> " . $row["bandwidth1"] . 
	    "<br><input type=text name=DC::bandwidth::$vlan::$vnode1 size=6/>".
	    "</td>\n";
	echo "  <td> " . $row["lossrate1"] . 
	    "<br><input type=text name=DC::plr::$vlan::$vnode1 size=6/>" .
	    "</td>\n";
	echo "  <td> " . $row["q1_limit"] .
	    ($row["q1_qinbytes"] ? "b" : "s") . 
	    "<br><input type=text name=DC::limit::$vlan::$vnode1 size=6/>" .
	    "</td>\n";
	if ($row["q0_red"]) {
	    echo "  <td> " . $row["q1_weight"] . 
	    "<br><input type=text name=DC::q_weight::$vlan::$vnode1 size=6/>" .
		"</td>\n";
	    echo "  <td> " . $row["q1_minthresh"] . 
	    "<br><input type=text name=DC::minthresh::$vlan::$vnode1 size=6/>".
		"</td>\n";
	    echo "  <td> " . $row["q1_maxthresh"] . 
	    "<br><input type=text name=DC::thresh::$vlan::$vnode1 size=6/>" .
		"</td>\n";
	    echo "  <td> " . $row["q1_linterm"] . 
	    "<br><input type=text name=DC::linterm::$vlan::$vnode1 size=6/>" .
		"</td>\n";
	}
	else {
	    echo "<td>n/a</td>\n";
	    echo "<td>n/a</td>\n";
	    echo "<td>n/a</td>\n";
	    echo "<td>n/a</td>\n";
	}
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
for ($i = 0; $i < $num; $i++) {
    $row = mysql_fetch_array( $result_linkdelays );

    $vlan  = $row["vlan"];
    $vnode = $row["vnode"];

    if (strcmp($last, $vlan)) {
	$last = $vlan;
	echo "<tr>\n";
	echo "  <td><font color=blue>$vlan</font></td>\n";
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
	if ($row["q_red"]) {
	    echo "  <td> " .
		"<input type=text name=DC::q_weight::$vlan::allnodes size=6/>".
		"</td>\n";
	    echo "  <td> " .
	       "<input type=text name=DC::minthresh::$vlan::allnodes size=6/>".
		"</td>\n";
	    echo "  <td> " .
		"<input type=text name=DC::thresh::$vlan::allnodes size=6/>" .
		"</td>\n";
	    echo "  <td> " .
		"<input type=text name=DC::linterm::$vlan::allnodes size=6/>" .
		"</td>\n";
	}
	else {
	    echo "<td>n/a</td>\n";
	    echo "<td>n/a</td>\n";
	    echo "<td>n/a</td>\n";
	    echo "<td>n/a</td>\n";
	}
	echo "</tr>\n";
    }
    echo "<tr>\n";
    echo "  <td>&nbsp</td>\n";
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
    if ($row["q_red"]) {
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
    }
    else {
	    echo "<td>n/a</td>\n";
	    echo "<td>n/a</td>\n";
	    echo "<td>n/a</td>\n";
	    echo "<td>n/a</td>\n";
    }
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
      "<input type=checkbox value=\"1\" name=modbase>
              &nbsp; <b>Save?</b> (Check this box if you want these settings to
                           be used next time the experiment is swapped in)".
      "<br><br>\n".
      "<input type=submit value='Execute'>\n" .
      "</form>\n";

if (STUDLY() || $EXPOSELINKTEST) {
echo "<br>
      After you change the settings, you can run
      <a href=linktest.php3?pid=$pid&eid=$eid>Linktest</a> to make sure the
      links are configured properly,<br>
      but <b><em>only</em></b> if you clicked the 'Save' box above!\n";

echo "<br><br> 
      <b>We strongly recommend that you always use
      <a href=linktest.php3?pid=$pid&eid=$eid>Linktest</a> or some
      other testing mechanism to ensure that your links and lans are
      behaving as you expect them to.</b>\n";
} 

PAGEFOOTER();
?>

