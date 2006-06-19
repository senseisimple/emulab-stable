<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2005, 2006 University of Utah and the Flux Group.
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
	      CHECKLOGIN_USERSTATUS|CHECKLOGIN_PSWDEXPIRED|
	      CHECKLOGIN_WEBONLY|CHECKLOGIN_WIKIONLY);
$isadmin = ISADMIN($uid);


# Shell options we support. Maybe stick in DB someday.
$shelllist = array( 'tcsh', 'bash', 'csh', 'sh' );

# used if db slot for user is NULL (should not happen.)
$defaultshell = 'tcsh';

# See below.
$wikionly = 0;

#
# Spit the form out using the array of data and error strings (if any).
# 
function SPITFORM($formfields, $errors)
{
    global $TBDB_UIDLEN, $TBDB_PIDLEN, $TBDB_GIDLEN, $isadmin;
    global $target_uid, $wikionly;
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

    # For indicating that fields are optional or not.
    $optfield = ($wikionly ? "" : "*");

    echo "<table align=center border=1> 
          <tr>
            <td align=center colspan=3>
                <b>Fields marked with * are required.</b>
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
                  <td colspan=2>${optfield}Title/Position:</td>
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
                  <td colspan=2>${optfield}Institutional<br>Affiliation:</td>
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
                  <td colspan=2>Email Address[<b>1</b>]:</td>
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

	#
	# Postal Address
        #
	echo "<tr><td colspan=3>${optfield}Address:<br /><center>
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
                  <td colspan=2>${optfield}Phone #:</td>
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
	echo "<tr></tr>\n";
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

	if (!$wikionly) {
	    #
            # Windows Password.  Initial random default is based on the Unix
	    # password hash.
	    #   
	    # A separate password is kept for experiment nodes running Windows.
	    # It is presented behind-the-scenes to rdesktop and Samba by our
	    # Web# interface, but you may still need to type it.
	    # The default password is randomly generated.
	    # You may change it to something easier to remember.
	    #
	    echo "<tr>
                      <td colspan=2>Windows Password[<b>1,4</b>]:</td>
                      <td class=left>
                          <input type=text
                                 name=\"formfields[w_password1]\"
                                 value=\"" . $formfields[w_password1] . "\"
                                 size=8></td>
                  </tr>\n";

	    echo "<tr>
                      <td colspan=2>Retype Windows Password:</td>
                      <td class=left>
                          <input type=text
                                 name=\"formfields[w_password2]\"
                                 size=8></td>
                 </tr>\n";

            #
	    # Planetlab bit. This should really be a drop down menu of the
	    #                choices.
            #
	    if ($formfields[user_interface] == TBDB_USER_INTERFACE_PLAB) {
		$checked = "checked";
	    } else {
		$checked = "";
	    }

	    echo "<tr>
		      <td colspan=2>Use simplified PlanetLab view:</td>
		      <td class=left>
		         <input type='checkbox'
                                name=\"formfields[user_interface]\"
                                value=\"" . TBDB_USER_INTERFACE_PLAB . "\"
			        $checked>
		      </td>
	          </tr>\n";
	}

        #
	# Notes
	#
	if ($isadmin) {
	    echo "<tr>
                      <td colspan=2>Admin Notes:</td>
                      <td class=left>
                         <textarea name=\"formfields[notes]\"
                                   rows=2 cols=40>" .
		                   ereg_replace("\r", "", $formfields[notes]) .
		        "</textarea>
                      </td>
                  </tr>\n";
	}

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
                 regarding passwords and email addresses.\n";
    if (!$wikionly) {
	echo "<li> You can also
                 <a href='showpubkeys.php3?target_uid=$target_uid'>
                 edit your ssh public keys</a> and your
                 <a href='showsfskeys.php3?target_uid=$target_uid'>
		 sfs public keys</a>.
            <li> The City, State, ZIP/Postal Code, and Country fields 
                 were added later, so
                 some early users will be forced to adjust their addresses
                 before they can proceed. Sorry for the inconvenience.
            <li> A separate password is kept for experiment nodes running
                 Windows.  It is presented behind-the-scenes to rdesktop and
                 Samba by our Web interface, but you may still need to type
                 it.  The default password is randomly generated.  You may
                 change it to something easier to remember.\n";
    }
    echo "</ol>
          </blockquote></blockquote>
          </h4>\n";
}

