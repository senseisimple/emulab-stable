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
$query_result = mysql_db_query($TBDBNAME,
	"SELECT pid FROM proj_memb WHERE uid=\"$uid\" and pid=\"$exp_pid\"");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("You are not a member of Project $exp_pid for ".
            "Experiment: $exp_eid.", 1);
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
