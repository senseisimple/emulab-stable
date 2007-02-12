<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("node_defs.php");

#
# Only admin people!
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

if (! $isadmin) {
    USERERROR("You do not have permission to pre-reserve nodes!", 1);
}

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("node",    PAGEARG_NODE);
$optargs = OptionalPageArguments("clear",   PAGEARG_BOOLEAN,
				 "submit",  PAGEARG_STRING,
				 "project", PAGEARG_PROJECT);

#
# Need these below
#
$node_id = $node->node_id();

#
# Clear and zap back.
#
if (isset($clear) && $clear) {
    DBQueryFatal("update nodes set reserved_pid=NULL ".
		 "where node_id='$node_id'");

    header("Location: " . CreateURL("shownode", $node));
    return;
}

#
# Spit the form out using the array of data. 
# 
function SPITFORM($node, $reserved_pid, $error)
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

    $url = CreateURL("prereserve_node", $node);
    echo "<br>
          <table align=center border=1> 
          <form action='$url' method=post>\n";

    #
    # Select Project
    #
    echo "<tr>
              <td>Select Project:</td>
              <td><select name=\"pid\">
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
if (!isset($submit)) {
    SPITFORM($node, $TBOPSPID, null);
    PAGEFOOTER();
    return;
}

#
# Otherwise, must validate and redisplay if errors.
#
if (!isset($project)) {
    SPITFORM($node, $TBOPSPID, "Must supply a project name");
    PAGEFOOTER();
    return;
}
$pid = $project->pid();

#
# Set the pid and zap back to shownode page.
#
DBQueryFatal("update nodes set reserved_pid='$pid' ".
	     "where node_id='$node_id'");

header("Location: " . CreateURL("shownode", $node));
