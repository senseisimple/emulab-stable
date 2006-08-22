<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2005, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# This page is a generic toggle page, like adminmode.php3, but more
# generalized. There are a set of things you can toggle, and each of
# those items has a permission check and a set (pair) of valid values.
#
# Usage: toggle.php?type=swappable&value=1&pid=foo&eid=bar
# (type & value are required, others are optional and vary by type)
#
# No PAGEHEADER since we spit out a Location header later. See below.
#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid, CHECKLOGIN_USERSTATUS|CHECKLOGIN_WEBONLY);
$isadmin = ISADMIN($uid);

# List of valid toggles
$toggles = array("adminon", "webfreeze", "cvsweb", "lockdown",
		 "cvsrepo_public");

# list of valid values for each toggle
$values  = array("adminon"        => array(0,1),
		 "webfreeze"      => array(0,1),
		 "cvsweb"         => array(0,1),
		 "lockdown"       => array(0,1),
		 "cvsrepo_public" => array(0,1));

# list of valid extra variables for the each toggle, and mandatory flag.
$optargs = array("adminon"        => array(),
		 "webfreeze"      => array("target_uid" => 1),
		 "cvsweb"         => array("target_uid" => 1),
		 "lockdown"       => array("pid" => 1, "eid" => 1),
		 "cvsrepo_public" => array("pid" => 1));

# Mandatory page arguments.
$type  = $_GET['type'];
$value = $_GET['value'];

# Pedantic page argument checking. Good practice!
if (!isset($type) || !isset($value)) {
    PAGEARGERROR();
}

if (! in_array($type, $toggles)) {
    PAGEARGERROR("There is no toggle for $type!");
}
if (! in_array($value, $values[$type])) {
    PAGEARGERROR("The value '$value' is illegal for the $type toggle!");
}

# Check optional args and bind locally.
while (list ($arg, $required) = each ($optargs[$type])) {
    if (!isset($_GET[$arg])) {
	if ($required)
	    PAGEARGERROR("Toggle '$type' requires argument '$arg'");
	else
	    unset($$arg);
    }
    else {
	$$arg = addslashes($_GET[$arg]);
    }
}

#
# Permissions checks, and do the toggle...
#
if ($type == "adminon") {
    # must be admin
    # Do not check if they are admin mode (ISADMIN), check if they
    # have the power to change to admin mode!
    if (! ($CHECKLOGIN_STATUS & CHECKLOGIN_ISADMIN) ) {
	USERERROR("You do not have permission to toggle $type!", 1);
    }
    SETADMINMODE($uid, $value);
}
elseif ($type == "webfreeze") {
    # must be admin
    if (! $isadmin) {
	USERERROR("You do not have permission to toggle $type!", 1);
    }
    if (!TBCurrentUser($target_uid)) {
	PAGEARGERROR("Target user '$target_uid' is not a valid user!");
    }
    DBQueryFatal("update users set weblogin_frozen='$value' ".
		 "where uid='$target_uid'");
}
elseif ($type == "cvsweb") {
    # must be admin
    if (! $isadmin) {
	USERERROR("You do not have permission to toggle $type!", 1);
    }
    if (!TBCurrentUser($target_uid)) {
	PAGEARGERROR("Target user '$target_uid' is not a valid user!");
    }
    DBQueryFatal("update users set cvsweb='$value' ".
		 "where uid='$target_uid'");
}
elseif ($type == "lockdown") {
    # must be admin
    if (! $isadmin) {
	USERERROR("You do not have permission to toggle $type!", 1);
    }
    if (!TBValidExperiment($pid, $eid)) {
	PAGEARGERROR("Experiment $pid/$eid is not a valid experiment!");
    }
    DBQueryFatal("update experiments set lockdown='$value' ".
		 "where pid='$pid' and eid='$eid'");
}
elseif ($type == "cvsrepo_public") {
    # Must validate the pid since we allow non-admins to do this.
    if (! TBvalid_pid($pid)) {
	PAGEARGERROR("Invalid characters in $pid");
    }
    if (!TBValidProject($pid)) {
	PAGEARGERROR("Project $pid is not a valid project!");
    }
    # Must be admin or project/group root.
    if (!$isadmin &&
	! TBMinTrust(TBGrpTrust($uid, $pid, $pid), $TBDB_TRUST_GROUPROOT)) {
	USERERROR("You do not have permission to toggle $type!", 1);
    }
    DBQueryFatal("update projects set cvsrepo_public='$value' ".
		 "where pid='$pid'");
    SUEXEC($uid, $pid, "webcvsrepo_ctrl $pid", SUEXEC_ACTION_DIE);
}
else {
    USERERROR("Nobody has permission to toggle $type!", 1);
}
    
#
# Spit out a redirect 
#
if (isset($HTTP_REFERER) && $HTTP_REFERER != "" &&
    strpos($HTTP_REFERER,$_SERVER[SCRIPT_NAME])===false) {
    # Make sure the referer isn't me!
    header("Location: $HTTP_REFERER");
}
else {
    if (isset($target_uid)) {
	header("Location: $TBBASE/showuser.php3?target_uid=$target_uid");
    } elseif (isset($pid) && isset($eid)) {
	header("Location: $TBBASE/showexp.php3?pid=$pid&eid=$eid");
    } else {
	header("Location: $TBBASE/showuser.php3");
    }
}

?>
