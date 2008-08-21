<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2008 University of Utah and the Flux Group.
# All rights reserved.
#
#
# Login support: Beware empty spaces (cookies)!
#

# These global are to prevent repeated calls to the DB. 
#
$CHECKLOGIN_STATUS		= -1;
$CHECKLOGIN_UID			= 0;
$CHECKLOGIN_IDX			= null;
$CHECKLOGIN_NOLOGINS		= -1;
$CHECKLOGIN_WIKINAME            = "";
$CHECKLOGIN_IDLETIME            = 0;
$CHECKLOGIN_HASHKEY             = null;
$CHECKLOGIN_HASHHASH            = null;
$CHECKLOGIN_USER                = null;

#
# New Mapping. 
#
define("CHECKLOGIN_NOSTATUS",		-1);
define("CHECKLOGIN_NOTLOGGEDIN",	0);
define("CHECKLOGIN_LOGGEDIN",		1);
define("CHECKLOGIN_TIMEDOUT",		2);
define("CHECKLOGIN_MAYBEVALID",		4);
define("CHECKLOGIN_STATUSMASK",		0x0000ff);
define("CHECKLOGIN_MODMASK",		0xffff00);
#
# These are modifiers of the above status fields. They are stored
# as a bit field in the top part. This is intended to localize as
# many queries related to login as possible. 
#
define("CHECKLOGIN_NEWUSER",		0x000100);
define("CHECKLOGIN_UNVERIFIED",		0x000200);
define("CHECKLOGIN_UNAPPROVED",		0x000400);
define("CHECKLOGIN_ACTIVE",		0x000800);
define("CHECKLOGIN_USERSTATUS",		0x000f00);
define("CHECKLOGIN_PSWDEXPIRED",	0x001000);
define("CHECKLOGIN_FROZEN",		0x002000);
define("CHECKLOGIN_ISADMIN",		0x004000);
define("CHECKLOGIN_TRUSTED",		0x008000);
define("CHECKLOGIN_CVSWEB",		0x010000);
define("CHECKLOGIN_ADMINON",		0x020000);
define("CHECKLOGIN_WEBONLY",		0x040000);
define("CHECKLOGIN_PLABUSER",		0x080000);
define("CHECKLOGIN_STUDLY",		0x100000);
define("CHECKLOGIN_WIKIONLY",		0x200000);
define("CHECKLOGIN_OPSGUY",		0x400000);	# Member of emulab-ops.
define("CHECKLOGIN_ISFOREIGN_ADMIN",	0x800000);	# Admin of another Emulab. 

#
# Constants for tracking possible login attacks.
#
define("DOLOGIN_MAXUSERATTEMPTS",	15);
define("DOLOGIN_MAXIPATTEMPTS",		25);

#
# Generate a hash value suitable for authorization. We use the results of
# microtime, combined with a random number.
# 
function GENHASH() {
    $fp = fopen("/dev/urandom", "r");
    if (! $fp) {
        TBERROR("Error opening /dev/urandom", 1);
    }
    $random_bytes = fread($fp, 128);
    fclose($fp);

    $hash  = mhash (MHASH_MD5, bin2hex($random_bytes) . " " . microtime());
    return bin2hex($hash);
}

#
# Return the value of what we told the browser to remember for the login.
# Currently, this is an email address, stored in a long term cookie.
#
function REMEMBERED_ID() {
    global $TBEMAILCOOKIE;

    if (isset($_COOKIE[$TBEMAILCOOKIE])) {
	return $_COOKIE[$TBEMAILCOOKIE];
    }
    return null;
}

#
# Return the value of the currently logged in uid, or null if not
# logged in. This interface is deprecated and being replaced.
# 
function GETLOGIN() {
    global $CHECKLOGIN_USER;
    
    if (CheckLogin($status))
	return $CHECKLOGIN_USER->uid();

    return FALSE;
}

#
# Return the value of the UID cookie. This does not check to see if
# this person is currently logged in. We just want to know what the
# browser thinks, if anything.
# 
function GETUID() {
    global $TBNAMECOOKIE;
    $status_archived = TBDB_USERSTATUS_ARCHIVED;

    if (isset($_GET['nocookieuid'])) {
	$uid = $_GET['nocookieuid'];
	
	#
	# XXX - nocookieuid is sent by netbuild applet in URL. A few other java
        # apps as well, and we retain this for backwards compatability.
	#
        # Pedantic check
	if (! preg_match("/^[-\w]+$/", $uid)) {
	    return FALSE;
	}
	$safe_uid = addslashes($uid);

	#
	# Map this to an index (from a uid).
	#
	$query_result =
	    DBQueryFatal("select uid_idx from users ".
			 "where uid='$safe_uid' and ".
			 "      status!='$status_archived'");
    
	if (! mysql_num_rows($query_result))
	    return FALSE;
	
	$row = mysql_fetch_array($query_result);
	return $row[0];
    }
    elseif (isset($_COOKIE[$TBNAMECOOKIE])) {
	$idx = $_COOKIE[$TBNAMECOOKIE];

        # Pedantic check
	if (! preg_match("/^[-\w]+$/", $idx)) {
	    return FALSE;
	}

	return $idx;
    }
    return FALSE;
}

