<html>
<head>
<title>Utah Testbed Modify User Information</title>
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
if (!isset($uid) ||
    strcmp($uid, "") == 0) {
  $formerror = "Username";
}
if (!isset($usr_name) ||
    strcmp($usr_name, "") == 0) {
  $formerror = "Full Name";
}
if (!isset($usr_email) ||
    strcmp($usr_email, "") == 0) {
  $formerror = "Email Address";
}
if (!isset($usr_addr) ||
    strcmp($usr_addr, "") == 0) {
  $formerror = "Mailing Address";
}
if (!isset($usr_phone) ||
    strcmp($usr_phone, "") == 0) {
  $formerror = "Phone #";
}
if (!isset($old_password) || strcmp($old_password, "") == 0) {
  $formerror = "Old Password";
}

if ($formerror != "No Error") {
  USERERROR("Missing field; Please go back and fill out ".
            "the \"$formerror\" field!", 1);
}

#
# Verify the old password.
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
    $encoding = crypt("$old_password", $salt);
    if (strcmp($encoding, $db_encoding)) {
	USERERROR("The password provided was incorrect. ".
                  "Please go back and retype the password.", 1);
    }
}

#
# Now see if the user is requesting to change the password. We do the usual
# checks to make sure the two fields agree and that it passes our tests for
# safe passwords.
#
if (isset($new_password1) && strcmp($new_password2, "")) {
    if (strcmp($new_password1, $new_password2)) {
	USERERROR("You typed different passwords in each of the two password ".
		  "entry fields. <br> Please go back and correct them.", 1);
    }

    $mypipe = popen(escapeshellcmd(
    "/usr/testbed/bin/checkpass $new_password1 $uid '$usr_name:$usr_email'"),
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
               "$usr_name ($uid) just tried change his password\n".
               "but checkpass pipe did not open (returned '$mypipe').", 1);
    }

    #
    # Password is good. Insert into database.
    #
    $encoding = crypt("$new_password1");
    $insert_result  = mysql_db_query($TBDBNAME, 
		"UPDATE users SET usr_pswd=\"$encoding\" WHERE uid=\"$uid\"");

    if (! $insert_result) {
        $err = mysql_error();
        TBERROR("Database Error changing password for $uid: $err", 1);
    }
}

array_walk($HTTP_POST_VARS, 'addslashes');

#
# Now change the rest of the information.
#
$insert_result = mysql_db_query($TBDBNAME, 
	"UPDATE users SET ".
	"usr_name=\"$usr_name\",       ".
	"usr_email=\"$usr_email\",     ".
	"usr_addr=\"$usr_addr\",       ".
	"usr_phone=\"$usr_phone\",     ".
	"usr_expires=\"$usr_expires\"  ".
	"WHERE uid=\"$uid\"");

if (! $insert_result) {
    $err = mysql_error();
    TBERROR("Database Error changing user info for $uid: $err", 1);
}

?>
<center>
<br>
<br>
<h3>User information successfully modified!<h3><p>
</center>
</body>
</html>
