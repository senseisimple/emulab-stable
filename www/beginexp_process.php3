<html>
<head>
<title>Utah Testbed Begin Experiment</title>
<link rel='stylesheet' href='tbstyle.css' type='text/css'>
</head>
<body>
<?php
include("defs.php3");

#
# Only known and logged in users can begin experiments.
#
if (!isset($uid)) {
    USERERROR("You must be logged in to change your user information!", 1);
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
# First off, sanity check the form to make sure all the required fields
# were provided. I do this on a per field basis so that we can be
# informative. Be sure to correlate these checks with any changes made to
# the project form. Note that this sequence of  statements results in
# only the last bad field being displayed, but thats okay. The user will
# eventually figure out that fields marked with * mean something!
#
$formerror="No Error";
if (!isset($uid) ||
    strcmp($uid, "") == 0) {
  $formerror = "Username";
}
if (!isset($password) ||
    strcmp($password, "") == 0) {
  $formerror = "Password";
}
if (!isset($exp_pid) ||
    strcmp($exp_pid, "") == 0) {
  $formerror = "Select Project";
}
if (!isset($exp_id) ||
    strcmp($exp_id, "") == 0) {
  $formerror = "Experiment Name (short)";
}
if (!isset($exp_name) ||
    strcmp($exp_name, "") == 0) {
  $formerror = "Experiment Name (long)";
}
if (!isset($exp_created) ||
    strcmp($exp_created, "") == 0) {
  $formerror = "Experiment Created";
}
if (!isset($exp_nsfile) ||
    strcmp($exp_nsfile, "") == 0 ||
    strcmp($exp_nsfile, "none") == 0) {
  $formerror = "Your NS file";
}
if ($formerror != "No Error") {
  USERERROR("Missing field; Please go back and fill out ".
            "the \"$formerror\" field!", 1);
}

#
# Verify the password.
#
$pswd_result = mysql_db_query($TBDBNAME,
	"SELECT usr_pswd FROM users WHERE uid=\"$uid\"");
if (!$pswd_result) {
    TBERROR("Database Error retrieving password for $uid: $err\n", 1);
}
if ($row = mysql_fetch_row($pswd_result)) {
    $db_encoding = $row[0];
    $salt = substr($db_encoding,0,2);
    if ($salt[0] == $salt[1]) { $salt = $salt[0]; }
    $encoding = crypt("$password", $salt);
    if (strcmp($encoding, $db_encoding)) {
	USERERROR("The password provided was incorrect. ".
                  "Please go back and retype the password.", 1);
    }
}

#
# Make sure the experiment ID does not already exist.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT eid FROM experiments WHERE eid=\"$exp_id\"");
if ($row = mysql_fetch_row($query_result)) {
  USERERROR("The experiment name \"$exp_id\" you have chosen is already ".
            "in use. Please select another.", 1);
}

#
# Next, is this person a member of the project specified, and is the trust
# equal to group or local root?
#
# XXX Split across grp_memb and proj_memb. grp_memb needs to be flushed, but
# right now that has all the info we need. 
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT * FROM grp_memb WHERE gid=\"$exp_pid\" and uid=\"$uid\"");
if (($row = mysql_fetch_array($query_result)) == 0) {
  USERERROR("You are not a member of Project $exp_pid, so you cannot begin ".
            "an experiment in that project.", 1);
}
$trust = $row[trust];
if (strcmp($trust, "group_root") && strcmp($trust, "local_root")) {
  USERERROR("You are not group or local root in Project $exp_pid, so you ".
            "cannot begin an experiment in that project.", 1);
}

#
# We are going to write out the NS file to a subdir in the users
# home directory.
# 


echo "$exp_nsfile<p>";
echo "$exp_nsfile_name<p>";
echo "$exp_nsfile_size<p>";
echo "$exp_nsfile_type<p>";

copy($exp_nsfile, "/tmp/foo.ns");

?>
</body>
</html>
