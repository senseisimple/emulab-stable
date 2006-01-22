<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only admin people!
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

if (! $isadmin) {
    USERERROR("You do not have permission to pre-reserve nodes!", 1);
}

#
# Verify page arguments.
# 
if (!isset($node_id) ||
    strcmp($node_id, "") == 0) {
    USERERROR("You must provide a node ID.", 1);
}
if (!TBvalid_node_id($node_id)) {
    PAGEARGERROR("Illegal characters in node_id");
}

#
# Check to make sure that this is a valid nodeid
#
if (! TBValidNodeName($node_id)) {
    USERERROR("$node_id is not a valid node name!", 1);
}

#
# Clear and zap back.
#
if (isset($clear)) {
    DBQueryFatal("update nodes set reserved_pid=NULL ".
		 "where node_id='$node_id'");

    header("Location: $TBBASE/shownode.php3?node_id=$node_id");
    return;
}

#
# Spit the form out using the array of data. 
# 
function SPITFORM($node_id, $reserved_pid, $error)
{
    #
    # Standard Testbed Header
    #
    PAGEHEADER("Pre Reserve a node to a project");

    #
    # Get list of projects.
    #
    $query_result =
	DBQueryFatal("select pid from projects where approved!=0 ".
		     "order by pid");

    if (mysql_num_rows($query_result) == 0) {
	USERERROR("There are no projects!", 1);
    }

    if ($error) {
	echo "<center>
               <font size=+1 color=red>$error</font>
              </center>\n";
    }

    echo "<br>
          <table align=center border=1> 
          <form action='prereserve_node.php3?node_id=$node_id' method=post>\n";

    #
    # Select Project
    #
    echo "<tr>
              <td>Select Project:</td>
              <td><select name=\"reserved_pid\">
                      <option value=''>Please Select &nbsp</option>\n";

    while ($row = mysql_fetch_array($query_result)) {
	$pid      = $row['pid'];
	$selected = "";

	if ($reserved_pid == $pid)
	    $selected = "selected";
	
	echo "        <option $selected value='$pid'>$pid </option>\n";
    }
    echo "       </select>";
    echo "    </td>
          </tr>\n";

    echo "<tr>
              <td align=center colspan=2>
                  <b><input type=submit name=submit value=Submit></b>
              </td>
          </tr>\n";

    echo "</form>
          </table>\n";
}

#
# On first load, display a virgin form and exit.
#
if (! $submit) {
    SPITFORM($node_id, $TBOPSPID, False);
    PAGEFOOTER();
    return;
}

#
# Otherwise, must validate and redisplay if errors.
#
$error = False;

#
# Project:
#
if (!isset($reserved_pid) || $reserved_pid == "") {
    $error = "Must supply a project name";
}
elseif (!TBvalid_pid($reserved_pid)) {
    $error = "Illegal characters in project name";
}
elseif (!TBValidProject($reserved_pid)) {
    $error = "Invalid project name";
}
if ($error) {
    SPITFORM($node_id, $TBOPSPID, $error);
    PAGEFOOTER();
    return;
}

#
# Set the pid and zap back to shownode page.
#
DBQueryFatal("update nodes set reserved_pid='$reserved_pid' ".
	     "where node_id='$node_id'");

header("Location: $TBBASE/shownode.php3?node_id=$node_id");
