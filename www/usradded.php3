<?php
include("defs.php3");

echo "<html>
      <head>
      <title>Joining a project</title>
      <link rel='stylesheet' href='tbstyle.css' type='text/css'>
      </head>
      <body>";
#
# First off, sanity check the form to make sure all the required fields
# were provided. I do this on a per field basis so that we can be
# informative. Be sure to correlate these checks with any changes made to
# the project form. Note that this sequence of statements results in
# only the last bad field being displayed, but thats okay. The user will
# eventually figure out that fields marked with * mean something!
#
$formerror="No Error";
if (!isset($uid) ||
    strcmp($uid, "") == 0) {
  $formerror = "UserName";
}
if (!isset($usr_email) ||
    strcmp($usr_email, "") == 0) {
  $formerror = "Email Address";
}
if (!isset($usr_name) ||
    strcmp($usr_name, "") == 0) {
  $formerror = "Full Name";
}
if (!isset($pid) ||
    strcmp($pid, "") == 0) {
  $formerror = "Project";
}
if (!isset($usr_affil) ||
    strcmp($usr_affil, "") == 0) {
  $formerror = "Institutional Afilliation";
}
if (!isset($usr_title) ||
    strcmp($usr_title, "") == 0) {
  $formerror = "Title/Position";
}

if ($formerror != "No Error") {
  echo "<h3><br><br>
        Missing field; Please go back and fill out the \"$formerror\" field!\n
        </h3>
        </body>
        </html>";
  die("");
}

#
# Database limit; UID must be 8 chars or less.
#
# XXX Note CONSTANT in expression!
#
if (strlen($uid) > 8) {
    USERERROR("The name \"$uid\" is too long! ".
              "Please select another.", 1);
}

#
# See if this is a new user or one returning.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT usr_pswd FROM users WHERE uid=\"$uid\"");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error retrieving info for $uid: $err\n", 1);
}
if (mysql_num_rows($query_result) > 0) {
    $returning = 1;
}
else {
    $returning = 0;
}

#
# If a user returning, then the login must be valid to continue any further.
# For a new user, the password must pass our tests.
#
if ($returning) {
    if (CHECKLOGIN($uid) != 1) {
        USERERROR("You are not logged in. Please log in and try again.", 1);
    }
}
else {
    if (strcmp($password1, $password2)) {
        USERERROR("You typed different passwords in each of the two password ".
                  "entry fields. <br> Please go back and correct them.",
                  1);
    }
    $mypipe = popen(escapeshellcmd(
    "/usr/testbed/bin/checkpass $password1 $uid '$usr_name:$usr_email'"),
    "w+");
    if ($mypipe) { 
        $retval=fgets($mypipe, 1024);
        if (strcmp($retval,"ok\n") != 0) {
            USERERROR("The password you have chosen will not work: ".
                      "<br><br>$retval<br>", 1);
        } 
    }
    else {
        TBERROR("TESTBED: checkpass failure\n".
                "\n$usr_name ($uid) just tried to set up a testbed ".
                "account,\n".
                "but checkpass pipe did not open (returned '$mypipe').", 1);
    }
}