#
# Verify a login by sucking UIDs current hash value out of the database.
# If the login has expired, or of the hashkey in the database does not
# match what came back in the cookie, then the UID is no longer logged in.
#
# Returns a combination of the CHECKLOGIN values above.
#
function LoginStatus() {
    global $TBAUTHCOOKIE, $TBLOGINCOOKIE, $HTTP_COOKIE_VARS, $TBAUTHTIMEOUT;
    global $CHECKLOGIN_STATUS, $CHECKLOGIN_UID, $CHECKLOGIN_NODETYPES;
    global $CHECKLOGIN_WIKINAME, $TBOPSPID;
    global $EXPOSEARCHIVE, $EXPOSETEMPLATES;
    global $CHECKLOGIN_HASHKEY, $CHECKLOGIN_HASHHASH;
    global $CHECKLOGIN_IDX, $CHECKLOGIN_USER;
    
    #
    # If we already figured this out, do not duplicate work!
    #
    if ($CHECKLOGIN_STATUS != CHECKLOGIN_NOSTATUS) {
	return $CHECKLOGIN_STATUS;
    }

    # No UID in the browser? Obviously not logged in!
    if (($uid_idx = GETUID()) == FALSE) {
	$CHECKLOGIN_STATUS = CHECKLOGIN_NOTLOGGEDIN;
	return $CHECKLOGIN_STATUS;
    }
    $CHECKLOGIN_IDX = $uid_idx;

    # for java applet, we can send the key in the $auth variable,
    # rather than passing it is a cookie.
    if (isset($_GET['nocookieauth'])) {
	$curhash = $_GET['nocookieauth'];
    }
    elseif (array_key_exists($TBAUTHCOOKIE, $HTTP_COOKIE_VARS)) {
	$curhash = $HTTP_COOKIE_VARS[$TBAUTHCOOKIE];
    }
    if (array_key_exists($TBLOGINCOOKIE, $HTTP_COOKIE_VARS)) {
	$hashhash = $HTTP_COOKIE_VARS[$TBLOGINCOOKIE];
    }

    #
    # We have to get at least one of the hashes. The Java applets do not
    # send it, but web browsers will.
    #
    if (!isset($curhash) && !isset($hashhash)) {
	$CHECKLOGIN_STATUS = CHECKLOGIN_NOTLOGGEDIN;
	return $CHECKLOGIN_STATUS;
    }
    if (isset($curhash) &&
	! preg_match("/^[\w]+$/", $curhash)) {
	$CHECKLOGIN_STATUS = CHECKLOGIN_NOTLOGGEDIN;
	return $CHECKLOGIN_STATUS;
    }
    if (isset($hashhash) &&
	! preg_match("/^[\w]+$/", $hashhash)) {
	$CHECKLOGIN_STATUS = CHECKLOGIN_NOTLOGGEDIN;
	return $CHECKLOGIN_STATUS;
    }

    if (isset($curhash)) {
	$CHECKLOGIN_HASHKEY = $safe_curhash = addslashes($curhash);
    }
    if (isset($hashhash)) {
	$CHECKLOGIN_HASHHASH = $safe_hashhash = addslashes($hashhash);
    }
    $safe_idx = addslashes($uid_idx);
    
    #
    # Note that we get multiple rows back because of the group_membership
    # join. No big deal.
    # 
    $query_result =
	DBQueryFatal("select NOW()>=u.pswd_expires,l.hashkey,l.timeout, ".
		     "       status,admin,cvsweb,g.trust,l.adminon,webonly, " .
		     "       user_interface,n.type,u.stud,u.wikiname, ".
		     "       u.wikionly,g.pid,u.foreign_admin,u.uid_idx, " .
		     "       p.allow_workbench ".
		     " from users as u ".
		     "left join login as l on l.uid_idx=u.uid_idx ".
		     "left join group_membership as g on g.uid_idx=u.uid_idx ".
		     "left join projects as p on p.pid_idx=g.pid_idx ".
		     "left join nodetypeXpid_permissions as n on g.pid=n.pid ".
		     "where u.uid_idx='$safe_idx' and ".
		     (isset($curhash) ?
		      "l.hashkey='$safe_curhash'" :
		      "l.hashhash='$safe_hashhash'"));

    # No such user.
    if (! mysql_num_rows($query_result)) { 
	$CHECKLOGIN_STATUS = CHECKLOGIN_NOTLOGGEDIN;
	return $CHECKLOGIN_STATUS;
    }
    
    #
    # Scan the rows. All the info is duplicate, except for the trust
    # values and the pid. pid is a hack.
    #
    $trusted   = 0;
    $opsguy    = 0;
    $workbench = 0;
    
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
	$adminon  = $row[7];
	$webonly  = $row[8];
	$interface= $row[9];

	$type     = $row[10];
	$stud     = $row[11];
	$wikiname = $row[12];
	$wikionly = $row[13];

	# Check for an ops guy.
	$pid = $row[14];
	if ($pid == $TBOPSPID) {
	    $opsguy = 1;
	}

	# Set foreign_admin=1 for admins of another Emulab.
	$foreign_admin   = $row[15];
	$uid_idx         = $row[16];
	$workbench      += $row[17];

	$CHECKLOGIN_NODETYPES[$type] = 1;
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
	DBQueryFatal("DELETE FROM login WHERE uid_idx='$uid_idx'");
	$CHECKLOGIN_STATUS = CHECKLOGIN_NOTLOGGEDIN;
	return $CHECKLOGIN_STATUS;
    }

    #
    # Check for expired login. Remove this entry from the logins table to
    # keep it from getting cluttered.
    #
    if (time() > $timeout) {
	DBQueryFatal("delete from login where ".
		     "uid_idx='$uid_idx' and hashkey='$hashkey'");
	$CHECKLOGIN_STATUS = CHECKLOGIN_TIMEDOUT;
	return $CHECKLOGIN_STATUS;
    }
    $CHECKLOGIN_IDLETIME = time() - ($timeout - $TBAUTHTIMEOUT);

    #
    # We know the login has not expired. The problem is that we might not
    # have received a cookie since that is set to transfer only when using
    # https. However, we do not want the menu to be flipping back and forth
    # each time the user uses http (say, for documentation), and so the lack
    # of a cookie does not provide enough info to determine if the user is
    # logged in or not from the current browser. Also, we want to allow for
    # a user to switch browsers, and not get confused by getting a uid but
    # no valid cookie from the new browser. In that case the user should just
    # be able to login from the new browser; gets a standard not-logged-in
    # front page. In order to accomplish this, we need another cookie that is
    # set on login, cleared on logout.
    #
    if (isset($curhash)) {
	#
	# Got a cookie (https).
	#
	if ($curhash != $hashkey) {
	    #
	    # User is not logged in from this browser. Must be stale.
	    # 
	    $CHECKLOGIN_STATUS = CHECKLOGIN_NOTLOGGEDIN;
	    return $CHECKLOGIN_STATUS;
	}
	else {
            #
	    # User is logged in.
	    #
	    $CHECKLOGIN_STATUS = CHECKLOGIN_LOGGEDIN;
	}
    }
    else {
	#
	# No cookie. Might be because its http, so there is no way to tell
	# if user is not logged in from the current browser without more
	# information. We use another cookie for this, which is a crc of
	# of the real hash, and simply tells us what menu to draw, but does
	# not impart any privs!
	#
	if (isset($hashhash) &&
	    $hashhash == bin2hex(mhash(MHASH_CRC32, $hashkey))) {
            #
            # The login is probably valid, but we have no proof yet. 
            #
	    $CHECKLOGIN_STATUS = CHECKLOGIN_MAYBEVALID;
	}
	else {
	    #
	    # No hash of the hash, so assume no real cookie either. 
	    # 
	    $CHECKLOGIN_STATUS = CHECKLOGIN_NOTLOGGEDIN;
	    return $CHECKLOGIN_STATUS;
	}
    }

    # Cache this now; someone will eventually want it.
    $CHECKLOGIN_USER = User::Lookup($uid_idx);
    if (! $CHECKLOGIN_USER) {
	$CHECKLOGIN_STATUS = CHECKLOGIN_NOTLOGGEDIN;
	return $CHECKLOGIN_STATUS;
    }

    #
    # Now add in the modifiers.
    #
    # Do not expire passwords for admin users.
    if ($expired && !$admin)
	$CHECKLOGIN_STATUS |= CHECKLOGIN_PSWDEXPIRED;
    if ($admin)
	$CHECKLOGIN_STATUS |= CHECKLOGIN_ISADMIN;
    if ($adminon)
	$CHECKLOGIN_STATUS |= CHECKLOGIN_ADMINON;
    if ($webonly)
	$CHECKLOGIN_STATUS |= CHECKLOGIN_WEBONLY;
    if ($wikionly)
	$CHECKLOGIN_STATUS |= CHECKLOGIN_WIKIONLY;
    if ($trusted)
	$CHECKLOGIN_STATUS |= CHECKLOGIN_TRUSTED;
    if ($stud)
	$CHECKLOGIN_STATUS |= CHECKLOGIN_STUDLY;
    if ($cvsweb)
	$CHECKLOGIN_STATUS |= CHECKLOGIN_CVSWEB;
    if ($interface == TBDB_USER_INTERFACE_PLAB)
	$CHECKLOGIN_STATUS |= CHECKLOGIN_PLABUSER;
    if (strcmp($status, TBDB_USERSTATUS_NEWUSER) == 0)
	$CHECKLOGIN_STATUS |= CHECKLOGIN_NEWUSER;
    if (strcmp($status, TBDB_USERSTATUS_UNAPPROVED) == 0)
	$CHECKLOGIN_STATUS |= CHECKLOGIN_UNAPPROVED;
    if (strcmp($status, TBDB_USERSTATUS_UNVERIFIED) == 0)
	$CHECKLOGIN_STATUS |= CHECKLOGIN_UNVERIFIED;
    if (strcmp($status, TBDB_USERSTATUS_ACTIVE) == 0)
	$CHECKLOGIN_STATUS |= CHECKLOGIN_ACTIVE;
    if (isset($wikiname) && $wikiname != "")
	$CHECKLOGIN_WIKINAME = $wikiname;
    if ($opsguy)
	$CHECKLOGIN_STATUS |= CHECKLOGIN_OPSGUY;
    if ($foreign_admin)
	$CHECKLOGIN_STATUS |= CHECKLOGIN_ISFOREIGN_ADMIN;

    #
    # Set the magic enviroment variable, if appropriate, for the sake of
    # any processes we might spawn. We prepend an HTTP_ on the front of
    # the variable name, so that it will get through suexec.
    #
    if ($admin && $adminon) {
    	putenv("HTTP_WITH_TB_ADMIN_PRIVS=1");
    }
    #
    # This environment variable is likely to become the new method for
    # specifying the credentials of the invoking user. Still thinking
    # about this, but the short story is that the web interface should
    # not invoke so much stuff as the user, but rather as a neutral user
    # with implied credentials. 
    #
    putenv("HTTP_INVOKING_USER=" . $CHECKLOGIN_USER->webid());
    
    # XXX Temporary.
    if ($stud) {
	$EXPOSEARCHIVE = 1;
    }
    if ($workbench) {
	$EXPOSETEMPLATES = $EXPOSEARCHIVE = 1;
    }
    return $CHECKLOGIN_STATUS;
}

