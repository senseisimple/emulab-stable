<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2010 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("experiment", PAGEARG_EXPERIMENT);
$optargs = OptionalPageArguments("submit",     PAGEARG_STRING,
				 "formfields", PAGEARG_ARRAY);

# Need these below.
$pid	  = $experiment->pid();
$eid	  = $experiment->eid();
$gid	  = $experiment->gid();
$state	  = $experiment->state();
$unix_gid = $experiment->UnixGID();

#
# Standard Testbed Header
#
PAGEHEADER("Delay Control");

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
if ($state != $TB_EXPTSTATE_ACTIVE) {
    USERERROR("Experiment $eid must be active to change its traffic ".
	      "shaping configuration!", 1);
}

#
# Verify permission.
#
if (! $experiment->AccessCheck($this_user, $TB_EXPT_MODIFY)) {
    USERERROR("You do not have permission to modify experiment $eid!", 1);
}

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

#
# Spit the form out using the array of data.
#
function SPITFORM($formfields, $errors)
{
    global $experiment, $pid, $eid, $result_delays, $result_linkdelays;
    global $EXPOSELINKTEST;

    if ($errors) {
	echo "<table class=nogrid
                     align=center border=0 cellpadding=6 cellspacing=0>
              <tr>
                 <th align=center colspan=2>
                   <font size=+1 color=red>
                      &nbsp;Oops, please fix the following errors!&nbsp;
                   </font>
                 </td>
              </tr>\n";

	while (list ($name, $message) = each ($errors)) {
	    echo "<tr>
                     <td align=right>
                       <font color=red>$name:&nbsp;</font></td>
                     <td align=left>
                       <font color=red>$message</font></td>
                  </tr>\n";
	}
	echo "</table><br>\n";
    }

    echo $experiment->PageHeader();
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

    $url = CreateURL("delaycontrol", $experiment);

    print "<form action='$url' method=post>\n" .
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

    # Get current state.
    $result_delays =
	DBQueryFatal("select * from delays ".
		     "where eid='$eid' and pid='$pid' and noshaping=0 " .
		     "order by vname,vnode0,vnode1");
    $result_linkdelays =
	DBQueryFatal("select * from linkdelays ".
		     "where eid='$eid' and pid='$pid' " .
		     "order by vlan,vnode");

    $num  = mysql_num_rows( $result_delays );
    $last = "";
    if ($num)
	mysql_data_seek($result_delays, 0);
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
		 "<input type=text
			 name=\"formfields[DC::delay::$vlan::allnodes]\"
			 size=6/>" .
		 "</td>\n";
	    echo "  <td> " .
		 "<input type=text
			 name=\"formfields[DC::bandwidth::$vlan::allnodes]\"
			 size=6/>" .
		 "</td>\n";
	    echo "  <td> " .
		 "<input type=text
			 name=\"formfields[DC::plr::$vlan::allnodes]\"
			 size=6/>" .
		 "</td>\n";
	    echo "  <td> " . 
		 "<input type=text
			 name=\"formfields[DC::limit::$vlan::allnodes]\"
			 size=6/>" .
		 "</td>\n";

	    if ($row["q0_red"]) {
		echo "  <td> " .
		     "<input type=text
			      name=\"formfields[DC::q_weight::$vlan::allnodes]\"
			      size=6/>".
		     "</td>\n";
		echo "  <td> " .
		     "<input type=text
			     name=\"formfields[DC::minthresh::$vlan::allnodes]\"
			     size=6/>".
		      "</td>\n";
		echo "  <td> " .
		     "<input type=text
			     name=\"formfields[DC::thresh::$vlan::allnodes]\"
			     size=6/>" .
		     "</td>\n";
		echo "  <td> " .
		     "<input type=text
			     name=\"formfields[DC::linterm::$vlan::allnodes]\"
			     size=6/>" .
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
	     "<br><input type=text
			 name=\"formfields[DC::delay::$vlan::$vnode0]\"
			 size=6/>" .
	     "</td>\n";
	echo "  <td> " . $row["bandwidth0"] . 
	     "<br><input type=text
			 name=\"formfields[DC::bandwidth::$vlan::$vnode0]\"
			 size=6/>" .
	     "</td>\n";
	echo "  <td> " . $row["lossrate0"] . 
	     "<br><input type=text
			 name=\"formfields[DC::plr::$vlan::$vnode0]\"
			 size=6/>" .
	     "</td>\n";
	echo "  <td> " . $row["q0_limit"] .
	     ($row["q0_qinbytes"] ? "b" : "s") . 
	     "<br><input type=text
			 name=\"formfields[DC::limit::$vlan::$vnode0]\"
			 size=6/>" .
	     "</td>\n";
	if ($row["q0_red"]) {
	    echo "  <td> " . $row["q0_weight"] . 
		 "<br><input type=text
			 name=\"formfields[DC::q_weight::$vlan::$vnode0]\"
			 size=6/>".
		 "</td>\n";
	    echo "  <td> " . $row["q0_minthresh"] . 
		"<br><input type=text
			    name=\"formfields[DC::minthresh::$vlan::$vnode0]\"
			    size=6/>".
		"</td>\n";
	    echo "  <td> " . $row["q0_maxthresh"] . 
		 "<br><input type=text
			     name=\"formfields[DC::thresh::$vlan::$vnode0]\"
			     size=6/>" .
		 "</td>\n";
	    echo "  <td> " . $row["q0_linterm"] . 
		 "<br><input type=text
			     name=\"formfields[DC::linterm::$vlan::$vnode0]\"
			     size=6/>" .
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
		 "<br><input type=text
			     name=\"formfields[DC::delay::$vlan::$vnode1]\"
			     size=6/>" .
		 "</td>\n";
	    echo "  <td> " . $row["bandwidth1"] . 
		 "<br><input type=text
			     name=\"formfields[DC::bandwidth::$vlan::$vnode1]\"
			     size=6/>".
		 "</td>\n";
	    echo "  <td> " . $row["lossrate1"] . 
		 "<br><input type=text
			     name=\"formfields[DC::plr::$vlan::$vnode1]\"
			     size=6/>" .
		 "</td>\n";
	    echo "  <td> " . $row["q1_limit"] .
		($row["q1_qinbytes"] ? "b" : "s") . 
		 "<br><input type=text
			     name=\"formfields[DC::limit::$vlan::$vnode1]\"
			     size=6/>" .
		 "</td>\n";
	    if ($row["q0_red"]) {
		echo "  <td> " . $row["q1_weight"] . 
		     "<br><input type=text
				 name=\"formfields[DC::q_weight::$vlan::$vnode1]\"
				 size=6/>" .
		     "</td>\n";
		echo "  <td> " . $row["q1_minthresh"] . 
		     "<br><input type=text
				 name=\"formfields[DC::minthresh::$vlan::$vnode1]\"
				 size=6/>".
		     "</td>\n";
		echo "  <td> " . $row["q1_maxthresh"] . 
		     "<br><input type=text
				 name=\"formfields[DC::thresh::$vlan::$vnode1]\"
				 size=6/>" .
		     "</td>\n";
		echo "  <td> " . $row["q1_linterm"] . 
		     "<br><input type=text
				 name=\"formfields[DC::linterm::$vlan::$vnode1]\"
				 size=6/>" .
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
    if ($num)
	mysql_data_seek($result_linkdelays, 0);
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
		 "<input type=text
			 name=\"formfields[DC::delay::$vlan::allnodes]\"
			 size=6/>" .
		 "</td>\n";
	    echo "  <td> " .
		 "<input type=text
			 name=\"formfields[DC::bandwidth::$vlan::allnodes]\"
			 size=6/>" .
		 "</td>\n";
	    echo "  <td> " .
		 "<input type=text
			 name=\"formfields[DC::plr::$vlan::allnodes]\"
			 size=6/>" .
		 "</td>\n";
	    echo "  <td> " . 
		 "<input type=text
			 name=\"formfields[DC::limit::$vlan::allnodes]\"
			 size=6/>" .
		 "</td>\n";
	    if ($row["q_red"]) {
		echo "  <td> " .
		     "<input type=text
			     name=\"formfields[DC::q_weight::$vlan::allnodes]\"
			     size=6/>".
		     "</td>\n";
		echo "  <td> " .
		     "<input type=text
			     name=\"formfields[DC::minthresh::$vlan::allnodes]\"
			     size=6/>".
		     "</td>\n";
		echo "  <td> " .
		     "<input type=text
			     name=\"formfields[DC::thresh::$vlan::allnodes]\"
			     size=6/>" .
		     "</td>\n";
		echo "  <td> " .
		     "<input type=text
			     name=\"formfields[DC::linterm::$vlan::allnodes]\"
			     size=6/>" .
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
	     "<br><input type=text
			 name=\"formfields[DC::delay::$vlan::$vnode]\"
			 size=6/>" .
	     "</td>\n";
	echo "  <td> " . $row["bandwidth"] . 
	     "<br><input type=text
			 name=\"formfields[DC::bandwidth::$vlan::$vnode]\"
			 size=6/>" .
	     "</td>\n";
	echo "  <td> " . $row["lossrate"] . 
	     "<br><input type=text
			 name=\"formfields[DC::plr::$vlan::$vnode]\"
			 size=6/>" .
	     "</td>\n";
	echo "  <td> " . $row["q_limit"] .
	    ($row["q_qinbytes"] ? "b" : "s") . 
	     "<br><input type=text
			 name=\"formfields[DC::limit::$vlan::$vnode]\"
			 size=6/>" .
	     "</td>\n";
	if ($row["q_red"]) {
	    echo "  <td> " . $row["q_weight"] . 
		 "<br><input type=text
			     name=\"formfields[DC::q_weight::$vlan::$vnode]\"
			     size=6/>" .
		 "</td>\n";
	    echo "  <td> " . $row["q_minthresh"] . 
		 "<br><input type=text
			     name=\"formfields[DC::minthresh::$vlan::$vnode]\"
			     size=6/>" .
		 "</td>\n";
	    echo "  <td> " . $row["q_maxthresh"] . 
		 "<br><input type=text
			     name=\"formfields[DC::thresh::$vlan::$vnode]\"
			     size=6/>" .
		 "</td>\n";
	    echo "  <td> " . $row["q_linterm"] . 
		 "<br><input type=text
			     name=\"formfields[DC::linterm::$vlan::$vnode]\"
			     size=6/>" .
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

    echo "</table>
	   <input type=checkbox
	          name=\"formfields[modbase]\"
		  value=Yep";
    if (isset($formfields["modbase"]) &&
	strcmp($formfields["modbase"], "Yep") == 0)
	echo "    checked";
    echo " >\n";
    echo " &nbsp; <b>Save?</b> (Check this box if you want these settings to
			       be used next time the experiment is swapped in)
	  <br><br>
	  <input type=submit name=submit value=Execute>
	  </form>\n";

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
}

#
# On first load, display initial values.
#
if (!isset($submit)) {
    $defaults = array();
    $defaults["modbase"] = 0;

    SPITFORM($defaults, 0);
    PAGEFOOTER();
    return;
}

#
# Otherwise, must validate and redisplay if errors
#
$errors = array();

#
# Array of changes, indexed by [lan:node]
# 
$changes = array();
while (list ($header, $value) = each ($formfields)) {
    $changestring = strstr($header, "DC::");
    if (! $changestring) {
	continue;
    }
    if (!isset($value) || !strcmp($value, "") ||
	!preg_match('/^[0-9\.bs]*$/', "$value")) {
	continue;
    }

    # Too bad sscanf is broken ...
    $temp      = substr($header, 4);
    $param     = substr($temp, 0, strpos($temp, "::"));
    $temp      = substr($temp, strpos($temp, "::") + 2);
    $lan       = substr($temp, 0, strpos($temp, "::"));
    $vnode     = substr($temp, strpos($temp, "::") + 2);

    ##echo "$param $lan $vnode $value<br>\n";

    #
    # Must taint check! This stuff is going to a shell program. 
    # 
    if (!preg_match('/^[-\w]+$/', $param)) {
	$errors["Param $param"] = "Invalid characters";
	continue;
    }
    if (!preg_match('/^[-\w]+$/', $lan)) {
	$errors["Lan $param"] = "Invalid characters";
	continue;
    }
    if (!preg_match('/^[-\w]+$/', $vnode)) {
	$errors["Vnode $param"] = "Invalid characters";
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
		$qlimitarg = "queue-in-bytes=0";
	    }
	    elseif ($lastchr == "b") {
		$qlimitarg = "queue-in-bytes=1";
	    }
	    $value = substr($value, 0, strlen($value) - 1);
	}
    }

    if (! array_key_exists($lan, $changes)) {
	$changes["$lan"] = array();
    }

    if (!isset($vnode) || $vnode == "") {
	if (! array_key_exists("allnodes", $changes["$lan"])) {
	    $changes["$lan"]["allnodes"] = "";
	}
	$changes["$lan"]["allnodes"] .= "${param}=${value} $qlimitarg ";
    }
    else {
	if (! array_key_exists("$vnode", $changes["$lan"])) {
	    $changes["$lan"]["$vnode"] = "";
	}
	$changes["$lan"]["$vnode"] .= "${param}=${value} $qlimitarg ";
    }
}

#
# If any errors, respit the form with the current values and the
# error messages displayed. Iterate until happy.
# 
if (count($errors)) {
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    return;
}

# Call delay_config to make changes on each lan:node pair.
foreach ($changes as $lan => $foo) {
    foreach ($foo as $vnode => $string) {

	#
	# Build up argument array to pass along.
	#
	$args = array();

	# Required.
	$args["pid"] = $pid;
	$args["eid"] = $eid;
	$args["link"] = $lan;

	# Optional.
	if (isset($formfields["modbase"])) {
	    $args["modify"] = strcmp($formfields["modbase"], "Yep") ? 0 : 1;
	}
	# This could be "allnodes" or a source vnode name.  Skip if "allnodes".
	if (strcmp($vnode, "allnodes")) {
	    $args["vnode"] = $vnode;
	}

	# Separate the params out of the string gathered for this lan:node pair.
	foreach (explode(' ', trim($string)) as $param_value) {
	    if ($param_value != "") {
		list($param, $value) = explode('=', $param_value);
		$args[$param] = $value;
	    }
	}

	if (! ($result = ChangeDelayConfig($uid, $pid, $unix_gid,
					   $args, $errors))) {
	    # Always respit the form so that the form fields are not lost.
	    # I just hate it when that happens so lets not be guilty of it ourselves.
	    SPITFORM($formfields, $errors);
	    PAGEFOOTER();
	    return;
	}
    }
}

# Always respit the form so that the form fields are not lost.
# I just hate it when that happens so lets not be guilty of it ourselves.
SPITFORM($formfields, $errors);
PAGEFOOTER();
return;

#
# When there's a DelayConfig class, this will be a Class function to change them...
#
function ChangeDelayConfig($uid, $pid, $unix_gid, $args, &$errors) {
    global $suexec_output, $suexec_output_array, $TBADMINGROUP;

    #
    # Generate a temporary file and write in the XML goo.
    #
    $xmlname = tempnam("/tmp", "delay_config");
    if (! $xmlname) {
	TBERROR("Could not create temporary filename", 0);
	$errors[] = "Transient error(1); please try again later.";
	return null;
    }
    if (! ($fp = fopen($xmlname, "w"))) {
	TBERROR("Could not open temp file $xmlname", 0);
	$errors[] = "Transient error(2); please try again later.";
	return null;
    }

    fwrite($fp, "<PubKey>\n");
    foreach ($args as $name => $value) {
	fwrite($fp, "<attribute name=\"$name\">");
	fwrite($fp, "  <value>" . htmlspecialchars($value) . "</value>");
	fwrite($fp, "</attribute>\n");
    }
    fwrite($fp, "</PubKey>\n");
    fclose($fp);
    chmod($xmlname, 0666);

    #
    # Need proper auditing or logging. For now, run as normal and
    # if something breaks we get the mail from the web interface.
    # This might change depending on how often we get email!
    #
    $retval = SUEXEC($uid, "$pid,$unix_gid", "webdelay_config -X $xmlname",
		     SUEXEC_ACTION_IGNORE);
    if ($retval) {
	# Ug, I know this hardwired return value is bad! 
	if ($retval == 2) {
	    # A usage error, reported to user. At some point the
	    # approach is to do all the argument checking first.
	    if (count($suexec_output_array)) {
		for ($i = 0; $i < count($suexec_output_array); $i++) {
		    $line = $suexec_output_array[$i];
		    if (preg_match("/^([-\w]+):\s*(.*)$/",
				   $line, $matches)) {
			$errors[$matches[1]] = $matches[2];
		    }
		    else
			$errors[] = $line;
		}
	    }
	    else
		$errors[] = "Transient error(3, $retval); please try again later.";
	}
	else {
	    # All other errors reported to tbops since they are bad!
	    $errors[] = "Transient error(4, $retval); please try again later.";
	    SUEXECERROR(SUEXEC_ACTION_CONTINUE);
	}
	return null;
    }

    # There are no return value(s) to parse at the end of the output.

    # Unlink this here, so that the file is left behind in case of error.
    # We can then edit the pubkeys by hand from the xmlfile, if desired.
    unlink($xmlname);

    return true; 
}

?>

