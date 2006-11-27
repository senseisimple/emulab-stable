<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2005, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid, CHECKLOGIN_USERSTATUS|CHECKLOGIN_WEBONLY);
$isadmin = ISADMIN($uid);

#
# Verify page/form arguments. Note that the target uid comes initially as a
# page arg, but later as a form argument, hence this odd check.
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
	PAGEARGERROR("Invalid form arguments.");
    }
    $formfields = $_POST['formfields'];
    $target_uid = $formfields['target_uid'];
}

# Pedantic check of uid before continuing.
if ($target_uid == "" || !TBvalid_uid($target_uid)) {
    PAGEARGERROR("Invalid uid: '$target_uid'");
}

#
# Check to make sure thats this is a valid UID.
#
if (! TBCurrentUser($target_uid)) {
    USERERROR("The user $target_uid is not a valid user", 1);
}

#
# Verify that this uid is a member of one of the projects that the
# target_uid is in. Must have proper permission in that group too. 
#
if (!$isadmin &&
    strcmp($uid, $target_uid)) {

    if (! TBUserInfoAccessCheck($uid, $target_uid, $TB_USERINFO_READINFO)) {
	USERERROR("You do not have permission to view ${user}'s keys!", 1);
    }
}

function SPITFORM($formfields, $errors)
{
    global $isadmin, $target_uid, $BOSSNODE;

    #
    # Standard Testbed Header, now that we know what we want to say.
    #
    if (strcmp($uid, $target_uid)) {
	PAGEHEADER("SSH Public Keys for user: $target_uid");
    }
    else {
	PAGEHEADER("My SSH Public Keys");
    }

    #
    # Get the list and show it.
    #
    $query_result =
	DBQueryFatal("select * from user_pubkeys where uid='$target_uid'");

    if (mysql_num_rows($query_result)) {
	echo "<table align=center border=1 cellpadding=2 cellspacing=2>\n";

	echo "<center>
                Current ssh public keys for user $target_uid.
              </center><br>\n";

	echo "<tr>
                 <th>Delete?</th>
                 <th>Key</th>
              </tr>\n";

	while ($row = mysql_fetch_array($query_result)) {
	    $comment = $row[comment];
	    $pubkey  = $row[pubkey];
	    $date    = $row[stamp];
	    $idx     = $row[idx];
	    $fnote   = "";

	    if (strstr($comment, $BOSSNODE)) {
		$fnote = "[<b>1</b>]";
	    }
	    $chunky  = chunk_split("$pubkey $fnote", 75, "<br>\n");

	    echo "<tr>
                     <td align=center>
                       <A href='deletepubkey.php3?target_uid=$target_uid" .
	                  "&key=$idx'><img alt=X src=redball.gif></A>
                     </td>
                     <td>$chunky</td>
                  </tr>\n";
	}
	echo "</table>\n";
    }
    else {
	echo "<center>
             There are no public keys on file for user $target_uid!
             </center>\n";
    }
    echo "<blockquote><blockquote><blockquote>
          <ol>
            <li> Please do not delete your Emulab generated public key.
          </ol>
          </blockquote></blockquote></blockquote>\n";

    echo "<br><hr size=4>\n";
    echo "<center>
          Enter ssh public keys for user
                    ${target_uid} [<b>1,2</b>]
          <br>
          (<em>We <b>strongly</b>
             encourage the use of Protocol 2 keys only!</em> [<b>6</b>])
          </center><br>\n";

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
          <form enctype=multipart/form-data
                action=showpubkeys.php3 method=post>\n";
    echo "<input type=hidden name=\"formfields[target_uid]\" ".
	         "value=$target_uid>\n";

    #
    # SSH public key
    # 
    echo "<tr>
              <td rowspan><center>Upload (4K max)[<b>3,4</b>]<br>
                              <b>Or</b><br>
                           Insert Key
                          </center></td>

              <td rowspan>
                  <input type=hidden name=MAX_FILE_SIZE value=4096>
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
    # Verify with password.
    #
    if (!$isadmin) {
	echo "<tr>
                  <td>Password[<b>5</b>]:</td>
                  <td class=left>
                      <input type=password
                             name=\"formfields[password]\"
                             size=12></td>
              </tr>\n";
    }

    echo "<tr>
              <td colspan=2 align=center>
                 <b><input type=submit name=submit value='Add New Keys'></b>
              </td>
          </tr>\n";

    echo "</form>
          </table>\n";

    echo "<blockquote><blockquote><blockquote>
          <ol>
            <li> Please consult our
                 <a href = 'docwrapper.php3?docname=security.html#SSH'>
                 security policies</a> for information
                 regarding ssh public keys.
            <li> You should not hand edit your your authorized_keys file on
                 Emulab (.ssh/authorized_keys) since modifications via this
                 page will overwrite the file.
            <li> Note to <a href=http://www.opera.com><b>Opera 5</b></a> users:
                 The file upload mechanism is broken in Opera, so you cannot
                 specify a local file for upload. Instead, please paste your
                 key in.
            <li> Typically, the file you want to upload is your
                 identity.pub, contained in your .ssh directory.
            <li> As a security precaution, you must supply your password
                 when adding new ssh public keys. 
            <li> Protocol 2 keys are more secure then Protocol 1 keys, and
                 Emulab will soon <b>require</b> Protocol 2 keys.
          </ol>
          </blockquote></blockquote></blockquote>\n";

    echo "<font color=red>NOTE:</font> We use the
          <a href=www.openssh.org>OpenSSH</a> key format, which has a slightly
          different protocol 2 public key format than some of the commercial 
          vendors such as <a href=www.ssh.com>SSH Communications</a>. If you
          use one of these commercial vendors, then please upload the public
          key file and we will convert it for you. <i>Please do not paste
          it in.</i>\n";
}

#
# On first load, display a form of current values.
#
if (! isset($_POST['submit'])) {
    $defaults = array();
    
    SPITFORM($defaults, 0);
    PAGEFOOTER();
    return;
}

#
# Otherwise, must validate and redisplay if errors
#
$errors = array();

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
	$formfields[usr_key] = ereg_replace("[\n]", "", $formfields[usr_key]);
	$usr_key = $formfields[usr_key];
	$addpubkeyargs = "-k '$usr_key' ";
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
	$addpubkeyargs = "$localfile";
	chmod($localfile, 0644);	
    }
}

#
# Must verify passwd to add keys.
#
if (isset($addpubkeyargs)) {
    if (! $isadmin) {
	if (!isset($formfields[password]) ||
	    strcmp($formfields[password], "") == 0) {
	    $errors["Password"] = "Must supply a verification password";
	}
	elseif (VERIFYPASSWD($target_uid, $formfields[password]) != 0) {
	    $errors["Password"] = "Incorrect password";
	}
    }
}
else {
    $errors["Missing Args"] = "Please supply a key or a keyfile";
}

# Spit the errors
if (count($errors)) {
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    return;
}

#
# Okay, first run the script in verify mode to see if the key is
# parsable. If it is, then do it for real.
#
if (ADDPUBKEY($uid, "webaddpubkey -n -u $target_uid $addpubkeyargs")) {
    $errors["Pubkey Format"] = "Could not be parsed. Is it a public key?";
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    return;
}
#
# Insert key, update authkeys files and nodes if appropriate.
#
ADDPUBKEY($uid, "webaddpubkey -u $target_uid $addpubkeyargs");

#
# Redirect back, avoiding a POST in the history.
# 
header("Location: showpubkeys.php3?target_uid=$target_uid");
?>
