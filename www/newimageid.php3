<?php
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Create a new ImageID");

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
if (!isset($imageid) ||
    strcmp($imageid, "") == 0) {
  FORMERROR("ImageID");
}
if (!isset($pid) ||
    strcmp($pid, "") == 0) {
  FORMERROR("Select Project");
}
if (!isset($description) ||
    strcmp($description, "") == 0) {
  FORMERROR("ImageID Description");
}
if (!isset($loadpart) ||
    strcmp($loadpart, "") == 0) {
  FORMERROR("Starting DOS Slice");
}
if (!isset($loadlength) ||
    strcmp($loadlength, "") == 0) {
  FORMERROR("Number of DOS Slice");
}
if (!isset($path) ||
    strcmp($path, "") == 0) {
  FORMERROR("Path");
}
if (!isset($default_osid) ||
    strcmp($default_osid, "") == 0) {
  FORMERROR("Default OSID");
}
if (isset($loadaddr) &&
    strcmp($loadaddr, "") == 0) {
    unset($loadaddr);
}

#
# Check ID and others for sillyness.
#
if (! ereg("^[\.-_a-zA-Z0-9]+$", $imageid)) {
    USERERROR("The ImageID must consist of alphanumeric characters and ".
	      "dash, dot, or underscore!", 1);
}
if (! ereg("^[\.-_a-zA-Z0-9\/]+$", $path)) {
    USERERROR("The path contains invalid characters!", 1);
}

if (isset($loadaddr) &&
    !ereg("^[\.0-9]+$", $loadaddr)) {
	USERERROR("The load address contains invalid characters!", 1);
}

#
# Check sanity of the loadpart and loadlength.
#
if (! ereg("^[0-9]+$", $loadpart) ||
    $loadpart < 0 || $loadpart > 4) {
    USERERROR("The Load Partition must 0, or 1,2,3, or 4!", 1);
}
if (! ereg("^[0-9]+$", $loadlength) ||
    $loadlength < 1 || $loadlength > 4) {
    USERERROR("The Load Length must be 1,2,3, or 4!", 1);
}
if (($loadpart != 0 && $loadlength != 1)) {
    USERERROR("Currently, only single slices or partial disks are allowed. ".
	      "If you specify a non-zero starting load partition, the load ".
	      "load length must be one (a single slice). If you specify zero ".
	      "for the starting load partition, then you can include any or ".
	      "all of the slices (1-4). Note that '0' means to start at the ".
	      "boot sector, while '1' means to start at the beginning of the ".
	      "first slice (typically starting at sector 63).", 1);
}

#
# Check sanity of the OSIDs for each slice. Permission checks not needed.
#
#
# Silently kill off all the extraneous OSIDs.
#
for ($i = 1; $i <= 4; $i++) {
    $foo      = "part${i}_osid";
    $thisosid = $$foo;

    if (($loadpart && $i == $loadpart) ||
	(!$loadpart && $i <= $loadlength)) {

	if (!isset($thisosid) ||
	    strcmp($thisosid, "none") == 0) {
	    USERERROR("You must select an OSID for Slice $i!", 1);
	}
	    
	if (!TBValidOSID($thisosid)) {
	    USERERROR("The OSID '$this_osid' in not a valid OSID!", 1);
	}
	$$foo = "'$thisosid'";
    }
    else {
	$$foo = "NULL";
    }
}

if (!isset($default_osid) ||
    strcmp($default_osid, "none") == 0) {
    USERERROR("You must select a default boot OSID!", 1);
}
if (!TBValidOSID($default_osid)) {
    USERERROR("The Default OSID '$default_osid' in not a valid OSID!", 1);
}

#
# Database limits
#
if (strlen($osid) > $TBDB_IMAGEID_IMAGEIDLEN) {
    USERERROR("The ImageID name is too long! Please select another.", 1);
}

#
# Of course, the ImageID record may not already exist in the DB.
#
if (TBValidImageID($imageid)) {
    USERERROR("The ImageID '$imageid' already exists! ".
              "Please select another.", 1);
}

#
# Certain of these values must be escaped or otherwise sanitized.
# 
$description = addslashes($description);

#
# Only admin types can set the PID to none (for sharing).
#
if (!isset($pid) ||
    strcmp($pid, "none") == 0) {
    if (!$isadmin) {
	USERERROR("Only Emulab Administrators can specify 'None' for the ".
                  "project.", 1);
    }
    unset($pid);
}
else {
    #
    # Verify permission.
    #
    if (!TBProjAccessCheck($uid, $pid, 0, $TB_PROJECT_MAKEIMAGEID)) {
	USERERROR("You do not have permission to create ImageID $imageid!", 1);
    }
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
if (isset($loadaddr)) {
    $loadaddr = "'$loadaddr'";
}
else {
    $loadaddr = "NULL";
}

$query_result =
    DBQueryFatal("INSERT INTO images ".
		 "(imageid, description, loadpart, loadlength, ".
		 " part1_osid, part2_osid, part3_osid, part4_osid, ".
		 " default_osid, path, pid, load_address) ".
		 "VALUES ".
		 "  ('$imageid', '$description', $loadpart, $loadlength, ".
		 "   $part1_osid, $part2_osid, $part3_osid, $part4_osid, ".
		 "   '$default_osid', '$path', $pid, $loadaddr)");

#
# Dump os_info record.
# 
SHOWIMAGEID($imageid, 0);

# Edit option.
$fooid = rawurlencode($imageid);
echo "<p><center>
       Do you want to edit this ImageID?
       <A href='editimageid_form.php3?imageid=$fooid'>Yes</a>
      </center>\n";

echo "<br><br>\n";

# Delete option.
echo "<p><center>
       Do you want to delete this ImageID?
       <A href='deleteimageid.php3?imageid=$fooid'>Yes</a>
      </center>\n";    

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
