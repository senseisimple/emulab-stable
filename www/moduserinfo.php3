<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

$changed_password = "No";

#
# No PAGEHEADER since we spit out a Location header later. See below.
# 

#
# Only known and logged in users can modify info.
#
# Note different test though, since we want to allow logged in
# users with expired passwords to change them.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid, CHECKLOGIN_USERSTATUS|CHECKLOGIN_PSWDEXPIRED);
$isadmin = ISADMIN($uid);

#
# Spit the form out using the array of data and error strings (if any).
# 
function SPITFORM($formfields, $errors)
{
    global $TBDB_UIDLEN, $TBDB_PIDLEN, $TBDB_GIDLEN, $isadmin;
    global $target_uid;
    
    #
    # Standard Testbed Header. Written late cause of password
    # expiration interaction. See below.
    #
    PAGEHEADER("Modify User Information");

    if ($errors) {
	echo "<table align=center border=0 cellpadding=0 cellspacing=2>
              <tr>
                 <td align=center colspan=3>
                   <font size=+1 color=red>
                      Oops, please fix the following errors!
                   </font>
                 </td>
              </tr>\n";

	while (list ($name, $message) = each ($errors)) {
	    echo "<tr>
                     <td align=right><font color=red>$name:</font></td>
                     <td>&nbsp &nbsp</td>
                     <td align=left><font color=red>$message</font></td>
                  </tr>\n";
	}
	echo "</table><br>\n";
    }

    echo "<table align=center border=1> 
          <tr>
            <td align=center colspan=3>
                Fields marked with * are required;
                those marked + are highly recommended.
            </td>
          </tr>\n

          <form action=moduserinfo.php3 method=post>\n";

        #
        # UserName. This is a constant field. 
        #
        echo "<tr>
                  <td colspan=2>Username:</td>
                  <td class=left>
                      $formfields[target_uid]
                      <input type=hidden
                             name=\"formfields[target_uid]\"
                             value=\"" . $formfields[target_uid] . "\"
                             size=$TBDB_UIDLEN
   	                     maxlength=$TBDB_UIDLEN>
              </td>
             </tr>\n";

	#
	# Full Name
	#
        echo "<tr>
                  <td colspan=2>*Full Name:</td>
                  <td class=left>
                      <input type=text
                             name=\"formfields[usr_name]\"
                             value=\"" . $formfields[usr_name] . "\"
	                     size=30>
                  </td>
              </tr>\n";

        #
	# Title/Position:
	# 
	echo "<tr>
                  <td colspan=2>*Title/Position:</td>
                  <td class=left>
                      <input type=text
                             name=\"formfields[usr_title]\"
                             value=\"" . $formfields[usr_title] . "\"
	                     size=30>
                  </td>
              </tr>\n";

        #
	# Affiliation:
	# 
	echo "<tr>
                  <td colspan=2>*Institutional<br>Affiliation:</td>
                  <td class=left>
                      <input type=text
                             name=\"formfields[usr_affil]\"
                             value=\"" . $formfields[usr_affil] . "\"
	                     size=40>
                  </td>
              </tr>\n";

	#
	# User URL
	#
	echo "<tr>
                  <td colspan=2>Home Page URL:</td>
                  <td class=left>
                      <input type=text
                             name=\"formfields[usr_URL]\"
                             value=\"" . $formfields[usr_URL] . "\"
	                     size=45>
                  </td>
              </tr>\n";

	#
	# Email:
	#
	echo "<tr>
                  <td colspan=2>*Email Address[<b>1</b>]:</td>
                  <td class=left> ";
	if ($isadmin)
	    echo "    <input type=text ";
	else
	    echo "    $formfields[usr_email]
                      <input type=hidden ";

	echo "               name=\"formfields[usr_email]\"
                             value=\"" . $formfields[usr_email] . "\"
	                     size=30>";
        echo "    </td>
              </tr>\n";

	#
	# Postal Address
	#
	echo "<tr>
                  <td colspan=2>*Postal Address:</td>
                  <td class=left>
                      <input type=text
                             name=\"formfields[usr_addr]\"
                             value=\"" . $formfields[usr_addr] . "\"
	                     size=40>
                  </td>
              </tr>\n";

	#
	# Phone
	#
	echo "<tr>
                  <td colspan=2>*Phone #:</td>
                  <td class=left>
                      <input type=text
                             name=\"formfields[usr_phone]\"
                             value=\"" . $formfields[usr_phone] . "\"
	                     size=15>
                  </td>
              </tr>\n";

	#
	# Password. Note that we do not resend the password. User
	# must retype on error.
	#
	echo "<tr>
                  <td colspan=2>Password[<b>1</b>]:</td>
                  <td class=left>
                      <input type=password
                             name=\"formfields[password1]\"
                             size=8></td>
              </tr>\n";

        echo "<tr>
                  <td colspan=2>Retype Password:</td>
                  <td class=left>
                      <input type=password
                             name=\"formfields[password2]\"
                             size=8></td>
             </tr>\n";

    echo "<tr>
              <td colspan=3 align=center>
                 <b><input type=submit name=submit value=Submit></b>
              </td>
          </tr>\n";

    echo "</form>
          </table>\n";

    echo "<h4><blockquote><blockquote>
          <ol>
            <li> Please consult our
                 <a href = 'docwrapper.php3?docname=security.html'>
                 security policies</a> for information
                 regarding passwords and email addresses.
            <li> You can also
                 <a href='showpubkeys.php3?target_uid=$target_uid'>
                 edit your ssh public keys.</a>
          </ol>
          </blockquote></blockquote>
          </h4>\n";
}

