<?php
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Create a new OSID");

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
if (!isset($osid) ||
    strcmp($osid, "") == 0) {
  FORMERROR("OSID");
}
if (!isset($pid) ||
    strcmp($pid, "") == 0) {
  FORMERROR("Select Project");
}
if (!isset($description) ||
    strcmp($description, "") == 0) {
  FORMERROR("OSID Description");
}
if (!isset($OS) ||
    strcmp($OS, "") == 0) {
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
# Check osid for sillyness.
#
if (! ereg("^[-_a-zA-Z0-9]+$", $osid)) {
    USERERROR("The OSID must be alphanumeric characters only!", 1);
}

if (isset($os_path)) {
    if (strcmp($os_version, "") == 0) {
	unset($os_path);
    }
    else {
	if (! ereg("^[\.-_a-zA-Z0-9\/]+$", $os_path)) {
	    USERERROR("The path contains invalid characters!", 1);
	}
    }
}

if (isset($os_version)) {
    if (strcmp($os_version, "") &&
	! ereg("^[\.-_a-zA-Z0-9]+$", $os_version)) {
	USERERROR("The version string contains invalid characters!", 1);
    }
}
else {
    $os_version = "";
}

#
# Database limits
#
if (strlen($osid) > $TBDB_OSID_OSIDLEN) {
    USERERROR("The OSID \"$osid\" is too long! Please select another.", 1);
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
# Only admin types can set the PID to none (for sharing).
#
if (strcmp($pid, "none") == 0) {
    if (!$isadmin) {
	USERERROR("Only Emulab Administrators can specify `none' for the ".
                  "project.", 1);
    }
    unset($pid);
}

#
# Check that uid has permission to create an OSID in the project specified.
#
if (!$isadmin) {
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT pid FROM proj_memb WHERE uid=\"$uid\" and pid=\"$pid\" ".
	"and (trust='local_root' or trust='group_root')");
    
    if (! $query_result) {
	$err = mysql_error();
	TBERROR("Database Error finding project membership: $uid: $err\n", 1);
    }
    if (mysql_num_rows($query_result) == 0) {
	USERERROR("You do have permission to create an OSID in ".
		  "project: `$pid'.", 1);
    }
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
$os_features = join(",", $os_features_array);

#
# Of course, the OSID record may not already exist in the DB.
# 
$query_result = mysql_db_query($TBDBNAME,
       "SELECT osid FROM os_info WHERE osid=\"$osid\"");
    
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error finding os_info record: $osid: $err\n", 1);
}
if (mysql_num_rows($query_result) != 0) {
    USERERROR("The OSID `$osid' already exists. Please select another", 1);
}

#
# And insert the record!
#
if (isset($pid)) {
    $pid = "'$pid'";
}
else {
    $pid = "NULL";
}
if (isset($os_path)) {
    $os_path = "'$os_path'";
}
else {
    $os_path = "NULL";
}

$query_result = mysql_db_query($TBDBNAME,
	"INSERT INTO os_info ".
	"(osid, description, OS, version, path, magic, machinetype, ".
	" osfeatures, pid) ".
	"VALUES ('$osid', '$description', '$OS', '$os_version', $os_path, ".
        "        '$os_magic', '$os_machinetype', '$os_features', $pid)");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error adding new OSID $osid: $err\n", 1);
}

#
# Dump os_info record.
# 
SHOWOSINFO($osid);

# Terminate option.
echo "<p><center>
       Do you want to remove this OSID?
       <A href='deleteosid.php3?osid=$osid'>Yes</a>
      </center>\n";    

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
