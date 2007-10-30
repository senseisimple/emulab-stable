<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
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
# Verify page arguments.
#
$optargs = OptionalPageArguments("submit",       PAGEARG_STRING,
				 "finished",     PAGEARG_BOOLEAN,
				 "formfields",   PAGEARG_ARRAY);

#
# See if we are in an initial Emulab setup.
#
$FirstInitState = (TBGetFirstInitState() == "createproject");

#
# If a uid came in, then we check to see if the login is valid.
# If the login is not valid. We require that the user be logged in
# to start a second project.
#
if ($this_user && !$FirstInitState) {
    # Allow unapproved users to create multiple projects ...
    # Must be verified though.
    CheckLoginOrDie(CHECKLOGIN_UNAPPROVED|CHECKLOGIN_WEBONLY);
    $proj_head_uid = $this_user->uid();
    $returning = 1;
}
else {
    #
    # No uid, so must be new.
    #
    $returning = 0;
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
    global $TBDB_UIDLEN, $TBDB_PIDLEN, $TBDOCBASE, $WWWHOST;
    global $usr_keyfile, $FirstInitState;
    global $ACCOUNTWARNING, $EMAILWARNING;
    global $WIKISUPPORT, $WIKIHOME, $USERSELECTUIDS;
    
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
        # UID:
        #
	if ($USERSELECTUIDS || $FirstInitState == "createproject") {
	    echo "<tr>
                      <td colspan=2>*<a
                             href='docwrapper.php3?docname=security.html'
                             target=_blank>Username</a>
                                (alphanumeric, lowercase):</td>
                      <td class=left>
                          <input type=text
                                 name=\"formfields[proj_head_uid]\"
                                 value=\"" . $formfields["proj_head_uid"] . "\"
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
                             value=\"" . $formfields["usr_name"] . "\"
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
                                 value=\"" . $formfields["wikiname"] . "\"
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
                             value=\"" . $formfields["usr_title"] . "\"
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
                             value=\"" . $formfields["usr_affil"] . "\"
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
                             value=\"" . $formfields["usr_URL"] . "\"
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
                             value=\"" . $formfields["usr_email"] . "\"
                             onchange=\"alert('$EMAILWARNING')\"
	                     size=30>
                  </td>
              </tr>\n";


	echo "<tr><td colspan=3>*Postal Address:<br /><center>
		<table>
		  <tr><td>Line 1</td><td colspan=3>
                    <input type=text
                           name=\"formfields[usr_addr]\"
                           value=\"" . $formfields["usr_addr"] . "\"
	                   size=45></td></tr>
		  <tr><td>Line 2</td><td colspan=3>
                    <input type=text
                           name=\"formfields[usr_addr2]\"
                           value=\"" . $formfields["usr_addr2"] . "\"
	                   size=45></td></tr>
		  <tr><td>City</td><td>
                    <input type=text
                           name=\"formfields[usr_city]\"
                           value=\"" . $formfields["usr_city"] . "\"
	                   size=25></td>
		      <td>State/Province</td><td>
                    <input type=text
                           name=\"formfields[usr_state]\"
                           value=\"" . $formfields["usr_state"] . "\"
	                   size=2></td></tr>
		  <tr><td>ZIP/Postal Code</td><td>
                    <input type=text
                           name=\"formfields[usr_zip]\"
                           value=\"" . $formfields["usr_zip"] . "\"
	                   size=10></td>
		      <td>Country</td><td>
                    <input type=text
                           name=\"formfields[usr_country]\"
                           value=\"" . $formfields["usr_country"] . "\"
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
                             value=\"" . $formfields["usr_phone"] . "\"
	                     size=15>
                  </td>
              </tr>\n";

	#
	# SSH public key
	#
	echo "<tr>
                 <td colspan=2>Upload your SSH Pub Key[<b>2</b>]:<br>
                                   (1K max)</td>
   
                 <td>
                      <input type=hidden name=MAX_FILE_SIZE value=1024>
                      <input type=file
                             name=usr_keyfile
                             value=\"" .
	                           (isset($_FILES['usr_keyfile']) ?
				    $_FILES['usr_keyfile']['name'] : "") . "\"
	                     size=50>
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
                             value=\"" . $formfields["password1"] . "\"
                             size=8></td>
              </tr>\n";

        echo "<tr>
                  <td colspan=2>*Retype Password:</td>
                  <td class=left>
                      <input type=password
                             name=\"formfields[password2]\"
                             value=\"" . $formfields["password2"] . "\"
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
                         value=\"" . $formfields["pid"] . "\"
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
                         value=\"" . $formfields["proj_name"] . "\"
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
                         value=\"" . $formfields["proj_URL"] . "\"
                         size=45>
              </td>
          </tr>\n";

    #
    # Publicly visible.
    #
    if (!isset($formfields["proj_public"])) {
	$formfields["proj_public"] = "";
    }
    echo "<tr>
              <td colspan=2>*Can we list your project publicly as
                             an \"Emulab User?\":
                  <br>
                  (See our <a href=\"projectlist.php3\"
                              target=\"Users\">Users</a> page)
              </td>
              <td><input type=checkbox value=checked
                         name=\"formfields[proj_public]\"
                         " . $formfields["proj_public"] . ">
                         Yes &nbsp
 	          <br>
                  *If \"No\" please tell us why not:<br>
                  <input type=text
                         name=\"formfields[proj_whynotpublic]\"
                         value=\"" . $formfields["proj_whynotpublic"] . "\"
	                 size=45>
             </td>
      </tr>\n";

    #
    # Will you add a link?
    #
    if (!isset($formfields["proj_linked"])) {
	$formfields["proj_linked"] = "";
    }
    echo "<tr>
              <td colspan=2>*Will you add a link on your project page
                        to <a href=\"$TBDOCBASE\" target='_blank'>$WWWHOST</a>?
              </td>
              <td><input type=checkbox value=checked
                         name=\"formfields[proj_linked]\"
                         " . $formfields["proj_linked"] . ">
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
                         value=\"" . $formfields["proj_funders"] . "\"
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
                         value=\"" . $formfields["proj_members"] . "\"
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
                         value=\"" . $formfields["proj_pcs"] . "\"
                         size=4>
              </td>
          </tr>\n";

    echo "<tr>
              <td colspan=2>Request Access to 
                  <a href=\"$TBDOCBASE/docwrapper.php3?docname=widearea.html\"
                      target='_blank'>Planetlab PCs</a>:</td>
              <td class=left>
                  <input type=checkbox value=checked
                         name=\"formfields[proj_plabpcs]\" " .
	                  (isset($formfields["proj_plabpcs"]) ?
			   $formfields["proj_plabpcs"] : "") . ">Yes &nbsp
              </td>
          </tr>\n";

    echo "<tr>
              <td colspan=2>Request Access to 
                 <a href=\"$TBDOCBASE/docwrapper.php3?docname=widearea.html\"
                    target='_blank'>wide-area PCs</a>:</td>
              <td class=left>
                  <input type=checkbox value=checked
                         name=\"formfields[proj_ronpcs]\" " .
	                  (isset($formfields["proj_ronpcs"]) ?
			   $formfields["proj_ronpcs"] : "") . ">Yes &nbsp
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
	            ereg_replace("\r", "", $formfields["proj_why"]) .
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
                   then please specify the path to your
                   your identity.pub file. <font color=red>NOTE:</font>
                   We use the <a href=http://www.openssh.org target='_blank'>OpenSSH</a>
                   key format,
                   which has a slightly different protocol 2 public key format
                   than some of the commercial vendors such as
                   <a href=http://www.ssh.com target='_blank'>SSH Communications</a>. If you
                   use one of these commercial vendors, then please
                   upload the public key file and we will convert it
                   for you.\n";
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
    $defaults["proj_head_uid"]  = "";
    $defaults["usr_name"]       = "";
    $defaults["wikiname"]       = "";
    $defaults["usr_title"]      = "";
    $defaults["usr_affil"]      = "";
    $defaults["usr_URL"]        = "$HTTPTAG";
    $defaults["usr_email"]      = "";
    $defaults["usr_addr"]       = "";
    $defaults["usr_addr2"]      = "";
    $defaults["usr_city"]       = "";
    $defaults["usr_state"]      = "";
    $defaults["usr_zip"]        = "";
    $defaults["usr_country"]    = "USA";
    $defaults["usr_phone"]      = "";
    $defaults["password1"]      = "";
    $defaults["password2"]      = "";
    
    $defaults["pid"]            = "";
    $defaults["proj_name"]      = "";
    $defaults["proj_URL"]       = "$HTTPTAG";
    $defaults["proj_public"]    = "checked";
    $defaults["proj_whynotpublic"] = "";
    $defaults["proj_linked"]    = "checked";
    $defaults["proj_funders"]   = "";
    $defaults["proj_members"]   = "";
    $defaults["proj_pcs"]       = "";
    $defaults["proj_ronpcs"]    = "";
    $defaults["proj_plabpcs"]   = "";
    $defaults["proj_why"]       = "";

    if ($FirstInitState == "createproject") {
	$defaults["pid"]          = "testbed";
	$defaults["proj_pcs"]     = "256";
	$defaults["proj_members"] = "256";
	$defaults["proj_funders"] = "none";
	$defaults["proj_name"]    = "Your Testbed Project";
	$defaults["proj_why"]     = "This project is used for testbed ".
	    "administrators to develop and test new software. ";
    }
    
    SPITFORM($defaults, $returning, 0);
    PAGEFOOTER();
    return;
}

# Form submitted. Make sure we have a formfields array.
if (!isset($formfields)) {
    PAGEARGERROR("Invalid form arguments.");
}

#TBERROR("A\n\n" . print_r($formfields, TRUE), 0);

#
# Otherwise, must validate and redisplay if errors
#
$errors = array();

#
# These fields are required!
#
if (! $returning) {
    if ($USERSELECTUIDS || $FirstInitState == "createproject") {
	if (!isset($formfields["proj_head_uid"]) ||
	    strcmp($formfields["proj_head_uid"], "") == 0) {
	    $errors["Username"] = "Missing Field";
	}
	elseif (!TBvalid_uid($formfields["proj_head_uid"])) {
	    $errors["UserName"] = TBFieldErrorString();
	}
	elseif (User::Lookup($formfields["proj_head_uid"]) ||
		posix_getpwnam($formfields["proj_head_uid"])) {
	    $errors["UserName"] = "Already in use. Pick another";
	}
    }
    if (!isset($formfields["usr_title"]) ||
	strcmp($formfields["usr_title"], "") == 0) {
	$errors["Job Title/Position"] = "Missing Field";
    }
    elseif (! TBvalid_title($formfields["usr_title"])) {
	$errors["Job Title/Position"] = TBFieldErrorString();
    }
    if (!isset($formfields["usr_name"]) ||
	strcmp($formfields["usr_name"], "") == 0) {
	$errors["Full Name"] = "Missing Field";
    }
    elseif (! TBvalid_usrname($formfields["usr_name"])) {
	$errors["Full Name"] = TBFieldErrorString();
    }
    # Make sure user name has at least two tokens!
    $tokens = preg_split("/[\s]+/", $formfields["usr_name"],
			 -1, PREG_SPLIT_NO_EMPTY);
    if (count($tokens) < 2) {
	$errors["Full Name"] = "Please provide a first and last name";
    }
    if ($WIKISUPPORT) {
	if (!isset($formfields["wikiname"]) ||
	    strcmp($formfields["wikiname"], "") == 0) {
	    $errors["WikiName"] = "Missing Field";
	}
	elseif (! TBvalid_wikiname($formfields["wikiname"])) {
	    $errors["WikiName"] = TBFieldErrorString();
	}
	elseif (User::LookupByWikiName($formfields["wikiname"])) {
	    $errors["WikiName"] = "Already in use. Pick another";
	}
    }
    if (!isset($formfields["usr_affil"]) ||
	strcmp($formfields["usr_affil"], "") == 0) {
	$errors["Affiliation"] = "Missing Field";
    }
    elseif (! TBvalid_affiliation($formfields["usr_affil"])) {
	$errors["Affiliation"] = TBFieldErrorString();
    }
    if (!isset($formfields["usr_email"]) ||
	strcmp($formfields["usr_email"], "") == 0) {
	$errors["Email Address"] = "Missing Field";
    }
    elseif (! TBvalid_email($formfields["usr_email"])) {
	$errors["Email Address"] = TBFieldErrorString();
    }
    elseif (User::LookupByEmail($formfields["usr_email"])) {
        #
        # Treat this error separate. Not allowed.
        #
	$errors["Email Address"] =
	    "Already in use. <b>Did you forget to login?</b>";
    }
    if (isset($formfields["usr_URL"]) &&
	strcmp($formfields["usr_URL"], "") &&
	strcmp($formfields["usr_URL"], $HTTPTAG) &&
	! CHECKURL($formfields["usr_URL"], $urlerror)) {
	$errors["Home Page URL"] = $urlerror;
    }
    if (!isset($formfields["usr_addr"]) ||
	strcmp($formfields["usr_addr"], "") == 0) {
	$errors["Address 1"] = "Missing Field";
    }
    elseif (! TBvalid_addr($formfields["usr_addr"])) {
	$errors["Address 1"] = TBFieldErrorString();
    }
    # Optional
    if (isset($formfields["usr_addr2"]) &&
	!TBvalid_addr($formfields["usr_addr2"])) {
	$errors["Address 2"] = TBFieldErrorString();
    }
    if (!isset($formfields["usr_city"]) ||
	strcmp($formfields["usr_city"], "") == 0) {
	$errors["City"] = "Missing Field";
    }
    elseif (! TBvalid_city($formfields["usr_city"])) {
	$errors["City"] = TBFieldErrorString();
    }
    if (!isset($formfields["usr_state"]) ||
	strcmp($formfields["usr_state"], "") == 0) {
	$errors["State"] = "Missing Field";
    }
    elseif (! TBvalid_state($formfields["usr_state"])) {
	$errors["State"] = TBFieldErrorString();
    }
    if (!isset($formfields["usr_zip"]) ||
	strcmp($formfields["usr_zip"], "") == 0) {
	$errors["ZIP/Postal Code"] = "Missing Field";
    }
    elseif (! TBvalid_zip($formfields["usr_zip"])) {
	$errors["Zip/Postal Code"] = TBFieldErrorString();
    }
    if (!isset($formfields["usr_country"]) ||
	strcmp($formfields["usr_country"], "") == 0) {
	$errors["Country"] = "Missing Field";
    }
    elseif (! TBvalid_country($formfields["usr_country"])) {
	$errors["Country"] = TBFieldErrorString();
    }
    if (!isset($formfields["usr_phone"]) ||
	strcmp($formfields["usr_phone"], "") == 0) {
	$errors["Phone #"] = "Missing Field";
    }
    elseif (!TBvalid_phone($formfields["usr_phone"])) {
	$errors["Phone #"] = TBFieldErrorString();
    }
    if (!isset($formfields["password1"]) ||
	strcmp($formfields["password1"], "") == 0) {
	$errors["Password"] = "Missing Field";
    }
    if (!isset($formfields["password2"]) ||
	strcmp($formfields["password2"], "") == 0) {
	$errors["Confirm Password"] = "Missing Field";
    }
    elseif (strcmp($formfields["password1"], $formfields["password2"])) {
	$errors["Confirm Password"] = "Does not match Password";
    }
    elseif (! CHECKPASSWORD((($USERSELECTUIDS ||
			     $FirstInitState == "createproject") ?
			     $formfields["proj_head_uid"] : "ignored"),
			    $formfields["password1"],
			    $formfields["usr_name"],
			    $formfields["usr_email"], $checkerror)) {
	$errors["Password"] = "$checkerror";
    }
}

if (!isset($formfields["pid"]) ||
    strcmp($formfields["pid"], "") == 0) {
    $errors["Project Name"] = "Missing Field";
}
else {
    if (!TBvalid_newpid($formfields["pid"])) {
	$errors["Project Name"] = TBFieldErrorString();
    }
    elseif (Project::LookupByPid($formfields["pid"])) {
	$errors["Project Name"] =
	    "Already in use. Select another";
    }
}

if (!isset($formfields["proj_name"]) ||
    strcmp($formfields["proj_name"], "") == 0) {
    $errors["Project Description"] = "Missing Field";
}
elseif (! TBvalid_description($formfields["proj_name"])) {
    $errors["Project Description"] = TBFieldErrorString();
}
if (!isset($formfields["proj_URL"]) ||
    strcmp($formfields["proj_URL"], "") == 0 ||
    strcmp($formfields["proj_URL"], $HTTPTAG) == 0) {    
    $errors["Project URL"] = "Missing Field";
}
elseif (! CHECKURL($formfields["proj_URL"], $urlerror)) {
    $errors["Project URL"] = $urlerror;
}
if (!isset($formfields["proj_funders"]) ||
    strcmp($formfields["proj_funders"], "") == 0) {
    $errors["Funding Sources"] = "Missing Field";
}
elseif (! TBvalid_description($formfields["proj_funders"])) {
    $errors["Funding Sources"] = TBFieldErrorString();
}
if (!isset($formfields["proj_members"]) ||
    strcmp($formfields["proj_members"], "") == 0) {
    $errors["#of Members"] = "Missing Field";
}
elseif (! TBvalid_num_members($formfields["proj_members"])) {
    $errors["#of Members"] = TBFieldErrorString();
}
if (!isset($formfields["proj_pcs"]) ||
    strcmp($formfields["proj_pcs"], "") == 0) {
    $errors["#of PCs"] = "Missing Field";
}
elseif (! TBvalid_num_pcs($formfields["proj_pcs"])) {
    $errors["#of PCs"] = TBFieldErrorString();
}

if (isset($formfields["proj_plabpcs"]) &&
    strcmp($formfields["proj_plabpcs"], "") &&
    strcmp($formfields["proj_plabpcs"], "checked")) {
    $errors["Planetlab Access"] = "Bad Value";
}
if (isset($formfields["proj_ronpcs"]) &&
    strcmp($formfields["proj_ronpcs"], "") &&
    strcmp($formfields["proj_ronpcs"], "checked")) {
    $errors["Ron Access"] = "Bad Value";
}
if (!isset($formfields["proj_why"]) ||
    strcmp($formfields["proj_why"], "") == 0) {
    $errors["How and Why?"] = "Missing Field";
}
elseif (! TBvalid_why($formfields["proj_why"])) {
    $errors["How and Why?"] = TBFieldErrorString();
}
if ((!isset($formfields["proj_public"]) ||
     strcmp($formfields["proj_public"], "checked")) &&
    (!isset($formfields["proj_whynotpublic"]) ||
     strcmp($formfields["proj_whynotpublic"], "") == 0)) {
    $errors["Why Not Public?"] = "Missing Field";
}
if (isset($formfields["proj_linked"]) &&
    strcmp($formfields["proj_linked"], "") &&
    strcmp($formfields["proj_linked"], "checked")) {
    $errors["Link to Us"] = "Bad Value";
}

# Present these errors before we call out to do anything else.
if (count($errors)) {
    SPITFORM($formfields, $returning, $errors);
    PAGEFOOTER();
    return;
}

#
# Create the User first, then the Project/Group.
# Certain of these values must be escaped or otherwise sanitized.
#
if (!$returning) {
    $args = array();
    $args["name"]	   = $formfields["usr_name"];
    $args["email"]         = $formfields["usr_email"];
    $args["address"]       = $formfields["usr_addr"];
    $args["address2"]      = $formfields["usr_addr2"];
    $args["city"]          = $formfields["usr_city"];
    $args["state"]         = $formfields["usr_state"];
    $args["zip"]           = $formfields["usr_zip"];
    $args["country"]       = $formfields["usr_country"];
    $args["phone"]         = $formfields["usr_phone"];
    $args["shell"]         = 'tcsh';
    $args["title"]         = $formfields["usr_title"];
    $args["affiliation"]   = $formfields["usr_affil"];
    $args["password"]      = $formfields["password1"];
    $args["wikiname"]      = ($WIKISUPPORT ? $formfields["wikiname"] : "");

    if (isset($formfields["usr_URL"]) &&
	$formfields["usr_URL"] != $HTTPTAG && $formfields["usr_URL"] != "") {
	$args["URL"] = $formfields["usr_URL"];
    }
    if ($USERSELECTUIDS || $FirstInitState == "createproject") {
	$args["login"] = $formfields["proj_head_uid"];
    }

    # Backend verifies pubkey and returns error.
    if (isset($_FILES['usr_keyfile']) &&
	$_FILES['usr_keyfile']['name'] != "" &&
	$_FILES['usr_keyfile']['name'] != "none") {

	$localfile = $_FILES['usr_keyfile']['tmp_name'];
	$args["pubkey"] = file_get_contents($localfile);
    }

    # Just collect the user XML args here and pass the file to NewNewProject.
    # Underneath, newproj calls newuser with the XML file.
    #
    # Calling newuser down in Perl land makes creation of the leader account
    # and the project "atomic" from the user's point of view.  This avoids a
    # problem when the DB is locked for daily backup: in newproject, the call
    # on NewNewUser would block and then unblock and get done; meanwhile the
    # PHP thread went away so we never returned here to call NewNewProject.
    #
    if (! ($newuser_xml = User::NewNewUserXML($args, $errors)) != 0) {
	$errors["Error Creating User XML"] = $error;
	TBERROR("B\n${error}\n\n" . print_r($args, TRUE), 0);
	SPITFORM($formfields, $returning, $errors);
	PAGEFOOTER();
	return;
    }
}

#
# Now for the new Project
#
$args = array();
if (isset($newuser_xml)) {
    $args["newuser_xml"]   = $newuser_xml;
}
if ($returning) {
    # An existing, logged-in user is starting the project.
    $args["leader"]	   = $this_user->uid();
}
$args["name"]		   = $formfields["pid"];
$args["short description"] = $formfields["proj_name"];
$args["URL"]               = $formfields["proj_URL"];
$args["members"]           = $formfields["proj_members"];
$args["num_pcs"]           = $formfields["proj_pcs"];
$args["long description"]  = $formfields["proj_why"];
$args["funders"]           = $formfields["proj_funders"];
$args["whynotpublic"]      = $formfields["proj_whynotpublic"];

if (!isset($formfields["proj_public"]) ||
    $formfields["proj_public"] != "checked") {
    $args["public"] = 0;
}
else {
    $args["public"] = 1;
}
if (!isset($formfields["proj_linked"]) ||
    $formfields["proj_linked"] != "checked") {
    $args["linkedtous"] = 0;
}
else {
    $args["linkedtous"] = 1;
}
if (isset($formfields["proj_plabpcs"]) &&
    $formfields["proj_plabpcs"] == "checked") {
    $args["plab"] = 1;
}
if (isset($formfields["proj_ronpcs"]) &&
    $formfields["proj_ronpcs"] == "checked") {
    $args["ron"] = 1;
}

if (! ($project = Project::NewNewProject($args, $error))) {
    $errors["Error Creating Project"] = $error;
    TBERROR("C\n${error}\n\n" . print_r($args, TRUE), 0);
    SPITFORM($formfields, $returning, $errors);
    PAGEFOOTER();
    return;
}

#
# Need to do some extra work for the first project; eventually move to backend
# 
if ($FirstInitState) {
    $leader = $project->GetLeader();
    $proj_head_uid = $leader->uid();
    # Set up the management group (emulab-ops).
    Group::Initialize($proj_head_uid);
    
    #
    # Move to next phase. 
    # 
    $pid = $formfields["pid"];
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
