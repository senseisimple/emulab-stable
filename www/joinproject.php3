<?php
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
    LOGGEDINORDIE($uid);
    $joining_uid = $uid;
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
    global $TBDB_UIDLEN, $TBDB_PIDLEN, $TBDB_GIDLEN;
    global $usr_keyfile;
    
    PAGEHEADER("Apply for Project Membership");

    if ($errors) {
	echo "<table align=center border=0 cellpadding=0 cellspacing=2>
              <tr>
                 <td nowrap align=center colspan=3>
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

          <form action=joinproject.php3 method=post>\n";

    if (! $returning) {
        #
        # UserName:
        #
        echo "<tr>
                  <td colspan=2>*Username (no blanks, lowercase):</td>
                  <td class=left>
                      <input type=text
                             name=\"formfields[joining_uid]\"
                             value=\"" . $formfields[joining_uid] . "\"
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
                             value=\"" . $formfields[usr_key] . "\"
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
    # Group Name:
    #
    echo "<tr>
              <td colspan=2>Group Name:<br>
              (Leave blank unless you <em>know</em> the group name)</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[gid]\"
                         value=\"" . $formfields[gid] . "\"
	                 size=$TBDB_GIDLEN maxlength=$TBDB_GIDLEN>
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
                   your identity.pub file.
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
# The conclusion of a join request. See below.
# 
if (isset($finished)) {
    PAGEHEADER("Apply for Project Membership");

    #
    # Generate some warm fuzzies.
    #
    echo "<p>
          The project leader has been notified of your application.
          He/She will make a decision and either approve or deny your
          application, and you will be notified via email as soon as a
          decision has been made.";

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
    $defaults[usr_URL] = "$HTTPTAG";
    
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
    if (!isset($formfields[joining_uid]) ||
	strcmp($formfields[joining_uid], "") == 0) {
	$errors["Username"] = "Missing Field";
    }
    else {
	if (! ereg("^[a-zA-Z][-_a-zA-Z0-9]+$", $formfields[joining_uid])) {
	    $errors["UserName"] =
		"Must be lowercase alphanumeric only<br>".
		"and must begin with a lowercase alpha";
	}
	elseif (strlen($formfields[joining_uid]) > $TBDB_UIDLEN) {
	    $errors["UserName"] =
		"Too long! Must be less than or to $TBDB_UIDLEN";
	}
	elseif (TBCurrentUser($formfields[joining_uid])) {
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
    elseif (! ereg("^[\(]*[0-9][-\(\)0-9]+$", $formfields[usr_phone])) {
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
    elseif (! CHECKPASSWORD($formfields[joining_uid],
			    $formfields[password1],
			    $formfields[usr_name],
			    $formfields[usr_email], $checkerror)) {
	$errors["Password"] = "$checkerror";
    }
    if (isset($formfields[usr_key]) &&
	strcmp($formfields[usr_key], "") &&
	! ereg("^[0-9a-zA-Z\@\. ]*$", $formfields[usr_key])) {
	$errors["PubKey"] = "Invalid characters";
    }
}
if (!isset($formfields[pid]) ||
    strcmp($formfields[pid], "") == 0) {
    $errors["Project Name"] = "Missing Field";
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
    $joining_uid       = $formfields[joining_uid];
    $usr_title         = addslashes($formfields[usr_title]);
    $usr_name          = addslashes($formfields[usr_name]);
    $usr_affil         = addslashes($formfields[usr_affil]);
    $usr_email         = $formfields[usr_email];
    $usr_addr          = addslashes($formfields[usr_addr]);
    $usr_phone         = $formfields[usr_phone];
    $password1         = $formfields[password1];
    $password2         = $formfields[password2];

    if (! isset($formfields[usr_URL]) ||
	strcmp($formfields[usr_URL], "") == 0 ||
	strcmp($formfields[usr_URL], $HTTPTAG) == 0) {
	$usr_URL = "";
    }
    else {
	$usr_URL = $formfields[usr_URL];
    }

    #
    # Pasted in key.
    # 
    if (isset($formfields[usr_key]) &&
	strcmp($formfields[usr_key], "")) {
	$usr_key = $formfields[usr_key];
    }
    
    #
    # If usr provided a file for the key, it overrides the paste in text.
    # Must read and check it.
    #
    # XXX I allow only a single line of stuff. The rest is ignored for now.
    #
    if (isset($usr_keyfile) &&
	strcmp($usr_keyfile, "") &&
	strcmp($usr_keyfile, "none")) {
	$keyfilegoo = file($usr_keyfile);

	if (! ereg("^[0-9a-zA-Z\@\. ]*$", $keyfilegoo[0])) {
	    $errors["PubKey File Contents"] = "Invalid characters";
	}
	else {
	    $usr_key = $keyfilegoo[0];
	}
    }
}
else {
    #
    # Grab info from the DB for the email message below. Kinda silly.
    #
    $query_result =
	DBQueryFatal("select * from users where uid='$joining_uid'");
    
    $row = mysql_fetch_array($query_result);
    
    $usr_title	= $row[usr_title];
    $usr_name	= $row[usr_name];
    $usr_affil	= $row[usr_affil];
    $usr_email	= $row[usr_email];
    $usr_addr	= $row[usr_addr];
    $usr_phone	= $row[usr_phone];
    $usr_URL    = $row[usr_URL];
}
$pid          = $formfields[pid];
$usr_expires  = date("Y:m:d", time() + (86400 * 120));

if (isset($formfields[gid]) && strcmp($formfields[gid], "")) {
    $gid = $formfields[gid];
}
else {
    $gid = $pid;
}

if (! ereg("^[a-zA-Z][-_a-zA-Z0-9]+$", $pid) ||
    strlen($pid) > $TBDB_PIDLEN || ! TBValidProject($pid)) {
    $errors["Project Name"] = "Invalid Project Name";
}
elseif (! ereg("^[a-zA-Z][-_a-zA-Z0-9]+$", $gid) ||
	strlen($gid) > $TBDB_GIDLEN ||
	!TBValidGroup($pid, $gid)) {
    $errors["Group Name"] = "Invalid Group Name";
}
elseif (TBGroupMember($joining_uid, $pid, $gid, $approved)) {
    $errors["Membership"] = "You are already a member";
}

if (count($errors)) {
    SPITFORM($formfields, $returning, $errors);
    PAGEFOOTER();
    return;
}

#
# For a new user:
# * Create a new account in the database.
# * Add user email to the list of email address.
# * Generate a mail message to the user with the verification key.
#
if (! $returning) {
    $encoding = crypt("$password1");

    DBQueryFatal("INSERT INTO users ".
	"(uid,usr_created,usr_expires,usr_name,usr_email,usr_addr,".
	" usr_URL,usr_phone,usr_title,usr_affil,usr_pswd,unix_uid,".
	" status,pswd_expires) ".
	"VALUES ('$joining_uid', now(), '$usr_expires', '$usr_name', ".
        "'$usr_email', ".
	"'$usr_addr', '$usr_URL', '$usr_phone', '$usr_title', '$usr_affil', ".
        "'$encoding', NULL, 'newuser', ".
	"date_add(now(), interval 1 year))");

    if (isset($usr_key)) {
	DBQueryFatal("update users set home_pubkey='$usr_key' ".
		     "where uid='$joining_uid'");
    }
    
    $key = GENKEY($joining_uid);

    TBMAIL("$usr_name '$joining_uid' <$usr_email>",
	 "Your New User Key",
	 "\n".
         "Dear $usr_name ($joining_uid):\n\n".
         "This is your account verification key: $key\n\n".
         "Please return to:\n\n".
	 "         $TBWWW\n\n".
	 "and log in using the user name and password you gave us when you\n".
	 "applied. You will then find an option on the menu called\n".
	 "'New User Verification'. Select this option, and on the page\n".
	 "enter your key. You will then be verified as a user. When you \n".
	 "have been both verified and approved by the project leader, you \n".
	 "will be marked as an active user, and will be granted full access\n".
	 "to your user account.\n\n".
         "Thanks,\n".
         "Testbed Ops\n".
         "Utah Network Testbed\n",
         "From: $TBMAIL_APPROVAL\n".
         "Bcc: $TBMAIL_AUDIT\n".
         "Errors-To: $TBMAIL_WWW");
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

#
# The mail message to the approval list.
#
TBMAIL("$leader_name '$leader_uid' <$leader_email>",
     "$joining_uid $pid Project Join Request",
     "$usr_name is trying to join your group $gid in project $pid.\n".
     "\n".
     "Contact Info:\n".
     "Name:            $usr_name\n".
     "Emulab ID:       $joining_uid\n".
     "Email:           $usr_email\n".
     "User URL:        $usr_URL\n".
     "Title:           $usr_title\n".
     "Affiliation:     $usr_affil\n".
     "Address:         $usr_addr\n".
     "Phone:           $usr_phone\n".
     "\n".
     "Please return to $TBWWW,\n".
     "log in, and select the 'New User Approval' page to enter your\n".
     "decision regarding $usr_name's membership in your project.\n\n".
     "Thanks,\n".
     "Testbed Ops\n".
     "Utah Network Testbed\n",
     "From: $TBMAIL_APPROVAL\n".
     "Bcc: $TBMAIL_AUDIT\n".
     "Errors-To: $TBMAIL_WWW");

#
# Spit out a redirect so that the history does not include a post
# in it. The back button skips over the post and to the form.
# See above for conclusion.
# 
header("Location: joinproject.php3?finished=1");

?>
