<html>
<head>
<title>Terminate Experiment</title>
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
# Verify that this uid is a member of the project for the experiment.
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

</center>
</body>
</html>
