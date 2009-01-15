<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2009 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# No PAGEHEADER here since we spit out a Location header later. See below.
# 
# We want to allow logged in users with expired passwords to change them.
#
$this_user = CheckLoginOrDie(CHECKLOGIN_USERSTATUS|CHECKLOGIN_PSWDEXPIRED|
			     CHECKLOGIN_WEBONLY|CHECKLOGIN_WIKIONLY);
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

# Shell options we support. Maybe stick in DB someday.
$shelllist = array( 'tcsh', 'bash', 'csh', 'sh', 'zsh' );

# Used if db slot for user is NULL (should not happen.)
$defaultshell = 'tcsh';

# See below.
$wikionly = 0;

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("target_user", PAGEARG_USER,
				 "submit",      PAGEARG_STRING,
				 "formfields",  PAGEARG_ARRAY);

#
# Spit the form out using the array of data and error strings (if any).
# 
function SPITFORM($formfields, $errors)
{
    global $TBDB_UIDLEN, $TBDB_PIDLEN, $TBDB_GIDLEN, $isadmin;
    global $target_user, $wikionly;
    global $shelllist, $defaultshell;
    global $WIKIDOCURL;

    $username = $target_user->uid();
    $uid_idx  = $target_user->uid_idx();
    $webid    = $target_user->webid();
    
    #
    # Standard Testbed Header. Written late cause of password
    # expiration interaction. See below.
    #
    PAGEHEADER("Modify User Information");
    ###STARTBUSY("Making user profile changes");

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
    $url      = CreateURL("moduserinfo", $target_user);

    echo "<table align=center border=1> 
          <tr>
             <td align=center colspan=3>
                 <em>(Fields marked with * are required)</em>
             </td>
          </tr>
          <form action='$url' method=post>\n";

        #
        # UserName. This is a constant field. 
        #
        echo "<tr>
                  <td colspan=2>Username:</td>
                  <td class=left>$username ($uid_idx)
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
                             value=\"" . $formfields["usr_name"] . "\"
	                     size=30>
                  </td>
              </tr>\n";

        #
	# Title/Position:
	# 
	echo "<tr>
                  <td colspan=2>${optfield}Job Title/Position:</td>
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
                      <td colspan=2>*Institutional Affiliation:</td>
                      <td class=left>
			<table>
                          <tr>
                          <td>Name</td>
                          <td><input type=text
                                 name=\"formfields[usr_affil]\"
                                 value=\"" . $formfields["usr_affil"] . "\"
	                         size=40></td></tr>
			  <tr>
                          <td>Abbreviation:</td>
                          <td><input type=text
                                 name=\"formfields[usr_affil_abbrev]\"
                                 value=\"" . $formfields["usr_affil_abbrev"] . "\"
	                         size=16 maxlength=16> (e.g. MIT)</td>
			  </tr>
        		</table>
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
                  <td colspan=2>Email Address[<b>1</b>]:</td>
                  <td class=left> ";
	if ($isadmin)
	    echo "    <input type=text ";
	else
	    echo "    $formfields[usr_email]
                      <input type=hidden ";

	echo "               name=\"formfields[usr_email]\"
                             value=\"" . $formfields["usr_email"] . "\"
	                     size=30>";
        echo "    </td>
              </tr>\n";

        #
        # Country needs a default for older users.
        #
	if (!isset($formfields["usr_country"]) ||
	    $formfields["usr_country"] == "") {
	    $formfields["usr_country"] = "USA";
	}

	#
	# Postal Address
        #
	echo "<tr><td colspan=3>${optfield}Address:<br /><center>
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

        # Default Shell
	echo "<tr><td colspan=2>Shell:</td>
                  <td class=left>";
	echo "<select name=\"formfields[usr_shell]\">";
	foreach ($shelllist as $s) {
	    $selected = "";

	    if ((!isset($formfields["usr_shell"]) &&
		 strcmp($defaultshell, $s) == 0) ||
		strcmp($formfields["usr_shell"],$s) == 0) {
		$selected = "selected";
	    }
	    echo "<option $selected value='$s'>$s</option>";
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
                             value=\"" . $formfields["usr_phone"] . "\"
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
                             value=\"" . $formfields["password1"] . "\"
                             size=8></td>
              </tr>\n";

        echo "<tr>
                  <td colspan=2>Retype Password:</td>
                  <td class=left>
                      <input type=password
                             name=\"formfields[password2]\"
                             value=\"" . $formfields["password2"] . "\"
                             size=8></td>
             </tr>\n";

	if (!$wikionly) {
	    #
            # Windows Password.  Initial random default is based on the Unix
	    # password hash.
	    #   
	    # A separate password is kept for experiment nodes running Windows.
	    # It is presented behind-the-scenes to rdesktop and Samba by our
	    # Web interface, but you may still need to type it.
	    # The default password is randomly generated.
	    # You may change it to something easier to remember.
	    #
	    echo "<tr>
                      <td colspan=2>Windows Password[<b>1,4</b>]:</td>
                      <td class=left>
                          <input type=text
                                 name=\"formfields[w_password1]\"
                                 value=\"" . $formfields["w_password1"] . "\"
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
	    $checked = $formfields["user_interface"];
	    echo "<tr>
		      <td colspan=2>Use simplified PlanetLab view:</td>
		      <td class=left>
		         <input type='checkbox'
                                name=\"formfields[user_interface]\"
                                value=checked $checked>
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
		                   ereg_replace("\r", "",
						$formfields["notes"]) .
		        "</textarea>
                      </td>
                  </tr>\n";
	}

    echo "<tr>
              <td align=center colspan=3>
                  <b><input type=submit name=submit value=Submit></b>
              </td>
          </tr>\n";

    echo "</form>
          </table>\n";

    echo "<h4><blockquote><blockquote>
          <ol>
            <li> Please consult our
                 <a href = '$WIKIDOCURL/Security'>
                 security policies</a> for information
                 regarding passwords and email addresses.\n";
    if (!$wikionly) {
	$pubkey_url = CreateURL("showpubkeys", $target_user);
	
	echo "<li> You can also
                 <a href='$pubkey_url'>edit your ssh public keys</a>.
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

# Early error checking on $target_user.
$errors  = array();

#
# The target uid and the current uid will be the same, unless its a priv user
# (admin,PI) modifying someone elses data. Must verify this case. Note that
# the target uid comes initially as a page arg, but later as a form argument
#
if (!isset($submit)) {
    if (!isset($target_user)) {
	$target_user = $this_user;
    }
}
else {
    if (!isset($target_user) || !isset($formfields)) {
	$errors["Args"] = "Invalid form arguments!";
    }
}

# Need this below.
$target_uid = $target_user->uid();

#
# Admin types can change anyone. 
#
if (!$isadmin && 
    !$target_user->AccessCheck($this_user, $TB_USERINFO_MODIFYINFO)) {
    $errors["Project"] = 
	"You do not have permission to modify information for ".
	    "user: $target_uid!";
}

#
# On first load, display a form consisting of current user values, and exit.
#
if (!isset($submit)) {
    $defaults = array();
    $defaults["user"]        = $target_user->webid();
    $defaults["usr_email"]   = $target_user->email();
    $defaults["usr_URL"]     = $target_user->URL();
    $defaults["usr_addr"]    = $target_user->addr();
    $defaults["usr_addr2"]   = $target_user->addr2();
    $defaults["usr_city"]    = $target_user->city();
    $defaults["usr_state"]   = $target_user->state();
    $defaults["usr_zip"]     = $target_user->zip();
    $defaults["usr_country"] = $target_user->country();
    $defaults["usr_name"]    = $target_user->name();
    $defaults["usr_phone"]   = $target_user->phone();
    $defaults["usr_title"]   = $target_user->title();
    $defaults["usr_affil"]   = $target_user->affil();
    $defaults["usr_affil_abbrev"] = $target_user->affil_abbrev();
    $defaults["usr_shell"]   = $target_user->shell();
    $defaults["notes"]       = $target_user->notes();
    $defaults["password1"]   = "";
    $defaults["password2"]   = "";
    $defaults["user_interface"] =
	($target_user->user_interface() == TBDB_USER_INTERFACE_PLAB ?
	 "checked" : "");

    $wikionly                = $target_user->wikionly();

    # Show and keep the Windows password if user-set, otherwise fill in the
    # random one.
    if ($target_user->w_pswd() != "") {
	$defaults["w_password1"] =
	    $defaults["w_password2"] = $target_user->w_pswd();
    }
    else {
	#
	# The initial random default for the Windows Password is based on the
	# Unix encrypted password, in particular the random salt if it's an
	# MD5 crypt, consisting of the 8 chars after an initial "$1$" and
	# followed by "$".
	#
	$unixpwd = explode('$', $target_user->pswd());
	if (strlen($unixpwd[0]) > 0)
	    # When there's no $ at the beginning, its not an MD5 hash.
	    $randpwd = substr($unixpwd[0],0,8);
	else
	    $randpwd = substr($unixpwd[2],0,8); # The MD5 salt string.
	$defaults["w_password1"] = $defaults["w_password2"] = $randpwd;
    }

    SPITFORM($defaults, $errors);
    PAGEFOOTER();
    return;
}

#
# If any errors, respit the form with the current values and the
# error messages displayed. Iterate until happy.
# 
if (count($errors)) {
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    return;
}

#
# Build up argument array to pass along.
#
$args = array();

# Always pass the password fields if specified.
if (isset($formfields["password1"]) && $formfields["password1"] != "") {
    $args["password1"] = $formfields["password1"];
}
if (isset($formfields["password2"]) && $formfields["password2"] != "") {
    $args["password2"] = $formfields["password2"];
}
if (isset($formfields["w_password1"]) && $formfields["w_password1"] != "") {
    $args["w_password1"] = $formfields["w_password1"];
}
if (isset($formfields["w_password2"]) && $formfields["w_password2"] != "") {
    $args["w_password2"] = $formfields["w_password2"];
}

# Skip passing ones that are not changing from the default (DB state.)
if (isset($formfields["usr_name"]) && $formfields["usr_name"] != "" &&
    ($formfields["usr_name"] != $target_user->name())) {
    $args["usr_name"]	= $formfields["usr_name"];
}
if (isset($formfields["usr_email"]) && $formfields["usr_email"] != "" &&
    ($formfields["usr_email"] != $target_user->email())) {
    $args["usr_email"]	= $formfields["usr_email"];
}
if (isset($formfields["usr_title"]) && $formfields["usr_title"] != "" &&
    $formfields["usr_title"] != $target_user->title()) {
    $args["usr_title"]	= $formfields["usr_title"];
}
if (isset($formfields["usr_affil"]) && $formfields["usr_affil"] != "" &&
    $formfields["usr_affil"] != $target_user->affil()) {
    $args["usr_affil"]	= $formfields["usr_affil"];
}
if (isset($formfields["usr_affil_abbrev"]) && $formfields["usr_affil_abbrev"] != "" &&
    $formfields["usr_affil"] != $target_user->affil_abbrev()) {
    $args["usr_affil_abbrev"]	= $formfields["usr_affil_abbrev"];
}
if (isset($formfields["usr_shell"]) && $formfields["usr_shell"] != "" &&
    $formfields["usr_shell"] != $target_user->shell()) {
    $args["usr_shell"]	= $formfields["usr_shell"];
}
if (isset($formfields["usr_URL"]) && $formfields["usr_URL"] != "" &&
    $formfields["usr_URL"] != $target_user->URL()) {
    $args["usr_URL"]	= $formfields["usr_URL"];
}
if (isset($formfields["usr_addr"]) && $formfields["usr_addr"] != "" &&
    $formfields["usr_addr"] != $target_user->addr()) {
    $args["usr_addr"]	= $formfields["usr_addr"];
}
if (isset($formfields["usr_addr2"]) &&
    $formfields["usr_addr2"] != $target_user->addr2()) {
    $args["usr_addr2"]	= $formfields["usr_addr2"];
}
if (isset($formfields["usr_city"]) && $formfields["usr_city"] != "" &&
    $formfields["usr_city"] != $target_user->city()) {
    $args["usr_city"]	= $formfields["usr_city"];
}
if (isset($formfields["usr_state"]) && $formfields["usr_state"] != "" &&
    $formfields["usr_state"] != $target_user->state()) {
    $args["usr_state"]	= $formfields["usr_state"];
}
if (isset($formfields["usr_zip"]) && $formfields["usr_zip"] != "" &&
    $formfields["usr_zip"] != $target_user->zip()) {
    $args["usr_zip"]	= $formfields["usr_zip"];
}
if (isset($formfields["usr_country"]) && $formfields["usr_country"] != "" &&
    $formfields["usr_country"] != $target_user->country()) {
    $args["usr_country"]	= $formfields["usr_country"];
}
if (isset($formfields["usr_phone"]) && $formfields["usr_phone"] != "" &&
    $formfields["usr_phone"] != $target_user->phone()) {
    $args["usr_phone"]	= $formfields["usr_phone"];
}
if (isset($formfields["user_interface"]) &&
    $formfields["user_interface"] == "checked") {
    $desired_interface = TBDB_USER_INTERFACE_PLAB;
}
else {
    $desired_interface = TBDB_USER_INTERFACE_EMULAB;
}
if ($desired_interface != $target_user->user_interface()) {
    $args["user_interface"] = $desired_interface;
}

if (isset($formfields["notes"]) &&
    $formfields["notes"] != $target_user->notes()) {
    $args["notes"]	= $formfields["notes"];
}

if (! ($result = User::ModUserInfo($target_user, $uid, $args, $errors))) {
    # Always respit the form so that the form fields are not lost.
    # I just hate it when that happens so lets not be guilty of it ourselves.
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    return;
}

PAGEHEADER("Modify User Information");

###STOPBUSY();

echo "<center><h3>Done!</h3></center>\n";
PAGEREPLACE(CreateURL("showuser", $target_user) . "#PROFILE");

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
