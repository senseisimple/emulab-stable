<?php
#
# Login support: Beware empty spaces (cookies)!
#

$CHECKLOGIN_NOTLOGGEDIN = 0;
$CHECKLOGIN_LOGGEDIN    = 1;
$CHECKLOGIN_TIMEDOUT    = -1;
$CHECKLOGIN_MAYBEVALID  = 2;

#
# Generate a hash value suitable for authorization. We use the results of
# microtime, combined with a random number.
# 
function GENHASH() {
    $fp = fopen("/dev/urandom", "r");
    if (! $fp) {
        TBERROR("Error opening /dev/urandom", 1);
    }
    $random_bytes = fread($fp, 8);
    fclose($fp);

    $hash  = mhash (MHASH_MD5, bin2hex($retval) . " " . microtime());
    return bin2hex($hash);
}

#
# Return the value of the currently logged in uid, or null if not
# logged in. Basically, check the browser to see if its sending a UID
# and HASH back, and then check the DB to see if the user is really
# logged in.
# 
function GETLOGIN() {
    global $CHECKLOGIN_LOGGEDIN;

    if (($uid = GETUID()) == FALSE)
	    return FALSE;

    if (CHECKLOGIN($uid) == $CHECKLOGIN_LOGGEDIN)
	    return $uid;

    return FALSE;
}

#
# Return the value of the UID cookie. This does not check to see if
# this person is currently logged in. We just want to know what the
# browser thinks, if anything.
# 
function GETUID() {
    global $TBNAMECOOKIE, $HTTP_COOKIE_VARS;

    $curname = $HTTP_COOKIE_VARS[$TBNAMECOOKIE];
    if ($curname == NULL)
	    return FALSE;

    return $curname;
}

#
# Verify a login by sucking a UIDs current hash value out of the database.
# If the login has expired, or of the hashkey in the database does not
# match what came back in the cookie, then the UID is no longer logged in.
#
# Returns: if not logged in ever.
#          if logged in okay
#          if login timed out
#          if login record exists, is not timed out, but no hash cookie.
#             this case will be caught later when the user tries to do
#             something for which a valid login is required.
#
function CHECKLOGIN($uid) {
    global $TBDBNAME, $TBAUTHCOOKIE, $HTTP_COOKIE_VARS, $TBAUTHTIMEOUT;
    global $CHECKLOGIN_NOTLOGGEDIN, $CHECKLOGIN_LOGGEDIN;
    global $CHECKLOGIN_TIMEDOUT, $CHECKLOGIN_MAYBEVALID;

    $curhash = $HTTP_COOKIE_VARS[$TBAUTHCOOKIE];

    $query_result = mysql_db_query($TBDBNAME,
	"SELECT hashkey, timeout FROM login WHERE uid=\"$uid\"");
    if (! $query_result) {
        $err = mysql_error();
        TBERROR("Database Error retrieving login info for $uid: $err\n", 1);
    }

    # Not logged in.
    if (($row = mysql_fetch_array($query_result)) == 0) {
	return $CHECKLOGIN_NOTLOGGEDIN;
    }

    $hashkey = $row[hashkey];
    $timeout = $row[timeout];

    # A match?
    if ($timeout > time()) {
        if (strcmp($curhash, $hashkey) == 0) {
	    #
   	    # We update the time in the database. Basically, each time the
	    # user does something, we bump the logout further into the future.
	    # This avoids timing them out just when they are doing useful work.
	    #
	    $timeout = time() + $TBAUTHTIMEOUT;

	    $query_result = mysql_db_query($TBDBNAME,
			"UPDATE login set timeout='$timeout' ".
			"WHERE uid=\"$uid\"");
	    if (! $query_result) {
		$err = mysql_error();
		TBERROR("Database Error updating login timeout for ".
			"$uid: $err", 1);
	    }
	    return $CHECKLOGIN_LOGGEDIN;
	}
	elseif (!isset($curhash) || !$curhash || $curhash == NULL) {
	    #
	    # A login is valid, but we have no proof yet. Proof will be
	    # demanded later by whatever page wants it.
	    # 
	    return $CHECKLOGIN_MAYBEVALID;
	}
    }

    #
    # Clear out the database entry for completeness.
    #
    $query_result = mysql_db_query($TBDBNAME,
	"DELETE FROM login WHERE uid=\"$uid\"");
    if (! $query_result) {
        $err = mysql_error();
        TBERROR("Database Error deleting login info for $uid: $err\n", 1);
    }

    return $CHECKLOGIN_TIMEDOUT;
}

#
# This one checks for login, but then dies with an appropriate error
# message.
#
function LOGGEDINORDIE($uid) {
    global $CHECKLOGIN_NOTLOGGEDIN, $CHECKLOGIN_LOGGEDIN;
    global $CHECKLOGIN_TIMEDOUT, $CHECKLOGIN_MAYBEVALID;

    $status = CHECKLOGIN($uid);
    switch ($status) {
    case $CHECKLOGIN_NOTLOGGEDIN:
        USERERROR("You do not appear to be logged in!", 1);
        break;
    case $CHECKLOGIN_LOGGEDIN:
        return $uid;
        break;
    case $CHECKLOGIN_TIMEDOUT:
        USERERROR("Your login has timed out! Please log in again.", 1);
        break;
    case $CHECKLOGIN_MAYBEVALID:
        USERERROR("Your login cannot be verified. Are cookies turned on?", 1);
        break;
    }
    TBERROR("LOGGEDINORDIE failed mysteriously", 1);
}

