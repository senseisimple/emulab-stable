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
$optargs = OptionalPageArguments("simple", PAGEARG_BOOLEAN,
				 "reset",  PAGEARG_STRING,
				 "email",  PAGEARG_STRING,
				 "phone",  PAGEARG_STRING);

# Display a simpler version of this page.
if (!isset($simple)) {
    $simple = 0;
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

# Must use https!
if (!isset($_SERVER["SSL_PROTOCOL"])) {
    PAGEHEADER("Forgot Your Password?", $view);
    USERERROR("Must use https:// to access this page!", 1);
}

#
# Must not be logged in.
# 
if (CheckLogin($check_status)) {
    PAGEHEADER("Forgot Your Password?", $view);

    echo "<h3>
              You are logged in. You must already know your password!
          </h3>\n";
    
    PAGEFOOTER($view);
    die("");
}

#
# Spit out the form.
# 
function SPITFORM($email, $phone, $failed, $simple, $view)
{
    global	$TBBASE;
    global	$WIKIDOCURL;
    
    PAGEHEADER("Forgot Your Password?", $view);

    if ($failed) {
	echo "<center>
              <font size=+1 color=red>
              The email/phone you provided does not match.
	      Please try again.
              </font>
              </center><br>\n";
    }
    else {
	echo "<center>
              <font size=+1>
              Please provide your email address and phone number.<br><br>
              </font>
              </center>\n";
    }

    echo "<table align=center border=1>
          <form action=${TBBASE}/password.php3 method=post>
          <tr>
              <td>Email Address:</td>
              <td><input type=text
                         value=\"$email\"
                         name=email size=30></td>
          </tr>
          <tr>
              <td>Phone Number:</td>
              <td><input type=text
                         value=\"$phone\"
                         name=phone size=20></td>
          </tr>
          <tr>
             <td align=center colspan=2>
                 <b><input type=submit value=\"Reset Password\"
                           name=reset></b>
             </td>
          </tr>\n";
    
    if ($simple) {
	echo "<input type=hidden name=simple value=$simple>\n";
    }

    echo "</form>
          </table>\n";

    echo "<br><blockquote>
          Please provide your phone number in standard dashed notation;
          no extensions or room numbers, etc. We will do our best to match it up
          against our user records.
          <br><br>
          If the email address and phone number you give us matches
          our user records, we will email a URL that will allow you to change
          your password.

          <br><br>
          <b>Please read this <a href='$WIKIDOCURL/kb69'>
          Knowledge Base Entry</a> if you get an error
          when trying to use the link we email to you!</b>
          </blockquote>\n";
}

#
# If not clicked, then put up a form.
#
if (!isset($reset)) {
    if (!isset($email))
	$email = "";
    if (!isset($phone))
	$phone = "";
    
    SPITFORM($email, $phone, 0, $simple, $view);
    return;
}

#
# Reset clicked. See if we find a user with the given email/phone. If not
# zap back to the form. 
#
if (!isset($phone) || $phone == "" || !TBvalid_phone($phone) ||
    !isset($email) || $email == "" || !TBvalid_email($email)) {
    SPITFORM($email, $phone, 1, $simple, $view);
    return;
}

if (! ($user = User::LookupByEmail($email))) {
    SPITFORM($email, $phone, 2, $simple, $view);
    return;
}
$uid       = $user->uid();
$usr_phone = $user->phone();
$uid_name  = $user->name();
$uid_email = $user->email();

#
# Compare phone by striping out anything but the numbers.
#
if (preg_replace("/[^0-9]/", "", $phone) !=
    preg_replace("/[^0-9]/", "", $usr_phone)) {
    SPITFORM($email, $phone, 3, $simple, $view);
    return;
}

#
# Yep. Generate a random key and send the user an email message with a URL
# that will allow them to change their password. 
#
$key  = md5(uniqid(rand(),1));
$keyA = substr($key, 0, 16);
$keyB = substr($key, 16);

# Send half of the key to the browser and half in the email message.
setcookie($TBAUTHCOOKIE, $keyA, 0, "/",
	  $TBAUTHDOMAIN, $TBSECURECOOKIES);

# It is okay to spit this now that we have sent the cookie.
PAGEHEADER("Forgot Your Password?", $view);

$user->SetChangePassword($key, "UNIX_TIMESTAMP(now())+(60*30)");

TBMAIL("$uid_name <$uid_email>",
       "Password Reset requested by '$uid'",
       "\n".
       "Here is your password reset authorization URL. Click on this link\n".
       "within the next 30 minutes, and you will be allowed to reset your\n".
       "password. If the link expires, you can request a new one from the\n".
       "web interface.\n".
       "\n".
       "    ${TBBASE}/chpasswd.php3?user=$uid&key=$keyB&simple=$simple\n".
       "\n".
       "The request originated from IP: " . $_SERVER['REMOTE_ADDR'] . "\n".
       "\n".
       "Thanks,\n".
       "Testbed Operations\n",
       "From: $TBMAIL_OPS\n".
       "Bcc: $TBMAIL_AUDIT\n".
       "Errors-To: $TBMAIL_WWW");

echo "<br>
      An email message has been sent to your account. In it you will find a
      URL that will allow you to change your password. The link will <b>expire 
      in 30 minutes</b>. If the link does expire before you have a chance to
      use it, simply come back and request a <a href='password.php3'>new one</a>.
      \n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
