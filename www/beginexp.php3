<?php
include("defs.php3");

#
# No PAGEHEADER since we spit out a Location header later. See below.
#

#
# Only known and logged in users can begin experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Default priorities Needs to move someplace else!
# 
$priorities         = array();
$priorities["high"] = "high";
$priorities["low"]  = "low";

#
# Helper function to respit on error.
#
function RESPIT($tag, $value)
{
    global $formfields;
    
    $errors[$tag] = $value;
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    die("");
}

#
# Spit the form out using the array of data. 
# 
function SPITFORM($formfields, $errors)
{
    global $TBDB_PIDLEN, $TBDB_GIDLEN, $TBDB_EIDLEN;
    global $nsdata, $projlist, $priorities, $exp_nsfile;
    
    PAGEHEADER("Begin a Testbed Experiment");

    if ($errors) {
	echo "<table align=center border=0 cellpadding=0 cellspacing=2>
              <tr>
                 <td nowrap align=center colspan=3>
                   <font size=+1 color=red>
                      Oops, please fix the following errors!
                   </font>
                 </td>
              </tr>\n";

	while (list ($name, $message) = each ($errors)) {
	    echo "<tr>
                     <td align=right><font color=red>$name:</font></td>
                     <td>&nbsp &nbsp</td>
                     <td align=left><font color=red>$message</font></td>
                  </tr>\n";
	}
	echo "</table><br>\n";
    }

    echo "<table align=center border=1> 
          <tr>
              <td align=center colspan=3>
                 <em>(Fields marked with * are required)</em>
              </td>
          </tr>\n";

    echo "<form enctype=multipart/form-data
                action=beginexp.php3 method=post>\n";

    #
    # Select Project
    #
    echo "<tr>
              <td colspan=2>*Select Project:</td>
              <td><select name=\"formfields[exp_pid]\">
                      <option value=''>Please Select &nbsp</option>\n";
    
    while (list($project) = each($projlist)) {
	$selected = "";

	if (strcmp($formfields[exp_pid], $project) == 0)
	    $selected = "selected";
	
	echo "        <option $selected value=\"$project\">
                             $project </option>\n";
    }
    echo "       </select>";
    echo "    </td>
          </tr>\n";

    #
    # Name:
    #
    echo "<tr>
              <td colspan=2>*Name (no blanks):</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[exp_id]\"
                         value=\"" . $formfields[exp_id] . "\"
	                 size=$TBDB_EIDLEN
                         maxlength=$TBDB_EIDLEN>
              </td>
          </tr>\n";

    #
    # Description
    #
    echo "<tr>
              <td colspan=2>*Description:<br>
                  (a short pithy sentence)</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[exp_description]\"
                         value=\"" . $formfields[exp_description] . "\"
	                 size=50>
              </td>
          </tr>\n";

    #
    # NS file
    #
    if (isset($nsdata)) {
        echo "<tr>
                  <td colspan=2>*Your Auto Generated NS file: &nbsp</td>
                      <input type=hidden name=nsdata value=$nsdata>
                  <td><a target=_blank href=spitnsdata.php3?nsdata=$nsdata>
                      View NS File</a></td>
              </tr>\n";
    }
    else {
	echo "<tr>
                  <td rowspan>*Your NS file: &nbsp</td>

                  <td rowspan><center>Upload (20K max)[<b>1</b>]<br>
                                   <br>
                                   Or<br>
                                   <br>
                               On Server (/proj or /users)
                              </center></td>

                  <td rowspan>
                      <input type=hidden name=MAX_FILE_SIZE value=20000>
	              <input type=file
                             name=exp_nsfile
                             value=\"" . $exp_nsfile . "\"
	                     size=40>
                      <br>
                      <br>
	              <input type=text
                             name=\"formfields[exp_localnsfile]\"
                             value=\"" . $formfields[exp_localnsfile] . "\"
	                     size=50>
                  </td>
              </tr>\n";	
    }

    #
    # Swappable?
    # 
    echo "<tr>
  	      <td colspan=2>Swappable?[<b>2</b>]:</td>
              <td class=left>
                  <input type=checkbox
                         name=\"formfields[exp_swappable]\"
                         value=Yep";

    if (isset($formfields[exp_swappable]) &&
	strcmp($formfields[exp_swappable], "Yep") == 0)
	echo "           checked";
	
    echo "               > Yes
              </td>
          </tr>\n";

    #
    # Priority
    #
    echo "<tr>
	      <td colspan=2>Priority[<b>3</b>]:</td>
              <td class=left>\n";

    reset($priorities);
    while (list ($prio, $value) = each($priorities)) {
	$checked = "";
	
	if (isset($formfields["exp_priority"]) &&
	    ! strcmp($formfields["exp_priority"], $prio))
	    $checked = "checked";

	echo "<input $checked type=radio value=$prio
                     name=\"formfields[exp_priority]\">
                     $prio &nbsp\n";
    }
    echo "    </td>
          </tr>\n";

    #
    # Select a group
    # 
    echo "<tr>
              <td colspan=2>Group[<b>4</b>]:</td>
              <td><select name=\"formfields[exp_gid]\">
                    <option value=''>Default Group </option>\n";

    reset($projlist);
    while (list($project, $grouplist) = each($projlist)) {
	for ($i = 0; $i < count($grouplist); $i++) {
	    $group    = $grouplist[$i];

	    if (strcmp($project, $group)) {
		$selected = "";

		if (isset($formfields[exp_gid]) &&
		    isset($formfields[exp_pid]) &&
		    strcmp($formfields[exp_pid], $project) == 0 &&
		    strcmp($formfields[exp_gid], $group) == 0)
		    $selected = "selected";
		
		echo "<option $selected value=\"$group\">
                           $project/$group</option>\n";
	    }
	}
    }
    echo "     </select>
             </td>
          </tr>\n";

    #
    # Batch Experiment?
    #
    echo "<tr>
  	      <td colspan=2>Batch Experiment?[<b>5</b>]:</td>
              <td class=left>
                  <input type=checkbox
                         name=\"formfields[exp_batched]\"
                         value=Yep";

	if (isset($formfields[exp_batched]) &&
	    strcmp($formfields[exp_batched], "Yep") == 0)
	    echo "           checked";
	
	echo "                       > Yes
                  </td>
              </tr>\n";

    echo "<tr>
              <td align=center colspan=3>
                 <b><input type=submit value=Submit name=submit></b></td>
         </tr>
        </form>
        </table>\n";

    echo "<blockquote><blockquote><blockquote>
          <ol>
             <li>Note to <a href=http://www.opera.com><b>Opera 5</b></a> users:
                 The file upload mechanism is broken in Opera, so you cannot
                 specify a local file for upload. Instead, please specify a
                 file that is resident on the server. 
             <li>Check if your experiment can be swapped out and swapped back 
	         in without harm to your experiment. Useful for scheduling when
	         resources are tight. More information on swapping
	         is contained in the 
	         <a href='$TBDOCBASE/faq.php3#UTT-Swapping'>Emulab FAQ</a>.
             <li>You get brownie points for marking your experiments as Low
                 Priority, which indicates that we can swap you out before high
	         priority experiments.
             <li>Leave as the default group, or pick a subgroup that
                 corresponds to the project you selected.
             <li>Check this if you want to create a
                 <a href='$TBDOCBASE/tutorial/tutorial.php3#BatchMode'>
                 batch mode</a> experiment.
         </ol>
         </blockquote></blockquote></blockquote>\n";

    echo "<p><blockquote>
          <ul>
	     <li> Looking for a
                  <a href='$TBDOCBASE/faq.php3#UTT-NETBUILD'>GUI</a>
                  to help you create your topology?
                  <a href='buildui/bui.php3'>Check it out!</a>
             <li> Please <a href='nscheck_form.php3'>syntax check</a> your NS
                  file first!
             <li> You can view a <a href='showosid_list.php3'>list of OSIDs</a>
                  that are available for you to use in your NS file.
             <li> Create your own <a href='newimageid_explain.php3'>
                  custom disk images</a>.
          </ul>
          </blockquote>\n";
}

