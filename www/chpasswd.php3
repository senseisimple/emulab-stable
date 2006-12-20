<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2005, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

# Display a simpler version of this page
$simple = 0;
if (isset($_REQUEST['simple'])) {
    $simple = $_REQUEST['simple'];
}

# Form arguments.
$user      = $_REQUEST['user'];
$keyB      = $_REQUEST['key'];
# We also need the other half of the key from the browser.
$keyA      = $HTTP_COOKIE_VARS[$TBAUTHCOOKIE];

# If the browser part is missing, direct user to answer
if ((isset($keyB) && $keyB != "") && (!isset($keyA) || $keyA == "")) {
    PAGEHEADER("Reset Your Password", $view);

    USERERROR("Oops, not able to proceed!<br>".
	      "Please read this ".
	      "<a href='kb-show.php3?xref_tag=forgotpassword'>".
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
if (!isset($SSL_PROTOCOL)) {
    PAGEHEADER("Reset Your Password", $view);
    USERERROR("Must use https:// to access this page!", 1);
}

#
# Turn off some of the decorations and menus for the simple view
#
if ($simple) {
    $view = array('hide_banner' => 1, 'hide_copyright' => 1,
	'hide_sidebar' => 1);
} else {
    $view = array();
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
$expires  = "date_add(now(), interval 1 year)";

$target_user->SetPassword($encoding, $expires);

if (HASREALACCOUNT($target_uid)) {
    STARTBUSY("Resetting your password");

    SUEXEC($target_uid, "nobody", "webtbacct passwd $target_uid",
	   SUEXEC_ACTION_DIE);
    
    CLEARBUSY();
}

TBMAIL("$usr_name <$usr_email>",
       "Password Reset for '$target_uid'",
       "\n".
       "The password for '$target_uid' has been reset via the web interface.\n".
       "If this message is unexpected, please contact Testbed Operations\n".
       "($TBMAILADDR_OPS) immediately!\n".
       "\n".
       "The change originated from IP: " . $_SERVER['REMOTE_ADDR'] . "\n".
       "\n".
       "Thanks,\n".
       "Testbed Operations\n",
       "From: $TBMAIL_OPS\n".
       "Bcc: $TBMAIL_AUDIT\n".
       "Errors-To: $TBMAIL_WWW");

echo "<br>
      Your password has been changed.\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
