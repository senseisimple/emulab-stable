<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Terminate Project and Remove all Trace");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Currently, only admin users can do this. Change later.
#
$isadmin = ISADMIN($uid);
if (! $isadmin) {
    USERERROR("You do not have permission to remove project '$pid'", 1);
}

#
# Confirm a real project
# 
$query_result = mysql_db_query($TBDBNAME,
	"SELECT head_uid FROM projects where pid='$pid'");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error getting project head_uid: $err\n", 1);
}
if (mysql_num_rows($query_result) == 0) {
    USERERROR("No such project '$pid'", 1);
}
$row = mysql_fetch_row($query_result);
$head_uid = $row[0];

#
# Check user. 
#
if (!$isadmin) {
    if ($uid != $head_uid) {
	USERERROR("Only the project leader or an administrator may ".
		  "terminate a project!", 1);
    }
}

#
# Check to see if there are any active experiments. Abort if there are.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT * FROM experiments where pid='$pid'");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error getting experiment list for $pid: $err\n", 1);
}
if (mysql_num_rows($query_result)) {
    USERERROR("Project '$pid' has active experiments. You must terminate ".
	      "those experiments before you can remove the project!", 1);
}

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we should
# probably redirect the browser back up a level.
#
if ($canceled) {
    echo "<center><h2>
          Project removal canceled!
          </h2></center>\n";
    
    PAGEFOOTER();
    return;
}

if (!$confirmed) {
    echo "<center><h2>
          Are you <b>REALLY</b> sure you want to remove Project '$pid?'
          </h2>\n";
    
    echo "<form action=\"deleteproject.php3\" method=\"post\">";
    echo "<input type=hidden name=pid value=\"$pid\">\n";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

if (!$confirmed_twice) {
    echo "<center><h2>
	  Okay, lets be sure.<br>
          Are you <b>REALLY REALLY</b> sure you want to remove Project '$pid?'
          </h2>\n";
    
    echo "<form action=\"deleteproject.php3\" method=\"post\">";
    echo "<input type=hidden name=pid value=\"$pid\">\n";
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
	"delete FROM proj_memb where pid='$pid'");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error removing $pid from project membership table: ".
	    "$err\n", 1);
}

#
# Then the project table itself.
# 
$query_result = mysql_db_query($TBDBNAME,
	"delete FROM projects where pid='$pid'");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error removing $pid from project table: ".
	    "$err\n", 1);
}

#
# Remove the project directory. 
#
SUEXEC($uid, "flux", "rmprojdir_wrapper $pid", 0);

#
# Warm fuzzies.
#
echo "<center><h2>
     Project '$pid' has been removed with prejudice!
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
     "TESTBED: Project $pid removed",
     "Project '$pid' has been removed by $uid ($uid_name).\n\n".
     "Please remember to remove the backup directory in /proj\n\n",
     "From: $uid_name <$uid_email>\n".
     "Errors-To: $TBMAIL_WWW");

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
