<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Delete an OS Descriptor");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Must provide the OSID!
# 
if (!isset($osid) ||
    strcmp($osid, "") == 0) {
  USERERROR("The OSID was not provided!", 1);
}

if (! TBValidOSID($osid)) {
    USERERROR("The OSID '$osid' is not valid!", 1);
}

#
# Verify permission.
#
if (!TBOSIDAccessCheck($uid, $osid, $TB_OSID_DESTROY)) {
    USERERROR("You do not have permission to delete OS Descriptor $osid!", 1);
}

#
# Get user level info.
#
if (!TBOSInfo($osid, $osname, $pid)) {
    USERERROR("OS Descriptor '$osid' is no longer valid!", 1);
}

$conflicts = 0;

#
# Check to see if the OSID is being used. Force whatever images are using
# it to be deleted or changed. This subsumes EZ created images/osids.
#
$query_result =
    DBQueryFatal("select * from images ".
		 "where part1_osid='$osid' or part2_osid='$osid' or ".
		 "      part3_osid='$osid' or part4_osid='$osid' or ".
		 "      default_osid='$osid'");

if (mysql_num_rows($query_result)) {
    echo "<center>The following images are using this OS Descriptor.<br>
          They must be deleted first!</center><br>\n";
          
    echo "<table border=1 cellpadding=2 cellspacing=2 align='center'>\n";

    echo "<tr>
              <td align=center>Image</td>
              <td align=center>PID</td>
          </tr>\n";

    while ($row = mysql_fetch_array($query_result)) {
	$imageid   = $row['imageid'];
	$url       = rawurlencode($imageid);
	$imagename = $row['imagename'];
	$pid       = $row[pid];

	echo "<tr>
                <td><A href='showimageid.php3?imageid=$url'>$imagename</A>
                    </td>
	        <td>$pid</td>
              </tr>\n";
    }
    echo "</table>\n";
    $conflicts++;
}

# Ditto for node_types ...
$query_result =
    DBQueryFatal("select class,a.type from node_type_attributes as a ".
		 "left join node_types as nt on nt.type=a.type ".
		 "where (a.attrkey='default_osid' or ".
		 "       a.attrkey='jail_osid' or ".
		 "       a.attrkey='delay_osid') and ".
		 "       a.attrvalue='$osid'");

if (mysql_num_rows($query_result)) {
    echo "<br> <center>
            The following node_types are using this OS Descriptor<br>
            in the osid and/or delay_osid fields.<br>
            They must be deleted first!
          </center><br>\n";
          
    echo "<table border=1 cellpadding=2 cellspacing=2 align='center'>\n";

    echo "<tr>
              <td align=center>Class</td>
              <td align=center>Type</td>
          </tr>\n";

    while ($row = mysql_fetch_array($query_result)) {
	$class   = $row['class'];
	$type    = $row['type'];

	echo "<tr>
                <td>$class</td>
	        <td>$type</td>
              </tr>\n";
    }
    echo "</table>\n";
    $conflicts++;
}

if ($conflicts) {
    PAGEFOOTER();
    return;
}

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we should
# probably redirect the browser back up a level.
#
if ($canceled) {
    echo "<center><h2><br>
          OS Descriptor removal canceled!
          </h2></center>\n";
    
    PAGEFOOTER();
    return;
}

if (!$confirmed) {
    echo "<center><h2><br>
          Are you <b>REALLY</b>
          sure you want to delete OS Descriptor '$osname' in Project $pid?
          </h2>\n";
    
    echo "<form action=\"deleteosid.php3\" method=\"post\">";
    echo "<input type=hidden name=osid value=\"$osid\">\n";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

#
# Delete the record,
#
DBQueryFatal("DELETE FROM os_info WHERE osid='$osid'");

echo "<p>
      <center><h2>
      OS Descriptor '$osname' in Project $pid has been deleted!
      </h2></center>\n";

echo "<br>
      <a href='showosid_list.php3'>Back to OS Descriptor list</a>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