#
# See if nsdata was provided. Clear it if an empty string, otherwise
# reencode it *only* if not from the form. It appears that php decodes
# it for you when it comes in as GET argument, but leaves it encoded
# when its a POST argument.
#
if (isset($nsdata)) {
    if (strcmp($nsdata, "") == 0) 
	unset($nsdata);
    elseif (! isset($submit))
	$nsdata = rawurlencode($nsdata);
}

#
# See what projects the uid can create experiments in. Must be at least one.
#
$projlist = TBProjList($uid, $TB_PROJECT_CREATEEXPT);

if (! count($projlist)) {
    USERERROR("You do not appear to be a member of any Projects in which ".
	      "you have permission to create new experiments.", 1);
}

#
# On first load, display a virgin form and exit.
#
if (! isset($submit)) {
    $defaults = array();

    $defaults[exp_swappable] = "Yep";
    $defaults[exp_priority]  = "low";
    SPITFORM($defaults, 0);
    PAGEFOOTER();
    return;
}

#
# Otherwise, must validate and redisplay if errors
#
$errors = array();

#
# Project:
# 
if (!isset($formfields[exp_pid]) ||
    strcmp($formfields[exp_pid], "") == 0) {
    $errors["Project"] = "Not Selected";
}
elseif (!TBValidProject($formfields[exp_pid])) {
    $errors["Project"] = "No such project";
}

