<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Show Experiment Information");

#
# Only known and logged in users can end experiments.
#
$uid = "";
if ( ereg("php3\?([[:alnum:]]+)",$REQUEST_URI,$Vals) ) {
    $uid=$Vals[1];
    addslashes($uid);
} else {
    unset($uid);
}
LOGGEDINORDIE($uid);

$isadmin = ISADMIN($uid);

#
# Verify form arguments.
# 
if (!isset($exp_pideid) ||
    strcmp($exp_pideid, "") == 0) {
    USERERROR("You must provide an experiment ID.", 1);
}

#
# First get the project (PID) from the form parameter, which came in
# as <pid>$$<eid>.
#
$exp_eid = strstr($exp_pideid, "$$");
$exp_eid = substr($exp_eid, 2);
$exp_pid = substr($exp_pideid, 0, strpos($exp_pideid, "$$", 0));

#
# Check to make sure thats this is a valid PID/EID tuple.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT * FROM experiments WHERE ".
        "eid=\"$exp_eid\" and pid=\"$exp_pid\"");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("The experiment $exp_eid is not a valid experiment ".
            "in project $exp_pid.", 1);
}
$exprow = mysql_fetch_array($query_result);

#
# Verify that this uid is a member of the project for the experiment
# being displayed.
#
if (!$isadmin) {
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT pid FROM proj_memb WHERE uid=\"$uid\" and pid=\"$exp_pid\"");
    if (mysql_num_rows($query_result) == 0) {
        USERERROR("You are not a member of Project $exp_pid for ".
                  "Experiment: $exp_eid.", 1);
    }
}
?>

<center>
<h1>Experiment Information</h1>
<table align="center" border="1">

<?php

$exp_expires = $exprow[expt_expires];
$exp_name    = $exprow[expt_name];
$exp_created = $exprow[expt_created];
$exp_start   = $exprow[expt_start];
$exp_end     = $exprow[expt_end];
$exp_created = $exprow[expt_created];
$exp_head    = $exprow[expt_head_uid];

#
# Generate the table.
# 
echo "<tr>
          <td>Name: </td>
          <td class=\"left\">$exp_eid</td>
      </tr>\n";

echo "<tr>
          <td>Long Name: </td>
          <td class=\"left\">$exp_name</td>
      </tr>\n";

echo "<tr>
          <td>Project: </td>
          <td class=\"left\">$exp_pid</td>
      </tr>\n";

echo "<tr>
          <td>Experiment Head: </td>
          <td class=\"left\">$exp_head</td>
      </tr>\n";

echo "<tr>
          <td>Created: </td>
          <td class=\"left\">$exp_created</td>
      </tr>\n";

echo "<tr>
          <td>Starts: </td>
          <td class=\"left\">$exp_start</td>
      </tr>\n";

echo "<tr>
          <td>Ends: </td>
          <td class=\"left\">$exp_end</td>
      </tr>\n";

echo "<tr>
          <td>Expires: </td>
          <td class=\"left\">$exp_expires</td>
      </tr>\n";

?>
</table>

<?php

#
# Suck out the node information.
# 
$reserved_result = mysql_db_query($TBDBNAME,
	"SELECT * FROM reserved WHERE ".
        "eid=\"$exp_eid\" and pid=\"$exp_pid\"");
if (mysql_num_rows($reserved_result)) {
    echo "<h3>Reserved Nodes</h3>
          <table align=center border=1>
          <tr>
              <td>Node ID</td>
              <td>Node Type</td>
              <td>Default Image</td>
          </tr>\n";

    #
    # I'm so proud!
    #
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT nodes.node_id, nodes.type, nodes.def_boot_image_id ".
        "FROM nodes LEFT JOIN reserved ".
        "ON nodes.node_id=reserved.node_id ".
        "WHERE reserved.eid=\"$exp_eid\" and reserved.pid=\"$exp_pid\"");

    while ($row = mysql_fetch_array($query_result)) {
        $node_id = $row[node_id];
        $type    = $row[type];
        $defid   = $row[def_boot_image_id];
        echo "<tr>
                  <td>$node_id</td>
                  <td>$type</td>
                  <td>$defid</td>
              </tr>\n";
    }
    echo "</table>\n";
}

#
# Lets dump the project information too.
# 
$query_result = mysql_db_query($TBDBNAME,
	"SELECT * FROM projects WHERE pid=\"$exp_pid\"");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error getting info for project $exp_pid: $err\n", 1);
}
$row = mysql_fetch_array($query_result);

echo "<center>
      <h3>Project Information</h3>
      </center>
      <table align=center border=1>\n";

$proj_created	= $row[created];
$proj_expires	= $row[expires];
$proj_name	= $row[name];
$proj_URL	= $row[URL];
$proj_affil	= $row[affil];
$proj_addr	= $row[addr];
$proj_head_uid	= $row[head_uid];
$control_node	= $row[control_node];

#
# Generate the table.
# 
echo "<tr>
          <td>Name: </td>
          <td class=\"left\">$exp_pid</td>
      </tr>\n";

echo "<tr>
          <td>Long Name: </td>
          <td class=\"left\">$proj_name</td>
      </tr>\n";

echo "<tr>
          <td>Project Head: </td>
          <td class=\"left\">$proj_head_uid</td>
      </tr>\n";

echo "<tr>
          <td>URL: </td>
          <td class=\"left\">
              <A href='$proj_URL'>$proj_URL</A></td>
      </tr>\n";

echo "<tr>
          <td>Created: </td>
          <td class=\"left\">$proj_created</td>
      </tr>\n";

echo "<tr>
          <td>Expires: </td>
          <td class=\"left\">$proj_expires</td>
      </tr>\n";

echo "</table>
      </center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