#
# The target uid and the current uid will generally be the same, unless
# its an admin user modifying someone elses data. Must verify this case.
#
if (! isset($submit)) {
    if (! isset($target_uid))
	$target_uid = $uid;
}
elseif (! isset($formfields[target_uid]) ||
	strcmp($formfields[target_uid], "") == 0) {
    USERERROR("You must supply a target user id!", 1);
}
else {
    $target_uid = $formfields[target_uid];
}

#
# Admin types can change anyone. 
#
if (! $isadmin &&
    $uid != $target_uid &&
    ! TBUserInfoAccessCheck($uid, $target_uid, $TB_USERINFO_MODIFYINFO)) {
    USERERROR("You do not have permission to modify information for ".
	      "user: $target_uid!", 1);
}

#
# The conclusion of a modify operation. See below.
# 
if (isset($finished)) {
    PAGEHEADER("Modify User Information");
    
    echo "<center>
          <br>
          <h3>User information successfully modified!</h3><p>
          </center>\n";
    
    SHOWUSER($target_uid);
    PAGEFOOTER();
    return;
}

#
# Suck the current info out of the database.
#
$query_result =
    DBQueryFatal("select * from users where uid='$target_uid'");

if (mysql_num_rows($query_result) == 0) {
    USERERROR("No such user: $target_uid!", 1);
}

#
# On first load, display a form consisting of current user values, and exit.
#
if (! isset($submit)) {
    $defaults = array();
    
    #
    # Construct a defaults array based on current DB info.
    # 
    $row = mysql_fetch_array($query_result);
    $defaults[target_uid]  = $target_uid;
    $defaults[usr_email]   = $row[usr_email];
    $defaults[usr_URL]     = $row[usr_URL];
    $defaults[usr_addr]    = $row[usr_addr];
    $defaults[usr_name]    = $row[usr_name];
    $defaults[usr_phone]   = $row[usr_phone];
    $defaults[usr_title]   = $row[usr_title];
    $defaults[usr_affil]   = $row[usr_affil];
    
    SPITFORM($defaults, 0);
    PAGEFOOTER();
    return;
}

#
# Otherwise, must validate and redisplay if errors
#
$errors = array();

#
# These fields are required!
#
if (!isset($formfields[usr_title]) ||
    strcmp($formfields[usr_title], "") == 0) {
    $errors["Title/Position"] = "Missing Field";
}
if (!isset($formfields[usr_name]) ||
    strcmp($formfields[usr_name], "") == 0) {
    $errors["Full Name"] = "Missing Field";
}
if (!isset($formfields[usr_affil]) ||
    strcmp($formfields[usr_affil], "") == 0) {
    $errors["Affiliation"] = "Missing Field";
}
if (!isset($formfields[usr_email]) ||
    strcmp($formfields[usr_email], "") == 0) {
    $errors["Email Address"] = "Missing Field";
}
else {
    $usr_email    = $formfields[usr_email];
    $email_domain = strstr($usr_email, "@");
    
    if (! $email_domain ||
	strcmp($usr_email, $email_domain) == 0 ||
	strlen($email_domain) <= 1 ||
	! strstr($email_domain, ".")) {
	$errors["Email Address"] = "Looks invalid!";
    }
}
if (isset($formfields[usr_URL]) &&
    strcmp($formfields[usr_URL], "") &&
    strcmp($formfields[usr_URL], $HTTPTAG) &&
    ! CHECKURL($formfields[usr_URL], $urlerror)) {
    $errors["Home Page URL"] = $urlerror;
}
if (!isset($formfields[usr_addr]) ||
    strcmp($formfields[usr_addr], "") == 0) {
    $errors["Postal Address"] = "Missing Field";
}
if (!isset($formfields[usr_phone]) ||
    strcmp($formfields[usr_phone], "") == 0) {
    $errors["Phone #"] = "Missing Field";
}
elseif (! ereg("^[\(]*[0-9][-\(\) 0-9ext]+$", $formfields[usr_phone])) {
    $errors["Phone"] = "Invalid characters";
}
if (isset($formfields[password1]) &&
    strcmp($formfields[password1], "")) {
    if (!isset($formfields[password2]) ||
	strcmp($formfields[password2], "") == 0) {
	$errors["Confirm Password"] = "Missing Field";
    }
    elseif (strcmp($formfields[password1], $formfields[password2])) {
	$errors["Confirm Password"] = "Does not match Password";
    }
    elseif (! CHECKPASSWORD($formfields[target_uid],
			    $formfields[password1],
			    $formfields[usr_name],
			    $formfields[usr_email], $checkerror)) {
	$errors["Password"] = "$checkerror";
    }
}
if (count($errors)) {
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    return;
}

