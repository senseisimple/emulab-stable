<html>
<head>
<title>Show Project Information</title>
<link rel="stylesheet" href="tbstyle.css" type="text/css">
</head>
<body>
<?php
include("defs.php3");

#
# Note the difference with which this page gets it arguments!
# I invoke it using GET arguments, so uid and pid are are defined
# without having to find them in URI (like most of the other pages
# find the uid).
#

#
# Only known and logged in users can end experiments.
#
LOGGEDINORDIE($uid);

$isadmin = ISADMIN($uid);

#
# Verify form arguments.
# 
if (!isset($pid) ||
    strcmp($pid, "") == 0) {
    USERERROR("You must provide a project ID.", 1);
}

#
# Check to make sure thats this is a valid PID.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT * FROM projects WHERE pid=\"$pid\"");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("The project $pid is not a valid project.", 1);
}
$row = mysql_fetch_array($query_result);

#
# Verify that this uid is a member of the project for the experiment
# being displayed, or is an admin person.
#
if (!$isadmin) {
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT pid FROM proj_memb WHERE uid=\"$uid\" and pid=\"$pid\"");
    if (mysql_num_rows($query_result) == 0) {
        USERERROR("You are not a member of Project $pid.", 1);
    }
}

echo "<center>
      <h3>Project Information</h3>
      </center>
      <table align=center border=1>\n";

$proj_created	= $row[created];
$proj_expires	= $row[expires];
$proj_name	= $row[name];
$proj_URL	= $row[URL];
$proj_head_uid	= $row[head_uid];
$proj_pcs       = $row[num_pcs];
$proj_sharks    = $row[num_sharks];
$proj_why       = $row[why];
$control_node	= $row[control_node];

#
# Generate the table.
# 
echo "<tr>
          <td>Name: </td>
          <td class=\"left\">$pid</td>
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
          <td>#PCs: </td>
          <td class=\"left\">$proj_pcs</td>
      </tr>\n";

echo "<tr>
          <td>#Sharks: </td>
          <td class=\"left\">$proj_sharks</td>
      </tr>\n";

echo "<tr>
          <td>Created: </td>
          <td class=\"left\">$proj_created</td>
      </tr>\n";

echo "<tr>
          <td colspan='2'>Why?</td>
      </tr>\n";

echo "<tr>
          <td colspan='2' width=600>$proj_why</td>
      </tr>\n";

echo "</table>\n";

?>
</center>
</body>
</html>