#
# EID.
# 
if (!isset($formfields[exp_id]) ||
    strcmp($formfields[exp_id], "") == 0) {
    $errors["Experiment Name"] = "Missing Field";
}
else {
    if (! ereg("^[-a-zA-Z0-9]+$", $formfields[exp_id])) {
	$errors["Experiment Name"] =
	    "Must be alphanumeric characters only (dash okay)!";
    }
    elseif (strlen($formfields[exp_id]) > $TBDB_EIDLEN) {
	$errors["Experiment Name"] =
	    "Too long! Must be less than or equal to $TBDB_EIDLEN";
    }
}

#
# Description
#
if (!isset($formfields[exp_description]) ||
    strcmp($formfields[exp_description], "") == 0) {
    $errors["Description"] = "Missing Field";
}

#
# Swappable
#
if (isset($formfields[exp_swappable]) &&
    strcmp($formfields[exp_swappable], "") &&
    strcmp($formfields[exp_swappable], "Yep")) {
    $errors["Swappable"] = "Bad Value";
}

#
# Priority
#
if (isset($formfields[exp_priority]) &&
    strcmp($formfields[exp_priority], "") &&
    ! isset($priorities[$formfields[exp_priority]])) {
    $errors["Priority"] = "Bad Value";
}

#
# If any errors, respit the form with the current values and the
# error messages displayed. Iterate until happy.
# 
if (count($errors)) {
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    return;
}

$exp_name    = addslashes($formfields[exp_description]);
$exp_pid     = $formfields[exp_pid];
$exp_id      = $formfields[exp_id];
if (isset($formfields[exp_gid])) {
    $exp_gid = $formfields[exp_gid];
}
if (isset($formfields[exp_localnsfile])) {
    $exp_localnsfile = $formfields[exp_localnsfile];
}

#
# Continue with error checks.
#
if (TBValidExperiment($exp_pid, $exp_id)) {
    RESPIT("Experiment Name", "Already in use");
}

#
# Check group. If none specified, then use default group.
#
if (!isset($exp_gid) ||
    strcmp($exp_gid, "") == 0) {
    $exp_gid = $exp_pid;
}
if (!TBValidGroup($exp_pid, $exp_gid)) {
    RESPIT("Group", "No such Group");
}

