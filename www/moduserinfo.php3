<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

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
LOGGEDINORDIE($uid,
	      CHECKLOGIN_USERSTATUS|CHECKLOGIN_PSWDEXPIRED|CHECKLOGIN_WEBONLY);
$isadmin = ISADMIN($uid);


$shelllist = array( 'tcsh', 'bash', 'csh', 'sh' );
$defaultshell = 'tcsh';

#
# Spit the form out using the array of data and error strings (if any).
# 
function SPITFORM($formfields, $errors)
{
    global $TBDB_UIDLEN, $TBDB_PIDLEN, $TBDB_GIDLEN, $isadmin;
    global $target_uid;
    global $shelllist, $defaultshell;
    
    #
    # Standard Testbed Header. Written late cause of password
    # expiration interaction. See below.
    #
    PAGEHEADER("Modify User Information");

    if ($errors) {
	echo "<table class=nogrid
                     align=center border=0 cellpadding=6 cellspacing=0>
              <tr>
                 <th align=center colspan=2>
                   <font size=+1 color=red>
                      &nbsp;Oops, please fix the following errors!&nbsp;
                   </font>
                 </td>
              </tr>\n";

	while (list ($name, $message) = each ($errors)) {
	    echo "<tr>
                     <td align=right>
                       <font color=red>$name:&nbsp;</font></td>
                     <td align=left>
                       <font color=red>$message</font></td>
                  </tr>\n";
	}
	echo "</table><br>\n";
    }

    echo "<table align=center border=1> 
          <tr>
            <td align=center colspan=3>
                Fields marked with * are required.
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
        # Country needs a default for older users.
        #
	if (! strcmp($formfields[usr_country], "")) {
	    $formfields[usr_country] = "USA";
	}

	echo "<tr><td colspan=3>*Address:<br /><center>
		<table>
		  <tr><td>Line 1</td><td colspan=3>
                    <input type=text
                           name=\"formfields[usr_addr]\"
                           value=\"" . $formfields[usr_addr] . "\"
	                   size=45></td></tr>
		  <tr><td>Line 2</td><td colspan=3>
                    <input type=text
                           name=\"formfields[usr_addr2]\"
                           value=\"" . $formfields[usr_addr2] . "\"
	                   size=45></td></tr>
		  <tr><td>City</td><td>
                    <input type=text
                           name=\"formfields[usr_city]\"
                           value=\"" . $formfields[usr_city] . "\"
	                   size=25></td>
		      <td>State/Province</td><td>
                    <input type=text
                           name=\"formfields[usr_state]\"
                           value=\"" . $formfields[usr_state] . "\"
	                   size=2></td></tr>
		  <tr><td>ZIP/Postal Code</td><td>
                    <input type=text
                           name=\"formfields[usr_zip]\"
                           value=\"" . $formfields[usr_zip] . "\"
	                   size=10></td>
		      <td>Country</td><td>
                    <input type=text
                           name=\"formfields[usr_country]\"
                           value=\"" . $formfields[usr_country] . "\"
	                   size=15></td></tr>
               </table></center></td></tr>";

	#
	# Default Group
	#

	# $q = DBQueryWarn("select gid, pid

	# Default Shell
        echo "<tr><td colspan=2>Shell:</td>
                  <td class=left>";
        echo "<select name=\"formfields[usr_shell]\">";
	foreach ($shelllist as $s) {
	    if ((!isset($formfields[usr_shell]) &&
		0 == strcmp($defaultshell, $s)) ||
		0 == strcmp($formfields[usr_shell],$s)) {
		$sel = "selected='1'";
	    } else {
		$sel = "";
	    }
	    echo "<option value='$s' $sel>$s</option>";
	}	
        echo "</select></td></tr>";

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
                 edit your ssh public keys</a> and your
                 <a href='showsfskeys.php3?target_uid=$target_uid'>
		 sfs public keys</a>.
            <li> The City, State, ZIP/Postal Code, and Country fields 
                 were added later, so
                 some early users will be forced to adjust their addresses
                 before they can proceed. Sorry for the inconvenience.
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

$defaults = array();
    
#
# Construct a defaults array based on current DB info. Used for the initial
# form, and to determine if any changes were made. This is to avoid churning
# the passwd file for no reason, given that most people use this page
# simply yo change their password. 
# 
$row = mysql_fetch_array($query_result);
$defaults[target_uid]  = $target_uid;
$defaults[usr_email]   = $row[usr_email];
$defaults[usr_URL]     = $row[usr_URL];
$defaults[usr_addr]    = stripslashes($row[usr_addr]);
$defaults[usr_addr2]   = stripslashes($row[usr_addr2]);
$defaults[usr_city]    = stripslashes($row[usr_city]);
$defaults[usr_state]   = stripslashes($row[usr_state]);
$defaults[usr_zip]     = stripslashes($row[usr_zip]);
$defaults[usr_country] = stripslashes($row[usr_country]);
$defaults[usr_name]    = stripslashes($row[usr_name]);
$defaults[usr_phone]   = $row[usr_phone];
$defaults[usr_title]   = stripslashes($row[usr_title]);
$defaults[usr_affil]   = stripslashes($row[usr_affil]);
$defaults[usr_shell]   = stripslashes($row[usr_shell]);

#
# On first load, display a form consisting of current user values, and exit.
#
if (! isset($submit)) {
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
if (!isset($formfields[usr_shell]) ||
    !in_array($formfields[usr_shell], $shelllist)) {
    $errors["Shell"] = "Invalid Shell";
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
if (!$isadmin) {
    if (!isset($formfields[usr_addr]) ||
	strcmp($formfields[usr_addr], "") == 0) {
	$errors["Postal Address"] = "Missing Field";
    }
    if (!isset($formfields[usr_city]) ||
	strcmp($formfields[usr_city], "") == 0) {
	$errors["City"] = "Missing Field";
    }
    if (!isset($formfields[usr_state]) ||
	strcmp($formfields[usr_state], "") == 0) {
	$errors["State"] = "Missing Field";
    }
    if (!isset($formfields[usr_zip]) ||
	strcmp($formfields[usr_zip], "") == 0) {
	$errors["ZIP/Postal Code"] = "Missing Field";
    }
    if (!isset($formfields[usr_country]) ||
	strcmp($formfields[usr_country], "") == 0) {
	$errors["Country"] = "Missing Field";
    }
    if (!isset($formfields[usr_phone]) ||
	strcmp($formfields[usr_phone], "") == 0) {
	$errors["Phone #"] = "Missing Field";
    } 
    elseif (! ereg("^[-0-9ext\(\)\+\. ]+$", $formfields[usr_phone])) {
        $errors["Phone #"] = "Invalid characters";
    }
}
if (isset($formfields[password1]) &&
    strcmp($formfields[password1], "")) {
    if (!isset($formfields[password2]) ||
	strcmp($formfields[password2], "") == 0) {
	$errors["Retype Password"] = "Missing Field";
    }
    elseif (strcmp($formfields[password1], $formfields[password2])) {
	$errors["Retype Password"] = "Two Passwords Do Not Match";
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
$usr_city     = addslashes($formfields[usr_city]);
$usr_state    = addslashes($formfields[usr_state]);
$usr_zip      = addslashes($formfields[usr_zip]);
$usr_country  = addslashes($formfields[usr_country]);
$usr_phone    = $formfields[usr_phone];
$usr_shell    = $formfields[usr_shell];
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

if (! isset($formfields[usr_addr2])) {
    $usr_addr2 = "";
}
else {
    $usr_addr2 = addslashes($formfields[usr_addr2]);
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
    # Do it again. This ensures we use the current algorithm, not whatever
    # it was encoded with last time.
    #
    $new_encoding = crypt("$password1");

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

    if (HASREALACCOUNT($uid)) {
	SUEXEC($uid, "nobody", "webtbacct passwd $target_uid", 1);
    }
}

#
# Now change the rest of the information, but only if the user actually
# changed the info. We use the original info in the defaults array and
# the value in the formfields array to compare, cause of addslashes stuff. 
#
if (strcmp($defaults[usr_name],  $formfields[usr_name]) ||
    strcmp($defaults[usr_URL],   $formfields[usr_URL]) ||
    strcmp($defaults[usr_addr],  $formfields[usr_addr]) ||
    strcmp($defaults[usr_addr2], $formfields[usr_addr2]) ||
    strcmp($defaults[usr_city],  $formfields[usr_city]) ||
    strcmp($defaults[usr_state], $formfields[usr_state]) ||
    strcmp($defaults[usr_zip],   $formfields[usr_zip]) ||
    strcmp($defaults[usr_country],   $formfields[usr_country]) ||
    strcmp($defaults[usr_phone], $formfields[usr_phone]) ||
    strcmp($defaults[usr_title], $formfields[usr_title]) ||
    strcmp($defaults[usr_affil], $formfields[usr_affil]) ||
    strcmp($defaults[usr_shell], $formfields[usr_shell]) ||
    # Check this too, since we want to call out if the email addr changed.
    strcmp($defaults[usr_email], $formfields[usr_email])) {

    DBQueryFatal("UPDATE users SET ".
		 "usr_name=\"$usr_name\",       ".
		 "usr_URL=\"$usr_URL\",         ".
		 "usr_addr=\"$usr_addr\",       ".
		 "usr_addr2=\"$usr_addr2\",     ".
		 "usr_city=\"$usr_city\",       ".
		 "usr_state=\"$usr_state\",     ".
		 "usr_zip=\"$usr_zip\",         ".
		 "usr_country=\"$usr_country\", ".
		 "usr_phone=\"$usr_phone\",     ".
		 "usr_title=\"$usr_title\",     ".
		 "usr_affil=\"$usr_affil\",     ".
		 "usr_shell=\"$usr_shell\",     ".
		 "usr_modified=now()            ".
		 "WHERE uid=\"$target_uid\"");

    # Only to user. We care about password and email changes only.
    $BCC = "";
    if (strcmp($usr_email, $dbusr_email)) {
	$BCC = "Bcc: $TBMAIL_AUDIT\n";
    }
    
    TBMAIL("$usr_name <$usr_email>",
	   "User Information for '$target_uid' Modified",
	   "\n".
	   "User information for '$target_uid' changed by '$uid'.\n".
	   "\n".
	   "Name:              $usr_name\n".
	   "Email:             $usr_email\n".
	   "URL:               $usr_URL\n".
	   "Affiliation:       $usr_affil\n".
	   "Address1:          $usr_addr\n".
	   "Address2:          $usr_addr2\n".
	   "City:              $usr_city\n".
	   "State:             $usr_state\n".
	   "ZIP/Postal Code:   $usr_zip\n".
	   "Country:           $usr_country\n".
	   "Phone:             $usr_phone\n".
	   "Title:             $usr_title\n".
	   "Shell:             $usr_shell\n",
	   "From: $TBMAIL_OPS\n".
	   $BCC .
	   "Errors-To: $TBMAIL_WWW");

    #
    # mkacct updates the user gecos
    #
    if (HASREALACCOUNT($uid)) {
	SUEXEC($uid, "nobody", "webtbacct mod $target_uid", 1);
    }
}

#
# Spit out a redirect so that the history does not include a post
# in it. The back button skips over the post and to the form.
# See above for conclusion.
# 
header("Location: moduserinfo.php3?target_uid=$target_uid&finished=1");

?>
