<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# No PAGEHEADER since we spit out a Location header later. See below.
# 

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Check to make sure a valid experiment.
#
if (isset($pid) && strcmp($pid, "") &&
    isset($eid) && strcmp($eid, "")) {
    if (! TBvalid_eid($eid)) {
	PAGEARGERROR("$eid is contains invalid characters!");
    }
    if (! TBvalid_pid($pid)) {
	PAGEARGERROR("$pid is contains invalid characters!");
    }
    if (! TBValidExperiment($pid, $eid)) {
	USERERROR("$pid/$eid is not a valid experiment!", 1);
    }
    if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_MODIFY)) {
	USERERROR("You do not have permission to run linktest on $pid/$eid!",
		  1);
    }
}
else {
    PAGEARGERROR("Must specify pid and eid!");
}

$query_result = DBQueryFatal("select gid,linktest_level from experiments ".
				 "where pid='$pid' and eid='$eid'");
$row = mysql_fetch_array($query_result);
$gid = $row[0];

#
# See if a level came in. If not, then get the default from the DB.
#
if (!isset($level) || $level == "") {
    $level = $row[1];
}
elseif (! TBvalid_tinyint($level) ||
	$level < 0 || $level > TBDB_LINKTEST_MAX) {
    PAGEARGERROR("Linktest level must be an integer 0 <= level <= ".
		 TBDB_LINKTEST_MAX);
}

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we should
# probably redirect the browser back up a level.
#
if ($canceled) {
    PAGEHEADER("Run Linktest");
	
    echo "<center><h3><br>
          Operation canceled!
          </h3></center>\n";
    
    PAGEFOOTER();
    return;
}

if (!$confirmed) {
    PAGEHEADER("Run Linktest at level $level");

    echo "<font size=+2>Experiment <b>".
	"<a href='showproject.php3?pid=$pid'>$pid</a>/".
	"<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a></b></font>\n";

    echo "<center><font size=+2><br>
              Are you <b>REALLY</b>
                sure you want to run linktest at level $level?
              </font>\n";

    SHOWEXP($pid, $eid, 1);

    echo "<form action=linktest.php3 method=post>";
    echo "<input type=hidden name=pid value=$pid>\n";
    echo "<input type=hidden name=eid value=$eid>\n";

    echo "<table align=center border=1>\n";
    echo "<tr>
              <td><a href='$TBDOCBASE/doc/docwrapper.php3?".
	                  "docname=linktest.html'>Linktest</a> Option:</td>
              <td><select name=level>
                          <option value=0>Skip Linktest </option>\n";

    for ($i = 1; $i <= TBDB_LINKTEST_MAX; $i++) {
	$selected = "";

	if (strcmp("$level", "$i") == 0)
	    $selected = "selected";
	
	echo "        <option $selected value=$i>Level $i - " .
	    $linktest_levels[$i] . "</option>\n";
    }
    echo "       </select>";
    echo "    </td>
          </tr>
          </table><br>\n";

    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

#
# A cleanup function to keep the child from becoming a zombie, since
# the script is terminated, but the children are left to roam.
#
$fp = 0;

function SPEWCLEANUP()
{
    global $fp;

    if (connection_aborted() && $fp) {
	pclose($fp);
    }
    exit();
}
register_shutdown_function("SPEWCLEANUP");
ignore_user_abort(1);

# For backend.
TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);

$fp = popen("$TBSUEXEC_PATH $uid $unix_gid weblinktest -l $level -e $pid/$eid",
	    "r");
if (! $fp) {
    USERERROR("Linktest failed!", 1);
}

header("Content-Type: text/plain");
header("Expires: Mon, 26 Jul 1997 05:00:00 GMT");
header("Cache-Control: no-cache, must-revalidate");
header("Pragma: no-cache");
flush();

echo date("D M d G:i:s T");
echo "\n";
echo "Starting linktest run at level $level\n";
flush();
while (!feof($fp)) {
    $string = fgets($fp, 1024);
    echo "$string";
    flush();
}
$retval = pclose($fp);
$fp = 0;
if ($retval == 0)
    echo "Linktest run was successful!\n";
echo date("D M d G:i:s T");
echo "\n";

?>
