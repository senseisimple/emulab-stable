<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
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
$this_user = CheckLoginOrDie(CHECKLOGIN_USERSTATUS|CHECKLOGIN_WEBONLY);
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

# List of valid toggles
$toggles = array("adminon", "webfreeze", "cvsweb", "lockdown", "stud",
		 "cvsrepo_public", "workbench");

# list of valid values for each toggle
$values  = array("adminon"        => array(0,1),
		 "webfreeze"      => array(0,1),
		 "cvsweb"         => array(0,1),
		 "stud"           => array(0,1),
		 "lockdown"       => array(0,1),
		 "cvsrepo_public" => array(0,1),
		 "workbench"      => array(0,1));

# list of valid extra variables for the each toggle, and mandatory flag.
$optargs = array("adminon"        => array(),
		 "webfreeze"      => array("user" => 1),
		 "cvsweb"         => array("user" => 1),
		 "stud"           => array("user" => 1),
		 "lockdown"       => array("pid" => 1, "eid" => 1),
		 "cvsrepo_public" => array("pid" => 1),
		 "workbench"      => array("pid" => 1));

# Mandatory page arguments.
$reqargs = RequiredPageArguments("type",  PAGEARG_STRING,
				 "value", PAGEARG_STRING);

# Where we zap to.
$zapurl = null;

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
    SETADMINMODE($value);
}
elseif ($type == "webfreeze") {
    # must be admin
    if (! $isadmin) {
	USERERROR("You do not have permission to toggle $type!", 1);
    }
    if (! ($target_user = User::Lookup($user))) {
	PAGEARGERROR("Target user '$user' is not a valid user!");
    }
    $zapurl = CreateURL("showuser", $target_user);
    $target_user->SetWebFreeze($value);
}
elseif ($type == "cvsweb") {
    # must be admin
    if (! $isadmin) {
	USERERROR("You do not have permission to toggle $type!", 1);
    }
    if (! ($target_user = User::Lookup($user))) {
	PAGEARGERROR("Target user '$user' is not a valid user!");
    }
    $zapurl = CreateURL("showuser", $target_user);
    $target_user->SetCVSWeb($value);
}
elseif ($type == "stud") {
    # must be admin
    if (! $isadmin) {
	USERERROR("You do not have permission to toggle $type!", 1);
    }
    if (! ($target_user = User::Lookup($user))) {
	PAGEARGERROR("Target user '$user' is not a valid user!");
    }
    $zapurl = CreateURL("showuser", $target_user);
    $target_user->SetStudly($value);
}
elseif ($type == "lockdown") {
    # must be admin
    if (! $isadmin) {
	USERERROR("You do not have permission to toggle $type!", 1);
    }
    if (! ($experiment = Experiment::LookupByPidEid($pid, $eid))) {
	PAGEARGERROR("Experiment $pid/$eid is not a valid experiment!");
    }
    $zapurl = CreateURL("showexp", $experiment);
    $experiment->SetLockDown($value);
}
elseif ($type == "cvsrepo_public") {
    # Must validate the pid since we allow non-admins to do this.
    if (! TBvalid_pid($pid)) {
	PAGEARGERROR("Invalid characters in $pid");
    }
    if (! ($project = Project::Lookup($pid))) {
	PAGEARGERROR("Project $pid is not a valid project!");
    }
    # Must be admin or project/group root.
    if (!$isadmin &&
	! TBMinTrust(TBGrpTrust($uid, $pid, $pid), $TBDB_TRUST_GROUPROOT)) {
	USERERROR("You do not have permission to toggle $type!", 1);
    }
    $zapurl = CreateURL("showproject", $project);
    $project->SetCVSRepoPublic($value);
    SUEXEC($uid, $pid, "webcvsrepo_ctrl $pid", SUEXEC_ACTION_DIE);
}
elseif ($type == "workbench") {
    # Must validate the pid since we allow non-admins to do this.
    if (! TBvalid_pid($pid)) {
	PAGEARGERROR("Invalid characters in $pid");
    }
    if (! ($project = Project::Lookup($pid))) {
	PAGEARGERROR("Project $pid is not a valid project!");
    }
    # Must be admin
    if (!$isadmin) {
	USERERROR("You do not have permission to toggle $type!", 1);
    }
    $zapurl = CreateURL("showproject", $project);
    $project->SetAllowWorkbench($value);
}
else {
    USERERROR("Nobody has permission to toggle $type!", 1);
}
    
#
# Spit out a redirect 
#
if (isset($HTTP_REFERER) && $HTTP_REFERER != "" &&
    strpos($HTTP_REFERER,$_SERVER["SCRIPT_NAME"])===false) {
    # Make sure the referer is not me!
    header("Location: $HTTP_REFERER");
}
elseif ($zapurl) {
    header("Location: $zapurl");
}
else {
    header("Location: $TBBASE/showuser.php3");
}

?>
