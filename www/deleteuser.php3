<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Delete User Account");

#
# Only known and logged in users allowed.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Verify arguments.
# 
if (!isset($target_uid) ||
    strcmp($target_uid, "") == 0) {
    USERERROR("You must provide a User ID.", 1);
}

$isadmin = ISADMIN($uid);

#
# Confirm a real user
# 
$query_result = mysql_db_query($TBDBNAME,
	"SELECT uid FROM users where uid='$target_uid'");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error getting user $target_uid: $err\n", 1);
}
if (mysql_num_rows($query_result) == 0) {
    USERERROR("No such user '$target_uid'", 1);
}

#
# Check user. We will eventually allow project leaders to do this.
#
if (!$isadmin) {
    USERERROR("You do not have permission to remove user '$target_uid'", 1);
}

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we should
# probably redirect the browser back up a level.
#
if ($canceled) {
    echo "<center><h2><br>
          User Removal Canceled!
          </h2></center>\n";
    
    PAGEFOOTER();
    return;
}

if (!$confirmed) {
    echo "<center><h2><br>
          Are you <b>REALLY</b> sure you want to remove User '$target_uid?'
          </h2>\n";
    
    echo "<form action=\"deleteuser.php3\" method=\"post\">";
    echo "<input type=hidden name=target_uid value=\"$target_uid\">\n";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

if (!$confirmed_twice) {
    echo "<center><h2><br>
	  Okay, lets be sure.<br>
          Are you <b>REALLY REALLY</b> sure you want to remove
          User '$target_uid?'
          </h2>\n";
    
    echo "<form action=\"deleteuser.php3\" method=\"post\">";
    echo "<input type=hidden name=target_uid value=\"$target_uid\">\n";
    echo "<input type=hidden name=confirmed value=Confirm>\n";
    echo "<b><input type=submit name=confirmed_twice value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

#
# The project membership table needs to be cleansed.
#
$query_result = mysql_db_query($TBDBNAME,
	"delete FROM proj_memb where uid='$target_uid'");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error removing $target_uid from project ".
	    "membership table: $err\n", 1);
}

#
# Then the users table,
# 
$query_result = mysql_db_query($TBDBNAME,
	"delete FROM users where uid='$target_uid'");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error removing $target_uid from users table: ".
	    "$err\n", 1);
}

#
# Remove the user account from the control nodes.
#
SUEXEC($uid, "flux", "rmacct-ctrl_wrapper $target_uid", 0);

#
# Warm fuzzies.
#
echo "<center><h2>
     User '$target_uid' has been removed with prejudice!
     </h2></center>\n";

#
# Generate an email to the testbed list so we all know what happened.
#
$query_result = mysql_db_query($TBDBNAME,
	"select usr_name,usr_email FROM users where uid='$uid'");
$row = mysql_fetch_row($query_result);
$uid_name  = $row[0];
$uid_email = $row[1];

mail($TBMAIL_CONTROL,
     "TESTBED: User $target_uid removed",
     "User '$target_uid' has been removed by $uid ($uid_name).\n\n".
     "Please remember to remove the backup directory in /users\n\n",
     "From: $uid_name <$uid_email>\n".
     "Errors-To: $TBMAIL_WWW");

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
