<html>
<head>
<title>Confirming Verification</title>
<link rel="stylesheet" href="tbstyle.css" type="text/css">
</head>
<body>
<?php
include("defs.php3");

#
# Only known and logged in users can be verified. UID came in as a post var.
#
LOGGEDINORDIE($uid);

#
# Must provide the key!
# 
if (!isset($key) || strcmp($key, "") == 0) {
    USERERROR("Missing field; ".
              "Please go back and fill out the \"key\" field!", 1);
}

echo "<h1>Confirming Verification</h1><p>";

#
# The user is logged in, so all we need to do is confirm the key.
# Make sure it matches.
#
$keymatch = GENKEY($uid);

if (strcmp($key, $keymatch)) {
    USERERROR("The given key \"$key\" is incorrect. Please go back and ".
              "enter the correct key.", 1);
}

#
# Grab the status and do the modification.
#
$query_result = mysql_db_query($TBDBNAME,
	"select status from users where uid='$uid'");
if (!$query_result ||
    (($row = mysql_fetch_row($query_result)) == 0)) {
    $err = mysql_error();
    TBERROR("Database Error retrieving status for $uid: $err\n", 1);
}
$status = $row[0];

if (strcmp($status, "unverified") == 0) {
    $query_result = mysql_db_query($TBDBNAME,
	"update users set status='active' where uid='$uid'");
    if (!$query_result) {
        $err = mysql_error();
        TBERROR("Database Error setting status for $uid: $err\n", 1);
    }

    echo "<h3>Because your group leader has already approved ".
	 "your membership in the group, you are now an active user ".
	 "of the Testbed. Reload the frame at your left, and any options ".
	 "that are now available to you will appear.</h3>\n";
}
elseif (strcmp($status, "newuser") == 0) {
    $query_result = mysql_db_query($TBDBNAME,
	"update users set status='unapproved' where uid='$uid'");
    if (!$query_result) {
        $err = mysql_error();
        TBERROR("Database Error setting status for $uid: $err\n", 1);
    }

    echo "<h3>You have now been verified. However, your application ".
	 "has not yet been approved by the group leader. You will receive ".
	 "email when that has been done.</h3>\n";
}
else {
    USERERROR("You have already been verified, $uid. If you did not perform ".
	      "this verification, please notify Testbed Operations.", 1);
}
?>

</body>
</html>
