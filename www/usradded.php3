<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Join a Project");

#
# First off, sanity check the form to make sure all the required fields
# were provided. I do this on a per field basis so that we can be
# informative. Be sure to correlate these checks with any changes made to
# the project form. 
#
if (!isset($joining_uid) ||
    strcmp($joining_uid, "") == 0) {
  FORMERROR("UserName");
}
if (!isset($usr_email) ||
    strcmp($usr_email, "") == 0) {
  FORMERROR("Email Address");
}
if (!isset($usr_name) ||
    strcmp($usr_name, "") == 0) {
  FORMERROR("Full Name");
} else if (! ereg("^[a-zA-Z0-9 .\-]+$", $usr_name)) {
    USERERROR("Your Full Name can only contain alphanumeric characters, ".
	      "'-', and '.'", 1);
}

if (!isset($pid) ||
    strcmp($pid, "") == 0) {
  FORMERROR("Project");
}
if (!isset($usr_affil) ||
    strcmp($usr_affil, "") == 0) {
  FORMERROR("Institutional Afilliation");
}
if (!isset($usr_title) ||
    strcmp($usr_title, "") == 0) {
  FORMERROR("Title/Position");
}

#
# Check joining_uid for sillyness.
#
if (! ereg("^[a-z][a-z0-9]+$", $joining_uid)) {
    USERERROR("Your username ($joining_uid) must be composed of ".
	      "lowercase alphanumeric characters only, and must begin ".
	      "with a lowercase alpha character!", 1);
}

#
# Database limits
#
if (strlen($joining_uid) > $TBDB_UIDLEN) {
    USERERROR("The name \"$joining_uid\" is too long! ".
              "Please select one that is shorter than $TBDB_UIDLEN.", 1);
}

#
# Check that email address looks reasonable. We need the domain for
# below anyway.
#
$email_domain = strstr($usr_email, "@");
if (! $email_domain ||
    strcmp($usr_email, $email_domain) == 0 ||
    strlen($email_domain) <= 1 ||
    ! strstr($email_domain, ".")) {
    USERERROR("The email address `$usr_email' looks invalid!. Please ".
	      "go back and fix it up", 1);
}
$email_domain = substr($email_domain, 1);
$email_user   = substr($usr_email, 0, strpos($usr_email, "@", 0));

#
# Check URLs. 
#
if (strcmp($usr_url, $HTTPTAG) == 0) {
    $usr_url = "";
}
VERIFYURL($usr_url);

#
# Certain of these values must be escaped or otherwise sanitized.
#
$usr_name  = addslashes($usr_name);
$usr_affil = addslashes($usr_affil);
$usr_title = addslashes($usr_title);
$usr_addr  = addslashes($usr_addr);

#
# See if this is a new user or one returning.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT usr_pswd FROM users WHERE uid=\"$joining_uid\"");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error retrieving info for $joining_uid: $err\n", 1);
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
    if (CHECKLOGIN($joining_uid) != 1) {
        USERERROR("The Username '$joining_uid' is in use. ".
		  "If you already have an Emulab account, please go back ".
		  "and login before trying to join a new project.<br><br>".
		  "If you are a <em>new</em> Emulab user trying to join ".
                  "your first project, please go back and select a different ".
		  "Username.", 1);
    }
}
else {
    #
    # Check new username against CS logins so that external people do
    # not pick names that overlap with CS names.
    #
    if (! strstr($email_domain, "cs.utah.edu")) {
	$dbm = dbmopen($TBCSLOGINS, "r");
	if (! $dbm) {
	    TBERROR("Could not dbmopen $TBCSLOGINS from usradded.php3\n", 1);
	}
	if (dbmexists($dbm, $joining_uid)) {
	    dbmclose($dbm);
	    USERERROR("The username '$joining_uid' is already in use. ".
		      "Please go back and choose another.", 1);
	}
	dbmclose($dbm);
    }
    
    if (strcmp($password1, $password2)) {
        USERERROR("You typed different passwords in each of the two password ".
                  "entry fields. <br> Please go back and correct them.",
                  1);
    }
    $mypipe = popen(escapeshellcmd(
    "$TBCHKPASS_PATH $password1 $joining_uid '$usr_name:$usr_email'"),
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
                "\n$usr_name ($joining_uid) just tried to set up a testbed ".
                "account,\n".
                "but checkpass pipe did not open (returned '$mypipe').", 1);
    }
}

#
# If no group name provided, then use the "default group." 
#
if (!isset($gid) ||
    strcmp($gid, "") == 0) {
    $gid = $pid;
}

#
# Lets verify the project/group and quit early if its bogus.
# We could let things continue, resulting in a valid account but no
# membership, but I don't like that.
#
if (! TBValidGroup($pid, $gid)) {
    USERERROR("No such project or group $pid/$gid. ".
              "Please go back and try again.", 1);
}

