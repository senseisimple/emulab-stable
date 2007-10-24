<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie(CHECKLOGIN_USERSTATUS|CHECKLOGIN_WEBONLY);
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page/form arguments.
#
$optargs = OptionalPageArguments("target_user",   PAGEARG_USER,
				 "submit",        PAGEARG_STRING,
				 "formfields",    PAGEARG_ARRAY);

# Default to current user.
if (!isset($target_user)) {
    $target_user = $this_user;
}
$target_uid = $target_user->uid();

#
# Verify that this uid is a member of one of the projects that the
# target_uid is in. Must have proper permission in that group too. 
#
if (!$isadmin && 
    !$target_user->AccessCheck($this_user, $TB_USERINFO_READINFO)) {
    USERERROR("You do not have permission to view ${target_uid}'s keys!", 1);
}

#
# Spit the form out using the array of data.
#
function SPITFORM($formfields, $errors)
{
    global $isadmin, $target_user, $BOSSNODE;

    $target_uid = $target_user->uid();
    $uid_idx    = $target_user->uid_idx();
    $webid      = $target_user->webid();

    #
    # Standard Testbed Header, now that we know what we want to say.
    #
    PAGEHEADER("SSH Public Keys for user: $target_uid");

    #
    # Get the list and show it.
    #
    $query_result =&
	$target_user->TableLookUp("user_pubkeys", "*");

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
	    $comment = $row['comment'];
	    $pubkey  = $row['pubkey'];
	    $date    = $row['stamp'];
	    $idx     = $row['idx'];
	    $fnote   = "";

	    if (strstr($comment, $BOSSNODE)) {
		$fnote = "[<b>1</b>]";
	    }
	    $chunky  = chunk_split("$pubkey $fnote", 75, "<br>\n");

	    $delurl = CreateURL("deletepubkey", $target_user, "key", $idx);

	    echo "<tr>
                     <td align=center>
                       <A href='$delurl'>
                          <img alt='Delete Key' src=redball.gif></A>
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
    $url = CreateURL("showpubkeys", $target_user);

    echo "<table align=center border=1> 
          <form enctype=multipart/form-data
                action='$url' method=post>\n";

    #
    # SSH public key
    # 
    echo "<tr>
              <td>Upload Public Key[<b>3,4</b>]:<br>
                    (4K max)
              </td>

              <td>
                  <input type=hidden name=MAX_FILE_SIZE value=4096>
	          <input type=file
                         name=usr_keyfile
                         value=\"" .
	                           (isset($_FILES['usr_keyfile']) ?
				    $_FILES['usr_keyfile']['name'] : "") . "\"
	                 size=50>
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
          key file and we will convert it for you.\n";
}

#
# On first load, display a form of current values.
#
if (!isset($submit)) {
    $defaults = array();
    $defaults["password"] = "";
    SPITFORM($defaults, 0);
    PAGEFOOTER();
    return;
}

# Form submitted. Make sure we have a formfields array.
if (!isset($formfields)) {
    PAGEARGERROR("Invalid form arguments.");
}

#
# Otherwise, must validate and redisplay if errors
#
$errors = array();

#
# If usr provided a file for the key ...
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
	$keyfile = $localfile;
	chmod($localfile, 0644);	
    }
}

#
# Must verify passwd to add keys.
#
if (isset($keyfile)) {
    if (! $isadmin) {
	if (!isset($formfields["password"]) ||
	    strcmp($formfields["password"], "") == 0) {
	    $errors["Password"] = "Must supply a verification password";
	}
	elseif (VERIFYPASSWD($target_uid, $formfields["password"]) != 0) {
	    $errors["Password"] = "Incorrect password";
	}
    }
}
else {
    $errors["Missing Args"] = "Please supply keyfile";
}

# Spit the errors
if (count($errors)) {
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    return;
}

#
# Build up argument array to pass along.
#
$args = array();

$args["user"] = $target_uid;

if (isset($keyfile) && $keyfile != "") {
    $args["keyfile"] = $keyfile;
}

#
# Okay, first run the script in verify mode to see if the key is
# parsable. If it is, then do it for real.
#
$args["verify"] = 1;
if (! ($result = NewPubKey($args, $errors))) {
    $errors["Pubkey Format"] = "Could not be parsed. Is it a public key?";

    # Always respit the form so that the form fields are not lost.
    # I just hate it when that happens so lets not be guilty of it ourselves.
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    return;
}

#
# Insert key, update authkeys files and nodes if appropriate.
#
$args["verify"] = 0;
if (! ($result = NewPubKey($args, $errors))) {
    # Always respit the form so that the form fields are not lost.
    # I just hate it when that happens so lets not be guilty of it ourselves.
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    return;
}

#
# Redirect back, avoiding a POST in the history.
# 
header("Location: ". CreateURL("showpubkeys", $target_user));

#
# When there's a PubKeys class, this will be a Class function to edit them...
#
function NewPubKey($args, &$errors) {
    global $suexec_output, $suexec_output_array, $TBADMINGROUP;

    #
    # Generate a temporary file and write in the XML goo.
    #
    $xmlname = tempnam("/tmp", "addpubkey");
    if (! $xmlname) {
	TBERROR("Could not create temporary filename", 0);
	$errors[] = "Transient error(1); please try again later.";
	return null;
    }
    if (! ($fp = fopen($xmlname, "w"))) {
	TBERROR("Could not open temp file $xmlname", 0);
	$errors[] = "Transient error(2); please try again later.";
	return null;
    }

    fwrite($fp, "<PubKey>\n");
    foreach ($args as $name => $value) {
	fwrite($fp, "<attribute name=\"$name\">");
	fwrite($fp, "  <value>" . htmlspecialchars($value) . "</value>");
	fwrite($fp, "</attribute>\n");
    }
    fwrite($fp, "</PubKey>\n");
    fclose($fp);
    chmod($xmlname, 0666);

    $retval = SUEXEC("nobody", "nobody", "webaddpubkey -X $xmlname",
		     SUEXEC_ACTION_IGNORE);

    if ($retval) {
	if ($retval < 0) {
	    $errors[] = "Transient error(3, $retval); please try again later.";
	    SUEXECERROR(SUEXEC_ACTION_CONTINUE);
	}
	else {
	    # unlink($xmlname);
	    if (count($suexec_output_array)) {
		for ($i = 0; $i < count($suexec_output_array); $i++) {
		    $line = $suexec_output_array[$i];
		    if (preg_match("/^([-\w]+):\s*(.*)$/",
				   $line, $matches)) {
			$errors[$matches[1]] = $matches[2];
		    }
		    else
			$errors[] = $line;
		}
	    }
	    else
		$errors[] = "Transient error(4, $retval); please try again later.";
	}
	return null;
    }

    # There are no return value(s) to parse at the end of the output.

    # Unlink this here, so that the file is left behind in case of error.
    # We can then edit the pubkeys by hand from the xmlfile, if desired.
    unlink($xmlname);

    return true; 
}

?>
