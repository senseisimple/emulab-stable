<?php
#
# Beware empty spaces (cookies)!
# 
require("defs.php3");

#
# This page gets loaded as the result of a login click.
#
# $uid will be set by the login form. If the login is okay, we zap
# the user back to the main page. If the login fails, put continue
# with a normal page, but with an error message.
#
if (isset($login)) {
    #
    # Login button pressed. 
    #
    if (!isset($uid) ||
        strcmp($uid, "") == 0) {
            $login_status = $STATUS_LOGINFAIL;
    }
    else {
	#
	# Look to see if already logged in. If the user hits reload,
	# we are going to get another login post, and this could
	# update the current login. Try to avoid that if possible.
        #
	if (CHECKLOGIN($uid) == 1) {
            $login_status = $STATUS_LOGGEDIN;
	}
	elseif (DOLOGIN($uid, $password)) {
            $login_status = $STATUS_LOGINFAIL;
        }
        else {
            $login_status = $STATUS_LOGGEDIN;
        }
    }
}
else {
    $login_status = $STATUS_LOGGEDIN;
}

if ($login_status == $STATUS_LOGGEDIN) {
    header("Location: " . "index.php3");
    return;
}

#
# Standard Testbed Header
#
PAGEHEADER("Login Failed");

echo "<center><h3>Login attempt failed! Please try again.</h3></center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
