<?php
#
# Login support: Beware empty spaces (cookies)!
#

# These global are to prevent repeated calls to the DB. 
#
$CHECKLOGIN_STATUS		= -1;
$CHECKLOGIN_UID			= 0;
$CHECKLOGIN_NOLOGINS		= -1;

#
# New Mapping. 
#
define("CHECKLOGIN_NOSTATUS",		-1);
define("CHECKLOGIN_NOTLOGGEDIN",	0);
define("CHECKLOGIN_LOGGEDIN",		1);
define("CHECKLOGIN_TIMEDOUT",		2);
define("CHECKLOGIN_MAYBEVALID",		4);
define("CHECKLOGIN_STATUSMASK",		0x000ff);
define("CHECKLOGIN_MODMASK",		0xfff00);
#
# These are modifiers of the above status fields. They are stored
# as a bit field in the top part. This is intended to localize as
# many queries related to login as possible. 
#
define("CHECKLOGIN_NEWUSER",		0x00100);
define("CHECKLOGIN_UNVERIFIED",		0x00200);
define("CHECKLOGIN_UNAPPROVED",		0x00400);
define("CHECKLOGIN_ACTIVE",		0x00800);
define("CHECKLOGIN_USERSTATUS",		0x00f00);
define("CHECKLOGIN_PSWDEXPIRED",	0x01000);
define("CHECKLOGIN_FROZEN",		0x02000);
define("CHECKLOGIN_ISADMIN",		0x04000);
define("CHECKLOGIN_TRUSTED",		0x08000);
define("CHECKLOGIN_CVSWEB",		0x10000);
define("CHECKLOGIN_ADMINOFF",		0x20000);

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
    if (($uid = GETUID()) == FALSE)
	    return FALSE;

    $check = CHECKLOGIN($uid);

    if ($check & (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_MAYBEVALID))
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
# Returns a combination of the CHECKLOGIN values above.
#
function CHECKLOGIN($uid) {
    global $TBAUTHCOOKIE, $HTTP_COOKIE_VARS, $TBAUTHTIMEOUT;
    global $CHECKLOGIN_STATUS, $CHECKLOGIN_UID;

    #
    # If we already figured this out, do not duplicate work!
    #
    if ($CHECKLOGIN_STATUS != CHECKLOGIN_NOSTATUS) {
	return $CHECKLOGIN_STATUS;
    }
    $CHECKLOGIN_UID = $uid;

    $curhash = $HTTP_COOKIE_VARS[$TBAUTHCOOKIE];

    #
    # Note that we get multiple rows back because of the group_membership
    # join. No big deal.
    # 
    $query_result =
	DBQueryFatal("select NOW()>=u.pswd_expires,l.hashkey,l.timeout, ".
		     "       status,admin,cvsweb,g.trust,adminoff ".
		     " from users as u ".
		     "left join login as l on l.uid=u.uid ".
		     "left join group_membership as g on g.uid=u.uid ".
		     "where u.uid='$uid'");

    # No such user.
    if (! mysql_num_rows($query_result)) { 
	$CHECKLOGIN_STATUS = CHECKLOGIN_NOTLOGGEDIN;
	return $CHECKLOGIN_STATUS;
    }
    
    #
    # Scan the rows. All the info is duplicate, except for the trust
    # values. 
    #
    $trusted = 0;
    while ($row = mysql_fetch_array($query_result)) {
	$expired = $row[0];
	$hashkey = $row[1];
	$timeout = $row[2];
	$status  = $row[3];
	$admin   = $row[4];
	$cvsweb  = $row[5];

	if (! strcmp($row[6], "project_root") ||
	    ! strcmp($row[6], "group_root")) {
	    $trusted = 1;
	}
	$adminoff= $row[7];
    }

    #
    # If user exists, but login has no entry, quit now.
    #
    if (!$hashkey) {
	$CHECKLOGIN_STATUS = CHECKLOGIN_NOTLOGGEDIN;
	return $CHECKLOGIN_STATUS;
    }

    #
    # Check for frozen account. Might do something interesting later.
    #
    if (! strcmp($status, TBDB_USERSTATUS_FROZEN)) {
	DBQueryFatal("DELETE FROM login WHERE uid='$uid'");
	$CHECKLOGIN_STATUS = CHECKLOGIN_NOTLOGGEDIN;
	return $CHECKLOGIN_STATUS;
    }

    #
    # Check for expired login or for a hash that mismatches. Treat the same.
    #
    if ((time() > $timeout) ||
	(isset($curhash) && $curhash && strcmp($curhash, $hashkey))) {
	
        #
        # Clear out the database entry for completeness.
        #
	DBQueryFatal("DELETE FROM login WHERE uid='$uid'");
	$CHECKLOGIN_STATUS = CHECKLOGIN_TIMEDOUT;
	return $CHECKLOGIN_STATUS;
    }

    #
    # We know the login has not expired. We also know from the above
    # test that the hashkey was either null or matched. 
    # 
    if (strcmp($curhash, $hashkey) == 0) {
        #
   	# We update the time in the database. Basically, each time the
	# user does something, we bump the logout further into the future.
	# This avoids timing them out just when they are doing useful work.
	#
	$timeout = time() + $TBAUTHTIMEOUT;

	$query_result =
	    DBQueryFatal("UPDATE login set timeout='$timeout' ".
			 "WHERE uid='$uid'");

	$CHECKLOGIN_STATUS = CHECKLOGIN_LOGGEDIN;
    }
    else {
        #
	# A login is valid, but we have no proof yet. Cookies off?
	# 
	$CHECKLOGIN_STATUS = CHECKLOGIN_MAYBEVALID;
    }

    #
    # Now add in the modifiers.
    #
    if ($expired)
	$CHECKLOGIN_STATUS |= CHECKLOGIN_PSWDEXPIRED;
    if ($admin)
	$CHECKLOGIN_STATUS |= CHECKLOGIN_ISADMIN;
    if ($adminoff)
	$CHECKLOGIN_STATUS |= CHECKLOGIN_ADMINOFF;
    if ($trusted)
	$CHECKLOGIN_STATUS |= CHECKLOGIN_TRUSTED;
    if ($cvsweb)
	$CHECKLOGIN_STATUS |= CHECKLOGIN_CVSWEB;
    if (strcmp($status, TBDB_USERSTATUS_NEWUSER) == 0)
	$CHECKLOGIN_STATUS |= CHECKLOGIN_NEWUSER;
    if (strcmp($status, TBDB_USERSTATUS_UNAPPROVED) == 0)
	$CHECKLOGIN_STATUS |= CHECKLOGIN_UNAPPROVED;
    if (strcmp($status, TBDB_USERSTATUS_UNVERIFIED) == 0)
	$CHECKLOGIN_STATUS |= CHECKLOGIN_UNVERIFIED;
    if (strcmp($status, TBDB_USERSTATUS_ACTIVE) == 0)
	$CHECKLOGIN_STATUS |= CHECKLOGIN_ACTIVE;

    #
    # Set the magic enviroment variable, if appropriate, for the sake of
    # any processes we might spawn. We prepend an HTTP_ on the front of
	# the variable name, so that it'll get through suexec.
    #
    if ($admin && !$adminoff) {
    	putenv("HTTP_WITH_TB_ADMIN_PRIVS=1");
    }

    return $CHECKLOGIN_STATUS;
}

