<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2010 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

if (!$MAILMANSUPPORT) {
    header("Location: index.php3");
    return;
}

# No Pageheader since we spit out a redirection below.
$this_user = CheckLoginOrDie(CHECKLOGIN_USERSTATUS|
			     CHECKLOGIN_WEBONLY|CHECKLOGIN_WIKIONLY);
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments
#
$optargs = OptionalPageArguments("target_project", PAGEARG_PROJECT,
				 "target_group",   PAGEARG_GROUP,
				 "listname",       PAGEARG_STRING,
				 "asadmin",        PAGEARG_BOOLEAN,
				 "wantadmin",      PAGEARG_BOOLEAN,
				 "wantconfig",     PAGEARG_BOOLEAN);

#
# We will either show a specific list.
#
if (isset($target_project) || isset($target_group)) {
    if (! isset($target_group)) {
	$target_group = $target_project->DefaultGroup();
    }
    $pid = $target_group->pid();
    $gid = $target_group->gid();

    if ($target_group->IsProjectGroup())
	$listname = "$pid" . "-users";
    else
	$listname = "$pid-$gid" . "-users";
    
    #
    # Make sure the user is allowed! We must do a permission check since
    # we are asking mailman to generate a cookie without a password.
    #
    if (!$isadmin &&
	!$target_group->AccessCheck($this_user, $TB_PROJECT_READINFO)) {
	USERERROR("You are not a member of $pid/$gid.", 1);
    }

    #
    # By default, we want the user interface to the archives. However, an
    # admin can request access to the list admin interface, and we need
    # a different cookie for that.
    #
    $user_name  = $this_user->name();
    $user_email = $this_user->email();
    $user_email = rawurlencode($user_email);
    
    $cookietype = "user";
    $listiface  = "private";
    $optargs    = "?username=${user_email}";

    if (isset($wantadmin) && $isadmin) {
	$cookietype = "admin";
	$listiface  = "admin";
	$optargs    = "";
    }

    $retval = SUEXEC($uid, "nobody", "mmxlogin $uid $listname $cookietype",
		     SUEXEC_ACTION_IGNORE);

    #
    # If this was an admin trying to get to a list, then retry as admin.
    #
    if ($retval) {
	if ($isadmin && !isset($wantadmin)) {
	    $cookietype = "admin";
	    $listiface  = "admin";
	    $optargs    = "";

	    $retval = SUEXEC($uid, "nobody",
			     "mmxlogin $uid $listname $cookietype",
			     SUEXEC_ACTION_IGNORE);
	}
	if ($retval == 1) {
	    USERERROR("You are not a member of $pid/$gid.", 1);
	}
	elseif ($retval) {
	    SUEXECERROR(SUEXEC_ACTION_DIE);
	}
    }
    
    #
    # Parse the silly thing
    #
    # Set-Cookie: foo=2802; Path=/mailman/; Version=1;
    #
    if (!preg_match("/^Set-Cookie: ([-\w\+\.\%]+)=(\w*); ".
		    "Path=(\/[\w]+\/); Version=1;?$/",
		    $suexec_output, $matches)) {
	TBERROR($suexec_output, 1);
    }
    # TBERROR($matches[1] . ":" . $matches[2] . ":" . $matches[3], 0);

    setcookie($matches[1], $matches[2], 0, $matches[3], $TBAUTHDOMAIN, 0);

    $url = "${MAILMANURL}/$listiface/$listname/$optargs";
}
elseif (isset($listname) && $listname != "") {
    #
    # Zap to a specific list admin page. Must be an admin, or must be the
    # owner of the list. We do not track list membership, so members need to
    # find their lists on their own. 
    #
    if (! TBvalid_mailman_listname($listname)) {
	PAGEARGERROR("Invalid characters in $listname!");
    }
    $user_name  = $this_user->name();
    $user_email = $this_user->email();
    $user_email = rawurlencode($user_email);
	
    $optargs = "";
    #
    # Make sure the user is allowed! We must do a permission check since
    # we are asking mailman to generate a cookie without a password.
    #
    if (isset($wantadmin) || isset($asadmin)) {
	if (!$isadmin) {
	    $mm_result = DBQueryFatal("select * from mailman_listnames ".
				      "where listname='$listname'");

	    if (!mysql_num_rows($mm_result)) {
		USERERROR("No such list $listname!", 1);
	    }
	    $row = mysql_fetch_array($mm_result);
	    $owner_uid = $row['owner_uid'];

           #
           # Verify permission.
           #
	    if ($uid != $owner_uid) {
		USERERROR("You do not have permission to admin $listname!", 1);
	    }
	}
	$cookietype = "admin";
	if (isset($wantadmin)) {
	    $listiface  = "admin";
	} else {
	    $listiface  = "private";
        }
    }
    elseif (isset($wantconfig)) {
	$cookietype = "user";
	$listiface  = "options";
	$optargs    = "?email=${user_email}";
    }
    else {
	$cookietype = "user";
	$listiface  = "private";
	$optargs    = "?username=${user_email}";
    }

    SUEXEC($uid, "nobody", "mmxlogin $uid $listname $cookietype",
	   SUEXEC_ACTION_DIE);
    
    #
    # Parse the silly thing
    #
    # Set-Cookie: foo=2802; Path=/mailman/; Version=1;
    #
    if (!preg_match("/^Set-Cookie: ([-\w\+\.\%]+)=(\w*); ".
		    "Path=(\/[\w]+\/); Version=1;?$/",
		    $suexec_output, $matches)) {
	TBERROR($suexec_output, 1);
    }
    setcookie($matches[1], $matches[2], 0, $matches[3], $TBAUTHDOMAIN, 0);

    $url = "${MAILMANURL}/$listiface/$listname/$optargs";

    if (isset($link)) {
	$url .= $link;
    }
}
else {
    USERERROR("You are not a member of any mailing projects!", 1);
}

header("Location: ${url}");
?>