#
# For a new user:
# * Create a new account in the database.
# * Add user email to the list of email address.
# * Generate a mail message to the user with the verification key.
#
if (! $returning) {
    $unixuid_query  = "SELECT unix_uid FROM users ORDER BY unix_uid DESC";
    $unixuid_result = mysql_db_query($TBDBNAME, $unixuid_query);
    $row = mysql_fetch_row($unixuid_result);
    $unix_uid = $row[0];
    $unix_uid++;
    $encoding = crypt("$password1");

    $newuser_command = "INSERT INTO users ".
	"(uid,usr_created,usr_expires,usr_name,usr_email,usr_addr,".
	"usr_phone,usr_title,usr_affil,usr_pswd,unix_uid,status) ".
	"VALUES ('$uid',now(),'$usr_expires','$usr_name','$usr_email',".
	"'$usr_addr','$usr_phone','$usr_title','$usr_affil',".
        "'$encoding','$unix_uid','newuser')";
    $newuser_result  = mysql_db_query($TBDBNAME, $newuser_command);
    if (! $newuser_result) {
        $err = mysql_error();
        TBERROR("Database Error adding adding new user $uid: $err\n", 1);
    }

    #
    # Note, we should do this after the user comes back and does the
    # verification step! This ensures we have a valid email address
    # and the user really wants to use the testbed.
    # 
    $fp = fopen($TBLIST_USERS, "a");
    if (! $fp) {
        TBERROR("Could not open $TBLIST_USERS to add new project leader", 0);
    }
    else {
        fwrite($fp, "$usr_email\n");
        fclose($fp);
    }

    $key = GENKEY($uid);

    mail("$usr_email", "TESTBED: Your New User Key",
	 "\n".
         "Dear $usr_name:\n\n".
         "\tHere is your key to verify your account on the ".
         "Utah Network Testbed:\n\n".
         "\t\t$key\n\n".
         "Please return to $TBWWW and log in using\n".
	 "the user name and password you gave us when you applied. You will\n".
	 "then find an option on the menu called 'New User Verification'.\n".
	 "Select it, and on that page enter your password and your key.\n".
	 "You will then be verified as a user. When you have been both\n".
         "verified and approved by the head of the project, you will\n".
	 "be marked as an active user, and will be granted full access to\n".
  	 "your user account.\n\n".
         "Thanks,\n".
         "Testbed Ops\n".
         "Utah Network Testbed\n",
         "From: $TBMAIL_CONTROL\n".
         "Cc: $TBMAIL_CONTROL\n".
         "Errors-To: $TBMAIL_WWW");

    #
    # Generate some warm fuzzies.
    #
    echo "<center><h1>Adding new Testbed User!</h1></center>";

    echo "<p>As a new user of the Testbed, for
          security purposes, you will receive by e-mail a key. When you
          receive it, come back to the site, and log in. When you do, you
          will see a new menu option called 'New User Verification'. On
          that page, enter in your username, password, and the key,
          exactly as you received it in your e-mail. You will then be
          marked as a verified user.
          <p>Once you have been both verified
          and approved, you will be classified as an active user, and will 
          be granted full access to your user account.";
}

#
# Don't try to join twice!
# 
$query_result = mysql_db_query($TBDBNAME,
	"select * from grp_memb where uid='$uid' and gid='$pid'");
if (mysql_num_rows($query_result) > 0) {
    die("<h3><br><br>".
        "You have already applied for membership in project: $pid.".
        "</h3>");
}

#
# Add to the project, but with trust=none. The project leader will have
# to upgrade the trust level, making the new user real.
#
$query_result = mysql_db_query($TBDBNAME,
	"insert into grp_memb (uid,gid,trust) values ('$uid','$pid','none');");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error adding adding user $uid to group $pid: $err\n", 1);
}

#
# Generate an email message to the project leader. We have to get the
# email message out of the database, of course.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT grp_head_uid FROM groups WHERE gid='$pid'");
if (($row = mysql_fetch_row($query_result)) == 0) {
    $err = mysql_error();
    TBERROR("Database Error getting project leader for group $pid: $err\n", 1);
}
$group_leader_uid = $row[0];

$query_result = mysql_db_query($TBDBNAME,
	"SELECT usr_email FROM users WHERE uid='$group_leader_uid'");
if (($row = mysql_fetch_row($query_result)) == 0) {
    $err = mysql_error();
    TBERROR("Database Error getting email address for group leader ".
            "$group_leader_uid: $err\n", 1);
}
$group_leader_email = $row[0];

mail("$group_leader_email",
     "TESTBED: New Project Member",
     "\n$usr_name ($uid) is trying to join your project ($pid).\n".
     "$usr_name has the\n".
     "Testbed username $uid and email address $usr_email.\n$usr_name's ".
     "phone number is $usr_phone and address $usr_addr.\n\n".
     "Please return to $TBWWW\n".
     "log in, and select the 'New User Approval' page to enter your\n".
     "decision regarding $usr_name's membership in your project\n\n".
     "Thanks,\n".
     "Testbed Ops\n".
     "Utah Network Testbed\n",
     "From: $TBMAIL_CONTROL\n".
     "Cc: $TBMAIL_CONTROL\n".
     "Errors-To: $TBMAIL_WWW");

#
# Generate some warm fuzzies.
#
echo "<br>
      <p>The leader of project '$pid' has been notified of your application.
      He/She will make a decision and either approve or deny your application,
      and you will be notified as soon as a decision has been made.";
?>
</body>
</html>
