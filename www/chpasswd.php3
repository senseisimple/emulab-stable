<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

# Display a simpler version of this page
$simple = 0;
if (isset($_REQUEST['simple'])) {
    $simple = $_REQUEST['simple'];
}

# Form arguments.
$reset_uid = $_REQUEST['reset_uid'];
$keyB      = $_REQUEST['key'];
# We also need the other half of the key from the browser.
$keyA      = $HTTP_COOKIE_VARS[$TBAUTHCOOKIE];

if (!isset($reset_uid) || $reset_uid == "" || !TBvalid_uid($reset_uid) ||
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
if (($known_uid = GETUID()) != FALSE) {
    if (CHECKLOGIN($known_uid) & CHECKLOGIN_LOGGEDIN) {
	PAGEHEADER("Reset Your Password", $view);

	echo "<h3>
              You are logged in. You must already know your password!
              </h3>\n";

	PAGEFOOTER($view);
	die("");
    }
}

#
# Spit out the form.
# 
function SPITFORM($uid, $key, $failed, $simple, $view)
{
    global	$TBBASE;
    
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

    $args = "reset_uid=$uid&key=$key&simple=$simple";
	
    echo "<table align=center border=1>
          <form action=${TBBASE}/chpasswd.php3?${args} method=post>\n";

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
$query_result =
    DBQueryFatal("select chpasswd_key,chpasswd_expires,usr_email,usr_name ".
		 "   from users ".
		 "where uid='$reset_uid'");
# Silent error about invalid users.
if (!mysql_num_rows($query_result)) {
    PAGEARGERROR();    
}
$row       = mysql_fetch_row($query_result);
$usr_email = $row[2];
$usr_name  = $row[3];

# Silent error when there is no key/timeout set for the user.
if (!isset($row[0]) || !$row[1]) {
    PAGEARGERROR();    
}
if ($row[0] != $key) {
    USERERROR("You do not have permission to change your password!", 1);
}
if (time() > $row[1]) {
    USERERROR("Your key has expired. Please request a
               <a href='password.php3'>new key</a>.", 1);
}

#
# If not clicked, then put up a form.
#
if (! isset($reset)) {
    SPITFORM($reset_uid, $keyB, 0, $simple, $view);
    return;
}

#
# Reset clicked. Verify a proper password. 
#
$password1 = $_POST['password1'];
$password2 = $_POST['password2'];

if (!isset($password1) || $password1 == "" ||
    !isset($password2) || $password2 == "") {
    SPITFORM($reset_uid, $keyB, "You must supply a password", $simple, $view);
    return;
}
if ($password1 != $password2) {
    SPITFORM($reset_uid, $keyB, "Two passwords do not match", $simple, $view);
    return;
}
if (! CHECKPASSWORD($reset_uid, $password1, $usr_name, $usr_email, $checkerror)){
    SPITFORM($reset_uid, $keyB, $checkerror, $simple, $view);
    return;
}

# Clear the cookie from the browser.
setcookie($TBAUTHCOOKIE, "", time() - 1000000, "/", $TBAUTHDOMAIN, 0);

# Okay to spit this now that the cookie has been sent (cleared).
PAGEHEADER("Reset Your Password", $view);

$encoding = crypt("$password1");
$expires  = "date_add(now(), interval 1 year)";

DBQueryFatal("update users set ".
	     "       chpasswd_key=NULL,chpasswd_expires=0, ".
	     "       usr_pswd='$encoding',pswd_expires=$expires ".
	     "where uid='$reset_uid'");

if (HASREALACCOUNT($reset_uid)) {
    SUEXEC($reset_uid, "nobody", "webtbacct passwd $reset_uid", 1);
}

TBMAIL("$usr_name <$usr_email>",
       "Password Reset for '$reset_uid'",
       "\n".
       "The password for '$reset_uid' has been reset via the web interface.\n".
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