#
# Don't try to join twice!
#
if (TBGroupMember($joining_uid, $pid, $gid, $approved)) {
    USERERROR("You have already applied for membership in $pid/$gid!", 1);
}

#
# For a new user:
# * Create a new account in the database.
# * Add user email to the list of email address.
# * Generate a mail message to the user with the verification key.
#
if (! $returning) {
    $encoding = crypt("$password1");

    $newuser_command = "INSERT INTO users ".
	"(uid,usr_created,usr_expires,usr_name,usr_email,usr_addr,".
	"usr_URL,usr_phone,usr_title,usr_affil,usr_pswd,unix_uid,status) ".
	"VALUES ('$joining_uid', now(), '$usr_expires', '$usr_name', ".
        "'$usr_email', ".
	"'$usr_addr', '$usr_url', '$usr_phone', '$usr_title', '$usr_affil',".
        "'$encoding', NULL, 'newuser')";
    $newuser_result  = mysql_db_query($TBDBNAME, $newuser_command);
    if (! $newuser_result) {
        $err = mysql_error();
        TBERROR("Database Error adding adding new user $joining_uid: ".
                "$err\n", 1);
    }

    $key = GENKEY($joining_uid);

    mail("$usr_name '$joining_uid' <$usr_email>", "TESTBED: Your New User Key",
	 "\n".
         "Dear $usr_name ($joining_uid):\n\n".
         "\tHere is your key to verify your account on the ".
         "Utah Network Testbed:\n\n".
         "\t\t$key\n\n".
         "Please return to $TBWWW and log in using\n".
	 "the user name and password you gave us when you applied. You will\n".
	 "then find an option on the menu called 'New User Verification'.\n".
	 "Select that option, and on that page enter your key.\n".
	 "You will then be verified as a user. When you have been both\n".
         "verified and approved by the head of the project, you will\n".
	 "be marked as an active user, and will be granted full access to\n".
  	 "your user account.\n\n".
         "Thanks,\n".
         "Testbed Ops\n".
         "Utah Network Testbed\n",
         "From: $TBMAIL_APPROVAL\n".
         "Bcc: $TBMAIL_APPROVAL\n".
         "Errors-To: $TBMAIL_WWW");

    #
    # Generate some warm fuzzies.
    #
    echo "<center><h1>Adding new Testbed User!</h1></center>";

    echo "<p>As a new user of the Testbed, for
          security purposes, you will receive by e-mail a key. When you
          receive it, come back to the site, and log in. When you do, you
          will see a new menu option called 'New User Verification'. On
          that page, enter in your key
          exactly as you received it in your e-mail. You will then be
          marked as a verified user.
          <p>Once you have been both verified
          and approved, you will be classified as an active user, and will 
          be granted full access to your user account.";
}

#
# Add to the group, but with trust=none. The project/group leader will have
# to upgrade the trust level, making the new user real.
#
$query_result =
    DBQueryFatal("insert into group_membership ".
		 "(uid,gid,pid,trust,date_applied) ".
		 "values ('$joining_uid','$gid','$pid','none', now())");

#
# This could be a new user or an old user trying to join a specific group
# in a project. If the user is new to the project too, then must insert
# a group_membership in the default group for the project. 
#
if (! TBGroupMember($joining_uid, $pid, $pid, $approved)) {
    DBQueryFatal("insert into group_membership ".
		 "(uid,gid,pid,trust,date_applied) ".
		 "values ('$joining_uid','$pid','$pid','none', now())");
}

#
# Generate an email message to the group leader.
#
$query_result =
    DBQueryFatal("select usr_name,usr_email,leader from users as u ".
		 "left join groups as g on u.uid=g.leader ".
		 "where g.pid='$pid' and g.gid='$gid'");
if (($row = mysql_fetch_row($query_result)) == 0) {
    TBERROR("DB Error getting email address for group leader $leader!", 1);
}
$leader_name = $row[0];
$leader_email = $row[1];
$leader_uid = $row[2];

mail("$leader_name '$leader_uid' <$leader_email>",
     "TESTBED: $joining_uid $pid Project Join Request",
     "\n$usr_name ($joining_uid) is trying to join your group $gid\n".
     "in project $pid\n".
     "$usr_name has the\n".
     "Testbed username $joining_uid and email address $usr_email.\n".
     "$usr_name's phone number is $usr_phone and address $usr_addr.\n\n".
     "Please return to $TBWWW\n".
     "log in, and select the 'New User Approval' page to enter your\n".
     "decision regarding $usr_name's membership in your project\n\n".
     "Thanks,\n".
     "Testbed Ops\n".
     "Utah Network Testbed\n",
     "From: $TBMAIL_APPROVAL\n".
     "Bcc: $TBMAIL_APPROVAL\n".
     "Errors-To: $TBMAIL_WWW");

#
# Generate some warm fuzzies.
#
echo "<br>
      <p>The leader of project '$pid' has been notified of your application.
      He/She will make a decision and either approve or deny your application,
      and you will be notified as soon as a decision has been made.";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
