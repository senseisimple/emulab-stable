<html>
<head>
<title>Show Experiment Information</title>
<link rel="stylesheet" href="tbstyle.css" type="text/css">
</head>
<body>
<?php
include("defs.php3");

$uid = "";
if ( ereg("php3\?([[:alnum:]]+)",$REQUEST_URI,$Vals) ) {
  $uid=$Vals[1];
  addslashes($uid);
} else {
  unset($uid);
}

#
# Only known and logged in users can do this.
#
if (!isset($uid)) {
    USERERROR("You must be logged in to sho experiment information!", 1);
}

#
# Verify that the uid is known in the database.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT usr_pswd FROM users WHERE uid='$uid'");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error confirming user $uid: $err\n", 1);
}
if (($row = mysql_fetch_row($query_result)) == 0) {
    USERERROR("You do not appear to have an account!", 1);
}

#
# Verify that this uid is a member of the project for the experiment
# being displayed.
#
# First get the project (PID) for the experiment (EID) requested.
# Then check to see if the user (UID) is a member of that PID.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT * FROM experiments WHERE eid=\"$exp_eid\"");
if (($exprow = mysql_fetch_array($query_result)) == 0) {
  USERERROR("The experiment $exp_eid is not a valid experiment.", 1);
}
$pid = $exprow[pid];

$query_result = mysql_db_query($TBDBNAME,
	"SELECT pid FROM proj_memb WHERE uid=\"$uid\" and pid=\"$pid\"");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("You are not a member of the Project for Experiment: $exp_id.", 1);
}

?>

<center>
<h1>Experiment Information</h1>
<table align="center" border="1">

<?php

$exp_expires = $row[expt_expires];
$exp_name    = $row[expt_name];
$exp_created = $row[expt_created];
$exp_start   = $row[expt_start];
$exp_end     = $row[expt_end];
$exp_created = $row[expt_created];
$exp_head    = $row[expt_head_uid];

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
          <td class=\"left\">$pid</td>
      </tr>\n";

echo "<tr>
          <td>Project Head: </td>
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
</center>
</body>
</html>
