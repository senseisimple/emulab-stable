<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Create a Batch Mode Experiment");

$mydebug = 0;

#
# First off, sanity check the form to make sure all the required fields
# were provided. I do this on a per field basis so that we can be
# informative. Be sure to correlate these checks with any changes made to
# the project form. 
#
if (!isset($uid) ||
    strcmp($uid, "") == 0) {
  FORMERROR("Username");
}
if (!isset($exp_pid) ||
    strcmp($exp_pid, "") == 0) {
  FORMERROR("Select Project");
}
if (!isset($exp_id) ||
    strcmp($exp_id, "") == 0) {
  FORMERROR("Experiment Name (short)");
}
if (!isset($exp_name) ||
    strcmp($exp_name, "") == 0) {
  FORMERROR("Experiment Name (long)");
}

#
# Only known and logged in users can begin experiments. Name came in as
# a POST var.
#
LOGGEDINORDIE($uid);

#
# Database limits
#
if (strlen($exp_id) > $TBDB_EIDLEN) {
    USERERROR("The experiment name \"$exp_id\" is too long! ".
              "Please select another.", 1);
}

#
# Certain of these values must be escaped or otherwise sanitized.
# 
$exp_name = addslashes($exp_name);

#
# Must provide an NS file!
# 
$nonsfile = 0;
if (!isset($exp_nsfile) ||
    strcmp($exp_nsfile, "") == 0 ||
    strcmp($exp_nsfile, "none") == 0) {

    USERERROR("The NS file '$exp_nsfile_name' does not appear to be a ".
	      "valid filename. Please go back and try again.", 1);
}

#
# Make sure the PID/EID tuple does not already exist in the database.
# It may not exist in either the current experiments list, or the
# batch experiments list.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT eid FROM experiments ".
        "WHERE eid=\"$exp_id\" and pid=\"$exp_pid\"");
if ($row = mysql_fetch_row($query_result)) {
    USERERROR("The experiment name \"$exp_id\" you have chosen is already ".
              "in use in project $exp_pid. Please select another.", 1);
}

$query_result = mysql_db_query($TBDBNAME,
	"SELECT eid FROM batch_experiments ".
        "WHERE eid=\"$exp_id\" and pid=\"$exp_pid\"");
if ($row = mysql_fetch_row($query_result)) {
    USERERROR("The experiment name \"$exp_id\" you have chosen is already ".
              "in use in project $exp_pid. Please select another.", 1);
}

#
# Next, is this person a member of the project specified, and is the trust
# equal to group or local root?
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT * FROM proj_memb WHERE pid=\"$exp_pid\" and uid=\"$uid\"");
if (($row = mysql_fetch_array($query_result)) == 0) {
    USERERROR("You are not a member of Project $exp_pid, so you cannot begin ".
            "an experiment in that project.", 1);
}
$trust = $row[trust];
if (strcmp($trust, "group_root") && strcmp($trust, "local_root")) {
    USERERROR("You are not group or local root in Project $exp_pid, so you ".
              "cannot begin an experiment in that project.", 1);
}

#
# We need the unix gid for the project for running the scripts below.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT unix_gid from projects where pid=\"$exp_pid\"");
if (($row = mysql_fetch_row($query_result)) == 0) {
    TBERROR("Database Error: Getting GID for project $exp_pid.", 1);
}
$gid = $row[0];

#
# Create a temporary file with the goo in it.
#
$tmpfname = tempnam( "/tmp", "batch-$pid-$eid" );
$fp = fopen($tmpfname, "w");
if (! $fp) {
    TBERROR("Opening temporary file $tmpfname.", 1);
}

#
# XXX The batchexp script parses this file, so if you change something
#     here, go change it there too!
# 
fputs($fp, "EID:	$exp_id\n");
fputs($fp, "PID:	$exp_pid\n");
fputs($fp, "name:       $exp_name\n");
fputs($fp, "expires:    $exp_expires\n");
fputs($fp, "nsfile:	$exp_nsfile\n");
fclose($fp);

#
# XXX
# Set the permissions on the files so that the scripts can get to them.
# It is owned by nobody, and most likely protected. This leaves the
# script open for a short time. A potential security hazard we should
# deal with at some point, but since the files are on paper:/tmp, its
# a minor problem. 
#
chmod($tmpfname, 0666);
chmod($exp_nsfile, 0666);

echo "<center><br>";
echo "<h3>Starting batch experiment setup. Please wait a moment ...
          </center><br><br>
      </h3>";

flush();

#
# Run the scripts. We use a script wrapper to deal with changing
# to the proper directory and to keep most of these details out
# of this. 
#
$output = array();
$retval = 0;
$last   = time();

$result = exec("$TBSUEXEC_PATH $uid $gid webbatchexp $tmpfname",
 	       $output, $retval);

#
# Kill the tempfile since the script will have copied it by now.
#
unlink($tmpfname);

if ($retval) {
    echo "<br><br><h2>
          Setup Failure($retval): Output as follows:
          </h2>
          <br>
          <XMP>\n";
          for ($i = 0; $i < count($output); $i++) {
              echo "$output[$i]\n";
          }
    echo "</XMP>\n";
    
    die("");
}

echo "<center><br>";
echo "<h2>Experiment `$exp_id' in project `$exp_pid' has been batched!<br><br>
          You will be notified via email when the experiment has been run<br>";
echo "</h2>";
echo "</center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
