<?php
include("defs.php3");
include("showstuff.php3");

$changed_password = "No";

#
# Hold off drawing header until later!
#

#
# Only known and logged in users can modify info.
#
# Note different test though, since we want to allow logged in
# users with expired passwords to change them.
#
$uid = GETLOGIN();
LOGGEDINORDIE_SPECIAL($uid);

#
# The target_uid comes in as a POST var. It must be set. We allow
# admin users to modify other user info, so must check for that.
# 
if (!isset($target_uid) ||
    strcmp($target_uid, "") == 0) {
  FORMERROR("Username");
}

#
# Admin types can change anyone. Otherwise, must be project root, or group
# root in at least one of the same groups. This is not exactly perfect, but
# it will do. You should not make someone group root if you do not trust
# them to behave.
#
if ($uid != $target_uid) {
    $isadmin = ISADMIN($uid);

    if (! $isadmin) {
	if (! TBUserInfoAccessCheck($uid, $target_uid,
				    $TB_USERINFO_MODIFYINFO)) {
	    USERERROR("You do not have permission to modify user information ".
		      "for other users", 1);
	}
    }
}

#
# Now sanity check the form to make sure all the required fields
# were provided. I do this on a per field basis so that we can be
# informative. Be sure to correlate these checks with any changes made to
# the project form. Note that this sequence of  statements results in
# only the last bad field being displayed, but thats okay. The user will
# eventually figure out that fields marked with * mean something!
#
if (!isset($usr_name) ||
    strcmp($usr_name, "") == 0) {
  FORMERROR("Full Name");
}
if (!isset($usr_email) ||
    strcmp($usr_email, "") == 0) {
  FORMERROR("Email Address");
}
if (!isset($usr_addr) ||
    strcmp($usr_addr, "") == 0) {
  FORMERROR("Mailing Address");
}
if (!isset($usr_phone) ||
    strcmp($usr_phone, "") == 0) {
  FORMERROR("Phone #");
}
if (!isset($usr_title) ||
    strcmp($usr_title, "") == 0) {
  FORMERROR("Title/Position");
}
if (!isset($usr_affil) ||
    strcmp($usr_affil, "") == 0) {
  FORMERROR("Institutional Affiliation");
}

#
# Check that email address looks reasonable. Only admin types can change
# the email address.
#
TBUserInfo($target_uid, $dbusr_name, $dbusr_email);

if (strcmp($usr_email, $dbusr_email)) {
    if (!$isadmin) {
	USERERROR("Please contact Testbed Admin to change your email address!",
		  1);
    }
    $email_domain = strstr($usr_email, "@");
    if (! $email_domain ||
	strcmp($usr_email, $email_domain) == 0 ||
	strlen($email_domain) <= 1 ||
	! strstr($email_domain, ".")) {
	USERERROR("The email address `$usr_email' looks invalid! Please ".
		  "go back and fix it up", 1);
    }
    DBQueryFatal("update users set usr_email='$usr_email' ".
		 "where uid='$target_uid'");
}

#
# Check URLs. 
#
if (strcmp($usr_url, $HTTPTAG) == 0) {
    $usr_url = "";
}
VERIFYURL($usr_url);

#
# Now see if the user is requesting to change the password. We do the usual
# checks to make sure the two fields agree and that it passes our tests for
# safe passwords.
#
if ((isset($new_password1) && strcmp($new_password1, "")) ||
    (isset($new_password2) && strcmp($new_password2, ""))) {
    if (strcmp($new_password1, $new_password2)) {
	USERERROR("You typed different passwords in each of the two password ".
		  "entry fields. <br> Please go back and correct them.", 1);
    }

    $mypipe = popen(escapeshellcmd(
    "$TBCHKPASS_PATH $new_password1 $target_uid '$usr_name:$usr_email'"),
    "w+");
    if ($mypipe) { 
        $retval=fgets($mypipe, 1024);
        if (strcmp($retval,"ok\n") != 0) {
	    USERERROR("The password you have chosen will not work: ".
		      "<br><br>$retval<br>", 1);
        } 
    }
    else {
	TBERROR("checkpass failure\n".
               "$usr_name ($target_uid) just tried change his password\n".
               "but checkpass pipe did not open (returned '$mypipe').", 1);
    }

    #
    # Password is good. Insert into database.
    #
    $encoding = crypt("$new_password1");
    if ($uid != $target_uid)
	$expires = "now()";
    else
	$expires = "date_add(now(), interval 1 year)";
    
    $insert_result =
	DBQueryFatal("UPDATE users SET usr_pswd='$encoding', ".
		     "pswd_expires=$expires ".
		     "WHERE uid='$target_uid'");

    $changed_password = "Yes";
}

#
# Okay, draw the header now that the password has been changed.
# This is a convenience to avoid asking the user to refresh after
# an expired password.
#
PAGEHEADER("Modify User Information");

#
# Add slashes. These should really be ereg checks, kicking back special
# chars since they are more trouble then they are worth.
# 
$usr_name  = addslashes($usr_name);
$usr_addr  = addslashes($usr_addr);
$usr_title = addslashes($usr_title);
$usr_affil = addslashes($usr_affil);
$usr_phone = addslashes($usr_phone);

#
# Now change the rest of the information.
#
$insert_result =
    DBQueryFatal("UPDATE users SET ".
		 "usr_name=\"$usr_name\",       ".
		 "usr_URL=\"$usr_url\",         ".
		 "usr_addr=\"$usr_addr\",       ".
		 "usr_phone=\"$usr_phone\",     ".
		 "usr_title=\"$usr_title\",     ".
		 "usr_affil=\"$usr_affil\"      ".
		 "WHERE uid=\"$target_uid\"");

#
# Audit
#
TBUserInfo($uid, $uid_name, $uid_email);

TBMAIL("$usr_name <$usr_email>",
     "User Information for '$target_uid' Modified",
     "\n".
     "User information for '$target_uid' changed by '$uid'.\n".
     "\n".
     "Name:              $usr_name\n".
     "Email:             $usr_email\n".
     "URL:               $usr_url\n".
     "Affiliation:       $usr_affil\n".
     "Address:           $usr_addr\n".
     "Phone:             $usr_phone\n".
     "Title:             $usr_title\n".
     "Password Changed?: $changed_password\n".
     "\n\n".
     "Thanks,\n".
     "Testbed Ops\n".
     "Utah Network Testbed\n",
     "From: $uid_name <$uid_email>\n".
     "Cc: $TBMAIL_AUDIT\n".
     "Errors-To: $TBMAIL_WWW");

#
# Create the user accounts. Must be done *before* we create the
# project directory!
# 
SUEXEC($uid, "flux", "webmkacct $target_uid", 0);

echo "<center>
      <br>
      <h3>User information successfully modified!</h3><p>
      </center>\n";

SHOWUSER($target_uid);

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
