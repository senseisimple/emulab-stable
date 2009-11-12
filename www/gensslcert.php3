<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2009 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments
#
$optargs = OptionalPageArguments("target_user", PAGEARG_USER,
				 "submit",      PAGEARG_STRING,
				 "finished",    PAGEARG_BOOLEAN,
				 "formfields",  PAGEARG_ARRAY);

# Default to current user if not provided.
if (!isset($target_user)) {
     $target_user = $this_user;
}

# Need these below
$target_uid = $target_user->uid();

#
# The conclusion.
# 
if (isset($finished)) {
    PAGEHEADER("Download SSL Certificate for user: $target_uid");

    $url = CreateURL("getsslcert", $target_user);
    
    echo "<blockquote>
          <a href='$url'>Download</a> your 
          certificate and private key in PEM format, and then save
          it to a file in your .ssl directory.
          <br>
          <br>
          You can also download it in <a href='$url&p12=1'><em>pkc12</em></a>
          format for loading
          into your web browser (if you do not know what this means, or why
          you need to do this, then ignore this).
          </blockquote>\n";
	    
    PAGEFOOTER();
    return;
}

#
# Standard Testbed Header, now that we know what we want to say.
#
PAGEHEADER("Generate SSL Certificate for user: $target_uid");

#
# Only admin people can create SSL certs for another user.
#
if (!$isadmin && !$target_user->SameUser($this_user)) {
    USERERROR("You do not have permission to create SSL certs ".
	      "for $target_uid!", 1);
}

function SPITFORM($target_user, $formfields, $errors)
{
    global $isadmin, $BOSSNODE;

    $target_uid    = $target_user->uid();
    $target_webid  = $target_user->webid();

    echo "<blockquote>
          By downloading an encrypted SSL certificate, you are able to use
          Emulab's XMLRPC server from your desktop or home machine. This
          certificate must be pass phrase protected, and allows you to issue
          any of the RPC requests documented in the <a href=xmlrpcapi.php3>
          Emulab XMLRPC Reference</a>.</blockquote><br>\n";
    
    echo "<center>
          Create an SSL Certificate[<b>1</b>]
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
    echo "<input type=hidden name=\"formfields[user]\" ".
	         "value=$target_webid>\n";

    echo "<tr>
              <td>PassPhrase[<b>2</b>]:</td>
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
                  <td>Emulab Password[<b>3</b>]:</td>
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
            <li> This is an <b>encrypted key</b> and should <b>not</b> replace
                 your <tt>emulab.pem</tt> in your <tt>.ssl</tt> directory.
            <li> You must supply a passphrase to use when encrypting the
                 private key for your SSL certificate. You will be prompted
                 for this passphrase whenever you attempt to use it. Pick
                 a good one!";
    if (!$isadmin) {
	echo "<li> As a security precaution, you must supply your Emulab user
                 password when creating new ssl certificates. ";
    }
    echo "</ol>
          </blockquote></blockquote></blockquote>\n";
}

#
# On first load, display a form of current values.
#
if (! isset($_POST['submit'])) {
    $defaults = array();
    
    SPITFORM($target_user, $defaults, 0);
    PAGEFOOTER();
    return;
}

# Must get formfields.
if (!isset($formfields)) {
    PAGEARGERROR("Invalid form arguments; no formfields arrary.");
}

#
# Otherwise, must validate and redisplay if errors
#
$errors = array();

#
# Need this for checkpass.
#
$user_name  = $target_user->name();
$user_email = $target_user->email();

#TBERROR("$target_uid, $user_name, $user_email, " .
#	$formfields[passphrase1], 0); 

#
# Must supply a reasonable passphrase.
# 
if (!isset($formfields["passphrase1"]) ||
    strcmp($formfields["passphrase1"], "") == 0) {
    $errors["Passphrase"] = "Missing Field";
}
if (!isset($formfields["passphrase2"]) ||
    strcmp($formfields["passphrase2"], "") == 0) {
    $errors["Confirm Passphrase"] = "Missing Field";
}
elseif (strcmp($formfields["passphrase1"], $formfields["passphrase2"])) {
    $errors["Confirm Passphrase"] = "Does not match Passphrase";
}
elseif (! CHECKPASSWORD($target_uid,
			$formfields["passphrase1"],
			$user_name,
			$user_email, $checkerror)) {
    $errors["Passphrase"] = "$checkerror";
}

#
# Must verify passwd to create an SSL key.
#
if (! $isadmin) {
    if (!isset($formfields["password"]) ||
	strcmp($formfields["password"], "") == 0) {
	$errors["Password"] = "Must supply a verification password";
    }
    elseif (VERIFYPASSWD($target_uid, $formfields["password"]) != 0) {
	$errors["Password"] = "Incorrect password";
    }
}

# Spit the errors
if (count($errors)) {
    SPITFORM($target_user, $formfields, $errors);
    PAGEFOOTER();
    return;
}

#
# Insert key, update authkeys files and nodes if appropriate.
#
STARTBUSY("Generating Certificate");
SUEXEC($target_uid, "nobody",
       "webmkusercert -p " .
       escapeshellarg($formfields["passphrase1"]) . " $target_uid",
       SUEXEC_ACTION_DIE);
STOPBUSY();

#
# Redirect back, avoiding a POST in the history.
#
PAGEREPLACE(CreateURL("gensslcert", $target_user, "finished", 1));

PAGEFOOTER();
?>
