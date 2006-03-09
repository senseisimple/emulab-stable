<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

#
# The conclusion.
# 
if (isset($_GET['finished'])) {
    PAGEHEADER("Generate SSL Certificate");

    $target_uid = $_GET['target_uid'];
    
    echo "Your new SSL certificate has been created. You can
          <a href=getsslcert.php3?target_uid=$target_uid>download</a> your 
          certificate and private key in PEM format, and then save
          it to a file in your .ssl directory.\n";
	    
    PAGEFOOTER();
    return;
}

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
# Only admin people can create SSL certs for another user.
#
if (!$isadmin &&
    strcmp($uid, $target_uid)) {
    USERERROR("You do not have permission to create SSL certs for $user!", 1);
}

function SPITFORM($formfields, $errors)
{
    global $isadmin, $target_uid, $BOSSNODE;

    #
    # Standard Testbed Header, now that we know what we want to say.
    #
    if (strcmp($uid, $target_uid)) {
	PAGEHEADER("Generate SSL Certificate for user: $target_uid");
    }
    else {
	PAGEHEADER("Generate SSL Certificate");
    }

    echo "<blockquote>
          By downloading an encrypted SSL certificate, you are able to use
          Emulab's XMLRPC server from your desktop or home machine. This
          certificate must be pass phrase protected, and allows you to issue
          any of the RPC requests documented in the <a href=xmlrpcapi.php3>
          Emulab XMLRPC Reference</a>.</blockquote><br>\n";
    
    echo "<center>
          Create an SSL Certificate
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
                action=gensslcert.php3 method=post>\n";
    echo "<input type=hidden name=\"formfields[target_uid]\" ".
	         "value=$target_uid>\n";

    echo "<tr>
              <td>PassPhrase[<b>1</b>]:</td>
              <td class=left>
                  <input type=password
                         name=\"formfields[passphrase1]\"
                         size=24></td>
          </tr>\n";

    echo "<tr>
              <td>Confirm PassPhrase:</td>
              <td class=left>
                  <input type=password
                         name=\"formfields[passphrase2]\"
                         size=24></td>
          </tr>\n";

    #
    # Verify with password.
    #
    if (!$isadmin) {
	echo "<tr>
                  <td>Emulab Password[<b>2</b>]:</td>
                  <td class=left>
                      <input type=password
                             name=\"formfields[password]\"
                             size=12></td>
              </tr>\n";
    }

    echo "<tr>
              <td colspan=2 align=center>
                 <b><input type=submit name=submit value='Create SSL Cert'></b>
              </td>
          </tr>\n";

    echo "</form>
          </table>\n";

    echo "<blockquote><blockquote><blockquote>
          <ol>
            <li> You must supply a passphrase to use when encrypting the
                 private key for your SSL certificate. You will be prompted
                 for this passphrase whenever you attempt to use it. Pick
                 a good one!

            <li> As a security precaution, you must supply your Emulab user
                 password when creating new ssl certificates. 
          </ol>
          </blockquote></blockquote></blockquote>\n";
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

#
# Need this for checkpass.
#
TBUserInfo($target_uid, $user_name, $user_email);

#TBERROR("$target_uid, $user_name, $user_email, " .
#	$formfields[passphrase1], 0); 

#
# Must supply a reasonable passphrase.
# 
if (!isset($formfields[passphrase1]) ||
    strcmp($formfields[passphrase1], "") == 0) {
    $errors["Passphrase"] = "Missing Field";
}
if (!isset($formfields[passphrase2]) ||
    strcmp($formfields[passphrase2], "") == 0) {
    $errors["Confirm Passphrase"] = "Missing Field";
}
elseif (strcmp($formfields[passphrase1], $formfields[passphrase2])) {
    $errors["Confirm Passphrase"] = "Does not match Passphrase";
}
elseif (! CHECKPASSWORD($target_uid,
			$formfields[passphrase1],
			$user_name,
			$user_email, $checkerror)) {
    $errors["Passphrase"] = "$checkerror";
}

#
# Must verify passwd to create an SSL key.
#
if (! $isadmin) {
    if (!isset($formfields[password]) ||
	strcmp($formfields[password], "") == 0) {
	$errors["Password"] = "Must supply a verification password";
    }
    elseif (VERIFYPASSWD($target_uid, $formfields[password]) != 0) {
	$errors["Password"] = "Incorrect password";
    }
}

# Spit the errors
if (count($errors)) {
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    return;
}

#
# Insert key, update authkeys files and nodes if appropriate.
#
SUEXEC($target_uid, "nobody",
       "webmkusercert -p " .
       escapeshellarg($formfields[passphrase1]) . " $target_uid",
       SUEXEC_ACTION_DIE);

#
# Redirect back, avoiding a POST in the history.
# 
header("Location: gensslcert.php3?finished=1&target_uid=$target_uid");
?>
