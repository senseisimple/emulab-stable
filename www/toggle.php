<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
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
LOGGEDINORDIE($uid);

# List of valid toggles
$toggles = array("adminoff");

# list of valid values for each toggle
$values  = array("adminoff"  => array(0,1));

# list of valid extra variables for the each toggle, and mandatory flag.
$optargs = array("adminoff"  => array("target_uid" => 0));

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
if ($type == "adminoff") {
    # must be admin
    # Do not check if they are admin mode (ISADMIN), check if they
    # have the power to change to admin mode!
    if (! ($CHECKLOGIN_STATUS & CHECKLOGIN_ISADMIN) ) {
	USERERROR("You do not have permission to toggle $type!", 1);
    }
    # Admins can change status for other users.
    if (!isset($target_uid))
	$target_uid = $uid;
    elseif (!TBCurrentUser($target_uid)) {
	    PAGEARGERROR("Target user '$target_uid' is not a valid user!");
    }
    DBQueryFatal("update users set adminoff='$value' where uid='$target_uid'");
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
