<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

#
# Verify form arguments.
# 
if (!isset($target_uid) ||
    strcmp($target_uid, "") == 0) {
    $target_uid = $uid;
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
    global $isadmin, $usr_keyfile_name, $target_uid, $BOSSNODE;

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
                 <td>Delete?</td>
                 <td>Key</td>
              </tr>\n";

	while ($row = mysql_fetch_array($query_result)) {
	    $comment = $row[comment];
	    $pubkey  = $row[pubkey];
	    $date    = $row[stamp];
	    $fnote   = "";

	    if (strstr($comment, $BOSSNODE)) {
		$fnote = "[<b>1</b>]";
	    }
	    $chunky  = chunk_split("$pubkey $fnote", 75, "<br>\n");

	    echo "<tr>
                     <td align=center>
                       <A href='deletepubkey.php3?target_uid=$target_uid" .
	                  "&key=$comment'><img alt=X src=redball.gif></A>
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
          Enter new ssh public keys for user ${target_uid}[<b>1,2</b>].
          </center><br>\n";

    if ($errors) {
	echo "<table align=center border=0 cellpadding=0 cellspacing=2>
              <tr>
                 <td align=center colspan=3>
                   <font size=+1 color=red>
                      Oops, please fix the following errors!
                   </font>
                 </td>
              </tr>\n";

	while (list ($name, $message) = each ($errors)) {
	    echo "<tr>
                     <td align=right><font color=red>$name:</font></td>
                     <td>&nbsp</td>
                     <td align=left><font color=red>$message</font></td>
                  </tr>\n";
	}
	echo "</table><br>\n";
    }

    echo "<table align=center border=1> 
          <form enctype=multipart/form-data
                action=showpubkeys.php3?target_uid=$target_uid method=post>\n";

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
	                 size=50>
                  <br>
                  <br>
	          <input type=text
                         name=\"formfields[usr_key]\"
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
                             size=8></td>
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
          </ol>
          </blockquote></blockquote></blockquote>\n";
}

#
# On first load, display a form of current values.
#
if (! isset($submit) || isset($finished)) {
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
    # Replace any embedded newlines first.
    #
    $formfields[usr_key] = ereg_replace("[\n]", "", $formfields[usr_key]);

    if (! ereg("^[0-9a-zA-Z\@\. ]*$", $formfields[usr_key])) {
	$errors["PubKey"] = "Invalid characters";

	SPITFORM($formfields, $errors);
	PAGEFOOTER();
	return;
    }
    else {
	$usr_key[] = $formfields[usr_key];
    }
}

#
# If usr provided a file for the key, it overrides the paste in text.
# Must read and check it.
#
if (isset($usr_keyfile) &&
    strcmp($usr_keyfile, "") &&
    strcmp($usr_keyfile, "none")) {

    if (! ($fp = fopen($usr_keyfile, "r"))) {
	TBERROR("Could not open $usr_keyfile", 1);
    }
    while (!feof($fp)) {
	$buffer = fgets($fp, 4096);

	if (ereg("^[\n\#]", $buffer))
	    continue;

	if (! ereg("^[0-9a-zA-Z\@\.[:space:]\r\n]*$", $buffer)) {
	    $errors["PubKey File Contents"] = "Invalid characters";

	    fclose($fp);
	    SPITFORM($formfields, $errors);
	    PAGEFOOTER();
	    return;
	}
	$usr_key[] = Chop($buffer);
    }
    fclose($fp);
}

#
# Insert each key. The comment field serves as the secondary key
# to avoid duplication.
# 
if (isset($usr_key)) {
    if (! $isadmin) {
	if (!isset($formfields[password]) ||
	    strcmp($formfields[password], "") == 0) {
	    $errors["Password"] = "Must supply a verification password";
	}
	elseif (VERIFYPASSWD($target_uid, $formfields[password]) != 0) {
	    $errors["Password"] = "Incorrect password";
	}

	if (count($errors)) {
	    SPITFORM($formfields, $errors);
	    PAGEFOOTER();
	    return;
	}
    }
    
    $chunky = "";
    
    while (list ($idx, $stuff) = each ($usr_key)) {
	#
	# Need to separate out the comment field. 
	#
	$pieces = explode(" ", $stuff);

	if (count($pieces) != 4) {
	    if (count($pieces) != 1) {
		TBERROR("Bad Key for $target_uid: $stuff", 0);
	    }
	    continue;
	}
	# These have already been tested for bad chars above (ereg).
	$key     = "$pieces[0] $pieces[1] $pieces[2] $pieces[3]";
	$comment = $pieces[3];
	
	DBQueryFatal("replace into user_pubkeys ".
		     "values ('$target_uid', '$comment', '$key', now())");

	$chunky .= chunk_split($key, 70, "\n");
	$chunky .= "\n";
    }

    DBQueryFatal("update users set usr_modified=now() ".
		 "where uid='$target_uid'");

    #
    # Audit
    #
    TBUserInfo($uid, $uid_name, $uid_email);
    TBUserInfo($target_uid, $targuid_name, $targuid_email);

    TBMAIL("$targuid_name <$targuid_email>",
	   "SSH Public Key for '$target_uid' Added",
	   "\n".
	   "SSH Public Key for '$target_uid' added by '$uid'.\n".
	   "\n".
	   "$chunky\n".
	   "\n".
	   "Thanks,\n".
	   "Testbed Ops\n".
	   "Utah Network Testbed\n",
	   "From: $uid_name <$uid_email>\n".
	   "Cc: $TBMAIL_AUDIT\n".
	   "Errors-To: $TBMAIL_WWW");

    #
    # mkacct updates the user pubkeys.
    # 
    SUEXEC($uid, "flux", "webmkacct -a $target_uid", 0);
}

header("Location: showpubkeys.php3?target_uid=$target_uid&finished=1");
?>