#
# Attempt a login.
# 
function DOLOGIN($uid, $password) {
    global $TBDBNAME, $TBAUTHCOOKIE, $TBAUTHDOMAIN, $TBAUTHTIMEOUT;
    global $TBNAMECOOKIE, $TBSECURECOOKIES;

    $query_result = mysql_db_query($TBDBNAME,
	"SELECT usr_pswd FROM users WHERE uid=\"$uid\"");
    if (! $query_result) {
        $err = mysql_error();
        TBERROR("Database Error retrieving password for $uid: $err\n", 1);
    }

    #
    # Check password in the database against provided. 
    #
    if ($row = mysql_fetch_row($query_result)) {
        $db_encoding = $row[0];
        $salt = substr($db_encoding, 0, 2);
        if ($salt[0] == $salt[1]) { $salt = $salt[0]; }
        $encoding = crypt("$password", $salt);
        if (strcmp($encoding, $db_encoding)) {
            return -1;
        }
        #
        # Pass! Insert a record in the login table for this uid with
        # the new hash value. If the user is already logged in, thats
        # okay; just update it in place with a new hash and timeout. 
        #
	$timeout = time() + $TBAUTHTIMEOUT;
	$hashkey = GENHASH();
        $query_result = mysql_db_query($TBDBNAME,
		"SELECT timeout FROM login WHERE uid=\"$uid\"");
	if (mysql_num_rows($query_result)) {
		$query_result = mysql_db_query($TBDBNAME,
			"UPDATE login set ".
			"timeout='$timeout', hashkey='$hashkey' ".
			"WHERE uid=\"$uid\"");
	}
	else {
		$query_result = mysql_db_query($TBDBNAME,
			"INSERT into login (uid, hashkey, timeout) ".
                        "VALUES ('$uid', '$hashkey', '$timeout')");
	}
        if (! $query_result) {
            $err = mysql_error();
            TBERROR("Database Error logging in $uid: $err\n", 1);
        }

	#
	# Create a last login record.
	#
	$query_result = mysql_db_query($TBDBNAME,
	       "REPLACE into lastlogin (uid, time) VALUES ('$uid', NOW())");

	#
	# Issue the cookie requests so that subsequent pages come back
	# with the hash value and auth usr embedded.

	#
	# For the hashkey, we give it a longish timeout since we are going
	# to control the actual timeout via the database. This just avoids
	# having to update the hash as we update the timeout in the database
	# each time the user does something. Eventually the cookie will
	# expire and the user will be forced to log in again anyway. 
	#
	$timeout = time() + (60 * 60 * 24);
	setcookie($TBAUTHCOOKIE, $hashkey, $timeout, "/",
                  $TBAUTHDOMAIN, $TBSECURECOOKIES);

	#
	# We give this a really long timeout. We want to remember who the
	# the user was each time they load a page, and more importantly,
	# each time they come back to the main page so we can fill in their
	# user name. NOTE: This cookie is integral to authorization, since
	# we do not pass around the UID anymore, but look for it in the
	# cookie.
	# 
	$timeout = time() + (60 * 60 * 24 * 32);
	setcookie($TBNAMECOOKIE, $uid, $timeout, "/", $TBAUTHDOMAIN, 0);

	return 0;
    }
    #
    # No such user
    #
    return -1;
}

#
# Log out a UID. Simply clear the entry from the login table.
#
# Should we kill the cookie? 
# 
function DOLOGOUT($uid) {
    global $TBDBNAME, $TBSECURECOOKIES;

    $query_result = mysql_db_query($TBDBNAME,
	"SELECT hashkey timeout FROM login WHERE uid=\"$uid\"");
    if (! $query_result) {
        $err = mysql_error();
        TBERROR("Database Error retrieving login info for $uid: $err\n", 1);
    }

    # Not logged in.
    if (($row = mysql_fetch_array($query_result)) == 0) {
	return 0;
    }

    $hashkey = $row[hashkey];
    $timeout = time() - 1000000;

    $query_result = mysql_db_query($TBDBNAME,
	"DELETE FROM login WHERE uid=\"$uid\"");

    #
    # Issue a cookie request to delete the cookie. 
    #
    setcookie($TBAUTHCOOKIE, "", $timeout, "/", $TBAUTHDOMAIN, 0);

    return 0;
}

#
# Primitive "nologins" support.
#
function NOLOGINS() {
    global $TBDBNAME;

    $query_result = mysql_db_query($TBDBNAME,
	"SELECT nologins FROM nologins where nologins=1");
    if (! $query_result) {
        $err = mysql_error();
        TBERROR("Database Error nologins info: $err\n", 1);
    }

    # No entry
    if (($row = mysql_fetch_array($query_result)) == 0) {
	return 0;
    }

    $nologins = $row[nologins];
    return $nologins;
}

function LASTWEBLOGIN($uid) {
    global $TBDBNAME;

    $query_result = mysql_db_query($TBDBNAME,
	"SELECT time from lastlogin where uid=\"$uid\"");
    
    if (mysql_num_rows($query_result)) {
	$lastrow      = mysql_fetch_array($query_result);
	return $lastrow[time];
    }
    return 0;
}

#
# Beware empty spaces (cookies)!
# 
?>
