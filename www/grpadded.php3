<html>
<head>
<title>Utah Testbed Project Request</title>
<link rel='stylesheet' href='tbstyle.css' type='text/css'>
</head>
<body>
<?php
include("defs.php3");

#
# First off, sanity check the form to make sure all the required fields
# were provided. I do this on a per field basis so that we can be
# informative. Be sure to correlate these checks with any changes made to
# the project form. Note that this sequence of  statements results in
# only the last bad field being displayed, but thats okay. The user will
# eventually figure out that fields marked with * mean something!
#
$formerror="No Error";
if (!isset($gid) ||
    strcmp($gid, "ucb-omcast") == 0) {
  $formerror = "Name";
}
if (!isset($grp_head_uid) ||
    strcmp($grp_head_uid, "") == 0) {
  $formerror = "Username";
}
if (!isset($grp_name) ||
    strcmp($grp_name, "UCB Overlay Multicast") == 0) {
  $formerror = "Long Name";
}
if (!isset($usr_name) ||
    strcmp($usr_name, "") == 0) {
  $formerror = "Full Name";
}
if (!isset($grp_URL) ||
    strcmp($grp_URL, "http://www.cs.berkeley.edu/netgrp/omcast/") == 0) {
  $formerror = "URL";
}
if (!isset($email) ||
    strcmp($email, "") == 0) {
  $formerror = "Email Address";
}
if (!isset($usr_addr) ||
    strcmp($usr_addr, "") == 0) {
  $formerror = "Postal Address";
}
if (!isset($grp_affil) ||
    strcmp($grp_affil, "UCB Networks Group") == 0) {
  $formerror = "Research Afilliation";
}
if (!isset($usr_phones) ||
    strcmp($usr_phones, "") == 0) {
  $formerror = "Phone #";
}
#
# The first password field must always be filled in. The second only
# if a new user, and we will catch that later.
#
if (!isset($password1) ||
    strcmp($password1, "") == 0)
  $formerror = "Password";
}

if ($formerror != "No Error") {
  USERERROR("Missing field; ".
            "Please go back and fill out the \"$formerror\" field!", 1);
}

#
# This is a new project request. Make sure it does not already exist.
#
$project_query  = "SELECT gid FROM groups WHERE gid=\"$gid\"";
$project_result = mysql_db_query($TBDBNAME, $project_query);

if ($row = mysql_fetch_row($project_result)) {
  USERERROR("The project name \"$gid\" you have chosen is already in use. ".
            "Please select another.", 1);
}

#
# See if this is a new user or one returning. We have to query the database
# for the uid, and then do the password thing. For a user returning, the
# password must be valid. For a new user, the password must pass our tests.
#
$pswd_query  = "SELECT usr_pswd FROM users WHERE uid=\"$grp_head_uid\"";
$pswd_result = mysql_db_query($TBDBNAME, $pswd_query);
if (!$pswd_result) {
    TBERROR("Database Error retrieving password for $grp_head_uid: $err\n",
            1);
}
if ($row = mysql_fetch_row($pswd_result)) {
    $db_encoding = $row[0];
    $salt = substr($db_encoding,0,2);
    if ($salt[0] == $salt[1]) { $salt = $salt[0]; }
    $encoding = crypt("$password1", $salt);
    if (strcmp($encoding, $db_encoding)) {
        USERERROR("The password provided was incorrect. ".
                  "Please go back and retype the password.", 1);
    }
    $returning = 1;
}
else {
    if (strcmp($password1, $password2)) {
        USERERROR("You typed different passwords in each of the two password ".
                  "entry fields. <br> Please go back and correct them.",
                  1);
    }
    $mypipe = popen(escapeshellcmd(
    "/usr/testbed/bin/checkpass $password1 $grp_head_uid '$usr_name:$email'"),
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
                "\n$usr_name ($grp_head_uid) just tried to set up a testbed ".
                "account,\n".
                "but checkpass pipe did not open (returned '$mypipe').", 1);
    }
    $returning = 0;
}

array_walk($HTTP_POST_VARS, 'addslashes');

