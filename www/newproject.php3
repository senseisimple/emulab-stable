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
$uid = GETLOGIN();

#
# See if we are in an initial Emulab setup.
#
$FirstInitState = (TBGetFirstInitState() == "createproject");

#
# If a uid came in, then we check to see if the login is valid.
# If the login is not valid. We require that the user be logged in
# to start a second project.
#
if ($uid && !$FirstInitState) {
    # Allow unapproved users to create multiple projects ...
    # Must be verified though.
    LOGGEDINORDIE($uid, CHECKLOGIN_UNAPPROVED|CHECKLOGIN_WEBONLY);
    $proj_head_uid = $uid;
    $returning = 1;
}
else {
    #
    # No uid, so must be new.
    #
    $returning = 0;
}

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
    global $TBDB_UIDLEN, $TBDB_PIDLEN, $TBDOCBASE, $WWWHOST;
    global $usr_keyfile, $FirstInitState;
    global $ACCOUNTWARNING, $EMAILWARNING;
    global $WIKISUPPORT, $WIKIHOME;
    
    PAGEHEADER("Start a New Testbed Project");

    #
    # First initialization gets different text
    #
    if ($FirstInitState == "createproject") {
	echo "<center><font size=+1>
	      Please create your initial project.<br> A good Project Name
              for your first project is probably 'testbed', but you can
              choose anything you like.
              </font></center><br>\n";
    }
    else {
	echo "<center><font size=+1>
                 If you are a <font color=red>student
                 (undergrad or graduate)</font>, please
                 do not try to start a project! <br>Your advisor must do it.
                 <a href=docwrapper.php3?docname=auth.html target='_blank'>
                 Read this for more info.</a>
              </font></center><br>\n";

	if (! $returning) {
	    echo "<center><font size=+1>
                   If you already have an Emulab account,
                   <a href=login.php3?refer=1>
                   <font color=red>please log on first!</font></a>
                   </font></center><br>\n";
	}
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

          <form enctype=multipart/form-data name=myform
                action=newproject.php3 method=post>\n";

    if (! $returning) {
        #
        # Start user information stuff. Presented for new users only.
        #
	echo "<tr>
                  <th colspan=3>
                      Project Head Information:&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp
                      <font size=-2>
                       (Prospective project leaders please read our
                       <a href='docwrapper.php3?docname=policies.html' target='_blank'>
                       Administrative Policies</a>)</font>
                  </th>
              </tr>\n";

        #
        # UserName:
        #
        echo "<tr>
                  <td colspan=2>*<a
                         href='docwrapper.php3?docname=security.html'
                         target=_blank>Username</a>
                            (alphanumeric, lowercase):</td>
                  <td class=left>
                      <input type=text
                             name=\"formfields[proj_head_uid]\"
                             value=\"" . $formfields[proj_head_uid] . "\"
	                     size=$TBDB_UIDLEN
                             onchange=\"alert('$ACCOUNTWARNING')\"
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
                             onchange=\"SetWikiName(myform);\"
	                     size=30>
                  </td>
              </tr>\n";

	#
	# WikiName
	#
	if ($WIKISUPPORT) {
	    echo "<tr>
                      <td colspan=2>*
                          <a href=${WIKIHOME}/bin/view/TWiki/WikiName
                            target=_blank>WikiName</a>:<td class=left>
                          <input type=text
                                 name=\"formfields[wikiname]\"
                                 value=\"" . $formfields[wikiname] . "\"
	                         size=30>
                      </td>
                  </tr>\n";
	}

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
                             value=\"" . $_FILES['usr_keyfile']['name'] . "\"
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
    echo "<tr><th colspan=3>
               Project Information: 
               <!-- <em>(replace the example entries)</em> -->
              </th>
          </tr>\n";

    #
    # Project Name:
    #
    echo "<tr>
              <td colspan=2>*Project Name (alphanumeric):</td>
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
                             to <a href=\"$TBDOCBASE\" target='_blank'>$WWWHOST</a>?
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
        <a href=\"$TBDOCBASE/docwrapper.php3?docname=hardware.html#tbpcs\" target='_blank'>
                             PCs</a>:</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[proj_pcs]\"
                         value=\"" . $formfields[proj_pcs] . "\"
                         size=4>
              </td>
          </tr>\n";

    echo "<tr>
              <td colspan=2>Request Access to 
        <a href=\"$TBDOCBASE/docwrapper.php3?docname=widearea.html\" target='_blank'>
                             Planetlab PCs</a>:</td>
              <td class=left>
                  <input type=checkbox value=checked
                         name=\"formfields[proj_plabpcs]\"
                         " . $formfields[proj_plabpcs] . ">
                         Yes &nbsp
              </td>
          </tr>\n";

    echo "<tr>
              <td colspan=2>Request Access to 
        <a href=\"$TBDOCBASE/docwrapper.php3?docname=widearea.html\" target='_blank'>
                             wide-area PCs</a>:</td>
              <td class=left>
                  <input type=checkbox value=checked
                         name=\"formfields[proj_ronpcs]\"
                         " . $formfields[proj_ronpcs] . ">
                         Yes &nbsp
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
                 <a href = 'docwrapper.php3?docname=security.html' target='_blank'>
                 security policies</a> for information
                 regarding passwords and email addresses.\n";
    if (! $returning) {
	echo "<li> If you want us to use your existing ssh public key,
                   then either paste it in or specify the path to your
                   your identity.pub file. <font color=red>NOTE:</font>
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
# The conclusion of a newproject request. See below.
# 
if (isset($_GET['finished'])) {
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
if (! isset($_POST['submit'])) {
    $defaults = array();
    $defaults[proj_URL] = "$HTTPTAG";
    $defaults[usr_URL] = "$HTTPTAG";
    $defaults[usr_country] = "USA";
    $defaults[proj_ronpcs]  = "";
    $defaults[proj_plabpcs] = "";
    $defaults[proj_public] = "checked";
    $defaults[proj_linked] = "checked";

    if ($FirstInitState == "createproject") {
	$defaults[pid]          = "testbed";
	$defaults[proj_pcs]     = "256";
	$defaults[proj_members] = "256";
	$defaults[proj_funders] = "none";
	$defaults[proj_name]    = "Your Testbed Project";
	$defaults[proj_why]     = "This project is used for testbed ".
	    "administrators to develop and test new software. ";
    }
    
    SPITFORM($defaults, $returning, 0);
    PAGEFOOTER();
    return;
}
else {
    # Form submitted. Make sure we have a formfields array and a target_uid.
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
    if (!isset($formfields[proj_head_uid]) ||
	strcmp($formfields[proj_head_uid], "") == 0) {
	$errors["Username"] = "Missing Field";
    }
    elseif (!TBvalid_uid($formfields[proj_head_uid])) {
	$errors["UserName"] = TBFieldErrorString();
    }
    elseif (TBCurrentUser($formfields[proj_head_uid]) ||
	    posix_getpwnam($formfields[proj_head_uid])) {
	$errors["UserName"] = "Already in use. Pick another";
    }
    if (!isset($formfields[usr_title]) ||
	strcmp($formfields[usr_title], "") == 0) {
	$errors["Job Title/Position"] = "Missing Field";
    }
    elseif (! TBvalid_title($formfields[usr_title])) {
	$errors["Job Title/Position"] = TBFieldErrorString();
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
	elseif (TBCurrentWikiName($formfields[wikiname])) {
	    $errors["WikiName"] = "Already in use. Pick another";
	}
    }
    if (!isset($formfields[usr_affil]) ||
	strcmp($formfields[usr_affil], "") == 0) {
	$errors["Affiliation"] = "Missing Field";
    }
    elseif (! TBvalid_affiliation($formfields[usr_affil])) {
	$errors["Affiliation"] = TBFieldErrorString();
    }
    if (!isset($formfields[usr_email]) ||
	strcmp($formfields[usr_email], "") == 0) {
	$errors["Email Address"] = "Missing Field";
    }
    elseif (! TBvalid_email($formfields[usr_email])) {
	$errors["Email Address"] = TBFieldErrorString();
    }
    elseif (TBCurrentEmail($formfields[usr_email])) {
        #
        # Treat this error separate. Not allowed.
        #
	PAGEHEADER("Start a New Testbed Project");
	USERERROR("The email address '$formfields[usr_email]' is already in ".
		  "use by another user.<br>Perhaps you have ".
		  "<a href='password.php3?email=$formfields[usr_email]'>".
		  "forgotten your username.</a>", 1);
    }
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
    if (!TBvalid_newpid($formfields[pid])) {
	$errors["Project Name"] = TBFieldErrorString();
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
elseif (! TBvalid_description($formfields[proj_name])) {
    $errors["Project Description"] = TBFieldErrorString();
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
elseif (! TBvalid_description($formfields[proj_funders])) {
    $errors["Funding Sources"] = TBFieldErrorString();
}
if (!isset($formfields[proj_members]) ||
    strcmp($formfields[proj_members], "") == 0) {
    $errors["#of Members"] = "Missing Field";
}
elseif (! TBvalid_num_members($formfields[proj_members])) {
    $errors["#of Members"] = TBFieldErrorString();
}
if (!isset($formfields[proj_pcs]) ||
    strcmp($formfields[proj_pcs], "") == 0) {
    $errors["#of PCs"] = "Missing Field";
}
elseif (! TBvalid_num_pcs($formfields[proj_pcs])) {
    $errors["#of PCs"] = TBFieldErrorString();
}

if (isset($formfields[proj_plabpcs]) &&
    strcmp($formfields[proj_plabpcs], "") &&
    strcmp($formfields[proj_plabpcs], "checked")) {
    $errors["Planetlab Access"] = "Bad Value";
}
if (isset($formfields[proj_ronpcs]) &&
    strcmp($formfields[proj_ronpcs], "") &&
    strcmp($formfields[proj_ronpcs], "checked")) {
    $errors["Ron Access"] = "Bad Value";
}
if (!isset($formfields[proj_why]) ||
    strcmp($formfields[proj_why], "") == 0) {
    $errors["How and Why?"] = "Missing Field";
}
elseif (! TBvalid_why($formfields[proj_why])) {
    $errors["How and Why?"] = TBFieldErrorString();
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
    $usr_city          = addslashes($formfields[usr_city]);
    $usr_state         = addslashes($formfields[usr_state]);
    $usr_zip           = addslashes($formfields[usr_zip]);
    $usr_country       = addslashes($formfields[usr_country]);
    $usr_phone         = $formfields[usr_phone];
    $password1         = $formfields[password1];
    $password2         = $formfields[password2];
    $wikiname          = ($WIKISUPPORT ? $formfields[wikiname] : "");
    $usr_returning     = "No";

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
	    $addpubkeyargs = "$proj_head_uid $usr_keyfile";
	    chmod($usr_keyfile, 0644);	
	}
    }
    #
    # Verify key format.
    #
    if (isset($addpubkeyargs) &&
	ADDPUBKEY($proj_head_uid, "webaddpubkey -n $addpubkeyargs")) {
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
    $usr_addr2     = $row[usr_addr2];
    $usr_city	   = $row[usr_city];
    $usr_state	   = $row[usr_state];
    $usr_zip	   = $row[usr_zip];
    $usr_country   = $row[usr_country];
    $usr_phone	   = $row[usr_phone];
    $usr_URL       = $row[usr_URL];
    $wikiname      = $row[wikiname];
    $usr_returning = "Yes";
}
$pid               = $formfields[pid];
$proj_name	   = addslashes($formfields[proj_name]);
$proj_URL          = addslashes($formfields[proj_URL]);
$proj_funders      = addslashes($formfields[proj_funders]);
$proj_whynotpublic = addslashes($formfields[proj_whynotpublic]);
$proj_members      = $formfields[proj_members];
$proj_pcs          = $formfields[proj_pcs];
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
    $linked = 0;
}
else {
    $proj_linked = "Yes";
    $linked = 1;
}
if (isset($formfields[proj_plabpcs]) &&
    $formfields[proj_plabpcs] == "checked") {
    $proj_plabpcs = "Yes";
    $plabpcs = 1;
}
else {
    $proj_plabpcs = "No";
    $plabpcs = 0;
}
if (isset($formfields[proj_ronpcs]) &&
    $formfields[proj_ronpcs] == "checked") {
    $proj_ronpcs = "Yes";
    $ronpcs = 1;
}
else {
    $proj_ronpcs = "No";
    $ronpcs = 0;
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
    $encoding    = crypt("$password1");

    #
    # Must be done before user record is inserted!
    # XXX Since, user does not exist, must run as nobody. Script checks. 
    # 
    if (isset($addpubkeyargs)) {
	ADDPUBKEY($proj_head_uid, "webaddpubkey $addpubkeyargs");
    }
    # Initial mailman_password.
    $mailman_password = substr(GENHASH(), 0, 10);

    # Unique Unix UID.
    $unix_uid = TBGetUniqueIndex('next_uid');

    DBQueryFatal("INSERT INTO users ".
	 "(uid,usr_created,usr_expires,usr_name,usr_email,usr_addr,".
	 " usr_addr2,usr_city,usr_state,usr_zip,usr_country, ".
	 " usr_URL,usr_title,usr_affil,usr_phone,usr_shell,usr_pswd,unix_uid,".
	 " status,pswd_expires,usr_modified,wikiname,mailman_password) ".
	 "VALUES ('$proj_head_uid', now(), '$proj_expires', '$usr_name', ".
         "'$usr_email', ".
	 "'$usr_addr', '$usr_addr2', '$usr_city', '$usr_state', '$usr_zip', ".
	 "'$usr_country', ".
	 "'$usr_URL', '$usr_title', '$usr_affil', ".
	 "'$usr_phone', 'tcsh', '$encoding', $unix_uid, 'newuser', ".
	 "date_add(now(), interval 1 year), now(), '$wikiname', ".
	 "'$mailman_password')");

    DBQueryFatal("INSERT INTO user_stats (uid, uid_idx) ".
		 "VALUES ('$proj_head_uid', $unix_uid)");

    if (! $FirstInitState) {
	$key = TBGenVerificationKey($proj_head_uid);

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
	   "Testbed Operations\n",
	   "From: $TBMAIL_APPROVAL\n".
	   "Bcc: $TBMAIL_AUDIT\n".
	   "Errors-To: $TBMAIL_WWW");
    }
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
	     " num_pcplab, num_ron, public, public_whynot, linked_to_us)".
	     "VALUES ('$pid', now(), '$proj_expires','$proj_name', ".
	     "        '$proj_URL', '$proj_head_uid', '$proj_members', ".
	     "        '$proj_pcs', '$proj_why', ".
	     "        '$proj_funders', NULL, $plabpcs, $ronpcs, ".
	     "         $public, '$proj_whynotpublic', $linked)");

DBQueryFatal("INSERT INTO project_stats (pid) VALUES ('$pid')");

DBQueryFatal("INSERT INTO groups ".
	     "(pid, gid, leader, created, description, unix_gid, unix_name) ".
	     "VALUES ('$pid', '$pid', '$proj_head_uid', now(), ".
	     "        'Default Group', NULL, '$pid')");

DBQueryFatal("INSERT INTO group_stats (pid, gid) VALUES ('$pid', '$pid')");

DBQueryFatal("insert into group_membership ".
	     "(uid, gid, pid, trust, date_applied) ".
	     "values ('$proj_head_uid','$pid','$pid','none', now())");

#
# If a new user, do not send the full blown message until verified.
#
if ($returning || $FirstInitState) {
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
	   "Job Title:       $usr_title\n".
	   "Affiliation:     $usr_affil\n".
	   "Address 1:       $usr_addr\n".
	   "Address 2:       $usr_addr2\n".
	   "City:            $usr_city\n".
	   "State:           $usr_state\n".
	   "ZIP/Postal Code: $usr_zip\n".
	   "Country:         $usr_country\n".
	   "Phone:           $usr_phone\n".
	   "Members:         $proj_members\n".
	   "PCs:             $proj_pcs\n".
	   "Planetlab PCs:   $proj_plabpcs\n".
	   "RON PCs:         $proj_ronpcs\n".
	   "Unix GID:        $unix_name ($unix_gid)\n".
	   "Reasons:\n$proj_why\n\n".
	   "Please review the application and when you have made a \n".
	   "decision, go to $TBWWW and\n".
	   "select the 'Project Approval' page.\n\n".
	   "They are expecting a result within 72 hours.\n", 
	   "From: $usr_name '$proj_head_uid' <$usr_email>\n".
	   "Reply-To: $TBMAIL_APPROVAL\n".
	   "Errors-To: $TBMAIL_WWW");
}
else {
    TBMAIL($TBMAIL_APPROVAL,
	   "New Project '$pid' ($proj_head_uid)",
	   "'$usr_name' wants to start project '$pid'.\n".
	   "\n".
	   "Name:            $usr_name ($proj_head_uid)\n".
	   "Returning User?: No\n".
	   "\n".
	   "No action is necessary until the user has verified the account.\n",
	   "From: $usr_name '$proj_head_uid' <$usr_email>\n".
	   "Reply-To: $TBMAIL_APPROVAL\n".
	   "Errors-To: $TBMAIL_WWW");
}

if ($FirstInitState) {
    #
    # The first user gets admin status and some extra groups, etc.
    #
    DBQueryFatal("update users set ".
		 "  admin=1,status='". TBDB_USERSTATUS_UNAPPROVED . "' " .
		 "where uid='$proj_head_uid'");

    DBQueryFatal("insert into unixgroup_membership set ".
		 "uid='$proj_head_uid', gid='wheel'");
    
    DBQueryFatal("insert into unixgroup_membership set ".
		 "uid='$proj_head_uid', gid='$TBADMINGROUP'");
    
    DBQueryFatal("insert into group_membership ".
		 "(uid, gid, pid, trust, date_applied) ".
		 "values ('$proj_head_uid','$TBOPSPID','$TBOPSPID', ".
		 "'" . TBDB_TRUSTSTRING_GROUPROOT . "', now())");

    DBQueryFatal("update group_membership set ".
		 "  trust='" . TBDB_TRUSTSTRING_PROJROOT . "' ".
		 "where uid='$proj_head_uid' and pid='$pid'");

    #
    # Move to next phase. 
    # 
    TBSetFirstInitPid($pid);
    TBSetFirstInitState("approveproject");
    header("Location: approveproject.php3?pid=$pid&approval=approve");
    return;
}

#
# Spit out a redirect so that the history does not include a post
# in it. The back button skips over the post and to the form.
# See above for conclusion.
# 
header("Location: newproject.php3?finished=1");

?>
