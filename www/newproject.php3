<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# No PAGEHEADER since we spit out a Location header later. See below.
# 

#
# Get current user.
# 
$uid = GETLOGIN();

#
# If a uid came in, then we check to see if the login is valid.
# If the login is not valid. We require that the user be logged in
# to start a second project.
#
if ($uid) {
    # Allow unapproved users to create multiple projects ...
    # Must be verified though.
    LOGGEDINORDIE($uid, CHECKLOGIN_UNAPPROVED);
    $proj_head_uid = $uid;
    $returning = 1;
}
else {
    #
    # No uid, so must be new.
    #
    $returning = 0;
}

#
# Spit the form out using the array of data. 
# 
function SPITFORM($formfields, $returning, $errors)
{
    global $TBDB_UIDLEN, $TBDB_PIDLEN, $TBDOCBASE, $WWWHOST;
    global $usr_keyfile;

    PAGEHEADER("Start a New Testbed Project");
    
    echo "<center><font size=+1>
             If you are a student
             <font color=red>(undergrad or graduate)</font>, please
             <a href=auth.html>read this first</a>!
          </font></center><br>\n";

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
                Fields marked with * are required;
                those marked + are highly recommended.
            </td>
          </tr>\n

          <form enctype=multipart/form-data
                action=newproject.php3 method=post>\n";

    if (! $returning) {
        #
        # Start user information stuff. Presented for new users only.
        #
	echo "<tr>
                  <td colspan=3>
                      Project Head Information
                  </td>
              </tr>\n";

        #
        # UserName:
        #
        echo "<tr>
                  <td colspan=2>*Username (no blanks, lowercase):</td>
                  <td class=left>
                      <input type=text
                             name=\"formfields[proj_head_uid]\"
                             value=\"" . $formfields[proj_head_uid] . "\"
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
                  <td class=left>
                      <input type=text
                             name=\"formfields[usr_email]\"
                             value=\"" . $formfields[usr_email] . "\"
	                     size=30>
                  </td>
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
	# SSH public key
	#
	echo "<tr>
                  <td rowspan><center>
                               Your SSH Pub Key: &nbsp<br>
                                    [<b>2</b>]
                              </center></td>

                  <td rowspan><center>Upload (1K max)[<b>3</b>]<br>
                                  <b>Or</b><br>
                                 Insert Key
                              </center></td>

                  <td rowspan>
                      <input type=hidden name=MAX_FILE_SIZE value=1024>
                      <input type=file
                             name=usr_keyfile
                             value=\"" . $usr_keyfile . "\"
	                     size=50>
                      <br>
                      <br>
	              <input type=text
                             name=\"formfields[usr_key]\"
                             value=\"$formfields[usr_key]\"
	                     size=50
	                     maxlength=1024>
                  </td>
              </tr>\n";

	#
	# Password. Note that we do not resend the password. User
	# must retype on error.
	#
	echo "<tr>
                  <td colspan=2>*Password[<b>1</b>]:</td>
                  <td class=left>
                      <input type=password
                             name=\"formfields[password1]\"
                             size=8></td>
              </tr>\n";

        echo "<tr>
                  <td colspan=2>*Retype Password:</td>
                  <td class=left>
                      <input type=password
                             name=\"formfields[password2]\"
                             size=8></td>
             </tr>\n";
    }

    #
    # Project information
    #
    echo "<tr><td colspan=3><hr></td></tr>
          <tr><td colspan=3>
               Project Information <em>(replace the example entries)</em>
              </td>
          </tr>\n";

    #
    # Project Name:
    #
    echo "<tr>
              <td colspan=2>*Project Name (no blanks):</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[pid]\"
                         value=\"" . $formfields[pid] . "\"
	                 size=$TBDB_PIDLEN maxlength=$TBDB_PIDLEN>
              </td>
          </tr>\n";

    #
    # Project Description:
    #
    echo "<tr>
              <td colspan=2>*Project Description:</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[proj_name]\"
                         value=\"" . $formfields[proj_name] . "\"
	                 size=40>
              </td>
          </tr>\n";

    #
    # URL:
    #
    echo "<tr>
              <td colspan=2>*URL:</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[proj_URL]\"
                         value=\"" . $formfields[proj_URL] . "\"
                         size=45>
              </td>
          </tr>\n";

    #
    # Publicly visible.
    #
    echo "<tr>
              <td colspan=2>*Can we list your project publicly as
                             an \"Emulab User?\":
                  <br>
                  (See our <a href=\"projectlist.php3\"
                              target=\"Users\">Users</a> page)
              </td>
              <td><input type=checkbox value=checked
                         name=\"formfields[proj_public]\"
                         " . $formfields[proj_public] . ">
                         Yes &nbsp
 	          <br>
                  *If \"No\" please tell us why not:<br>
                  <input type=text
                         name=\"formfields[proj_whynotpublic]\"
                         value=\"" . $formfields[proj_whynotpublic] . "\"
	                 size=45>
             </td>
      </tr>\n";

    #
    # Will you add a link?
    #
    echo "<tr>
              <td colspan=2>*Will you add a link on your project page
                             to <a href=\"$TBDOCBASE\">$WWWHOST</a>?
              </td>
              <td><input type=checkbox value=checked
                         name=\"formfields[proj_linked]\"
                         " . $formfields[proj_linked] . ">
                         Yes &nbsp
              </td>
      </tr>\n";

    #
    # Funders/Grant numbers
    #
    echo "<tr>
              <td colspan=2>*Funding Sources and Grant Numbers:<br>
                  (Type \"none\" if not funded)</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[proj_funders]\"
                         value=\"" . $formfields[proj_funders] . "\"
	                 size=45>
              </td>
          </tr>\n";

    #
    # Nodes and PCs and Users
    #
    echo "<tr>
              <td colspan=2>*Estimated #of Project Members:</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[proj_members]\" 
                         value=\"" . $formfields[proj_members] . "\"
                         size=4>
              </td>
          </tr>\n";

    echo "<tr>
              <td colspan=2>*Estimated #of
        <a href=\"$TBDOCBASE/docwrapper.php3?docname=hardware.html#tbpcs\">
                             PCs</a>:</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[proj_pcs]\"
                         value=\"" . $formfields[proj_pcs] . "\"
                         size=4>
              </td>
          </tr>\n";

    if (0) {
    echo "<tr>
              <td colspan=2>*Estimated #of
        <a href=\"$TBDOCBASE/docwrapper.php3?docname=widearea.html\">
                             Planetlab PCs</a>:</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[proj_plabpcs]\"
                         value=\"" . $formfields[proj_plabpcs] . "\"
                         size=4>
              </td>
          </tr>\n";
    }

    echo "<tr>
              <td colspan=2>*Estimated #of
        <a href=\"$TBDOCBASE/docwrapper.php3?docname=widearea.html\">
                             MIT Testbed PCs</a>:</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[proj_ronpcs]\"
                         value=\"" . $formfields[proj_ronpcs] . "\"
                         size=4>
              </td>
          </tr>\n";

    #
    # Why!
    # 
    echo "<tr>
              <td colspan=3>
               *Please describe how and why you'd like to use the testbed.
              </td>
          </tr>
          <tr>
              <td colspan=3 align=center class=left>
                  <textarea name=\"formfields[proj_why]\"
                    rows=10 cols=60>" .
	            ereg_replace("\r", "", $formfields[proj_why]) .
	            "</textarea>
              </td>
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
                 regarding passwords and email addresses.\n";
    if (! $returning) {
	echo "<li> If you want us to use your existing ssh public key,
                   then either paste it in or specify the path to your
                   your identity.pub file. <font color=red>NOTE:</font>
                   We use the <a href=www.openssh.org>OpenSSH</a> key format,
                   which has a slightly different protocol 2 public key format
                   than some of the commercial vendors such as
                   <a href=www.ssh.com>SSH Communications</a>. If you
                   use one of these commercial vendors, then please
                   upload the public  key file and we will convert it
                   for you. <i>Please do not paste it in.</i>\n

              <li> Note to <a href=http://www.opera.com><b>Opera 5</b></a>
                   users: The file upload mechanism is broken in Opera, so
                   you cannot specify a local file for upload. Instead,
                   please paste your public key in.\n";
    }
    echo "</ol>
          </blockquote></blockquote>
          </h4>\n";
}

#
# The conclusion of a newproject request. See below.
# 
if (isset($finished)) {
    PAGEHEADER("Start a New Testbed Project");

    echo "<center><h2>
           Your project request has been successfully queued.
          </h2></center>
          Testbed Operations has been notified of your application.
          Most applications are reviewed within a day; some even within
          the hour, but sometimes as long as a week (rarely). We will notify
          you by e-mail when a decision has been made.\n";

    if (! $returning) {
	echo "<br>
              <p>
              In the meantime, as a new user of the Testbed you will receive
              a key via email.
              When you receive the message, please follow the instructions
              contained in the message on how to verify your account.\n";
    }
    PAGEFOOTER();
    return;
}

#
# On first load, display a virgin form and exit.
#
if (! isset($submit)) {
    $defaults = array();
    $defaults[proj_URL] = "$HTTPTAG";
    $defaults[usr_URL] = "$HTTPTAG";
    $defaults[proj_ronpcs]  = "0";
    $defaults[proj_plabpcs] = "0";
    $defaults[proj_public] = "checked";
    $defaults[proj_linked] = "checked";
    
    SPITFORM($defaults, $returning, 0);
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
if (! $returning) {
    if (!isset($formfields[proj_head_uid]) ||
	strcmp($formfields[proj_head_uid], "") == 0) {
	$errors["Username"] = "Missing Field";
    }
    else {
	if (! ereg("^[a-zA-Z][-_a-zA-Z0-9]+$", $formfields[proj_head_uid])) {
	    $errors["UserName"] =
		"Must be lowercase alphanumeric only<br>".
		"and must begin with a lowercase alpha";
	}
	elseif (strlen($formfields[proj_head_uid]) > $TBDB_UIDLEN) {
	    $errors["UserName"] =
		"Too long! Must be less than or equal to $TBDB_UIDLEN";
	}
	elseif (TBCurrentUser($formfields[proj_head_uid])) {
	    $errors["UserName"] =
		"Already in use. Select another";
	}
    }
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
    if (!isset($formfields[password1]) ||
	strcmp($formfields[password1], "") == 0) {
	$errors["Password"] = "Missing Field";
    }
    if (!isset($formfields[password2]) ||
	strcmp($formfields[password2], "") == 0) {
	$errors["Confirm Password"] = "Missing Field";
    }
    elseif (strcmp($formfields[password1], $formfields[password2])) {
	$errors["Confirm Password"] = "Does not match Password";
    }
    elseif (! CHECKPASSWORD($formfields[proj_head_uid],
			    $formfields[password1],
			    $formfields[usr_name],
			    $formfields[usr_email], $checkerror)) {
	$errors["Password"] = "$checkerror";
    }
}

if (!isset($formfields[pid]) ||
    strcmp($formfields[pid], "") == 0) {
    $errors["Project Name"] = "Missing Field";
}
else {
    if (! ereg("^[a-zA-Z][-_a-zA-Z0-9]+$", $formfields[pid])) {
	$errors["Project Name"] =
	    "Must be alphanumeric (includes _ and -)<br>".
	    "and must begin with an alpha";
    }
    elseif (strlen($formfields[pid]) > $TBDB_PIDLEN) {
	$errors["Project Name"] =
	    "Too long! Must be less than or equal to $TBDB_PIDLEN";
    }
    elseif (TBValidProject($formfields[pid])) {
	$errors["Project Name"] =
	    "Already in use. Select another";
    }
}
if (!isset($formfields[proj_name]) ||
    strcmp($formfields[proj_name], "") == 0) {
    $errors["Project Description"] = "Missing Field";
}
if (!isset($formfields[proj_URL]) ||
    strcmp($formfields[proj_URL], "") == 0 ||
    strcmp($formfields[proj_URL], $HTTPTAG) == 0) {    
    $errors["Project URL"] = "Missing Field";
}
elseif (! CHECKURL($formfields[proj_URL], $urlerror)) {
    $errors["Project URL"] = $urlerror;
}
if (!isset($formfields[proj_funders]) ||
    strcmp($formfields[proj_funders], "") == 0) {
    $errors["Funding Sources"] = "Missing Field";
}
if (!isset($formfields[proj_members]) ||
    strcmp($formfields[proj_members], "") == 0) {
    $errors["#of Members"] = "Missing Field";
}
elseif (! ereg("^[0-9]+$", $formfields[proj_members])) {
    $errors["#of Members"] = "Must be numeric";
}
if (!isset($formfields[proj_pcs]) ||
    strcmp($formfields[proj_pcs], "") == 0) {
    $errors["#of PCs"] = "Missing Field";
}
elseif (! ereg("^[0-9]+$", $formfields[proj_pcs])) {
    $errors["#of PCs"] = "Must be numeric";

}
if (0) {
if (!isset($formfields[proj_plabpcs]) ||
    strcmp($formfields[proj_plabpcs], "") == 0) {
    $errors["#of Planetlab PCs"] = "Missing Field";
}
elseif (! ereg("^[0-9]+$", $formfields[proj_plabpcs])) {
    $errors["#of Planetlab PCs"] = "Must be numeric";
}
} 
if (!isset($formfields[proj_ronpcs]) ||
    strcmp($formfields[proj_ronpcs], "") == 0) {
    $errors["#of RON PCs"] = "Missing Field";
}
elseif (! ereg("^[0-9]+$", $formfields[proj_ronpcs])) {
    $errors["#of RON PCs"] = "Must be numeric";
}
if (!isset($formfields[proj_why]) ||
    strcmp($formfields[proj_why], "") == 0) {
    $errors["Why?"] = "Missing Field";
}
if ((!isset($formfields[proj_public]) ||
     strcmp($formfields[proj_public], "checked")) &&
    (!isset($formfields[proj_whynotpublic]) ||
     strcmp($formfields[proj_whynotpublic], "") == 0)) {
    $errors["Why Not Public?"] = "Missing Field";
}
if (isset($formfields[proj_linked]) &&
    strcmp($formfields[proj_linked], "") &&
    strcmp($formfields[proj_linked], "checked")) {
    $errors["Link to Us"] = "Bad Value";
}

if (count($errors)) {
    SPITFORM($formfields, $returning, $errors);
    PAGEFOOTER();
    return;
}

#
# Certain of these values must be escaped or otherwise sanitized.
#
if (!$returning) {
    $proj_head_uid     = $formfields[proj_head_uid];
    $usr_title         = addslashes($formfields[usr_title]);
    $usr_name          = addslashes($formfields[usr_name]);
    $usr_affil         = addslashes($formfields[usr_affil]);
    $usr_email         = $formfields[usr_email];
    $usr_addr          = addslashes($formfields[usr_addr]);
    $usr_phone         = $formfields[usr_phone];
    $password1         = $formfields[password1];
    $password2         = $formfields[password2];
    $usr_returning     = "No";

    if (! isset($formfields[usr_URL]) ||
	strcmp($formfields[usr_URL], "") == 0 ||
	strcmp($formfields[usr_URL], $HTTPTAG) == 0) {
	$usr_URL = "";
    }
    else {
	$usr_URL = $formfields[usr_URL];
    }
    
    #
    # Pub Key.
    #
    if (isset($formfields[usr_key]) &&
	strcmp($formfields[usr_key], "")) {
        #
        # This is passed off to the shell, so taint check it.
        # 
	if (! preg_match("/^[-\w\s\.\@\+\/\=]*$/", $formfields[usr_key])) {
	    $errors["PubKey"] = "Invalid characters";
	}
	else {
            #
            # Replace any embedded newlines first.
            #
	    $formfields[usr_key] =
		ereg_replace("[\n]", "", $formfields[usr_key]);
	    $usr_key = $formfields[usr_key];
	    $addpubkeyargs = "-k $proj_head_uid '$usr_key' ";
	}
    }

    #
    # If usr provided a file for the key, it overrides the paste in text.
    #
    if (isset($usr_keyfile) &&
	strcmp($usr_keyfile, "") &&
	strcmp($usr_keyfile, "none")) {

	if (! stat($usr_keyfile)) {
	    $errors["PubKey File"] = "No such file";
	}
	else {
	    $addpubkeyargs = "$proj_head_uid $usr_keyfile";
	    chmod($usr_keyfile, 0640);	
	}
    }
    #
    # Verify key format.
    #
    if (isset($addpubkeyargs) &&
	ADDPUBKEY("nobody", "webaddpubkey -n $addpubkeyargs")) {
	$errors["Pubkey Format"] = "Could not be parsed. Is it a public key?";
    }

    if (count($errors)) {
	SPITFORM($formfields, $returning, $errors);
	PAGEFOOTER();
	return;
    }
}
else {
    #
    # Grab info from the DB for the email message below. Kinda silly.
    #
    $query_result =
	DBQueryFatal("select * from users where uid='$proj_head_uid'");
    
    $row = mysql_fetch_array($query_result);
    
    $usr_title	   = $row[usr_title];
    $usr_name	   = $row[usr_name];
    $usr_affil	   = $row[usr_affil];
    $usr_email	   = $row[usr_email];
    $usr_addr	   = $row[usr_addr];
    $usr_phone	   = $row[usr_phone];
    $usr_URL       = $row[usr_URL];
    $usr_returning = "Yes";
}
$pid               = $formfields[pid];
$proj_name	   = addslashes($formfields[proj_name]);
$proj_URL          = $formfields[proj_URL];
$proj_funders      = addslashes($formfields[proj_funders]);
$proj_whynotpublic = addslashes($formfields[proj_whynotpublic]);
$proj_members      = $formfields[proj_members];
$proj_pcs          = $formfields[proj_pcs];
#$proj_plabpcs      = $formfields[proj_plabpcs];
$proj_plabpcs      = 0;
$proj_ronpcs       = $formfields[proj_ronpcs];
$proj_why	   = addslashes($formfields[proj_why]);
$proj_expires      = date("Y:m:d", time() + (86400 * 120));

if (!isset($formfields[proj_public]) ||
    strcmp($formfields[proj_public], "checked")) {
    $proj_public = "No";
    $public = 0;
}
else {
    $proj_public = "Yes";
    $public = 1;
}
if (!isset($formfields[proj_linked]) ||
    strcmp($formfields[proj_linked], "checked")) {
    $proj_linked = "No";
}
else {
    $proj_linked = "Yes";
}

#
# Check that we can guarantee uniqueness of the unix group name.
# 
$query_result =
    DBQueryFatal("select gid from groups where unix_name='$pid'");

if (mysql_num_rows($query_result)) {
    TBERROR("Could not form a unique Unix group name for $pid!", 1);
}

#
# For a new user:
# * Create a new account in the database.
# * Generate a mail message to the user with the verification key.
# 
if (! $returning) {
    $encoding = crypt("$password1");

    #
    # Must be done before user record is inserted!
    # XXX Since, user does not exist, must run as nobody. Script checks. 
    # 
    if (isset($addpubkeyargs)) {
	ADDPUBKEY("nobody", "webaddpubkey $addpubkeyargs");
    }

    DBQueryFatal("INSERT INTO users ".
	 "(uid,usr_created,usr_expires,usr_name,usr_email,usr_addr,".
	 " usr_URL,usr_title,usr_affil,usr_phone,usr_pswd,unix_uid,".
	 " status,pswd_expires,usr_modified) ".
	 "VALUES ('$proj_head_uid', now(), '$proj_expires', '$usr_name', ".
	 "'$usr_email', '$usr_addr', '$usr_URL', '$usr_title', '$usr_affil', ".
	 "'$usr_phone', '$encoding', NULL, 'newuser', ".
	 "date_add(now(), interval 1 year), now())");

    $key = GENKEY($proj_head_uid);

    TBMAIL("$usr_name '$proj_head_uid' <$usr_email>",
      "Your New User Key",
      "\n".
      "Dear $usr_name:\n\n".
      "This is your account verification key: $key\n\n".
      "Please use this link to verify your user account:\n".
      "\n".
      "    ${TBBASE}/login.php3?vuid=$proj_head_uid&key=$key\n".
      "\n".
      "You will then be verified as a user. When you have been both\n".
      "verified and approved by Testbed Operations, you will be marked\n".
      "as an active user and granted full access to your account.\n".
      "\n".
      "Thanks,\n".
      "Testbed Ops\n".
      "Utah Network Testbed\n",
      "From: $TBMAIL_APPROVAL\n".
      "Bcc: $TBMAIL_AUDIT\n".
      "Errors-To: $TBMAIL_WWW");
}

#
# Now for the new Project
# * Create a new project in the database.
# * Create a new default group for the project.
# * Create a new group_membership entry in the database, default trust=none.
# * Generate a mail message to testbed ops.
#
DBQueryFatal("INSERT INTO projects ".
	     "(pid, created, expires, name, URL, head_uid, ".
	     " num_members, num_pcs, why, funders, unix_gid, ".
	     " num_pcplab, num_ron, public, public_whynot)".
	     "VALUES ('$pid', now(), '$proj_expires','$proj_name', ".
	     "        '$proj_URL', '$proj_head_uid', '$proj_members', ".
	     "        '$proj_pcs', '$proj_why', ".
	     "        '$proj_funders', NULL, $proj_plabpcs, $proj_ronpcs, ".
	     "         $public, '$proj_whynotpublic')");

DBQueryFatal("INSERT INTO groups ".
	     "(pid, gid, leader, created, description, unix_gid, unix_name) ".
	     "VALUES ('$pid', '$pid', '$proj_head_uid', now(), ".
	     "        'Default Group', NULL, '$pid')");

DBQueryFatal("insert into group_membership ".
	     "(uid, gid, pid, trust, date_applied) ".
	     "values ('$proj_head_uid','$pid','$pid','none', now())");

#
# Grab the unix GID that was assigned.
#
TBGroupUnixInfo($pid, $pid, $unix_gid, $unix_name);

#
# The mail message to the approval list.
# 
TBMAIL($TBMAIL_APPROVAL,
     "New Project '$pid' ($proj_head_uid)",
     "'$usr_name' wants to start project '$pid'.\n".
     "\n".
     "Name:            $usr_name ($proj_head_uid)\n".
     "Returning User?: $usr_returning\n".
     "Email:           $usr_email\n".
     "User URL:        $usr_URL\n".
     "Project:         $proj_name\n".
     "Expires:         $proj_expires\n".
     "Project URL:     $proj_URL\n".
     "Public URL:      $proj_public\n".
     "Why Not Public:  $proj_whynotpublic\n".
     "Link to Us?:     $proj_linked\n".
     "Funders:         $proj_funders\n".
     "Title:           $usr_title\n".
     "Affiliation:     $usr_affil\n".
     "Address:         $usr_addr\n".
     "Phone:           $usr_phone\n".
     "Members:         $proj_members\n".
     "PCs:             $proj_pcs\n".
     "Planetlab PCs:   $proj_plabpcs\n".
     "RON PCs:         $proj_ronpcs\n".
     "Unix GID:        $unix_name ($unix_gid)\n".
     "Reasons:\n$proj_why\n\n".
     "Please review the application and when you have made a decision,\n".
     "go to $TBWWW and\n".
     "select the 'Project Approval' page.\n\nThey are expecting a result ".
     "within 72 hours.\n", 
     "From: $usr_name '$proj_head_uid' <$usr_email>\n".
     "Reply-To: $TBMAIL_APPROVAL\n".
     "Errors-To: $TBMAIL_WWW");

#
# Spit out a redirect so that the history does not include a post
# in it. The back button skips over the post and to the form.
# See above for conclusion.
# 
header("Location: newproject.php3?finished=1");

?>
