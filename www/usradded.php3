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
if (!isset($grp) ||
    strcmp($grp, "") == 0) {
  $formerror = "Project";
}
#
# Not sure about the passwd. If the user is already known, then is he
# supposed to plug his passwd in?
#
if ((!isset($password1) || strcmp($password1, "") == 0) ||
    (!isset($password2) || strcmp($password2, "") == 0)) {
  $formerror = "Password";
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
# See if this is a new user or one returning. We have to query the database
# for the uid, and then do the password thing. For a user returning, the
# password must be valid. For a new user, the password must pass our tests.
#
$pswd_query  = "SELECT usr_pswd FROM users WHERE uid=\"$uid\"";
$pswd_result = mysql_db_query($TBDBNAME, $pswd_query);
if (!$pswd_result) {
    TBERROR("Database Error retrieving password for $uid: $err\n", 1);
}
if ($row = mysql_fetch_row($pswd_result)) {
    $db_encoding = $row[0];
    $salt = substr($db_encoding,0,2);
    if ($salt[0] == $salt[1]) { $salt = $salt[0]; }
    $encoding = crypt("$password1", $salt);
    if (strcmp($encoding, $db_encoding)) {
        die("<h3><br><br>".
            "The password provided was incorrect. ".
            "Please go back and retype the password.\n".
            "</h3>");
    }
    $returning = 1;
}
else {
    if (strcmp($password1, $password2)) {
        die("<h3><br><br>".
            "You typed different passwords in each of the two password ".
            "entry fields. <br> Please go back and correct them.\n".
            "</h3>");
    }
    $mypipe = popen(escapeshellcmd(
    "/usr/testbed/bin/checkpass $password1 $uid '$usr_name:$usr_email'"),
    "w+");
    if ($mypipe) { 
        $retval=fgets($mypipe, 1024);
        if (strcmp($retval,"ok\n") != 0) {
            die("<h3><br><br>".
                "The password you have chosen will not work: ".
                "<br><br>$retval<br>".
                "</h3>");
        } 
    }
    else {
        mail($TBMAIL_WWW, "TESTBED: checkpass failure",
             "\n$usr_name ($uid) just tried to set up a testbed ".
             "account,\n".
             "but checkpass pipe did not open (returned '$mypipe').\n".
             "\nThanks\n");
    }
    $returning = 0;
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
	"usr_phone,usr_pswd,unix_uid,status) ".
	"VALUES ('$uid',now(),'$usr_expires','$usr_name','$usr_email',".
	"'$usr_addr','$usr_phone','$encoding','$unix_uid','newuser')";
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
         "\tThank you for applying to use the Utah Network Testbed.\n".
         "As promised, here is your key to verify your account:\n\n".
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
         "Cc: $TBMAIL_WWW\n".
         "Errors-To: $TBMAIL_WWW");

    #
    # Generate some warm fuzzies.
    #
    echo "<h3>As a new user of the Testbed, for
          security purposes, you will receive by e-mail a key. When you
          receive it, come back to the site, and log in. When you do, you
          will see a new menu option called 'New User Verification'. On
          that page, enter in your username, password, and the key,
          exactly as you received it in your e-mail. You will then be
          marked as a verified user.<br>
          <h3>Once you have been both verified
          and approved, you will be classified as an active user, and will 
          be granted full access to your user account.</h3>";
}

#
# Don't try to join twice!
# 
$query_result = mysql_db_query($TBDBNAME,
	"select * from grp_memb where uid='$uid' and gid='$grp'");
if (mysql_num_rows($query_result) > 0) {
    die("<h3><br><br>".
        "You have already applied for membership in project: $grp.".
        "</h3>");
}

#
# Add to the project, but with trust=none. The project leader will have
# to upgrade the trust level, making the new user real.
#
$query_result = mysql_db_query($TBDBNAME,
	"insert into grp_memb (uid,gid,trust) values ('$uid','$grp','none');");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error adding adding user $uid to group $grp: $err\n", 1);
}

#
# Generate an email message to the project leader. We have to get the
# email message out of the database, of course.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT grp_head_uid FROM groups WHERE gid='$grp'");
if (($row = mysql_fetch_row($query_result)) == 0) {
    $err = mysql_error();
    TBERROR("Database Error getting project leader for group $grp: $err\n", 1);
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
     "\n$usr_name ($uid) is trying to join your project ($grp).\n".
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
     "Cc: $TBMAIL_WWW\n".
     "Errors-To: $TBMAIL_WWW");

#
# Generate some warm fuzzies.
#
echo "<br><br><h3>
      The leader of project '$grp' has been notified of your application.
      He/She will make a decision and either approve or deny your application,
      and you will be notified as soon as a decision has been made.<br><br>
      Thanks for using the Testbed!
      </h3>";

?>
</body>
</html>