#
# This one checks for login, but then dies with an appropriate error
# message. The modifier allows you to turn off checks for specified
# conditions. 
#
function LOGGEDINORDIE($uid, $modifier = 0) {
    if ($uid == FALSE)
        USERERROR("You do not appear to be logged in!", 1);
    
    $status = CHECKLOGIN($uid);

    switch ($status & CHECKLOGIN_STATUSMASK) {
    case CHECKLOGIN_NOTLOGGEDIN:
        USERERROR("You do not appear to be logged in!", 1);
        break;
    case CHECKLOGIN_TIMEDOUT:
        USERERROR("Your login has timed out! Please log in again.", 1);
        break;
    case CHECKLOGIN_MAYBEVALID:
        USERERROR("Your login cannot be verified. Are cookies turned on? ".
		  "Are you using https? Are you logged in using another ".
		  "browser or another machine?", 1);
        break;
    case CHECKLOGIN_LOGGEDIN:
	break;
    default:
	TBERROR("LOGGEDINORDIE failed mysteriously", 1);
    }

    $status = $status & ~$modifier;

    #
    # Check other conditions.
    #
    if ($status & CHECKLOGIN_PSWDEXPIRED)
        USERERROR("Your password has expired. ".
		  "<a href=moduserinfo.php3>Please change it now!</a>", 1);
    if ($status & CHECKLOGIN_FROZEN)
        USERERROR("Your account has been frozen!", 1);
    if ($status & (CHECKLOGIN_UNVERIFIED|CHECKLOGIN_NEWUSER))
        USERERROR("You have not verified your account yet!", 1);
    if ($status & CHECKLOGIN_UNAPPROVED)
        USERERROR("Your account has not been approved yet!", 1);

    #
    # Lastly, check for nologins here. This heads off a bunch of other
    # problems and checks we would need.
    #
    if (NOLOGINS() && !ISADMIN($uid))
        USERERROR("Sorry. The Web Interface is ".
		  "<a href=nologins.php3>Temporarily Unavailable!</a>", 1);

    return $uid;
}

