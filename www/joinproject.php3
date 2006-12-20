<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2005, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# No PAGEHEADER since we spit out a Location header later. See below.
#

#
# Get current user.
#
$this_user = CheckLogin($check_status);

#
# If a uid came in, then we check to see if the login is valid.
# We require that the user be logged in to start a second project.
#
if ($this_user) {
    # Allow unapproved users to join multiple groups ...
    # Must be verified though.
    CheckLoginOrDie(CHECKLOGIN_UNAPPROVED|
		    CHECKLOGIN_WEBONLY|CHECKLOGIN_WIKIONLY);
    $joining_uid = $this_user->uid();
    $returning = 1;
}
else {
    #
    # No uid, so must be new.
    #
    $returning = 0;
}

if (!isset($forwikionly)) {
    $forwikionly = 0;
}
unset($addpubkeyargs);

$ACCOUNTWARNING =
    "Before continuing, please make sure your username " .
    "reflects your normal login name. ".
    "Emulab accounts are not to be shared amongst users!";

$EMAILWARNING =
    "Before continuing, please make sure the email address you have ".
    "provided is current and non-pseudonymic. Redirections and anonymous ".
    "email addresses are not allowed.";

#
# Spit the form out using the array of data. 
# 
function SPITFORM($formfields, $returning, $errors)
{
    global $TBDB_UIDLEN, $TBDB_PIDLEN, $TBDB_GIDLEN;
    global $ACCOUNTWARNING, $EMAILWARNING;
    global $WIKISUPPORT, $forwikionly, $WIKIHOME, $USERSELECTUIDS;

    if ($forwikionly)
	PAGEHEADER("Wiki Registration");
    else
	PAGEHEADER("Apply for Project Membership");

    if (! $returning) {
	echo "<center>\n";

	if ($forwikionly) {
	    echo "<font size=+2>Register for an Emulab Wiki account</font>
                  <br><br>\n";
	}
        echo "<font size=+1>
               If you already have an Emulab account,
               <a href=login.php3?refer=1>
               <font color=red>please log on first!</font></a>
              </font>\n";
	if ($forwikionly) {
	    echo "<br>(You will already have a wiki account)\n";
	}
	echo "</center><br>\n";	
    }
    elseif ($forwikionly) {
	USERERROR("You already have a Wiki account!", 1);
    }

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
    echo "<SCRIPT LANGUAGE=JavaScript>
              function SetWikiName(theform) 
              {
	          var validchars = 'abcdefghijklmnopqrstuvwxyz0123456789';
                  var usrname    = theform['formfields[usr_name]'].value;
                  var wikiname   = '';
                  var docap      = 1;

		  for (var i = 0; i < usrname.length; i++) {
                      var letter = usrname.charAt(i).toLowerCase();

                      if (validchars.indexOf(letter) == -1) {
                          if (letter == ' ') {
                              docap = 1;
                          }
                          continue;
                      }
                      else {
                          if (docap == 1) {
                              letter = usrname.charAt(i).toUpperCase()
                              docap  = 0;
                          }
                          wikiname = wikiname + letter;
                      }
                  }
                  theform['formfields[wikiname]'].value = wikiname;
              }
          </SCRIPT>\n";

    echo "<table align=center border=1> 
          <tr>
            <td align=center colspan=3>
                Fields marked with * are required.
            </td>
          </tr>\n

          <form name=myform enctype=multipart/form-data
                action=" . ($forwikionly ?
			    "wikiregister.php3" : "joinproject.php3") . " " .
	        "method=post>\n";

    if (! $returning) {
	if ($USERSELECTUIDS) {
            #
            # UID.
            #
	    echo "<tr>
                      <td colspan=2>*<a
                             href='docwrapper.php3?docname=security.html'
                             target=_blank>Username</a>
                                (alphanumeric, lowercase):</td>
                      <td class=left>
                          <input type=text
                                 name=\"formfields[joining_uid]\"
                                 value=\"" . $formfields[joining_uid] . "\"
	                         size=$TBDB_UIDLEN
                                 onchange=\"alert('$ACCOUNTWARNING')\"
	                         maxlength=$TBDB_UIDLEN>
                      </td>
                  </tr>\n";
	}

	#
	# Full Name
	#
        echo "<tr>
                  <td colspan=2>*Full Name (first and last):</td>
                  <td class=left>
                      <input type=text
                             name=\"formfields[usr_name]\"
                             onchange=\"SetWikiName(myform);\"
                             value=\"" . $formfields[usr_name] . "\"
	                     size=30>
                  </td>
              </tr>\n";

	#
	# WikiName
	#
	if ($WIKISUPPORT) {
	    echo "<tr>
                      <td colspan=2>*<a
                            href=${WIKIHOME}/bin/view/TWiki/WikiName
                            target=_blank>WikiName</a>:<td class=left>
                          <input type=text
                                 name=\"formfields[wikiname]\"
                                 value=\"" . $formfields[wikiname] . "\"
	                         size=30>
                      </td>
                  </tr>\n";
	}

	if (! $forwikionly) {
            #
            # Title/Position:
	    #
	    echo "<tr>
                      <td colspan=2>*Job Title/Position:</td>
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
	}

	#
	# Email:
	#
	echo "<tr>
                  <td colspan=2>*Email Address[<b>1</b>]:</td>
                  <td class=left>
                      <input type=text
                             name=\"formfields[usr_email]\"
                             value=\"" . $formfields[usr_email] . "\"
                             onchange=\"alert('$EMAILWARNING')\"
	                     size=30>
                  </td>
              </tr>\n";

	if (! $forwikionly) {
	    #
	    # Postal Address
	    #
	    echo "<tr><td colspan=3>*Postal Address:<br /><center>
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
                                 value=\"" . $_FILES['usr_keyfile']['name'] .
		                       "\"
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
	}

	#
	# Password. Note that we do not resend the password. User
	# must retype on error.
	#
	echo "<tr>
                  <td colspan=2>*Password[<b>1</b>]:</td>
                  <td class=left>
                      <input type=password
                             name=\"formfields[password1]\"
                             value=\"$formfields[password1]\"
                             size=8></td>
              </tr>\n";

        echo "<tr>
                  <td colspan=2>*Retype Password:</td>
                  <td class=left>
                      <input type=password
                             name=\"formfields[password2]\"
                             value=\"$formfields[password2]\"
                             size=8></td>
             </tr>\n";
    }

    if (! $forwikionly) {
        #
        # Project Name:
        #
	echo "<tr>
                  <td colspan=2>*Project Name:</td>
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
                 <a href = 'docwrapper.php3?docname=security.html' target='_blank'>
                 security policies</a> for information
                 regarding passwords and email addresses.\n";
    if (!$returning && !$forwikionly) {
	echo "<li> If you want us to use your existing ssh public key,
                   then either paste it in or specify the path to your
                   your identity.pub file.  <font color=red>NOTE:</font>
                   We use the <a href=http://www.openssh.org target='_blank'>OpenSSH</a>
                   key format,
                   which has a slightly different protocol 2 public key format
                   than some of the commercial vendors such as
                   <a href=http://www.ssh.com target='_blank'>SSH Communications</a>. If you
                   use one of these commercial vendors, then please
                   upload the public  key file and we will convert it
                   for you. <i>Please do not paste it in.</i>\n

              <li> Note to <a href=http://www.opera.com target='_blank'><b>Opera 5</b></a>
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
if (isset($_GET['finished'])) {
    if ($forwikionly) 
	PAGEHEADER("Wiki Registration");
    else
	PAGEHEADER("Apply for Project Membership");

    #
    # Generate some warm fuzzies.
    #
    if ($forwikionly) {
	echo "An email message has been sent to your account so we may verify
              your email address. Please follow the instructions contained in
              that message, which will verify your account, and grant you
              access to the Wiki.\n";
    }
    elseif (! $returning) {
	echo "<p>
              As a pending user of the Testbed you will receive a key via email.
              When you receive the message, please follow the instructions
              contained in the message, which will verify your identity.
	      <br>
	      <p>
	      When you have done that, the project leader will be
	      notified of your application. ";
    }
    else {
          echo "<p>
	  	The project leader has been notified of your application. ";
    }

    echo "He/She will make a decision and either approve or deny your
          application, and you will be notified via email as soon as
	  that happens.\n";

    PAGEFOOTER();
    return;
}

#
# On first load, display a virgin form and exit.
#
if (! isset($_POST['submit'])) {
    $defaults = array();
    $defaults[usr_URL] = "$HTTPTAG";
    $defaults[usr_country] = "USA";

    #
    # These two allow presetting the pid/gid.
    # 
    if (isset($target_pid) && strcmp($target_pid, "")) {
	$defaults[pid] = $target_pid;
    }
    if (isset($target_gid) && strcmp($target_gid, "")) {
	$defaults[gid] = $target_gid;
    }
    
    SPITFORM($defaults, $returning, 0);
    PAGEFOOTER();
    return;
}
else {
    # Form submitted. Make sure we have a formfields array.
    if (!isset($_POST['formfields']) ||
	!is_array($_POST['formfields'])) {
	PAGEARGERROR("Invalid form arguments.");
    }
    $formfields = $_POST['formfields'];
}

#
# Otherwise, must validate and redisplay if errors
#
$errors = array();

#
# These fields are required!
#
if (! $returning) {
    if ($USERSELECTUIDS) {
	if (!isset($formfields[joining_uid]) ||
	    strcmp($formfields[joining_uid], "") == 0) {
	    $errors["Username"] = "Missing Field";
	}
	elseif (!TBvalid_uid($formfields[joining_uid])) {
	    $errors["UserName"] = TBFieldErrorString();
	}
	elseif (User::Lookup($formfields[joining_uid]) ||
		posix_getpwnam($formfields[joining_uid])) {
	    $errors["UserName"] = "Already in use. Pick another";
	}
    }
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
    if ($WIKISUPPORT) {
	if (!isset($formfields[wikiname]) ||
	    strcmp($formfields[wikiname], "") == 0) {
	    $errors["WikiName"] = "Missing Field";
	}
	elseif (! TBvalid_wikiname($formfields[wikiname])) {
	    $errors["WikiName"] = TBFieldErrorString();
	}
	elseif (User::LookupByWikiName($formfields[wikiname])) {
	    $errors["WikiName"] = "Already in use. Pick another";
	}
    }
    if (!$forwikionly) {
	if (!isset($formfields[usr_title]) ||
	    strcmp($formfields[usr_title], "") == 0) {
	    $errors["Job Title/Position"] = "Missing Field";
	}
	elseif (! TBvalid_title($formfields[usr_title])) {
	    $errors["Job Title/Position"] = TBFieldErrorString();
	}
	if (!isset($formfields[usr_affil]) ||
	    strcmp($formfields[usr_affil], "") == 0) {
	    $errors["Affiliation"] = "Missing Field";
	}
	elseif (! TBvalid_affiliation($formfields[usr_affil])) {
	    $errors["Affiliation"] = TBFieldErrorString();
	}
    }	
    if (!isset($formfields[usr_email]) ||
	strcmp($formfields[usr_email], "") == 0) {
	$errors["Email Address"] = "Missing Field";
    }
    elseif (! TBvalid_email($formfields[usr_email])) {
	$errors["Email Address"] = TBFieldErrorString();
    }
    elseif (User::LookupByEmail($formfields[usr_email])) {
	$errors["Email Address"] =
	    "Already in use. <b>Did you forget to login?</b>";
    }
    if (! $forwikionly) {
	if (isset($formfields[usr_URL]) &&
	    strcmp($formfields[usr_URL], "") &&
	    strcmp($formfields[usr_URL], $HTTPTAG) &&
	    ! CHECKURL($formfields[usr_URL], $urlerror)) {
	    $errors["Home Page URL"] = $urlerror;
	}
	if (!isset($formfields[usr_addr]) ||
	    strcmp($formfields[usr_addr], "") == 0) {
	    $errors["Address 1"] = "Missing Field";
	}
	elseif (! TBvalid_addr($formfields[usr_addr])) {
	    $errors["Address 1"] = TBFieldErrorString();
	}
        # Optional
	if (isset($formfields[usr_addr2]) &&
	    !TBvalid_addr($formfields[usr_addr2])) {
	    $errors["Address 2"] = TBFieldErrorString();
	}
	if (!isset($formfields[usr_city]) ||
	    strcmp($formfields[usr_city], "") == 0) {
	    $errors["City"] = "Missing Field";
	}
	elseif (! TBvalid_city($formfields[usr_city])) {
	    $errors["City"] = TBFieldErrorString();
	}
	if (!isset($formfields[usr_state]) ||
	    strcmp($formfields[usr_state], "") == 0) {
	    $errors["State"] = "Missing Field";
	}
	elseif (! TBvalid_state($formfields[usr_state])) {
	    $errors["State"] = TBFieldErrorString();
	}
	if (!isset($formfields[usr_zip]) ||
	    strcmp($formfields[usr_zip], "") == 0) {
	    $errors["ZIP/Postal Code"] = "Missing Field";
	}
	elseif (! TBvalid_zip($formfields[usr_zip])) {
	    $errors["Zip/Postal Code"] = TBFieldErrorString();
	}
	if (!isset($formfields[usr_country]) ||
	    strcmp($formfields[usr_country], "") == 0) {
	    $errors["Country"] = "Missing Field";
	}
	elseif (! TBvalid_country($formfields[usr_country])) {
	    $errors["Country"] = TBFieldErrorString();
	}
	if (!isset($formfields[usr_phone]) ||
	    strcmp($formfields[usr_phone], "") == 0) {
	    $errors["Phone #"] = "Missing Field";
	}
	elseif (!TBvalid_phone($formfields[usr_phone])) {
	    $errors["Phone #"] = TBFieldErrorString();
	}
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
    elseif (! CHECKPASSWORD(($USERSELECTUIDS ?
			     $formfields[joining_uid] : "ignored"),
			    $formfields[password1],
			    $formfields[usr_name],
			    $formfields[usr_email], $checkerror)) {
	$errors["Password"] = "$checkerror";
    }
}
if (!$forwikionly) {
    if (!isset($formfields[pid]) || $formfields[pid] == "") {
	$errors["Project Name"] = "Missing Field";
    }
    else {
        # Confirm pid/gid early to avoid spamming the page.
	$pid = $formfields[pid];

	if (isset($formfields[gid]) && $formfields[gid] != "") {
	    $gid = $formfields[gid];
	}
	else {
	    $gid = $pid;
	}

	if (!TBvalid_pid($pid) || !TBValidProject($pid)) {
	    $errors["Project Name"] = "Invalid Project Name";
	}
	elseif (!TBvalid_gid($gid) || !TBValidGroup($pid, $gid)) {
	    $errors["Group Name"] = "Invalid Group Name";
	}
    }
}

# Present these errors before we call out to do pubkey stuff; saves work.
if (count($errors)) {
    SPITFORM($formfields, $returning, $errors);
    PAGEFOOTER();
    return;
}

# Okay, do pubkey checks.
if (!$returning && !$forwikionly) {
    #
    # Pub key provided in form (paste in).
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

            #
            # Verify key format.
            #
	    if (ADDPUBKEY(null, "webaddpubkey -n -k '$usr_key' ")) {
		$errors["Pubkey Format"] =
		    "Could not be parsed. Is it a public key?";
	    }
	    else {
		$addpubkeyargs = "-k '$usr_key' ";
	    }
	}
    }

    #
    # If usr provided a file for the key, it overrides the paste in text.
    #
    if (isset($_FILES['usr_keyfile']) &&
	$_FILES['usr_keyfile']['name'] != "" &&
	$_FILES['usr_keyfile']['name'] != "none") {

	$localfile = $_FILES['usr_keyfile']['tmp_name'];

	if (! stat($localfile)) {
	    $errors["PubKey File"] = "No such file";
	}
        # Taint check shell arguments always! 
	elseif (! preg_match("/^[-\w\.\/]*$/", $localfile)) {
	    $errors["PubKey File"] = "Invalid characters";
	}
	else {
	    chmod($localfile, 0644);

            #
            # Verify key format.
            #
	    if (ADDPUBKEY(null, "webaddpubkey -n $localfile ")) {
		$errors["Pubkey Format"] =
		    "Could not be parsed. Is it a public key?";
	    }
	    else {
		$addpubkeyargs = "$localfile";
	    }
	}
    }
}

#
# Need the user, project and group objects for the rest of this.
#
if (!$forwikionly) {
    if (! ($project = Project::Lookup($pid))) {
	TBERROR("Could not lookup object for $pid!", 1);
    }
    if (! ($group = Group::LookupByPidGid($pid, $gid))) {
	TBERROR("Could not lookup object for $pid/$gid!", 1);
    }
    if ($returning) {
	$user = $this_user;
	if ($group->IsMember($user, $ignore)) {
	    $errors["Membership"] = "You are already a member";
	}
    }
}

# Done with sanity checks!
if (count($errors)) {
    SPITFORM($formfields, $returning, $errors);
    PAGEFOOTER();
    return;
}

#
# Create a new user.
#
if (! $returning) {
    #
    # Certain of these values must be escaped or otherwise sanitized.
    #
    $joining_uid       = ($USERSELECTUIDS ? $formfields[joining_uid] : null);
    $usr_name          = addslashes($formfields[usr_name]);
    $usr_email         = $formfields[usr_email];
    $password1         = $formfields[password1];
    $password2         = $formfields[password2];
    $wikiname          = ($WIKISUPPORT ? $formfields[wikiname] : "");

    if (!$forwikionly) {
	$usr_affil         = addslashes($formfields[usr_affil]);
	$usr_title         = addslashes($formfields[usr_title]);
	$usr_addr          = addslashes($formfields[usr_addr]);
	$usr_city          = addslashes($formfields[usr_city]);
	$usr_state         = addslashes($formfields[usr_state]);
	$usr_zip           = addslashes($formfields[usr_zip]);
	$usr_country       = addslashes($formfields[usr_country]);
	$usr_phone         = $formfields[usr_phone];
    }
    else {
	$usr_affil         = "";
	$usr_title         = "";
	$usr_addr          = "";
	$usr_city          = "";
	$usr_state         = "";
	$usr_zip           = "";
	$usr_country       = "";
	$usr_phone         = "";
    }

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

    $args = array();
    $args["usr_expires"]   = date("Y:m:d", time() + (86400 * 120));
    $args["usr_name"]	   = $usr_name;
    $args["usr_email"]     = $usr_email;
    $args["usr_addr"]      = $usr_addr;
    $args["usr_addr2"]     = $usr_addr2;
    $args["usr_city"]      = $usr_city;
    $args["usr_state"]     = $usr_state;
    $args["usr_zip"]       = $usr_zip;
    $args["usr_country"]   = $usr_country;
    $args["usr_URL"]       = $usr_URL;
    $args["usr_phone"]     = $usr_phone;
    $args["usr_shell"]     = 'tcsh';
    $args["usr_title"]     = $usr_title;
    $args["usr_affil"]     = $usr_affil;
    $args["usr_pswd"]      = crypt("$password1");
    $args["wikiname"]      = $wikiname;

    if (! ($user = User::NewUser($joining_uid,
				 TBDB_NEWACCOUNT_WIKIONLY, $args))) {
	TBERROR("Could not create new user '$usr_email'!", 1);
    }
    $joining_uid = $user->uid();

    if (!$forwikionly && isset($addpubkeyargs)) {
	ADDPUBKEY($joining_uid, "webaddpubkey -u $joining_uid $addpubkeyargs");
    }
}

#
# For wikionly registration, we are done.
# 
if ($forwikionly) {
    header("Location: wikiregister.php3?finished=1");
    exit();
}

#
# If joining a subgroup, also add to project group.
#
if ($pid != $gid && ! $project->IsMember($user, $ignore)) {
    if ($project->AddNewMember($user) < 0) {
	TBERROR("Could not add user $joining_uid to project group $pid", 1);
    }
}

#
# Add to the group, but with trust=none. The project/group leader will have
# to upgrade the trust level, making the new user real.
#
if ($group->AddNewMember($user) < 0) {
    TBERROR("Could not add user $joining_uid to group $pid/$gid", 1);
}

#
# Generate an email message to the proj/group leaders.
#
if ($returning) {
    $group->NewMemberNotify($user);
}

#
# Spit out a redirect so that the history does not include a post
# in it. The back button skips over the post and to the form.
# See above for conclusion.
# 
header("Location: joinproject.php3?finished=1");
