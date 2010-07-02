<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005, 2006, 2007 University of Utah and the Flux Group.
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
$reqargs = RequiredPageArguments("experiment", PAGEARG_EXPERIMENT);
$optargs = OptionalPageArguments("linklan",    PAGEARG_STRING,
				 "vnode",      PAGEARG_STRING,
				 "action",     PAGEARG_STRING);

# Need these below.
$pid = $experiment->pid();
$eid = $experiment->eid();

if (!$experiment->AccessCheck($this_user, $TB_EXPT_MODIFY)) {
    USERERROR("You do not have permission to run linktest on $pid/$eid!", 1);
}

#
# Must be active. The backend can deal with changing the base experiment
# when the experiment is swapped out, but we need to generate a form based
# on virt_lans instead of delays/linkdelays. Thats harder to do. 
#
if ($experiment->state() != $TB_EXPTSTATE_ACTIVE) {
    USERERROR("Experiment $eid must be active to monitor its links!", 1);
}

$query_result =
    DBQueryFatal("select * from traces ".
		 "where eid='$eid' AND pid='$pid' ".
		 "order by linkvname,vnode");

if (mysql_num_rows($query_result) == 0) {
    USERERROR("No links are being traced/monitored in $eid/$pid!", 1);
}

#
# Standard Testbed Header
#
PAGEHEADER("Link Monitoring");

echo $experiment->PageHeader();
echo "<br /><br />\n";

#
# Give a status message is redirected back here from linkmon_ctl.php3.
#
if (isset($linklan) && $linklan != "" &&
    isset($action) && $action != "") {
    echo "<center><font size=+1>";
    echo "Tracing/Monitoring on $linklan ".
	(isset($vnode) && $vnode != "" ? "($vnode) " : "") . "has been ";
    if ($action == "pause")
	echo "paused";
    elseif ($action == "restart")
	echo "restarted";
    elseif ($action == "snapshot")
	echo "snap shotted";
    elseif ($action == "kill")
	echo "killed";

    echo "</font></center><br>\n";
}

print "<table align=center>\n" .
      "<tr>" .
      " <th rowspan=2>Link Name</th>".
      " <th rowspan=2>Node</th>".
      " <th rowspan=2>Monitor</th>".
      " <td colspan=4 align=center><b>Trace/Monitor</b></th>".
      "</tr><tr>".
      " <th align=center>Pause</th>".
      " <th align=center>Restart</th>".
      " <th align=center>Snapshot</th>".
      " <th align=center>Kill</th>".
      "</tr>";

function SPITMONLINK($vlan, $vnode)
{
    global $experiment;

    $url  = CreateURL("linkmon_mon", $experiment, "linklan", $vlan);
    $link = "<a href='$url";
    if ($vnode)
	$link .= "&vnode=$vnode";
    $link .= "' target=_blank>";
    $link .= "<img border=0 src='/autostatus-icons/blueball.gif' ".
	          "alt='Monitor Link'>";
    $link .= "</a>\n";
    return $link;
}

function SPITTRACELINK($vlan, $vnode, $action, $color)
{
    global $experiment;

    $url  = CreateURL("linkmon_ctl", $experiment, "linklan", $vlan,
		      "action", $action, "vnode", $vnode);
    $link = "<a href='$url'>";
    $link .= "<img border=0 src='/autostatus-icons/${color}ball.gif' ".
	           "alt='$action'>";
    $link .= "</a>\n";
    return $link;
}

$num  = mysql_num_rows($query_result);
$last = "";
for ($i = 0; $i < $num; $i++) {
    $row = mysql_fetch_array($query_result);

    $vlan   = $row["linkvname"];
    $vnode  = $row["vnode"];

    if (strcmp($last, $vlan)) {
	$last = $vlan;
	echo "<tr>\n";
	echo "  <td><font color=blue>$vlan</font></td>\n";
	echo "  <td><font color=red>All Nodes</font></td>\n";
	echo "  <td align=center>&nbsp</td>\n";
	echo "  <td align=center> " .
	    SPITTRACELINK($vlan, null, "pause", "yellow") . "</td>\n";
	echo "  <td align=center> " .
	    SPITTRACELINK($vlan, null, "restart", "green") . "</td>\n";
	echo "  <td align=center> " .
	    SPITTRACELINK($vlan, null, "snapshot", "pink") . "</td>\n";
	echo "  <td align=center> " .
	    SPITTRACELINK($vlan, null, "kill", "red") . "</td>\n";
	echo "</tr>\n";
    }

    echo "<tr>\n";
    echo "  <td>&nbsp</td>\n";
    echo "  <td>$vnode</td>\n";
    echo "  <td align=center> " .
	SPITMONLINK($vlan, $vnode) . "</td>\n";
    echo "  <td align=center> " .
	SPITTRACELINK($vlan, $vnode, "pause", "yellow") . "</td>\n";
    echo "  <td align=center> " .
	SPITTRACELINK($vlan, $vnode, "restart", "green") . "</td>\n";
    echo "  <td align=center> " .
	SPITTRACELINK($vlan, $vnode, "snapshot", "pink") . "</td>\n";
    echo "  <td align=center> " .
	SPITTRACELINK($vlan, $vnode, "kill", "red") . "</td>\n";
    echo "</tr>\n";
}
echo "</table>\n";
echo "<center><font size=-1>".
     "The <a href='$WIKIDOCURL/AdvancedExample#Tracing'>".
     "Advanced Tutorial</a> has more info on link Tracing/Monitoring\n".
     "</font></center>\n";

echo "<br>
     <blockquote><blockquote>
     <table class=stealth>
     <tr>
       <td><b>Monitor:</b></td>
       <td>
           Bring up a new window, dispaying per-second traffic
                  statistics from the link
       </td>
      </tr>
      <tr>
       <td><b>Pause:</b></td>
       <td>
	   Pause packet capture (and monitoring)
       </td>
      </tr>
      <tr>
       <td><b>Restart:</b></td>
       <td>
	   Restart packet capture (and monitoring)
       </td>
      </tr>
      <tr>
       <td><b>Snapshot:</b></td>
       <td>
	    Close the packet capture files (syncing them to disk)
                and open a new set of files. Capturing continues.
       </td>
      </tr>
      <tr>
       <td><b>Kill:</b></td>
       <td>
	    Tell the packet capture processes to kill themselves. There
                is no way to restart them, except via reboot.
       </td>
     </table>
     <blockquote><blockquote>\n";

PAGEFOOTER();
?>