$usr_title    = addslashes($formfields[usr_title]);
$usr_name     = addslashes($formfields[usr_name]);
$usr_affil    = addslashes($formfields[usr_affil]);
$usr_email    = $formfields[usr_email];
$usr_addr     = addslashes($formfields[usr_addr]);
$usr_phone    = $formfields[usr_phone];
$password1    = $formfields[password1];
$password2    = $formfields[password2];

if (! isset($formfields[usr_URL]) ||
    strcmp($formfields[usr_URL], "") == 0 ||
    strcmp($formfields[usr_URL], $HTTPTAG) == 0) {
    $usr_URL = "";
}
else {
    $usr_URL = $formfields[usr_URL];
}

#
# Check that email address looks reasonable. Only admin types can change
# the email address. If its different, the user circumvented the form,
# and so its okay to blast it.
#
TBUserInfo($target_uid, $dbusr_name, $dbusr_email);

if (strcmp($usr_email, $dbusr_email)) {
    if (!$isadmin) {
	USERERROR("You are not allowed to change your password. <br> ".
		  "Please contact Testbed Operations.", 1);
    }
    DBQueryFatal("update users set usr_email='$usr_email' ".
		 "where uid='$target_uid'");
}

#
# Now see if the user is requesting to change the password. We checked
# them above when the form was submitted.
#
if ((isset($password1) && strcmp($password1, "")) &&
    (isset($password2) && strcmp($password2, ""))) {

    $query_result =
	DBQueryFatal("select usr_pswd from users WHERE uid='$target_uid'");
    if (! mysql_num_rows($query_result)) {
	TBERROR("Error getting usr_pswd for $target_uid", 1);
    }
    $row = mysql_fetch_array($query_result);
    $old_encoding = $row[usr_pswd];
    $new_encoding = crypt("$password1", $old_encoding);

    #
    # Compare. Must change it!
    # 
    if (! strcmp($old_encoding, $new_encoding)) {
	$errors["New Password"] = "New password is the same as old password";

	SPITFORM($formfields, $errors);
	PAGEFOOTER();
	return;
    }

    #
    # Insert into database. When changing password for someone else,
    # always set the expiration to right now so that the target user
    # is "forced" to change it. 
    #
    if ($uid != $target_uid)
	$expires = "now()";
    else
	$expires = "date_add(now(), interval 1 year)";
    
    $insert_result =
	DBQueryFatal("UPDATE users SET usr_pswd='$new_encoding', ".
		     "pswd_expires=$expires ".
		     "WHERE uid='$target_uid'");

    $changed_password = "Yes";
}

#
# Now change the rest of the information.
#
DBQueryFatal("UPDATE users SET ".
	     "usr_name=\"$usr_name\",       ".
	     "usr_URL=\"$usr_url\",         ".
	     "usr_addr=\"$usr_addr\",       ".
	     "usr_phone=\"$usr_phone\",     ".
	     "usr_title=\"$usr_title\",     ".
	     "usr_affil=\"$usr_affil\",     ".
	     "usr_modified=now()            ".
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
# mkacct updates the user gecos and password.
# 
SUEXEC($uid, "flux", "webmkacct -a $target_uid", 0);

#
# Spit out a redirect so that the history does not include a post
# in it. The back button skips over the post and to the form.
# See above for conclusion.
# 
header("Location: moduserinfo.php3?target_uid=$target_uid&finished=1");

?>
