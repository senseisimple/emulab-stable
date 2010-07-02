<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("user",   PAGEARG_STRING);
$optargs = OptionalPageArguments("simple", PAGEARG_BOOLEAN,
				 "key",    PAGEARG_STRING,
				 "reset",  PAGEARG_STRING);

# Display a simpler version of this page.
if (isset($simple) && $simple) {
    $simple = 1;
    $view = array('hide_banner' => 1,
		  'hide_copyright' => 1,
		  'hide_sidebar' => 1);
}
else {
    $simple = 0;
    $view   = array();
}

# Half the key in the URL.
$keyB = $key;
# We also need the other half of the key from the browser.
$keyA = (isset($_COOKIE[$TBAUTHCOOKIE]) ? $_COOKIE[$TBAUTHCOOKIE] : "");

# If the browser part is missing, direct user to answer
if ((isset($keyB) && $keyB != "") && (!isset($keyA) || $keyA == "")) {
    PAGEHEADER("Reset Your Password", $view);

    USERERROR("Oops, not able to proceed!<br>".
	      "Please read this ".
	      "<a href='$WIKIDOCURL/kb69'>".
	      "Knowledge Base Entry</a> to see what the likely cause is.", 1);
}

if (!isset($user) || $user == "" || !User::ValidWebID($user) ||
    !isset($keyA) || $keyA == "" || !preg_match("/^[\w]+$/", $keyA) ||
    !isset($keyB) || $keyB == "" || !preg_match("/^[\w]+$/", $keyB)) {
    PAGEARGERROR();
}
# The complete key.
$key = $keyA . $keyB;

# Must use https!
if (!isset($_SERVER["SSL_PROTOCOL"])) {
    PAGEHEADER("Reset Your Password", $view);
    USERERROR("Must use https:// to access this page!", 1);
}

#
# Must not be logged in.
# 
if (GETLOGIN() != FALSE) {
    PAGEHEADER("Reset Your Password", $view);

    echo "<h3>
              You are logged in. You must already know your password!
          </h3>\n";

    PAGEFOOTER($view);
    die("");
}

#
# Spit out the form.
# 
function SPITFORM($target_user, $key, $failed, $simple, $view)
{
    global	$TBBASE;

    $uid = $target_user->uid();
    
    PAGEHEADER("Reset Your Password", $view);

    if ($failed) {
	echo "<center>
              <font size=+1 color=red>
              $failed. Please try again.
              </font>
              </center><br>\n";
    }
    else {
	echo "<center>
              <font size=+1>
              Please enter a new password.<br><br>
              </font>
              </center>\n";
    }

    $chpass_url = CreateURL("chpasswd", $target_user,
			    "key", $key, "simple", $simple);
	
    echo "<table align=center border=1>
          <form action='${TBBASE}/$chpass_url' method=post>\n";

    echo "<tr>
              <td>Password:</td>
              <td class=left>
                  <input type=password
                         name=\"password1\"
                         size=12></td>
          </tr>\n";

    echo "<tr>
              <td>Retype Password:</td>
              <td class=left>
                  <input type=password
                         name=\"password2\"
                         size=12></td>
         </tr>\n";

    echo "<tr>
             <td align=center colspan=2>
                 <b><input type=submit value=\"Reset Password\"
                           name=reset></b></td>
          </tr>\n";
    

    echo "</form>
          </table>\n";
}

#
# Check to make sure that the key is valid and that the timeout has not
# expired.
#
if (! ($target_user = User::Lookup($user))) {
    # Silent error about invalid users.
    PAGEARGERROR();
}
$usr_email  = $target_user->email();
$usr_name   = $target_user->name();
$target_uid = $target_user->uid();

# Silent error when there is no key/timeout set for the user.
if (!$target_user->chpasswd_key() || !$target_user->chpasswd_expires()) {
    PAGEARGERROR();
}
if ($target_user->chpasswd_key() != $key) {
    USERERROR("You do not have permission to change your password!", 1);
}
if (time() > $target_user->chpasswd_expires()) {
    USERERROR("Your key has expired. Please request a
               <a href='password.php3'>new key</a>.", 1);
}

#
# If not clicked, then put up a form.
#
if (! isset($reset)) {
    SPITFORM($target_user, $keyB, 0, $simple, $view);
    PAGEFOOTER();
    return;
}

#
# Reset clicked. Verify a proper password. 
#
$password1 = $_POST['password1'];
$password2 = $_POST['password2'];

if (!isset($password1) || $password1 == "" ||
    !isset($password2) || $password2 == "") {
    SPITFORM($target_user, $keyB,
	     "You must supply a password", $simple, $view);
    PAGEFOOTER();
    return;
}
if ($password1 != $password2) {
    SPITFORM($target_user, $keyB,
	     "Two passwords do not match", $simple, $view);
    PAGEFOOTER();
    return;
}
if (! CHECKPASSWORD($target_uid,
		    $password1, $usr_name, $usr_email, $checkerror)){
    SPITFORM($target_user, $keyB, $checkerror, $simple, $view);
    PAGEFOOTER();
    return;
}

# Clear the cookie from the browser.
setcookie($TBAUTHCOOKIE, "", time() - 1000000, "/", $TBAUTHDOMAIN, 0);

# Okay to spit this now that the cookie has been sent (cleared).
PAGEHEADER("Reset Your Password", $view);

$encoding = crypt("$password1");
$safe_encoding = escapeshellarg($encoding);

STARTBUSY("Resetting your password");

#
# Invoke backend to deal with this.
#
if (!HASREALACCOUNT($target_uid)) {
    SUEXEC("nobody", "nobody",
	   "webtbacct passwd $target_uid $safe_encoding",
	   SUEXEC_ACTION_DIE);
}
else {
    SUEXEC($target_uid, "nobody",
	   "webtbacct passwd $target_uid $safe_encoding",
	   SUEXEC_ACTION_DIE);
}

CLEARBUSY();

echo "<br>
      Your password has been changed.\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
