<?php
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Edit ImageID Information");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

$isadmin = ISADMIN($uid);

#
# Verify form arguments.
# 
if (!isset($imageid) ||
    strcmp($imageid, "") == 0) {
    USERERROR("You must provide an ImageID.", 1);
}

#
# Check to make sure thats this is a valid Image.
#
$query_result = mysql_db_query($TBDBNAME,
       "SELECT pid FROM images WHERE imageid='$imageid'");
if (mysql_num_rows($query_result) == 0) {
    USERERROR("The ImageID `$imageid' is not a valid ImageID.", 1);
}
$row = mysql_fetch_array($query_result);
$pid = $row[pid];

#
# Verify that this uid is a member of the project that owns the IMAGEID.
#
if (!$isadmin && $pid) {
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT pid FROM proj_memb WHERE uid=\"$uid\" and pid=\"$pid\"");
    if (mysql_num_rows($query_result) == 0) {
        USERERROR("You are not a member of the project that owns ".
		  "ImageID $imageid.", 1);
    }
}

#
# Sanitize values and create string pieces.
#
if (isset($description) && strcmp($description, "")) {
    $foo = addslashes($description);
    
    $description = "'$foo'";
}
else {
    $description = "NULL";
}

if (isset($magic) && strcmp($magic, "")) {
    $foo = addslashes($magic);
    
    $magic = "'$foo'";
}
else {
    $magic = "NULL";
}

if (isset($path) && strcmp($path, "")) {
    $foo = addslashes($path);

    if (strcmp($path, $foo)) {
	USERERROR("The path must not contain special characters!", 1);
    }
    $path = "'$path'";
}
else {
    $path = "NULL";
}
if (isset($loadaddr) && strcmp($loadaddr, "")) {
    $foo = addslashes($loadaddr);

    if (strcmp($loadaddr, $foo)) {
	USERERROR("The load address must not contain special characters!", 1);
    }
    $loadaddr = "'$loadaddr'";
}
else {
    $loadaddr = "NULL";
}

#
# Create an update string
#
$query_string =
	"UPDATE images SET             ".
	"description=$description,     ".
	"path=$path,                   ".
	"magic=$magic,                 ".
        "load_address=$loadaddr        ";

$query_string = "$query_string WHERE imageid='$imageid'";

$insert_result = mysql_db_query($TBDBNAME, $query_string);
if (! $insert_result) {
    $err = mysql_error();
    TBERROR("Database Error changing imageid info for $inageid: $err", 1);
}

SHOWIMAGEID($imageid, 0);

#
# Edit option
#
$fooid = rawurlencode($imageid);
echo "<p><center>
       Do you want to edit this ImageID?
       <A href='editimageid_form.php3?imageid=$fooid'>Yes</a>
      </center>\n";    

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
