<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Begin an Experiment");

$mydebug = 1;

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
# Only known and logged in users can begin experiments.
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
# At this point enter the exp_id into the database so that it shows up as
# valid when the tb scripts run. We need to remove the entry if any of
# this fails!
#
$query_result = mysql_db_query($TBDBNAME,
	"INSERT INTO experiments ".
        "(eid, pid, expt_created, expt_expires, expt_name, ".
        "expt_head_uid, expt_start, expt_end) ".
        "VALUES ('$exp_id', '$exp_pid', '$exp_created', '$exp_expires', ".
        "'$exp_name', '$uid', '$exp_start', '$exp_end')");
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
          The ID for your experiment in project $exp_pid is $exp_id.<br><br>
          Since you did not provide an NS script, no nodes have been
          allocated. You must log in and run the tbsetup scripts
          yourself. For your convenience, we have created a directory
          hierarchy on the control node: $dirname
          </h2>\n";

    if (1) {
	mail($TBMAIL_WWW, "TESTBED: New Experiment Created",
	     "User:        $uid\n".
             "EID:         $exp_id\n".
             "PID:         $exp_pid\n".
             "Name:        $exp_name\n".
             "Created:     $exp_created\n".
             "Expires:     $exp_expires\n".
             "Start:       $exp_start\n".
             "End:         $exp_end\n",
             "From: $TBMAIL_WWW\n".
             "Errors-To: $TBMAIL_WWW");
    }
    echo "</body>
          </html>\n";
    die("");
}

#
# We run a wrapper script that does all the work of creating the directory
# and running the tb scripts. If it fails, we get back all the output and
# give that to the user. If it succeeds, we go and find the report file
# and give that to the user.
#
#   tbdoit <pid> <eid> <temp_nsfile>
#
# Later, when the experiment is ended, the directory will be deleted.
#
# There is similar path stuff in endexp.php3.  Be sure to sync that up
# if you change things here.
#
echo "<center><br>";
echo "<h3>Setting up experiment. This may take a few minutes ...
          </center><br><br>
	  Please do <em>not</em> click the 'Stop' button. This will cause
	  the experiment creation to terminate prematurely, which can cause
  	  problems for future (other) experiments.
      </h3>";

flush();

#
# Run the scripts. We use a script wrapper to deal with changing
# to the proper directory and to keep some of these details out
# of this. We just want to know if the experiment setup worked.
# The wrapper is going to go the extra step of running tbreport
# so that we can give the user warm fuzzies.
#
$output = array();
$retval = 0;
$result = exec("$TBSUEXEC_PATH $uid $gid ".
               "tbdoit $exp_pid $exp_id $exp_nsfile",
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

#
# Debugging!
# 
if ($mydebug) {
    echo "<XMP>\n";
    for ($i = 0; $i < count($output); $i++) {
        echo "$output[$i]\n";
    }
    echo "</XMP>\n";
}

echo "<center><br>";
echo "<h2>Experiment Configured!<br>";
echo "The ID for your experiment in project $exp_pid is $exp_id<br>";
echo "Here is a summary of the nodes that were allocated<br>";
echo "</h2></center><br>";

#
# The tbdoit scripts dumps report output to stdout. Look for it in the
# output and send that out to the user. We cannot open the actual file
# because the project directory is not readable by world.
#
for ($i = 0; $i < count($output); $i++) {
    if (strcmp($output[$i], "Dumping $exp_id.report") == 0)
        break;
}
echo "<XMP>\n";
for ($i = $i + 1; $i < count($output); $i++) {
    echo "$output[$i]\n";
}
echo "</XMP>\n";

#
# Lets generate a mail message for now so that we can see whats happening.
#
if (1) {
mail($TBMAIL_WWW, "TESTBED: New Experiment Created",
     "User:        $uid\n".
     "EID:         $exp_id\n".
     "PID:         $exp_pid\n".
     "Name:        $exp_name\n".
     "Created:     $exp_created\n".
     "Expires:     $exp_expires\n".
     "Start:       $exp_start\n".
     "End:         $exp_end\n".
     "Directory:   $dirname\n".
     "Summary:\n\n".
     "$summary\n",
     "From: $TBMAIL_WWW\n".
     "Errors-To: $TBMAIL_WWW");
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
