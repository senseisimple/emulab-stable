<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Start a New Project");

#
# First off, sanity check the form to make sure all the required fields
# were provided. I do this on a per field basis so that we can be
# informative. Be sure to correlate these checks with any changes made to
# the project form. 
#
if (!isset($pid) ||
    strcmp($pid, "") == 0) {
  FORMERROR("Name");
}
if (!isset($proj_head_uid) ||
    strcmp($proj_head_uid, "") == 0) {
  FORMERROR("Username");
}
if (!isset($proj_name) ||
    strcmp($proj_name, "") == 0) {
  FORMERROR("Long Name");
}
if (!isset($proj_members) ||
    strcmp($proj_members, "") == 0) {
  FORMERROR("Estimated #of Project Members");
}
if (!isset($proj_pcs) ||
    strcmp($proj_pcs, "") == 0) {
  FORMERROR("Estimated #of PCs");
}
if (!isset($proj_sharks) ||
    strcmp($proj_sharks, "") == 0) {
  FORMERROR("Estimated #of Sharks");
}
if (!isset($proj_why) ||
    strcmp($proj_why, "") == 0) {
  FORMERROR("Please describe your project");
}
if (!isset($usr_name) ||
    strcmp($usr_name, "") == 0) {
  FORMERROR("Full Name");
}
if (!isset($proj_URL) ||
    strcmp($proj_URL, "") == 0) {
  FORMERROR("Project URL");
}
if (!isset($usr_email) ||
    strcmp($usr_email, "") == 0) {
  FORMERROR("Email Address");
}
if (!isset($usr_addr) ||
    strcmp($usr_addr, "") == 0) {
  FORMERROR("Postal Address");
}
if (!isset($usr_affil) ||
    strcmp($usr_affil, "") == 0) {
  FORMERROR("Institutional Afilliation");
}
if (!isset($usr_title) ||
    strcmp($usr_title, "") == 0) {
  FORMERROR("Title/Position");
}
if (!isset($usr_phones) ||
    strcmp($usr_phones, "") == 0) {
  FORMERROR("Phone #");
}

#
# Check database length limits.
#
if (strlen($pid) > $TBDB_PIDLEN) {
    USERERROR("The project name \"$pid\" is too long! ".
              "Please select another.", 1);
}
if (strlen($proj_head_uid) > $TBDB_UIDLEN) {
    USERERROR("The name \"$proj_head_uid\" is too long! ".
              "Please select another.", 1);
}

#
# Certain of these values must be escaped or otherwise sanitized.
# 
$proj_why  = addslashes($proj_why);
$proj_name = addslashes($proj_name);
$usr_affil = addslashes($usr_affil);
$usr_title = addslashes($usr_title);
$usr_addr  = addslashes($usr_addr);

#
# This is a new project request. Make sure it does not already exist.
#
$project_query  = "SELECT pid FROM projects WHERE pid=\"$pid\"";
$project_result = mysql_db_query($TBDBNAME, $project_query);

if ($row = mysql_fetch_row($project_result)) {
  USERERROR("The project name \"$pid\" you have chosen is already in use. ".
            "Please select another.", 1);
}

#
# See if this is a new user or one returning.
#
$pswd_query  = "SELECT usr_pswd FROM users WHERE uid=\"$proj_head_uid\"";
$pswd_result = mysql_db_query($TBDBNAME, $pswd_query);
if (!$pswd_result) {
    $err = mysql_error();
    TBERROR("Database Error retrieving info for $proj_head_uid: $err\n", 1);
}
if ($row = mysql_fetch_row($pswd_result)) {
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
    if (CHECKLOGIN($proj_head_uid) != 1) {
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
    "/usr/testbed/bin/checkpass $password1 $proj_head_uid '$usr_name:$usr_email'"),
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
                "\n$usr_name ($proj_head_uid) just tried to set up a testbed ".
                "account,\n".
                "but checkpass pipe did not open (returned '$mypipe').", 1);
    }
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
        "usr_URL,usr_title,usr_affil,usr_phone,usr_pswd,unix_uid,status) ".
        "VALUES ('$proj_head_uid', now(), '$proj_expires', '$usr_name', ".
        "'$usr_email', '$usr_addr', '$usr_url', '$usr_title', '$usr_affil', ".
        "'$usr_phones', '$encoding', ".
        "'$unix_uid', 'newuser')";
    $newuser_result  = mysql_db_query($TBDBNAME, $newuser_command);
    if (! $newuser_result) {
        $err = mysql_error();
        TBERROR("Database Error adding adding new user $proj_head_uid: $err\n",
                1);
    }
    $key = GENKEY($proj_head_uid);

    mail("$usr_email", "TESTBED: Your New User Key",
	 "\n".
         "Dear $usr_name:\n\n".
         "    Here is your key to verify your account on the ".
         "Utah Network Testbed:\n\n".
         "\t\t$key\n\n".
         "Please return to $TBWWW and log in using\n".
	 "the user name and password you gave us when you applied. You will\n".
	 "then find an option on the menu called 'New User Verification'.\n".
	 "Select that option, and on that page enter your key.\n".
	 "You will then be verified as a user. When you have been both\n".
         "verified and approved by Testbed Operations, you will\n".
	 "be marked as an active user, and will be granted full access to\n".
  	 "your user account.\n\n".
         "Thanks,\n".
         "Testbed Ops\n".
         "Utah Network Testbed\n",
         "From: $TBMAIL_CONTROL\n".
         "Cc: $TBMAIL_CONTROL\n".
         "Errors-To: $TBMAIL_WWW");
}