#
# Is this user an admin type, and is his admin bit turned on.
# Its actually incorrect to look at the $uid. Its the currently logged
# in user that has to be admin. So ignore the uid and make sure
# there is a login status.
#
function ISADMIN($uid) {
    global $CHECKLOGIN_STATUS;
    
    if ($CHECKLOGIN_STATUS == CHECKLOGIN_NOSTATUS)
	TBERROR("ISADMIN: $uid is not logged in!", 1);

    return (($CHECKLOGIN_STATUS &
	     (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_ISADMIN|CHECKLOGIN_ADMINOFF)) ==
	    (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_ISADMIN));
}

# Is this user a real administrator (ignore onoff bit).
function ISADMININSTRATOR() {
    global $CHECKLOGIN_STATUS;
    
    if ($CHECKLOGIN_STATUS == CHECKLOGIN_NOSTATUS)
	TBERROR("ISADMIN: $uid is not logged in!", 1);

    return (($CHECKLOGIN_STATUS &
	     (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_ISADMIN)) ==
	    (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_ISADMIN));
}

#
# Attempt a login.
# 
function DOLOGIN($uid, $password) {
    global $TBDBNAME, $TBAUTHCOOKIE, $TBAUTHDOMAIN, $TBAUTHTIMEOUT;
    global $TBNAMECOOKIE, $TBSECURECOOKIES;

    if (! isset($password) ||
	strcmp($password, "") == 0) {
	return -1;
    }

    $query_result =
	DBQueryFatal("SELECT usr_pswd FROM users WHERE uid='$uid'");

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
        $query_result =
	    DBQueryFatal("SELECT timeout FROM login WHERE uid='$uid'");
	
	if (mysql_num_rows($query_result)) {
	    DBQueryFatal("UPDATE login set ".
			 "timeout='$timeout', hashkey='$hashkey' ".
			 "WHERE uid='$uid'");
	}
	else {
	    DBQueryFatal("INSERT into login (uid, hashkey, timeout) ".
			 "VALUES ('$uid', '$hashkey', '$timeout')");
	}

	#
	# Create a last login record.
	#
	DBQueryFatal("REPLACE into lastlogin (uid, time) ".
		     " VALUES ('$uid', NOW())");

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

	#
	# Set adminoff on new logins.
	#
	DBQueryFatal("update users set adminoff=1 where uid='$uid'");

	return 0;
    }
    #
    # No such user
    #
    return -1;
}


#
# Verify a password
# 
function VERIFYPASSWD($uid, $password) {
    if (! isset($password) ||
	strcmp($password, "") == 0) {
	return -1;
    }

    $query_result =
	DBQueryFatal("SELECT usr_pswd FROM users WHERE uid='$uid'");

    #
    # Check password in the database against provided. 
    #
    if ($row = mysql_fetch_row($query_result)) {
        $db_encoding = $row[0];
        $salt = substr($db_encoding, 0, 2);
	
        if ($salt[0] == $salt[1]) {
	    $salt = $salt[0];
	}
        $encoding = crypt("$password", $salt);
	
        if (strcmp($encoding, $db_encoding)) {
            return -1;
	}
	return 0;
    }
    return -1;
}

#
# Log out a UID.
#
function DOLOGOUT($uid) {
    global $TBDBNAME, $TBSECURECOOKIES, $CHECKLOGIN_STATUS;

    $CHECKLOGIN_STATUS = CHECKLOGIN_NOTLOGGEDIN;

    $query_result =
	DBQueryFatal("SELECT hashkey timeout FROM login WHERE uid='$uid'");

    # Not logged in.
    if (($row = mysql_fetch_array($query_result)) == 0) {
	return 0;
    }

    $hashkey = $row[hashkey];
    $timeout = time() - 1000000;

    DBQueryFatal("DELETE FROM login WHERE uid='$uid'");

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
    global $CHECKLOGIN_NOLOGINS;

    #
    # And lastly, check for NOLOGINS! 
    #
    if ($CHECKLOGIN_NOLOGINS >= 0)
	return $CHECKLOGIN_NOLOGINS;

    $query_result =
	DBQueryFatal("SELECT nologins FROM nologins where nologins=1");

    # No entry
    if (($row = mysql_fetch_array($query_result)) == 0) {
	$CHECKLOGIN_NOLOGINS = 0;
    }
    else {
	$CHECKLOGIN_NOLOGINS = $row[nologins];
    }

    return $CHECKLOGIN_NOLOGINS;
}

function LASTWEBLOGIN($uid) {
    global $TBDBNAME;

    $query_result =
	DBQueryFatal("SELECT time from lastlogin where uid='$uid'");
    
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