#
# For a new user:
# * Create a new account in the database.
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
        "VALUES ('$grp_head_uid', now(), '$grp_expires', '$usr_name', ".
        "'$email', '$usr_addr', '$usr_phones', '$encoding', ".
        "'$unix_uid', 'newuser')";
    $newuser_result  = mysql_db_query($TBDBNAME, $newuser_command);
    if (! $newuser_result) {
        $err = mysql_error();
        TBERROR("Database Error adding adding new user $grp_head_uid: $err\n",
                1);
    }
    $key = GENKEY($grp_head_uid);

    mail("$email", "TESTBED: Your New User Key",
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
         "verified and approved by the Testbed Approval Committee, you will\n".
	 "be marked as an active user, and will be granted full access to\n".
  	 "your user account.\n\n".
         "Thanks,\n".
         "Testbed Ops\n".
         "Utah Network Testbed\n",
         "From: $TBMAIL_CONTROL\n".
         "Cc: $TBMAIL_WWW\n".
         "Errors-To: $TBMAIL_WWW");
}

#
# Now for the new Project
# * Bump the unix GID.
# * Create a new group in the database.
# * Create a new group_membership entry in the database, default trust=none.
# * Generate a mail message to testbed ops.
#
$unixgid_query  = "SELECT unix_gid FROM groups ORDER BY unix_gid DESC";
$unixgid_result = mysql_db_query($TBDBNAME, $unixgid_query);
$row = mysql_fetch_row($unixgid_result);
$unix_gid = $row[0];
$unix_gid++;

$newgroup_command = "INSERT INTO groups ".
     "(gid,grp_created,grp_expires,grp_name,".
     "grp_URL,grp_affil,grp_addr,grp_head_uid,cntrl_node,unix_gid)".
     "VALUES ('$gid',now(), '$grp_expires','$grp_name','$grp_URL',".
     "'$grp_affil','$grp_addr','$grp_head_uid', '','$unix_gid')";
$newgroup_result  = mysql_db_query($TBDBNAME, $newgroup_command);
if (! $newgroup_result) {
    $err = mysql_error();
    TBERROR("Database Error adding adding new group $gid: $err\n", 1);
}

$newmemb_result = mysql_db_query($TBDBNAME,
			"insert into grp_memb (uid,gid,trust)".
			"values ('$grp_head_uid','$gid','none');");
if (! $newmemb_result) {
    $err = mysql_error();
    TBERROR("Database Error adding adding new group membership: $gid: $err\n",
            1);
}

mail($TBMAIL_APPROVAL,
     "TESTBED: New Group", "'$usr_name' wants to start group ".
     "'$gid'.\nContact Info:\nName:\t\t$usr_name ($grp_head_uid)\n".
     "Email:\t\t$email\nGroup:\t\t$grp_name\nURL:\t\t$grp_URL\n".
     "Affiliation:\t$grp_affil\nAddress:\t$grp_addr\n".
     "Phone:\t\t$usr_phones\n\n".
     "Reasons:\n$why\n\nPlease review the application and when you have\n".
     "made a decision, go to $TBWWW and\n".
     "select the 'Group Approval' page.\n\nThey are expecting a result ".
     "within 72 hours.\n", 
     "From: $usr_name <$email>\n".
     "Cc: $TBMAIL_WWW\n".
     "Errors-To: $TBMAIL_WWW");

#
# For new leaders, write their email addresses to files to be used for
# generating messages.
#
# Note, we should do this after the user comes back and does the
# verification step! This ensures we have a valid email address
# and the user really wants to use the testbed.
#
if (! $returning) {
    $fp = fopen($TBLIST_LEADERS, "a");
    if (! $fp) {
        TBERROR("Could not open $TBLIST_LEADERS to add new project leader", 0);
    }
    else {
        fwrite($fp, "$email\n");
        fclose($fp);
    }

    $fp = fopen($TBLIST_USERS, "a");
    if (! $fp) {
        TBERROR("Could not open $TBLIST_USERS to add new project leader", 0);
    }
    else {
        fwrite($fp, "$email\n");
        fclose($fp);
    }
}

#
# Now give the user some warm fuzzies
#
echo "<h1>Project '$gid' successfully added.</h1>
      <h2>The Testbed Approval Committee has been notified of your application.
      Most applications are reviewed within one week. We will notify
      you by e-mail at '$usr_name&nbsp;&lt;$email>' of their decision
      regarding your proposed project '$gid'.\n";

if (! $returning) {
    echo "<p>In the meantime, for
          security purposes, you will receive by e-mail a key. When you
          receive it, come back to the site, and log in. When you do, you
          will see a new menu option called 'New User Verification'. On
          that page, enter in your password and the key,
          exactly as you received it in your e-mail. You will then be
          marked as a verified user.
          <p>Once you have been both verified
          and approved, you will be classified as an active user, and will 
          be granted full access to your user account.
          </h2>";
}
?>
</body>
</html>
