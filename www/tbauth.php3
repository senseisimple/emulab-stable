<?php
#
# Login support: Beware empty spaces (cookies)!
# 

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
# Verify a login by sucking a UID's current hash value out of the database.
# If the login has expired, or of the hashkey in the database does not
# match what came back in the cookie, then the UID is no longer logged in.
#
# Should we advance the timeout since the user is still being active?
#
# Returns: 0 if not logged in ever.
#          1 if logged in okay
#         -1 if login timed out
#
function CHECKLOGIN($uid, $curhash) {
    global $TBDBNAME;

    $query_result = mysql_db_query($TBDBNAME,
	"SELECT hashkey, timeout FROM login WHERE uid=\"$uid\"");
    if (! $query_result) {
        $err = mysql_error();
        TBERROR("Database Error retrieving login info for $uid: $err\n", 1);
    }

    # Not logged in.
    if (($row = mysql_fetch_array($query_result)) == 0) {
	return 0;
    }

    $hashkey = $row[hashkey];
    $timeout = $row[timeout];

    # A match?
    if ($timeout > time() &&
        strcmp($curhash, $hashkey) == 0) {
	return 1;
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

    return -1;
}

#
# This one checks for login, but then dies with an appropriate error
# message.
#
function LOGGEDINORDIE($uid) {
    global $TBDBNAME, $TBAUTHCOOKIE, $TBAUTHDOMAIN, $TBAUTHTIMEOUT;
    global $HTTP_COOKIE_VARS;

    $curhash = $HTTP_COOKIE_VARS[$TBAUTHCOOKIE];

    $status = CHECKLOGIN($uid, $curhash);
    switch ($status) {
    case 0:
        USERERROR("You do not appear to be logged in!", 1);
        break;
    case 1:
        return $uid;
        break;
    case -1:
        USERERROR("Your login has timed out! Please log in again.", 1);
        break;
    }
    TBERROR("LOGGEDINORDIE failed mysteriously", 1);
}

#
# Attempt a login.
# 
function DOLOGIN($uid, $password) {
    global $TBDBNAME, $TBAUTHCOOKIE, $TBAUTHDOMAIN, $TBAUTHTIMEOUT;

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
	$timeout = time() + 10800;
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
	# Issue the cookie request so that subsequent pages come back
	# with the hash value embedded.
	#
	setcookie($TBAUTHCOOKIE, $hashkey, $timeout, "/", $TBAUTHDOMAIN, 0);

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
    global $TBDBNAME;

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
    $timeout = time() - 3600;

    $query_result = mysql_db_query($TBDBNAME,
	"DELETE FROM login WHERE uid=\"$uid\"");

    #
    # Issue a cookie request to delete the cookie. 
    #
    setcookie($TBAUTHCOOKIE, $hashkey, $timeout, "/", $TBAUTHDOMAIN, 0);

    return 0;
}

#
# Beware empty spaces (cookies)!
# 
?>