#
# Verify permissions.
#
if (! TBProjAccessCheck($uid, $exp_pid, $exp_gid, $TB_PROJECT_CREATEEXPT)) {
    RESPIT("Project/Group", "Not enough permission to create experiment");
}

#
# Not allowed to specify both a local and an upload!
#
$speclocal  = 0;
$specupload = 0;
$delnsfile  = 0;

if (isset($exp_localnsfile) && strcmp($exp_localnsfile, "")) {
    $speclocal = 1;
}
if (isset($exp_nsfile) && strcmp($exp_nsfile, "") &&
    strcmp($exp_nsfile, "none")) {
    $specupload = 1;
}

if ($speclocal && $specupload) {
    RESPIT("NS File", "Specified both a local and remote file");
}
if (!$specupload && strcmp($exp_nsfile_name, "")) {
    #
    # Catch an invalid filename.
    #
    RESPIT("NS File", "Local filename does not appear to be valid");
}

if ($speclocal) {
    #
    # No way to tell from here if this file actually exists, since
    # the web server runs as user nobody. The startexp script checks
    # for the file before going to ground, so the user will get immediate
    # feedback if the filename is bogus.
    #
    # Do not allow anything outside of /users or /proj. I do not think there
    # is a security worry, but good to enforce it anyway.
    #
    if (! ereg("^$TBPROJ_DIR/.*" ,$exp_localnsfile) &&
        ! ereg("^$TBUSER_DIR/.*" ,$exp_localnsfile)) {
	RESPIT("NS File",
	       "Server resident file must be in either ".
	       "$TBUSER_DIR/ or $TBPROJ_DIR/");
    }
    
    $nsfile = $exp_localnsfile;
    $nonsfile = 0;
}
elseif ($specupload) {
    #
    # XXX
    # Set the permissions on the NS file so that the scripts can get to it.
    # It is owned by nobody, and most likely protected. This leaves the
    # script open for a short time. A potential security hazard we should
    # deal with at some point.
    #
    chmod($exp_nsfile, 0666);
    $nsfile = $exp_nsfile;
    $nonsfile = 0;
}
elseif (isset($nsdata) && strcmp($nsdata, "")) {
    #
    # The NS file is encoded in the URL. Must create a temp file
    # to hold it, and pass through to the backend.
    #
    # XXX Hard to believe, but there is no equiv of mkstemp in php3.
    #
    $nsfile    = tempnam("/tmp", "$exp_pid-$exp_id.nsfile.");
    $delnsfile = 1;
    $nonsfile  = 0;

    if (! ($fp = fopen($nsfile, "w"))) {
	TBERROR("Could not create temporary file $nsfile", 1);
    }
    fwrite($fp, urldecode($nsdata));
    fclose($fp);
}
else {
    #
    # I am going to allow shell experiments to be created (No NS file),
    # but only by admin types.
    #
    if (! ISADMIN($uid)) {
	RESPIT("NS File", "You must provide an NS file");
    }
    $nonsfile = 1;
}

#
# Convert Priority, Swappable, Batched params to arguments to script.
#
if (isset($formfields[exp_swappable]) &&
    strcmp($formfields[exp_swappable], "Yep") == 0) {
    $exp_swappable = "-s";
}
else {
    $exp_swappable = "";
}

if (! isset($formfields[exp_priority]) ||
    strcmp($formfields[exp_priority], "") == 0) {
    $exp_priority  = "-n low";
}
else {
    $exp_priority  = "-n " . $priorities[$formfields[exp_priority]];
}

if (isset($formfields[exp_batched]) &&
    strcmp($formfields[exp_batched], "Yep") == 0) {
    $exp_batched   = 1;
    $batcharg      = "";
}
else {
    $exp_batched   = 0;
    $batcharg      = "-i";
}

#
# I'm not spitting out a redirection yet.
#
PAGEHEADER("Begin a Testbed Experiment");

#
# We need the email address for messages below.
#
TBUserInfo($uid, $user_name, $user_email);

#
# Set the experiment state bit to "new".
#
$exp_state   = "new";

