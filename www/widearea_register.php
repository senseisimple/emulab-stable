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
# If a uid came in, then we check to see if the login is valid.
# If the login is not valid. We require that the user be logged in.
#
if ($uid) {
    LOGGEDINORDIE($uid, CHECKLOGIN_UNAPPROVED);
    $usr_uid = $uid;
    $returning = 1;
}
else {
    #
    # No uid, so must be new.
    #
    $returning = 0;
}
$haveinfo = 0;

#
# Default connection type strings,
#
$connlist          = array();
$connlist["DSL"]   = "DSL";
$connlist["Cable"] = "Cable";
$connlist["ISDN"]  = "ISDN";
$connlist["T1"]    = "T1";
$connlist["Inet1"] = "Inet1";
$connlist["Inet2"] = "Inet2";
$connlist["T3"]    = "T3";

#
# Spit the form out using the array of data. 
# 
function SPITFORM($formfields, $returning, $errors)
{
    global $TBDB_UIDLEN, $TBDB_PIDLEN, $TBDOCBASE, $WWWHOST;
    global $usr_keyfile, $connlist;

    PAGEHEADER("Request an account on your Netbed node");
    
    echo "<center><font size=+1 color=red>
	     You must fill out this form if you want local access to
	     your widearea node.
          </font></center><br>\n";

    if (! $returning) {
	echo "<center><font size=+1>
               If you already have an Emulab account,
               <a href=login.php3?refer=1>
               <font color=red>please log on first!</font></a>
              </font></center><br>\n";
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

    echo "<table align=center border=1> 
          <tr>
            <td align=center colspan=3>
                Fields marked with * are required
            </td>
          </tr>\n

          <form enctype=multipart/form-data
                action=widearea_register.php method=post>\n";

    if (! $returning) {
        #
        # Start user information stuff. Presented for new users only.
        #
	echo "<tr>
                  <th colspan=3>
                      User Information:
                  </th>
              </tr>\n";

        #
        # UserName:
        #
        echo "<tr>
                  <td colspan=2>*User ID (no blanks, lowercase):</td>
                  <td class=left>
                      <input type=text
                             name=\"formfields[usr_uid]\"
                             value=\"" . $formfields[usr_uid] . "\"
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
                  <td colspan=2>*Email Address[<b>2</b>]:</td>
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
                               SSH Pub Key: &nbsp<br>
                                    [<b>3</b>]
                              </center></td>

                  <td rowspan><center>Upload (1K max)[<b>4</b>]<br>
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
                  <td colspan=2>*Password[<b>2</b>]:</td>
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
    # Node information
    #
    echo "<tr><th colspan=3>
               Node Information: 
              </th>
          </tr>\n";

    #
    # Node ID
    #
    echo "<tr>
              <td colspan=2>*Node IP (dotted notation):</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[IP]\"
                         value=\"" . $formfields[IP] . "\"
	                 size=16 maxlength=16>
              </td>
          </tr>\n";

    #
    # Original Key
    #
    echo "<tr>
              <td colspan=2>*CD Key[<b>1</b>]:</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[cdkey]\"
                         value=\"" . $formfields[cdkey] . "\"
	                 size=32>
              </td>
          </tr>\n";

    #
    # Postal Address of the node
    #
    echo "<tr>
              <td colspan=2>*City:</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[node_city]\"
                         value=\"" . $formfields[node_city] . "\"
	                 size=30>
              </td>
          </tr>\n";

    echo "<tr>
              <td colspan=2>*State:</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[node_state]\"
                         value=\"" . $formfields[node_state] . "\"
	                 size=20>
              </td>
          </tr>\n";

    #
    # ZIP / Postal Code
    #
    echo "<tr>
              <td colspan=2>*ZIP/Postal Code:</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[node_zip]\"
                         value=\"" . $formfields[node_zip] . "\"
	                 size=10>
              </td>
          </tr>\n";

    #
    # Country
    #
    echo "<tr>
              <td colspan=2>*Country:</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[node_country]\"
                         value=\"" . $formfields[node_country] . "\"
	                 size=15>
              </td>
          </tr>\n";

    #
    # Hardware
    #
    echo "<tr>
              <td colspan=2>*Processor type/speed:<br>
                            (ex: P4/1.7Ghz)</td>

              <td class=left>
                  <input type=text
                         name=\"formfields[node_type]\"
                         value=\"" . $formfields[node_type] . "\"
	                 size=15>
              </td>
          </tr>\n";

    #
    # Connection Type
    #
    echo "<tr>
              <td colspan=2>*Connection type:<br>
              <td><select name=\"formfields[node_conn]\">
                          <option value=none>Please Select </option>\n";

    while (list ($type, $value) = each($connlist)) {
	$selected = "";

	if (isset($formfields[node_conn]) &&
	    strcmp($formfields[node_conn], $type) == 0)
	    $selected = "selected";

	echo "<option $selected value=$type>$type &nbsp </option>\n";
    }
    echo "       </select>
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
            <li> This is the original key you used to unlock your CD.\n";

    if (! $returning) {
	echo "<li> Please consult our
                   <a href = 'docwrapper.php3?docname=security.html'>
                   security policies</a> for information
                   regarding passwords and email addresses.

              <li> You <b>must</b> provide an ssh public key if you want
                   to be able to log into your node, since we do not distribute
                   passwords to widearea nodes. Please either
                   paste it in or specify the path to your
                   your identity.pub file. <font color=red>NOTE:</font>
                   We use the <a href=http://www.openssh.org>OpenSSH</a>
                   key format,
                   which has a slightly different protocol 2 public key format
                   than some of the commercial vendors such as
                   <a href=http://www.ssh.com>SSH Communications</a>. If you
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
# The conclusion of a request. See below.
# 
if (isset($finished)) {
    PAGEHEADER("Request an account on your Netbed node");

    echo "<center><h2>
           Your account request has been successfully queued.
          </h2></center>
          Testbed Operations has been notified of your request. We will
	  notify you by email as soon as your request is granted.\n";

    if (! $returning) {
	echo "<br>
              <p>
              In the meantime, as a new user you will receive a key via email.
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
    $defaults[usr_country] = "USA";
    $defaults[node_country] = "USA";

    #
    # These two allow presetting the IP and cdkey
    # 
    if (isset($IP) && strcmp($IP, "")) {
	$defaults[IP] = $IP;
    }
    if (isset($cdkey) && strcmp($cdkey, "")) {
	$defaults[cdkey] = $cdkey;
    }
    
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
    if (!isset($formfields[usr_uid]) ||
	strcmp($formfields[usr_uid], "") == 0) {
	$errors["Username"] = "Missing Field";
    }
    else {
	if (! ereg("^[a-zA-Z][-_a-zA-Z0-9]+$", $formfields[usr_uid])) {
	    $errors["UserName"] =
		"Must be lowercase alphanumeric only<br>".
		"and must begin with a lowercase alpha";
	}
	elseif (strlen($formfields[usr_uid]) > $TBDB_UIDLEN) {
	    $errors["UserName"] =
		"Too long! Must be less than or equal to $TBDB_UIDLEN";
	}
	elseif (TBCurrentUser($formfields[usr_uid])) {
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
	$errors["Address 1"] = "Missing Field";
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
    elseif (! CHECKPASSWORD($formfields[usr_uid],
			    $formfields[password1],
			    $formfields[usr_name],
			    $formfields[usr_email], $checkerror)) {
	$errors["Password"] = "$checkerror";
    }
}

# IP
if (!isset($formfields[IP]) ||
    strcmp($formfields[IP], "") == 0) {
    $errors["Node IP Address"] = "Missing Field";
}
else {
    if (! ereg("^[0-9\.]+$", $formfields[IP])) {
	$errors["Node IP Address"] =
	    "Must be standard dotted notation";
    }
    elseif (!strcmp($IP, "1.1.1.1") || !TBIPtoNodeID($formfields[IP])) {
	$errors["Node IP Address"] =
	    "No such widearea node $formfields[IP]";
    }
    else {
	$node_id = TBIPtoNodeID($formfields[IP]);

	#
	# If there is an entry in the widearea_nodeinfo table, allow for
	# those entries to be left blank.
	#
	$query_result =
	    DBQueryFatal("select * from widearea_nodeinfo ".
			 "where node_id='$node_id'");

	if (mysql_num_rows($query_result)) {
	    $haveinfo = 1;
	}
    }
}

if (!isset($formfields[cdkey]) ||
    strcmp($formfields[cdkey], "") == 0) {
    $errors["CD Key"] = "Missing Field";
}
else {
    if (! ereg("^[a-zA-Z0-9\ ]+$", $formfields[cdkey])) {
	$errors["CD Key"] = "Invalid Characters";
    }
}

if (! $haveinfo) {
    if (!isset($formfields[node_city]) ||
	strcmp($formfields[node_city], "") == 0) {
	$errors["City"] = "Missing Field";
    }
    
    if (!isset($formfields[node_state]) ||
	strcmp($formfields[node_state], "") == 0) {
	$errors["State"] = "Missing Field";
    }

    if (!isset($formfields[node_country]) ||
	strcmp($formfields[node_country], "") == 0) {
	$errors["Country"] = "Missing Field";
    }

    if (!isset($formfields[node_zip]) ||
	strcmp($formfields[node_zip], "") == 0) {
	$errors["ZIP/Postal Code"] = "Missing Field";
    }
    elseif (! ereg("^[-0-9a-zA-Z\ ,]+$", $formfields[node_zip])) {
	$errors["ZIP/Postal Code"] = "Invalid characters";
    }

    if (!isset($formfields[node_type]) ||
	strcmp($formfields[node_type], "") == 0) {
	$errors["Processor type"] = "Missing Field";
    }
    elseif (! ereg("^[-0-9a-zA-Z\/\ \.]+$", $formfields[node_type])) {
	$errors["Processor type"] = "Invalid characters";
    }
    
    if (!isset($formfields[node_conn]) ||
	strcmp($formfields[node_conn], "") == 0 ||
	strcmp($formfields[node_conn], "none") == 0) {
	$errors["Connection Type"] = "Not Selected";
    }
    elseif (! isset($connlist[$formfields[node_conn]])) {
	$errors["Connection Type"] = "Invalid Connection Type";
    }
}

if (count($errors)) {
    SPITFORM($formfields, $returning, $errors);
    PAGEFOOTER();
    return;
}

#
# Certain of these values must be escaped or otherwise sanitized.
#
$IP            = $formfields[IP];
$node_id       = TBIPtoNodeID($IP);
$cdkey	       = str_replace(" ", "", $formfields[cdkey]);
$usr_expires   = date("Y:m:d", time() + (86400 * 120));

if (! $haveinfo) {
    $node_type     = $formfields[node_type];
    $node_conn     = $formfields[node_conn];
    $node_city     = addslashes($formfields[node_city]);
    $node_state    = addslashes($formfields[node_state]);
    $node_zip      = $formfields[node_zip];
    $node_country  = addslashes($formfields[node_country]);
}

#
# Verify that the key is valid. This will weed out bozos who like to
# fill in random forms. 
#
$query_result =
    DBQueryFatal("select * from widearea_privkeys as w ".
		 "where w.lockkey='$cdkey' and w.IP='$IP'");

if (!mysql_num_rows($query_result)) {
    $errors["CD Key"] = "Invalid CD Key for $node_id";
}

if (!$returning) {
    $usr_uid           = $formfields[usr_uid];
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
    $usr_returning     = "No";

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
	    $addpubkeyargs = "-k '$usr_key' ";
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
	    $addpubkeyargs = "$usr_keyfile";
	    chmod($usr_keyfile, 0644);	
	}
    }
    #
    # Verify key format.
    #
    if (isset($addpubkeyargs) &&
	ADDPUBKEY($usr_uid, "webaddpubkey -n -u $usr_uid $addpubkeyargs")) {
	$errors["Pubkey Format"] = "Could not be parsed. Is it a public key?";
    }
}
else {
    #
    # Grab info from the DB for the email message below. Kinda silly.
    #
    $query_result =
	DBQueryFatal("select * from users where uid='$usr_uid'");
    
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
    $usr_returning = "Yes";
}

#
# Check for an existing entry in widearea_accounts.
#
$query_result =
    DBQueryFatal("select * from widearea_accounts ".
		 "where uid='$usr_uid' and node_id='$node_id'");

if (mysql_num_rows($query_result)) {
    $errors["IP Address"] = "You have already requested an account on $IP";
}

if (count($errors)) {
    SPITFORM($formfields, $returning, $errors);
    PAGEFOOTER();
    return;
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
	ADDPUBKEY($usr_uid, "webaddpubkey -u $usr_uid $addpubkeyargs");
    }

    # Unique Unix UID.
    $unix_uid = TBGetUniqueIndex('next_uid');

    DBQueryFatal("INSERT INTO users ".
	 "(uid,usr_created,usr_expires,usr_name,usr_email,usr_addr,".
	 " usr_addr2,usr_city,usr_state,usr_zip,usr_country, ".
	 " usr_URL,usr_title,usr_affil,usr_phone,usr_pswd,unix_uid,".
	 " status,pswd_expires,usr_modified,webonly) ".
	 "VALUES ('$usr_uid', now(), '$usr_expires', '$usr_name', ".
         "'$usr_email', ".
	 "'$usr_addr', '$usr_addr2', '$usr_city', '$usr_state', '$usr_zip', ".
	 "'$usr_country', ".
	 "'$usr_URL', '$usr_title', '$usr_affil', ".
	 "'$usr_phone', '$encoding', $unix_uid, 'newuser', ".
	 "date_add(now(), interval 1 year), now(), 1)");
    
    DBQueryFatal("INSERT INTO user_stats (uid, uid_idx) ".
		 "VALUES ('$usr_uid', $unix_uid)");

    $key = TBGenVerificationKey($usr_uid);

    TBMAIL("$usr_name '$usr_uid' <$usr_email>",
      "Your New User Key",
      "\n".
      "Dear $usr_name:\n\n".
      "This is your account verification key: $key\n\n".
      "Please use this link to verify your user account:\n".
      "\n".
      "    ${TBBASE}/login.php3?vuid=$usr_uid&key=$key\n".
      "\n".
      "Once you have verified your account, Testbed Operations will be\n".
      "able to approve you. You MUST verify your account first! After you\n".
      "have been approved by Testbed Operations, an acount will be created\n".
      "for you on $node_id.\n".
      "\n".
      "Thanks,\n".
      "Testbed Operations\n",
      "From: $TBMAIL_APPROVAL\n".
      "Bcc: $TBMAIL_AUDIT\n".
      "Errors-To: $TBMAIL_WWW");
}

#
# Enter a new widearea_accounts record. To be approved later.
#
DBQueryFatal("insert into widearea_accounts ".
	     "(uid, node_id, trust, date_applied) ".
	     "values ('$usr_uid','$node_id','none',now())");

#
# Enter a new widearea info record for the node.
#
if (! $haveinfo) {
    DBQueryFatal("insert into widearea_nodeinfo ".
		 "(node_id, machine_type, contact_uid, connect_type, ".
		 " city, state, country, zip) ".
		 "values ('$node_id', '$node_type', '$usr_uid', ".
		 "        '$node_conn', '$node_city', '$node_state',  ".
		 "        '$node_country', '$node_zip')");
}

#
# The mail message to the approval list.
# 
TBMAIL($TBMAIL_APPROVAL,
     "Widearea Node Account Request on $node_id ($usr_uid)",
     "'$usr_name' is requesting an account on widearea node '$node_id'.\n".
     "\n".
     "Name:            $usr_name ($usr_uid)\n".
     "Node ID:         $node_id ($IP)\n".
     "CD Key:          $cdkey\n".
     "Returning User?: $usr_returning\n".
     "Email:           $usr_email\n".
     "User URL:        $usr_URL\n".
     "Title:           $usr_title\n".
     "Affiliation:     $usr_affil\n".
     "Address 1:       $usr_addr\n".
     "Address 2:       $usr_addr2\n".
     "City:            $usr_city\n".
     "State:           $usr_state\n".
     "ZIP/Postal Code: $usr_zip\n".
     "Country:         $usr_country\n".
     "Phone:           $usr_phone\n",
     "From: $usr_name '$usr_uid' <$usr_email>\n".
     "Reply-To: $TBMAIL_APPROVAL\n".
     "Errors-To: $TBMAIL_WWW");

#
# Spit out a redirect so that the history does not include a post
# in it. The back button skips over the post and to the form.
# See above for conclusion.
# 
header("Location: widearea_register.php?finished=1");

?>