#
# The target uid and the current uid will be the same, unless its a priv user
# (admin,PI) modifying someone elses data. Must verify this case. Note that
# the target uid comes initially as a page arg, but later as a form argument,
# hence this odd check.
#
if (! isset($_POST['submit'])) {
    # First page load. Default to current user.
    if (! isset($_GET['target_uid']))
	$target_uid = $uid;
    else
	$target_uid = $_GET['target_uid'];
}
else {
    # Form submitted. Make sure we have a formfields array and a target_uid.
    if (!isset($_POST['formfields']) ||
	!is_array($_POST['formfields']) ||
	!isset($_POST['formfields']['target_uid'])) {
	PAGEARGERROR("Invalid form arguments!");
    }
    $formfields = $_POST['formfields'];
    $target_uid = $formfields['target_uid'];
}

# Pedantic check of uid before continuing.
if ($target_uid == "" || !TBvalid_uid($target_uid)) {
    PAGEARGERROR("Invalid uid: '$target_uid'");
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
# Suck the current info out of the database.
#
$query_result =
    DBQueryFatal("select * from users where uid='$target_uid'");

if (mysql_num_rows($query_result) == 0) {
    USERERROR("No such user: $target_uid", 1);
}

$defaults = array();
    
#
# Construct a defaults array based on current DB info. Used for the initial
# form, and to determine if any changes were made. This is to avoid churning
# the passwd file for no reason, given that most people use this page
# simply to change their password. 
# 
$row = mysql_fetch_array($query_result);
$defaults[target_uid]  = $target_uid;
$defaults[usr_email]   = $row[usr_email];
$defaults[usr_URL]     = $row[usr_URL];
$defaults[usr_addr]    = $row[usr_addr];
$defaults[usr_addr2]   = $row[usr_addr2];
$defaults[usr_city]    = $row[usr_city];
$defaults[usr_state]   = $row[usr_state];
$defaults[usr_zip]     = $row[usr_zip];
$defaults[usr_country] = $row[usr_country];
$defaults[usr_name]    = $row[usr_name];
$defaults[usr_phone]   = $row[usr_phone];
$defaults[usr_title]   = $row[usr_title];
$defaults[usr_affil]   = $row[usr_affil];
$defaults[usr_shell]   = $row[usr_shell];
$defaults[notes]       = $row[notes];
$defaults[user_interface] = $row[user_interface];
$wikionly              = $row[wikionly];

# Show and keep the Windows password if user-set, otherwise fill in the random one.
if (strcmp($row[usr_w_pswd],""))
    $defaults[w_password1] = $defaults[w_password2] = $row[usr_w_pswd];
else {
    # The initial random default for the Windows Password is based on the Unix
    # encrypted password, in particular the random salt if it's an MD5 crypt,
    # consisting of the 8 characters after an initial "$1$" and followed by a "$". 
    $unixpwd = explode('$', $row[usr_pswd]);
    if (strlen($unixpwd[0]) > 0)
	# When there's no $ at the beginning, its not an MD5 hash.
	$randpwd = substr($unixpwd[0],0,8);
    else
	$randpwd = substr($unixpwd[2],0,8); # The MD5 salt string.
    $defaults[w_password1] = $defaults[w_password2] = $randpwd;
}

#
# On first load, display a form consisting of current user values, and exit.
#
if (! isset($_POST['submit'])) {
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
if (!isset($formfields[usr_name]) ||
    strcmp($formfields[usr_name], "") == 0) {
    $errors["Full Name"] = "Missing Field";
}
elseif (! TBvalid_usrname($formfields[usr_name])) {
    $errors["Full Name"] = TBFieldErrorString();
}
# Make sure user name has at least two tokens!
$tokens = preg_split("/[\s]+/", $formfields[usr_name],
		     -1, PREG_SPLIT_NO_EMPTY);
if (count($tokens) < 2) {
    $errors["Full Name"] = "Please provide a first and last name";
}
if (!$wikionly) {
    # WikiOnly can leave these fields blank, but must error check them anyway.
    if (!isset($formfields[usr_title]) ||
	strcmp($formfields[usr_title], "") == 0) {
	$errors["Title/Position"] = "Missing Field";
    }
    if (!isset($formfields[usr_affil]) ||
	strcmp($formfields[usr_affil], "") == 0) {
	$errors["Affiliation"] = "Missing Field";
    }
}
if (isset($formfields[usr_title]) &&
    ! TBvalid_title($formfields[usr_title])) {
    $errors["Title/Position"] = TBFieldErrorString();
}
if (isset($formfields[usr_affil]) &&
    ! TBvalid_affiliation($formfields[usr_affil])) {
    $errors["Affiliation"] = TBFieldErrorString();
}
if (!isset($formfields[usr_shell]) ||
    !in_array($formfields[usr_shell], $shelllist)) {
    $errors["Shell"] = "Invalid Shell";
}
if (isset($formfields[usr_URL]) &&
    strcmp($formfields[usr_URL], "") &&
    strcmp($formfields[usr_URL], $HTTPTAG) &&
    ! CHECKURL($formfields[usr_URL], $urlerror)) {
    $errors["Home Page URL"] = $urlerror;
}
if (!isset($formfields[usr_email]) ||
    strcmp($formfields[usr_email], "") == 0) {
    $errors["Email Address"] = "Missing Field";
}
elseif (! TBvalid_email($formfields[usr_email])) {
    $errors["Email Address"] = TBFieldErrorString();
}
elseif (! TBUniqueEmail($target_uid, $formfields[usr_email])) {
    $errors["Email Address"] = "Already in use by another user!";
}
if (!$isadmin && !$wikionly) {
    # Admins can leave these fields blank, but must error check them anyway.
    if (!isset($formfields[usr_addr]) ||
	strcmp($formfields[usr_addr], "") == 0) {
	$errors["Postal Address 1"] = "Missing Field";
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
}
if (isset($formfields[usr_addr]) &&
    !TBvalid_addr($formfields[usr_addr])) {
    $errors["Postal Address 1"] = TBFieldErrorString();
}
# Optional
if (isset($formfields[usr_addr2]) &&
    !TBvalid_addr($formfields[usr_addr2])) {
    $errors["Postal Address 2"] = TBFieldErrorString();
}
if (isset($formfields[usr_city]) &&
    !TBvalid_city($formfields[usr_city])) {
    $errors["City"] = TBFieldErrorString();
}
if (isset($formfields[usr_state]) &&
    !TBvalid_state($formfields[usr_state])) {
    $errors["State"] = TBFieldErrorString();
}
if (isset($formfields[usr_zip]) &&
    !TBvalid_zip($formfields[usr_zip])) {
    $errors["Zip/Postal Code"] = TBFieldErrorString();
}
if (isset($formfields[usr_country]) &&
    !TBvalid_country($formfields[usr_zip])) {
    $errors["Zip/Postal Code"] = TBFieldErrorString();
}
if (isset($formfields[usr_phone]) && $formfields[usr_phone] != "" &&
    !TBvalid_phone($formfields[usr_phone])) {
    $errors["Phone #"] = TBFieldErrorString();
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
if (isset($formfields[w_password1]) &&
    strcmp($formfields[w_password1], "") &&
    $formfields[w_password1] != $defaults[w_password1]) {
    if (!isset($formfields[w_password2]) ||
	strcmp($formfields[w_password2], "") == 0) {
	$errors["Retype Windows Password"] = "Missing Field";
    }
    elseif (strcmp($formfields[w_password1], $formfields[w_password2])) {
	$errors["Retype Windows Password"] = "Two Windows Passwords Do Not Match";
    }
    elseif (! CHECKPASSWORD($formfields[target_uid],
			    $formfields[w_password1],
			    $formfields[usr_name],
			    $formfields[usr_email], $checkerror)) {
	$errors["Windows Password"] = "$checkerror";
    }
}
if (count($errors)) {
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    return;
}

PAGEHEADER("Modify User Information");

$usr_name     = addslashes($formfields[usr_name]);
$usr_email    = $formfields[usr_email];
$password1    = $formfields[password1];
$password2    = $formfields[password2];
$usr_title    = addslashes($formfields[usr_title]);
$usr_affil    = addslashes($formfields[usr_affil]);
$usr_addr     = addslashes($formfields[usr_addr]);
$usr_city     = addslashes($formfields[usr_city]);
$usr_state    = addslashes($formfields[usr_state]);
$usr_zip      = addslashes($formfields[usr_zip]);
$usr_country  = addslashes($formfields[usr_country]);
$usr_phone    = $formfields[usr_phone];
$usr_shell    = $formfields[usr_shell];
$w_password1  = $formfields[w_password1];
$w_password2  = $formfields[w_password2];

if (! isset($formfields[usr_URL]) ||
    strcmp($formfields[usr_URL], "") == 0 ||
    strcmp($formfields[usr_URL], $HTTPTAG) == 0) {
    $usr_URL = "";
}
else {
    $usr_URL = addslashes($formfields[usr_URL]);
}

if (! isset($formfields[usr_addr2])) {
    $usr_addr2 = "";
}
else {
    $usr_addr2 = addslashes($formfields[usr_addr2]);
}

#
# Only admin types can change the email address. If its different, the
# user circumvented the form, and so its okay to blast it.
#
TBUserInfo($target_uid, $dbusr_name, $dbusr_email);

if (strcmp($usr_email, $dbusr_email)) {
    if (!$isadmin) {
	USERERROR("You are not allowed to change your email address. <br> ".
		  "Please contact Testbed Operations.", 1);
    }
    DBQueryFatal("update users set usr_email='$usr_email' ".
		 "where uid='$target_uid'");
}

STARTBUSY("Making user profile changes");

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

    if ($wikionly) {
	if ($CHECKLOGIN_STATUS & CHECKLOGIN_ACTIVE) {
	    SUEXEC("nobody", "nobody", "webtbacct passwd $target_uid",
		   SUEXEC_ACTION_DIE);
	}
    }
    elseif (HASREALACCOUNT($uid) && HASREALACCOUNT($target_uid)) {
	SUEXEC($uid, "nobody", "webtbacct passwd $target_uid",
	       SUEXEC_ACTION_DIE);
    }
}

#
# See if the user is requesting to change the Windows password. We checked
# them above when the form was submitted.
#
if (!$wikionly &&
    ((isset($w_password1) && strcmp($w_password1, "")) &&
     (isset($w_password2) && strcmp($w_password2, "")))) {

    #
    # Insert into database.
    $insert_result =
	DBQueryFatal("UPDATE users SET usr_w_pswd='$w_password1' ".
		     "WHERE uid='$target_uid'");

    if (HASREALACCOUNT($uid) && HASREALACCOUNT($target_uid)) {
	SUEXEC($uid, "nobody", "webtbacct wpasswd $target_uid", 1);
    }
}

#
# Only admins can change the notes field. We do not bother to generate
# any email or external updates for this.
#
if ($isadmin &&
    strcmp($defaults[notes], $formfields[notes])) {
    $notes = addslashes($formfields[notes]);

    DBQueryFatal("UPDATE users SET notes='$notes' ".
		 "WHERE uid='$target_uid'");
}

#
# Set the plab bit seperately since no need to call out to the backend.
#
if (isset($formfields[user_interface]) &&
    $formfields[user_interface] == TBDB_USER_INTERFACE_PLAB) {
    $user_interface = TBDB_USER_INTERFACE_PLAB;
}
else {
    $user_interface = TBDB_USER_INTERFACE_EMULAB;
}
if ($defaults[user_interface] != $user_interface) {
    DBQueryFatal("update users set ".
		 "user_interface='$user_interface' ".
		 "where uid=\"$target_uid\"");
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
	$BCC = "\nBcc: $TBMAIL_AUDIT";
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
	   "WikiOnly:	       $wikionly\n",
	   "From: $TBMAIL_OPS".
	   $BCC .
	   "Errors-To: $TBMAIL_WWW");

    #
    # mkacct updates the user gecos
    #
    if (HASREALACCOUNT($uid) && HASREALACCOUNT($target_uid)) {
	SUEXEC($uid, "nobody", "webtbacct mod $target_uid", 1);
    }
}

STOPBUSY();

#
# Spit out a redirect so that the history does not include a post
# in it. The back button skips over the post and to the form.
# 
PAGEREPLACE("showuser.php3?target_uid=$target_uid#PROFILE");

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
