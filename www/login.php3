<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

#
# These two for verification.
# 
if (!isset($key) || !strcmp($key, "")) {
    $key = 0;
}
if (!isset($vuid) || !strcmp($vuid, "")) {
    $vuid = 0;
}

#
# Must not be logged in already!
# 
if (($known_uid = GETUID()) != FALSE) {
    if (CHECKLOGIN($known_uid) & CHECKLOGIN_LOGGEDIN) {
	#
	# If doing a verification, zap to that page.
	#
	if ($key && (!$vuid || !strcmp($vuid, $known_uid))) {
	    header("Location: $TBBASE/verifyusr.php3?key=$key");
	    return;
	}

	PAGEHEADER("Login");

	echo "<h3>
              You are still logged in. Please log out first if you want
              to log in as another user!
              </h3>\n";
	    
	PAGEFOOTER();
	die("");
    }
}

#
# Spit out the form.
# 
function SPITFORM($uid, $key, $failed)
{
    global $TBDB_UIDLEN, $TBBASE;
    
    PAGEHEADER("Login");

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

    $keyarg = "";
    if ($key)
	$keyarg = "?key=$key";

    echo "<table align=center border=1>
          <form action='${TBBASE}/login.php3${keyarg}' method=post>
          <tr>
              <td>Username:</td>
              <td><input type=text
                         value=\"$uid\"
                         name=uid size=$TBDB_UIDLEN></td>
          </tr>
          <tr>
              <td>Password:</td>
              <td><input type=password name=password size=12></td>
          </tr>
          <tr>
             <td align=center colspan=2>
                 <b><input type=submit value=Login name=login></b></td>
          </tr>
          </form>
          </table>\n";

    echo "<center><h2>
          <a href='password.php3'>Forgot your password?</a>
          </h2></center>\n";
}

#
# Do not bother if NOLOGINS!
#
if (0 && NOLOGINS()) {
    PAGEHEADER("Login");
	    
    echo "<center>
          <font size=+1 color=red>
	   Logins are temporarily disabled. Please try again later.
          </font>
          </center><br>\n";

    PAGEFOOTER();
    die("");
}

#
# If not clicked, then put up a form.
#
if (! isset($login)) {
    if ($vuid)
	$known_uid = $vuid;
    SPITFORM($known_uid, $key, 0);
    PAGEFOOTER();
    return;
}

#
# Login clicked.
#
$STATUS_LOGGEDIN  = 1;
$STATUS_LOGINFAIL = 2;
$login_status     = 0;

if (!isset($uid) ||
    strcmp($uid, "") == 0) {
    $login_status = $STATUS_LOGINFAIL;
}
else {
    if (DOLOGIN($uid, $password)) {
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
    SPITFORM($uid, $key, 1);
    PAGEFOOTER();
    return;
}

if ($key) {
    #
    # If doing a verification, zap to that page.
    #
    header("Location: $TBBASE/verifyusr.php3?key=$key");
}
else {
    #
    # Zap back to front page in secure mode.
    # 
    header("Location: $TBBASE/");
}
return;

?>
