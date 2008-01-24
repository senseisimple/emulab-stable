<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2008 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("login",    PAGEARG_STRING,
				 "uid",      PAGEARG_STRING,
				 "password", PAGEARG_PASSWORD,
				 "key",      PAGEARG_STRING,
				 "vuid",     PAGEARG_STRING,
				 "simple",   PAGEARG_BOOLEAN,
				 "adminmode",PAGEARG_BOOLEAN,
				 "refer",    PAGEARG_BOOLEAN,
				 "referrer", PAGEARG_STRING);
				 
# Allow adminmode to be passed along to new login. Handy for letting admins
# log in when NOLOGINS() is on.
if (!isset($adminmode)) {
    $adminmode = 0;
}
# Display a simpler version of this page
if (! isset($simple)) {
    $simple = 0;
}
if (! isset($key)) {
    $key = null;
}
if (! isset($referrer)) {
    $referrer = null;
}

# See if referrer page requested that it be passed along so that it can be
# redisplayed after login. Save the referrer for form below.
if (isset($refer) &&
    isset($_SERVER['HTTP_REFERER']) && $_SERVER['HTTP_REFERER'] != "") {
    $referrer = $_SERVER['HTTP_REFERER'];

    # In order to get the auth cookies, pages need to go through https. But,
    # the user may have visited the last page with http. If they did, send them
    # back through https
    $referrer = preg_replace("/^http:/i","https:",$referrer);
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
# Must not be logged in already.
#
if (($this_user = CheckLogin($status))) {
    $this_webid = $this_user->webid();
    
    if ($status & CHECKLOGIN_LOGGEDIN) {
	#
	# If doing a verification for the logged in user, zap to that page.
	# If doing a verification for another user, then must login in again.
	#
	if (isset($key) && (!isset($vuid) || $vuid == $this_webid)) {
	    header("Location: $TBBASE/verifyusr.php3?key=$key");
	    return;
	}

	PAGEHEADER("Login",$view);

	echo "<h3>
              You are still logged in. Please log out first if you want
              to log in as another user!
              </h3>\n";

	PAGEFOOTER($view);
	die("");
    }
}

#
# Spit out the form.
#
# The uid can be an email address, and in fact defaults to that now. 
# 
function SPITFORM($uid, $key, $referrer, $failed, $adminmode, $simple, $view)
{
    global $TBDB_UIDLEN, $TBBASE;
    
    PAGEHEADER("Login",$view);

    if ($failed) {
	echo "<center>
              <font size=+1 color=red>
	      Login attempt failed! Please try again.
              </font>
              </center><br>\n";
    }

    echo "<center>
          <font size=+1>
          Please login to our secure server.<br>
          (You must have cookies enabled)
          </font>
          </center>\n";

    $pagearg = "";
    if ($adminmode == 1)
	$pagearg  = "?adminmode=1";
    if ($key)
	$pagearg .= (($adminmode == 1) ? "&" : "?") . "key=$key";

    echo "<table align=center border=1>
          <form action='${TBBASE}/login.php3${pagearg}' method=post>
          <tr>
              <td>Email Address:<br>
                   <font size=-2>(or UserName)</font></td>
              <td><input type=text
                         value=\"$uid\"
                         name=uid size=30></td>
          </tr>
          <tr>
              <td>Password:</td>
              <td><input type=password name=password size=12></td>
          </tr>
          <tr>
             <td align=center colspan=2>
                 <b><input type=submit value=Login name=login></b></td>
          </tr>\n";
    
    if ($referrer) {
	echo "<input type=hidden name=referrer value=$referrer>\n";
    }

    if ($simple) {
	echo "<input type=hidden name=simple value=$simple>\n";
    }

    echo "</form>
          </table>\n";

    echo "<center><h2>
          <a href='password.php3'>Forgot your password?</a>
          </h2></center>\n";
}

#
# If not clicked, then put up a form.
#
if (! isset($login)) {
    # Allow page arg to override what we think is the UID to log in as.
    # Use email address now, for the login uid. Still allow real uid though.
    if (isset($vuid)) {
	# For login during verification step, from email message.
	$login_id = $vuid;
    }
    else {
	$login_id = REMEMBERED_ID();
    }
    
    SPITFORM($login_id, $key, $referrer, 0, $adminmode, $simple, $view);
    PAGEFOOTER($view);
    return;
}

#
# Login clicked.
#
$STATUS_LOGGEDIN  = 1;
$STATUS_LOGINFAIL = 2;
$login_status     = 0;
$adminmode        = (isset($adminmode) && $adminmode == 1);

if (!isset($uid) || $uid == "" || !isset($password) || $password == "") {
    $login_status = $STATUS_LOGINFAIL;
}
else {
    if (DOLOGIN($uid, $password, $adminmode)) {
	# Short delay.
	sleep(1);
	$login_status = $STATUS_LOGINFAIL;
    }
    else {
	$login_status = $STATUS_LOGGEDIN;
    }
}

#
# Failed, then try again with an error message.
# 
if ($login_status == $STATUS_LOGINFAIL) {
    SPITFORM($uid, $key, $referrer, 1, $adminmode, $simple, $view);
    PAGEFOOTER($view);
    return;
}

if (isset($key)) {
    #
    # If doing a verification, zap to that page.
    #
    header("Location: $TBBASE/verifyusr.php3?key=$key");
}
elseif (isset($referrer)) {
    #
    # Zap back to page that started the login request.
    #
    header("Location: $referrer");
}
else {
    #
    # Zap back to front page in secure mode.
    # 
    header("Location: $TBBASE/");
}
return;

?>
