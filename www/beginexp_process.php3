<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Begin an Experiment");

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
if (!isset($exp_created) ||
    strcmp($exp_created, "") == 0) {
  FORMERROR("Experiment Created");
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
# I'm going to allow shell experiments to be created (No NS file).
# 
$nonsfile = 0;
if (!isset($exp_nsfile) ||
    strcmp($exp_nsfile, "") == 0 ||
    strcmp($exp_nsfile, "none") == 0) {

    #
    # Catch an invalid filename and quit now.
    #
    if (strcmp($exp_nsfile_name, "")) {
        USERERROR("The NS file '$exp_nsfile_name' does not appear to be a ".
                  "valid filename. Please go back and try again.", 1);
    }

    $nonsfile = 1;
}

#
# Make sure the PID/EID tuple does not already exist in the database.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT eid FROM experiments ".
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
# We need the user's name and email.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT usr_name,usr_email from users where uid=\"$uid\"");
if (($row = mysql_fetch_row($query_result)) == 0) {
    TBERROR("Database Error: Getting GID for project $exp_pid.", 1);
}
$user_name = $row[0];
$user_email = $row[1];

#
# Set the experiment ready bit to 1 if its a shell experiment.
#
if ($nonsfile) {
    $expt_ready = 1;
}
else {
    $expt_ready = 0;
}

#
# At this point enter the exp_id into the database so that it shows up as
# valid when the tb scripts run. We need to remove the entry if any of
# this fails!
#
$query_result = mysql_db_query($TBDBNAME,
	"INSERT INTO experiments ".
        "(eid, pid, expt_created, expt_expires, expt_name, ".
        "expt_head_uid, expt_start, expt_end, expt_ready) ".
        "VALUES ('$exp_id', '$exp_pid', '$exp_created', '$exp_expires', ".
        "'$exp_name', '$uid', '$exp_start', '$exp_end', '$expt_ready')");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error adding new experiment $exp_id: $err\n", 1);
}

#
# This is where the experiment hierarchy is going to be created.
#
$dirname = "$TBPROJ_DIR/$exp_pid/exp/$exp_id";

#
# If an experiment "shell" give some warm fuzzies and be done with it.
# The user is responsible for running the tb scripts on his/her own!
# The user will need to come back and terminate the experiment though
# to clear it out of the database.
#
if ($nonsfile) {
    $retval = SUEXEC($uid, $gid, "mkexpdir $exp_pid $exp_id", 0);

    echo "<center><br>
          <h2>Experiment Configured!
          </center><br><br>
          The ID for your experiment in project `$exp_pid' is `$exp_id.'
          <br><br>
          Since you did not provide an NS script, no nodes have been
          allocated. You must log in and run the tbsetup scripts
          yourself. For your convenience, we have created a directory
          hierarchy on the control node: $dirname
          </h2>\n";

    if (1) {
	mail("$user_name '$uid' <$user_email>", 
	     "TESTBED: '$exp_pid' '$exp_id' New Experiment Created",
	     "User:        $uid\n".
             "EID:         $exp_id\n".
             "PID:         $exp_pid\n".
             "Name:        $exp_name\n".
             "Created:     $exp_created\n".
             "Expires:     $exp_expires\n".
             "Start:       $exp_start\n".
             "End:         $exp_end\n",
             "From: $TBMAIL_WWW\n".
	     "Cc: $TBMAIL_WWW\n".
             "Errors-To: $TBMAIL_WWW");
    }
    echo "</body>
          </html>\n";
    die("");
}

echo "<center><br>";
echo "<h3>Starting experiment configuration. Please wait a moment ...
          </center><br><br>
      </h3>";

flush();

#
# XXX
# Set the permissions on the NS file so that the scripts can get to it.
# It is owned by nobody, and most likely protected. This leaves the
# script open for a short time. A potential security hazard we should
# deal with at some point.
#
chmod($exp_nsfile, 0666);

#
# Run the scripts. We use a script wrapper to deal with changing
# to the proper directory and to keep most of these details out
# of this. The user will be notified later. Its possible that the
# script will return non-zero status, but there are just a couple
# of conditions. Generally, the script exits and the user is told
# of errors later. 
#
$output = array();
$retval = 0;
$last   = time();

$result = exec("$TBSUEXEC_PATH $uid $gid webstartexp $exp_pid ".
	       "$exp_id $exp_nsfile",
 	       $output, $retval);

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
    
    $query_result = mysql_db_query($TBDBNAME,
	"DELETE FROM experiments WHERE eid='$exp_id' and pid=\"$exp_pid\"");

    die("");
}

echo "<center><br>";
echo "<h2>Experiment `$exp_id' in project `$exp_pid' is configuring!<br><br>
          You will be notified via email when the experiment has been fully<br>
	  configured and you are able to proceed.<br>";
echo "</h2>";
echo "</center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
