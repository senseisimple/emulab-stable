<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Create a new OS Descriptor");

#
# Only known and logged in users can create an OSID.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

#
# First off, sanity check the form to make sure all the required fields
# were provided. I do this on a per field basis so that we can be
# informative. Be sure to correlate these checks with any changes made to
# the project form. 
#
if (!isset($osname) ||
    strcmp($osname, "") == 0) {
  FORMERROR("Descriptor Name");
}
if (!isset($pid) ||
    strcmp($pid, "") == 0) {
  FORMERROR("Select Project");
}
if (!isset($description) ||
    strcmp($description, "") == 0) {
  FORMERROR("Description");
}
if (!isset($OS) ||
    strcmp($OS, "") == 0 ||
    (strcmp($OS, "Linux") &&
     strcmp($OS, "FreeBSD") &&
     strcmp($OS, "NetBSD") &&
     strcmp($OS, "OSKit") &&
     strcmp($OS, "Unknown"))) {
    FORMERROR("Operating System (OS)");
}

if (isset($os_path) &&
    strcmp($os_path, "") == 0) {
    unset($os_path);
}
if (isset($os_version) &&
    strcmp($os_version, "") == 0) {
    unset($os_version);
}

#
# Check osname for sillyness.
#
if (! ereg("^[-_a-zA-Z0-9\.]+$", $osname)) {
    USERERROR("The Descriptor name must consist of alphanumeric characters ".
	      "and dash, dot, or underscore!", 1);
}

if (isset($os_path)) {
    if (strcmp($os_path, "") == 0) {
	unset($os_path);
    }
    else {
	if (! ereg("^[-_a-zA-Z0-9\/\.:]+$", $os_path)) {
	    USERERROR("The path contains invalid characters!", 1);
	}
    }
}

if (! isset($os_version)) {
    if (! $isadmin) {
	USERERROR("You must supply a version string!", 1);
    }
    $os_version = "";
}
else if (strcmp($os_version, "") &&
	! ereg("^[-_a-zA-Z0-9\.]+$", $os_version)) {
	USERERROR("The version string contains invalid characters!", 1);
}

#
# Database limits
#
if (strlen($osname) > $TBDB_OSID_OSNAMELEN) {
    USERERROR("The Descriptor name is too long! Please select another.", 1);
}

#
# Certain of these values must be escaped or otherwise sanitized.
# 
$description = addslashes($description);
if (isset($os_magic)) {
    $os_magic = addslashes($os_magic);
}
else {
    $os_magic = "";
}

#
# Verify permission.
#
if (!TBProjAccessCheck($uid, $pid, 0, $TB_PROJECT_MAKEOSID)) {
    USERERROR("You do not have permission to create OS Descriptors ".
	      "in Project $pid!", 1);
}

#
# Only admin types can set the shared bit,
#
if (isset($os_shared) &&
    strcmp($os_shared, "Yep") == 0) {
    if (!$isadmin) {
	USERERROR("Only Emulab Administrators can set the shared flag!", 1);
    }
    $os_shared = 1;
}
else {
    $os_shared = 0;
}

#
# Only admin types can muck with the mustclean bit.
#
if (isset($os_clean) &&
    strcmp($os_clean, "Yep") == 0) {
    if (!$isadmin) {
	USERERROR("Only Emulab Administrators can set the clean flag!", 1);
    }
    $os_mustclean = 0;
}
else {
    $os_mustclean = 1;
}

#
# Form the os features set.
#
$os_features_array = array();
if (isset($os_feature_ping)) {
    $os_features_array[] = "ping";
}
if (isset($os_feature_ssh)) {
    $os_features_array[] = "ssh";
}
if (isset($os_feature_ipod)) {
    $os_features_array[] = "ipod";
}
if (isset($os_feature_ipod)) {
    $os_features_array[] = "isup";
}
$os_features = join(",", $os_features_array);

# Check op_mode
if (!isset($op_mode) ||
    strcmp($op_mode, "") == 0 ||
    (strcmp($op_mode, "MINIMAL") &&
     strcmp($op_mode, "NORMAL") &&
     strcmp($op_mode, "NORMALv1") &&
     strcmp($op_mode, "Unknown"))) {
    FORMERROR("Operational Mode (op_mode)");
}

#
# And insert the record!
#
if (isset($os_path)) {
    $os_path = "'$os_path'";
}
else {
    $os_path = "NULL";
}

DBQueryFatal("lock tables os_info write");

#
# Of course, the OS record may not already exist in the DB.
#
if (TBValidOS($pid, $osname)) {
    DBQueryFatal("unlock tables");
    USERERROR("The OS Descriptor '$osname' already exists in Project $pid! ".
              "Please select another.", 1);
}

#
# Just concat them to form a unique imageid. 
# 
$osid = "$pid-$osname";
if (TBValidOSID($osid)) {
    DBQueryFatal("unlock tables");
    TBERROR("Could not form a unique osid for $pid/$osname!", 1);
}

$query_result =
    DBQueryFatal("INSERT INTO os_info ".
		 "(osname, osid, description,OS,version,path,magic,op_mode, ".
		 " osfeatures, pid, shared, creator, mustclean, created) ".
		 "VALUES ('$osname', '$osid', '$description', '$OS', ".
		 "        '$os_version', $os_path, '$os_magic', '$op_mode', ".
		 "        '$os_features', '$pid', $os_shared, ".
	         "        '$uid', $os_mustclean, now())");

DBQueryFatal("unlock tables");

SUBPAGESTART();
SUBMENUSTART("More Options");
WRITESUBMENUBUTTON("Delete this OS Descriptor",
		   "deleteosid.php3?osid=$osid");
WRITESUBMENUBUTTON("Create a new OS Descriptor",
		   "newosid_form.php3");
WRITESUBMENUBUTTON("Create a new Image Descriptor",
		   "newimageid_explain.php3");
WRITESUBMENUBUTTON("OS Descriptor list",
		   "showosid_list.php3");
WRITESUBMENUBUTTON("Image Descriptor list",
		   "showimageid_list.php3");
SUBMENUEND();

#
# Dump os_info record.
# 
SHOWOSINFO($osid);

SUBPAGEEND();

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