#
# This one checks for login, but then dies with an appropriate error
# message. The modifier allows you to turn off checks for specified
# conditions. 
#
function LOGGEDINORDIE($uid, $modifier = 0, $login_url = NULL) {
    global $TBBASE, $BASEPATH;
    global $TBAUTHTIMEOUT, $CHECKLOGIN_HASHKEY, $CHECKLOGIN_IDX;

    #
    # We now ignore the $uid argument and let LoginStatus figure it out.
    #
    
    #
    # Allow the caller to specify a different URL to direct the user to
    #
    if (!$login_url) {
	$login_url = "$TBBASE/login.php3?refer=1";
    }

    $link = "\n<a href=\"$login_url\">Please ".
	"log in again.</a>\n";

    $status = LoginStatus();

    switch ($status & CHECKLOGIN_STATUSMASK) {
    case CHECKLOGIN_NOTLOGGEDIN:
        USERERROR("You do not appear to be logged in! $link", 1);
        break;
    case CHECKLOGIN_TIMEDOUT:
        USERERROR("Your login has timed out! $link", 1);
        break;
    case CHECKLOGIN_MAYBEVALID:
        USERERROR("Your login cannot be verified. Are cookies turned on? ".
		  "Are you using https? Are you logged in using another ".
		  "browser or another machine? $link", 1);
        break;
    case CHECKLOGIN_LOGGEDIN:
        #
	# Update the time in the database.
        # Basically, each time the user does something, we bump the
	# logout further into the future. This avoids timing them
	# out just when they are doing useful work.
        #
	if (! is_null($CHECKLOGIN_HASHKEY)) {
	    $timeout = time() + $TBAUTHTIMEOUT;

	    DBQueryFatal("UPDATE login set timeout='$timeout' ".
			 "where uid_idx='$CHECKLOGIN_IDX' and ".
			 "      hashkey='$CHECKLOGIN_HASHKEY'");
	}
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
    if (($status & CHECKLOGIN_WEBONLY) && ! ISADMIN())
        USERERROR("Your account does not permit you to access this page!", 1);
    if (($status & CHECKLOGIN_WIKIONLY) && ! ISADMIN())
        USERERROR("Your account does not permit you to access this page!", 1);

    #
    # Lastly, check for nologins here. This heads off a bunch of other
    # problems and checks we would need.
    #
    if (NOLOGINS() && !ISADMIN())
        USERERROR("Sorry. The Web Interface is ".
		  "<a href=nologins.php3>Temporarily Unavailable!</a>", 1);

    # No one should ever look at the return value of this function.
    return null;
}

#
# This is the new interface to the above function. 
#
function CheckLoginOrDie($modifier = 0, $login_url = NULL)
{
    global $CHECKLOGIN_USER;
    
    LOGGEDINORDIE(GETUID(), $modifier, $login_url);

    #
    # If this returns, login is valid. Return the user object to caller.
    #
    return $CHECKLOGIN_USER;
}

#
# This interface allows the return of the actual status. I know, its a
# global variable, but this interface is cleaner. 
#
function CheckLogin(&$status)
{
    global $CHECKLOGIN_USER;

    $status = LoginStatus();

    # If login looks valid, return the user. 
    if ($status & (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_MAYBEVALID))
	    return $CHECKLOGIN_USER;

    return null;
}

#
# Is this user an admin type, and is his admin bit turned on.
# Its actually incorrect to look at the $uid. Its the currently logged
# in user that has to be admin. So ignore the uid and make sure
# there is a login status.
#
function ISADMIN() {
    global $CHECKLOGIN_STATUS;
    
    if ($CHECKLOGIN_STATUS == CHECKLOGIN_NOSTATUS) {
	return 0;
    }

    return (($CHECKLOGIN_STATUS &
	     (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_ISADMIN|CHECKLOGIN_ADMINON)) ==
	    (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_ISADMIN|CHECKLOGIN_ADMINON));
}


# Is this user an admin of another Emulab.
function ISFOREIGN_ADMIN($uid = 1) {
    global $CHECKLOGIN_STATUS;

    # Definitely not, if not logged in.
    if ($CHECKLOGIN_STATUS == CHECKLOGIN_NOSTATUS) {
	return 0;
    }

    return (($CHECKLOGIN_STATUS &
	     (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_ISFOREIGN_ADMIN)) ==
	    (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_ISFOREIGN_ADMIN));
}

function STUDLY() {
    global $CHECKLOGIN_STATUS;
    
    if ($CHECKLOGIN_STATUS == CHECKLOGIN_NOSTATUS) {
	TBERROR("STUDLY: user is not logged in!", 1);
    }

    return (($CHECKLOGIN_STATUS &
	     (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_STUDLY)) ==
	    (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_STUDLY));
}

function OPSGUY() {
    global $CHECKLOGIN_STATUS;
    
    if ($CHECKLOGIN_STATUS == CHECKLOGIN_NOSTATUS) {
	TBERROR("OPSGUY: user is not logged in!", 1);
    }

    return (($CHECKLOGIN_STATUS &
	     (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_OPSGUY)) ==
	    (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_OPSGUY));
}

function WIKIONLY() {
    global $CHECKLOGIN_STATUS;
    
    if ($CHECKLOGIN_STATUS == CHECKLOGIN_NOSTATUS) {
	TBERROR("WIKIONLY: user is not logged in!", 1);
    }

    return (($CHECKLOGIN_STATUS &
	     (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_WIKIONLY)) ==
	    (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_WIKIONLY));
}

# Is this user a real administrator (ignore onoff bit).
function ISADMINISTRATOR() {
    global $CHECKLOGIN_STATUS;
    
    if ($CHECKLOGIN_STATUS == CHECKLOGIN_NOSTATUS)
	TBERROR("ISADMIN: $uid is not logged in!", 1);

    return (($CHECKLOGIN_STATUS &
	     (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_ISADMIN)) ==
	    (CHECKLOGIN_LOGGEDIN|CHECKLOGIN_ISADMIN));
}

#
# Toggle current login admin bit. Must be an administrator of course!
#
function SETADMINMODE($onoff) {
    global $CHECKLOGIN_HASHKEY, $CHECKLOGIN_IDX;
    
    # This makes sure the user is actually logged in secure (https).
    if (! ISADMINISTRATOR())
	return;

    # Be pedantic.
    if (! ($CHECKLOGIN_HASHKEY && $CHECKLOGIN_IDX))
	return;

    $onoff   = addslashes($onoff);
    $curhash = addslashes($CHECKLOGIN_HASHKEY);
    $uid_idx = $CHECKLOGIN_IDX;
    
    DBQueryFatal("update login set adminon='$onoff' ".
		 "where uid_idx='$uid_idx' and hashkey='$curhash'");
}

# Is this user a planetlab user? Returns 1 if they are, 0 if not.
function ISPLABUSER() {
    global $CHECKLOGIN_STATUS;

    if ($CHECKLOGIN_STATUS == CHECKLOGIN_NOSTATUS) {
	#
	# For users who are not logged in, we need to check the database
	#
	$uid = GETUID();
	if (!$uid) {
	    return 0;
	}
	# Lookup sanitizes argument.
	if (! ($user = User::Lookup($uid)))
	    return 0;

	if ($user->user_interface()) {
	    return ($user->user_interface() == TBDB_USER_INTERFACE_PLAB);
	}
	else {
	    return 0;
	}
    } else {
	#
	# For logged-in users, we recorded it in the the login status
	#
	return (($CHECKLOGIN_STATUS &
		 (CHECKLOGIN_PLABUSER)) ==
		(CHECKLOGIN_PLABUSER));
    }
}

#
# Check to see if a user is allowed, in some project, to use the given node
# type. Returns 1 if allowed, 0 if not.
#
# NOTE: This is NOT intended as a real permissions check. It is intended only
# for display purposes (ie. deciding whether or not to give the user a link to
# the plab_ez page.) It does not require the user to be actually logged in, so
# that it still works for pages fetched through http. Thus, it may be possible
# for a clever user to fake it out.
#
function NODETYPE_ALLOWED($type) {
    global $CHECKLOGIN_NODETYPES;

    if (! GETUID())
	return 0;

    if (isset($CHECKLOGIN_NODETYPES[$type])) {
	return 1;
    } else {
	return 0;
    }
}

#
# Attempt a login.
# 
function DOLOGIN($token, $password, $adminmode = 0) {
    global $TBAUTHCOOKIE, $TBAUTHDOMAIN, $TBAUTHTIMEOUT;
    global $TBNAMECOOKIE, $TBLOGINCOOKIE, $TBSECURECOOKIES;
    global $TBMAIL_OPS, $TBMAIL_AUDIT, $TBMAIL_WWW;
    global $WIKISUPPORT, $WIKICOOKIENAME;
    global $BUGDBSUPPORT, $BUGDBCOOKIENAME;
    
    # Caller makes these checks too.
    if ((!TBvalid_uid($token) && !TBvalid_email($token)) ||
	!isset($password) || $password == "") {
	return -1;
    }
    $now = time();

    #
    # Check for a frozen IP address; too many failures.
    #
    unset($iprow);
    unset($IP);
    if (isset($_SERVER['REMOTE_ADDR'])) {
	$IP = $_SERVER['REMOTE_ADDR'];
	
	$ip_result =
	    DBQueryFatal("select * from login_failures ".
			 "where IP='$IP'");

	if ($iprow = mysql_fetch_array($ip_result)) {
	    $ipfrozen = $iprow['frozen'];

	    if ($ipfrozen) {
		DBQueryFatal("update login_failures set ".
			     "       failcount=failcount+1, ".
			     "       failstamp='$now' ".
			     "where IP='$IP'");
		return -1;
	    }
	}
    }

    if (TBvalid_email($token)) {
	$user = User::LookupByEmail($token);
    }
    else {
	$user = User::Lookup($token);
    }
	    
    #
    # Check password in the database against provided. 
    #
    do {
      if ($user) {
	$uid         = $user->uid();
        $db_encoding = $user->pswd();
	$isadmin     = $user->admin();
	$frozen      = $user->weblogin_frozen();
	$failcount   = $user->weblogin_failcount();
	$failstamp   = $user->weblogin_failstamp();
	$usr_email   = $user->email();
	$usr_name    = $user->name();
	$uid_idx     = $user->uid_idx();
	$usr_email   = $user->email();

	# Check for frozen accounts. We do not update the IP record when
	# an account is frozen.
	if ($frozen) {
	    $user->UpdateWebLoginFail();
	    return -1;
	}
	
        $encoding = crypt("$password", $db_encoding);
        if (strcmp($encoding, $db_encoding)) {
	    #
	    # Bump count and check for too many consecutive failures.
	    #
	    $failcount++;
	    if ($failcount > DOLOGIN_MAXUSERATTEMPTS) {
		$user->SetWebFreeze(1);
		
		TBMAIL("$usr_name '$uid' <$usr_email>",
		   "Web Login Freeze: '$uid'",
		   "Your login has been frozen because there were too many\n".
		   "login failures from " . $_SERVER['REMOTE_ADDR'] . ".\n\n".
		   "Testbed Operations has been notified.\n",
		   "From: $TBMAIL_OPS\n".
		   "Cc: $TBMAIL_OPS\n".
		   "Bcc: $TBMAIL_AUDIT\n".
		   "Errors-To: $TBMAIL_WWW");
	    }
	    $user->UpdateWebLoginFail();
            break;
        }
	#
	# Pass!
	#

	#
	# Set adminmode off on new logins, unless user requested to be
	# logged in as admin (and is an admin of course!). This is
	# primarily to bypass the nologins directive which makes it
	# impossible for an admin to login when the web interface is
	# turned off. 
	#
	$adminon = 0;
	if ($adminmode && $isadmin) {
	    $adminon = 1;
	}

        #
        # Insert a record in the login table for this uid.
	#
	if (DOLOGIN_MAGIC($uid, $uid_idx, $usr_email, $adminon) < 0) {
	    return -1;
	}

	#
	# Usage stats. 
	#
	DBQueryFatal("update user_stats set ".
		     " weblogin_count=weblogin_count+1, ".
		     " weblogin_last=now() ".
		     "where uid_idx='$uid_idx'");

	# Clear IP record since we have a sucessful login from the IP.
	if (isset($IP)) {
	    DBQueryFatal("delete from login_failures where IP='$IP'");
	}
	return 0;
      }
    } while (0);
    #
    # No such user
    #
    if (!isset($IP)) {
	return -1;
    }

    $ipfrozen = 0;
    if (isset($iprow)) {
	$ipfailcount = $iprow['failcount'];

        #
        # Bump count.
        #
	$ipfailcount++;
    }
    else {
	#
	# First failure.
	# 
	$ipfailcount = 1;
    }

    #
    # Check for too many consecutive failures.
    #
    if ($ipfailcount > DOLOGIN_MAXIPATTEMPTS) {
	$ipfrozen = 1;
	    
	TBMAIL($TBMAIL_OPS,
	       "Web Login Freeze: '$IP'",
	       "Logins has been frozen because there were too many login\n".
	       "failures from $IP. Last attempted uid was '$token'.\n\n",
	       "From: $TBMAIL_OPS\n".
	       "Bcc: $TBMAIL_AUDIT\n".
	       "Errors-To: $TBMAIL_WWW");
    }
    DBQueryFatal("replace into login_failures set ".
		 "       IP='$IP', ".
		 "       frozen='$ipfrozen', ".
		 "       failcount='$ipfailcount', ".
		 "       failstamp='$now'");
    return -1;
}

function DOLOGIN_MAGIC($uid, $uid_idx, $email = null, $adminon = 0)
{
    global $TBAUTHCOOKIE, $TBAUTHDOMAIN, $TBAUTHTIMEOUT;
    global $TBNAMECOOKIE, $TBLOGINCOOKIE, $TBSECURECOOKIES, $TBEMAILCOOKIE;
    global $TBMAIL_OPS, $TBMAIL_AUDIT, $TBMAIL_WWW;
    global $WIKISUPPORT, $WIKICOOKIENAME;
    global $BUGDBSUPPORT, $BUGDBCOOKIENAME, $TRACSUPPORT, $TRACCOOKIENAME;
    
    # Caller makes these checks too.
    if (!TBvalid_uid($uid)) {
	return -1;
    }
    if (!TBvalid_uididx($uid_idx)) {
	return -1;
    }
    $now = time();

    #
    # Insert a record in the login table for this uid with
    # the new hash value. If the user is already logged in, thats
    # okay; just update it in place with a new hash and timeout. 
    #
    $timeout = $now + $TBAUTHTIMEOUT;
    $hashkey = GENHASH();
    $crc     = bin2hex(mhash(MHASH_CRC32, $hashkey));

    DBQueryFatal("replace into login ".
		 "  (uid,uid_idx,hashkey,hashhash,timeout,adminon) values ".
		 "  ('$uid', $uid_idx, '$hashkey', '$crc', '$timeout', $adminon)");

    #
    # Issue the cookie requests so that subsequent pages come back
    # with the hash value and auth usr embedded.

    #
    # For the hashkey, we use a zero timeout so that the cookie is
    # a session cookie; killed when the browser is exited. Hopefully this
    # keeps the key from going to disk on the client machine. The cookie
    # lives as long as the browser is active, but we age the cookie here
    # at the server so it will become invalid at some point.
    #
    setcookie($TBAUTHCOOKIE, $hashkey, 0, "/",
	      $TBAUTHDOMAIN, $TBSECURECOOKIES);

    #
    # Another cookie, to help in menu generation. See above in
    # checklogin. This cookie is a simple hash of the real hash,
    # intended to indicate if the current browser holds a real hash.
    # All this does is change the menu options presented, imparting
    # no actual privs. 
    #
    setcookie($TBLOGINCOOKIE, $crc, 0, "/", $TBAUTHDOMAIN, 0);

    #
    # We want to remember who the user was each time they load a page
    # NOTE: This cookie is integral to authorization, since we do not pass
    # around the UID anymore, but look for it in the cookie.
    #
    setcookie($TBNAMECOOKIE, $uid_idx, 0, "/", $TBAUTHDOMAIN, 0);

    #
    # This is a long term cookie so we can remember who the user was, and
    # and stick that in the login box.
    #
    if ($email) {
	$timeout = $now + (60 * 60 * 24 * 365);
	setcookie($TBEMAILCOOKIE, $email, $timeout, "/", $TBAUTHDOMAIN, 0);
    }

    #
    # Clear the existing Wiki cookie so that there is not an old one
    # for a different user, sitting in the brower. 
    # 
    if ($WIKISUPPORT) {
	$flushtime = time() - 1000000;
	    
	setcookie($WIKICOOKIENAME, "", $flushtime, "/",
		  $TBAUTHDOMAIN, $TBSECURECOOKIES);
    }
	
    #
    # Ditto for bugdb
    # 
    if ($BUGDBSUPPORT) {
	$flushtime = time() - 1000000;
	    
	setcookie($BUGDBCOOKIENAME, "", $flushtime, "/",
		  $TBAUTHDOMAIN, $TBSECURECOOKIES);
    }
    # These cookie names are still in flux. 
    if ($TRACSUPPORT) {
	$flushtime = time() - 1000000;
	    
	setcookie("trac_auth_emulab", "", $flushtime, "/",
		  $TBAUTHDOMAIN, $TBSECURECOOKIES);
	setcookie("trac_auth_emulab_priv", "", $flushtime, "/",
		  $TBAUTHDOMAIN, $TBSECURECOOKIES);
	setcookie("trac_auth_protogeni", "", $flushtime, "/",
		  $TBAUTHDOMAIN, $TBSECURECOOKIES);
	setcookie("trac_auth_protogeni_priv", "", $flushtime, "/",
		  $TBAUTHDOMAIN, $TBSECURECOOKIES);
    }
	
    DBQueryFatal("update users set ".
		 "       weblogin_failcount=0,weblogin_failstamp=0 ".
		 "where uid_idx='$uid_idx'");

    return 0;
}

#
# Verify a password
# 
function VERIFYPASSWD($uid, $password) {
    if (! isset($password) || $password == "") {
	return -1;
    }

    if (! ($user = User::Lookup($uid)))
	return -1;

    #
    # Check password in the database against provided. 
    #
    $encoding = crypt("$password", $user->pswd());
	
    if ($encoding == $user->pswd()) {
	return 0;
    }
    return -1;
}

#
# Log out a UID.
#
function DOLOGOUT($user) {
    global $CHECKLOGIN_STATUS, $CHECKLOGIN_USER;
    global $TBAUTHCOOKIE, $TBLOGINCOOKIE, $TBAUTHDOMAIN;
    global $WIKISUPPORT, $WIKICOOKIENAME, $HTTP_COOKIE_VARS;
    global $BUGDBSUPPORT, $BUGDBCOOKIENAME, $TRACSUPPORT, $TRACCOOKIENAME;

    if (! $CHECKLOGIN_USER)
	return 1;

    $uid_idx = $user->uid_idx();

    #
    # An admin logging out another user. Nothing else to do.
    #
    if (! $user->SameUser($CHECKLOGIN_USER)) {
	DBQueryFatal("delete from login where uid_idx='$uid_idx'");
	return 0;
    }

    $CHECKLOGIN_STATUS = CHECKLOGIN_NOTLOGGEDIN;

    $curhash  = "";
    $hashhash = "";

    if (isset($HTTP_COOKIE_VARS[$TBAUTHCOOKIE])) {
	$curhash = $HTTP_COOKIE_VARS[$TBAUTHCOOKIE];
    }
    if (isset($HTTP_COOKIE_VARS[$TBLOGINCOOKIE])) {
	$hashhash = $HTTP_COOKIE_VARS[$TBLOGINCOOKIE];
    }
    
    #
    # We have to get at least one of the hashes. 
    #
    if ($curhash == "" && $hashhash == "") {
	return 1;
    }
    if ($curhash != "" &&
	! preg_match("/^[\w]+$/", $curhash)) {
	return 1;
    }
    if ($hashhash != "" &&
	! preg_match("/^[\w]+$/", $hashhash)) {
	return 1;
    }

    DBQueryFatal("delete from login ".
		 " where uid_idx='$uid_idx' and ".
		 ($curhash != "" ?
		  "hashkey='$curhash'" :
		  "hashhash='$hashhash'"));

    # Delete by giving timeout in the past
    $timeout = time() - 3600;

    #
    # Issue a cookie request to delete the cookies. Delete with timeout in past
    #
    $timeout = time() - 3600;
    
    setcookie($TBAUTHCOOKIE, "", $timeout, "/", $TBAUTHDOMAIN, 0);
    setcookie($TBLOGINCOOKIE, "", $timeout, "/", $TBAUTHDOMAIN, 0);

    if ($TRACSUPPORT) {
	setcookie("trac_auth_emulab", "", $timeout, "/",
		  $TBAUTHDOMAIN, 0);
	setcookie("trac_auth_emulab_priv", "", $timeout, "/",
		  $TBAUTHDOMAIN, 0);
	setcookie("trac_auth_protogeni", "", $timeout, "/",
		  $TBAUTHDOMAIN, 0);
	setcookie("trac_auth_protogeni_priv", "", $timeout, "/",
		  $TBAUTHDOMAIN, 0);
    }
    if ($WIKISUPPORT) {
	setcookie($WIKICOOKIENAME, "", $timeout, "/", $TBAUTHDOMAIN, 0);
    }
    if ($BUGDBSUPPORT) {
	setcookie($BUGDBCOOKIENAME, "", $timeout, "/", $TBAUTHDOMAIN, 0);
    }

    return 0;
}

#
# Simple "nologins" support.
#
function NOLOGINS() {
    global $CHECKLOGIN_NOLOGINS;

    if ($CHECKLOGIN_NOLOGINS == -1) {
	$CHECKLOGIN_NOLOGINS = TBGetSiteVar("web/nologins");
    }
	
    return $CHECKLOGIN_NOLOGINS;
}

function LASTWEBLOGIN($uid) {
    global $TBDBNAME;

    $query_result =
        DBQueryFatal("select weblogin_last from users as u ".
		     "left join user_stats as s on s.uid_idx=u.uid_idx ".
		     "where u.uid='$uid'");
    
    if (mysql_num_rows($query_result)) {
	$lastrow      = mysql_fetch_array($query_result);
	return $lastrow["weblogin_last"];
    }
    return 0;
}

function HASREALACCOUNT($uid) {
    if (! ($user = User::Lookup($uid)))
	return 0;

    $status   = $user->status();
    $webonly  = $user->webonly();
    $wikionly = $user->wikionly();

    if ($webonly || $wikionly ||
	(strcmp($status, TBDB_USERSTATUS_ACTIVE) &&
	 strcmp($status, TBDB_USERSTATUS_FROZEN))) {
	return 0;
    }
    return 1;
}

#
# Beware empty spaces (cookies)!
# 
?>