# Shared experiments. (Deprecated for now!)
$exp_shared  = 0;

# Expiration is hardwired for now.
$exp_expires = date("Y:m:d", time() + (86400 * 120));

#
# Grab the unix GID for running scripts.
#
TBGroupUnixInfo($exp_pid, $exp_gid, $unix_gid, $unix_name);

#
# If an experiment "shell" give some warm fuzzies and be done with it.
# The user is responsible for running the tb scripts on his/her own!
# The user will need to come back and terminate the experiment though
# to clear it out of the database.
#
if ($nonsfile) {
    #
    # Stub Experiment record. Should do this elsewhere?
    #
    DBQueryFatal("INSERT INTO experiments ".
		 "(eid, pid, gid, expt_created, expt_expires, expt_name, ".
		 "expt_head_uid, state, shared) ".
		 "VALUES ('$exp_id', '$exp_pid', '$exp_gid', now(), ".
		 "        '$exp_expires', ".
		 "        '$exp_name', '$uid', '$exp_state', $exp_shared)");

    #
    # This is where the experiment hierarchy is going to be created.
    #
    $dirname = "$TBPROJ_DIR/$exp_pid/exp/$exp_id";
    
    SUEXEC($uid, $unix_gid, "mkexpdir $exp_pid $exp_gid $exp_id", 0);

    echo "<center><br>
          <h2>Experiment Configured!</h2>
          </center><br><br>
          <h3>
          The ID for your experiment in project `$exp_pid' is `$exp_id.'
          <br><br>
          Since you did not provide an NS script, no nodes have been
          allocated. You must log in and run the tbsetup scripts
          yourself. For your convenience, we have created a directory
          hierarchy on the control node: $dirname
          </h3>\n";

    if (1) {
	TBMAIL("$user_name '$uid' <$user_email>", 
	     "New Experiment Created: $exp_pid/$exp_id",
	     "User:        $uid\n".
             "EID:         $exp_id\n".
             "PID:         $exp_pid\n".
             "GID:         $exp_gid\n".
             "Name:        $exp_name\n".
             "Expires:     $exp_expires\n",
             "From: $TBMAIL_WWW\n".
	     "Bcc: $TBMAIL_LOGS\n".
             "Errors-To: $TBMAIL_WWW");
    }
    #
    # Standard Testbed Footer
    # 
    PAGEFOOTER();
    die("");
}

echo "<center><br>";
echo "<h2>Starting experiment configuration. Please wait a moment ...
      </h2></center>";

flush();

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

#
# Avoid SIGPROF in child.
# 
set_time_limit(0);

$result = exec("$TBSUEXEC_PATH $uid $unix_gid ".
	       "webbatchexp $batcharg -x \"$exp_expires\" -E \"$exp_name\" ".
	       "$exp_priority $exp_swappable ".
	       "-p $exp_pid -g $exp_gid -e $exp_id $nsfile",
 	       $output, $retval);

if ($delnsfile) {
    unlink($nsfile);
}

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
    
    PAGEFOOTER();
    die("");
}

echo "<br><br>";
echo "<font size=+1>
        Experiment
        <a href='showexp.php3?pid=$exp_pid&eid=$exp_id'>$exp_id</a>
        in project <A href='showproject.php3?pid=$exp_pid'>$exp_pid</A>
        is configuring!<br><br>\n";

if ($exp_batched) {
    echo "Batch Mode experiments will be run when enough resources become
          available. This might happen immediately, or it may take hours
	  or days. You will be notified via email when the experiment has
          been run. If you do not receive email notification within a
          reasonable amount of time, please contact $TBMAILADDR.\n";
}
else {
    echo "You will be notified via email when the experiment has been fully
	  configured and you are able to proceed. This typically takes less
          than 10 minutes, depending on the number of nodes you have requested.
          If you do not receive email notification within a reasonable amount
          of time, please contact $TBMAILADDR.\n";
}
echo "<br>
      </font>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