#
# Now for the new Project
# * Create a new project in the database.
# * Create a new project_membership entry in the database, default trust=none.
# * Generate a mail message to testbed ops.
#
$newproj_command = "INSERT INTO projects ".
     "(pid, created, expires, name, URL, head_uid, ".
     " num_members, num_pcs, num_sharks, why, unix_gid)".
     "VALUES ('$pid', now(), '$proj_expires','$proj_name','$proj_URL',".
     "'$proj_head_uid', '$proj_members', '$proj_pcs', '$proj_sharks', ".
     "'$proj_why', NULL)";
$newproj_result  = mysql_db_query($TBDBNAME, $newproj_command);
if (! $newproj_result) {
    $err = mysql_error();
    TBERROR("Database Error adding new project $pid: $err\n", 1);
}

$newmemb_result = mysql_db_query($TBDBNAME,
			"insert into proj_memb (uid,pid,trust)".
			"values ('$proj_head_uid','$pid','none');");
if (! $newmemb_result) {
    $err = mysql_error();
    TBERROR("Database Error adding new project membership: $pid: $err\n", 1);
}

#
# Grab the unix GID that was assigned.
# 
$unixgid_result = mysql_db_query($TBDBNAME,
	"SELECT unix_gid FROM projects where pid='$pid'");
$row = mysql_fetch_row($unixgid_result);
$unix_gid = $row[0];

#
# The mail message to the approval list.
# 
mail($TBMAIL_APPROVAL,
     "TESTBED: New Project", "'$usr_name' wants to start project '$pid'.\n".
     "Contact Info:\n".
     "Name:          $usr_name ($proj_head_uid)\n".
     "Email:         $usr_email\n".
     "User URL:      $usr_url\n".
     "Project:       $proj_name\n".
     "Expires:	     $proj_expires\n".
     "Project URL:   $proj_URL\n".
     "Title:         $usr_title\n".
     "Affiliation:   $usr_affil\n".
     "Address:       $usr_addr\n".
     "Phone:         $usr_phones\n".
     "Members:       $proj_members\n".
     "PCs:           $proj_pcs\n".
     "Sharks:        $proj_sharks\n".
     "Unix GID:      $unix_gid\n".
     "Reasons:\n$proj_why\n\n".
     "Please review the application and when you have\n".
     "made a decision, go to $TBWWW and\n".
     "select the 'Project Approval' page.\n\nThey are expecting a result ".
     "within 72 hours.\n", 
     "From: $usr_name <$usr_email>\n".
     "Cc: $TBMAIL_CONTROL\n".
     "Errors-To: $TBMAIL_WWW");

#
# Now give the user some warm fuzzies
#
echo "<center><h1>Project '$pid' successfully queued.</h1></center>
      Testbed Operations has been notified of your application.
      Most applications are reviewed within one week. We will notify
      you by e-mail at '$usr_name&nbsp;&lt;$usr_email>' of their decision
      regarding your proposed project '$pid'.\n";

if (! $returning) {
    echo "<p>In the meantime, for
          security purposes, you will receive by e-mail a key. When you
          receive it, come back to the site, and log in. When you do, you
          will see a new menu option called 'New User Verification'. On
          that page, enter in your key,
          exactly as you received it in your e-mail. You will then be
          marked as a verified user.
          <p>Once you have been both verified
          and approved, you will be classified as an active user, and will 
          be granted full access to your user account.";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
