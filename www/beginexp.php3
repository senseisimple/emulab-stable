<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# No PAGEHEADER since we spit out a Location header later. See below.
#

#
# Only known and logged in users can begin experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

$idleswaptimeout = TBGetSiteVar("idle/threshold");

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
    global $TBDB_PIDLEN, $TBDB_GIDLEN, $TBDB_EIDLEN, $TBDOCBASE;
    global $nsdata, $nsref, $projlist, $exp_nsfile;
    global $uid, $guid;
    global $idleswaptimeout;

    PAGEHEADER("Begin a Testbed Experiment");

    if ($errors) {
	echo "<table class=nogrid
                     align=center border=0 cellpadding=6 cellspacing=0>
              <tr>
                 <th align=center colspan=2>
                   <font size=+1 color=red>
                      &nbsp;Oops, please fix the following errors!&nbsp;
                   </font>
                 </td>
              </tr>\n";

	while (list ($name, $message) = each ($errors)) {
	    echo "<tr>
                     <td align=right>
                       <font color=red>$name:&nbsp;</font></td>
                     <td align=left>
                       <font color=red>$message</font></td>
                  </tr>\n";
	}
	echo "</table><br>\n";
    }
    else {
	# Temporary - July 17 was new form install - remove in
	# mid august or september
	echo "<h3>For more info about the new form, see the
		<a href='news.php3'>July 17 News item</a>.</h3>\n";
	    
	if (!isset($nsdata) && !isset($nsref)) {
	echo "<p><ul>
	    <li><b>If you have an NS file:</b><br> You may want to
                <b><a href='nscheck_form.php3'>syntax check it first</a></b>
	    <li><b>If you do not have an NS file:</b><br> You may want to
		<b><a href='buildui/bui.php3'>go to the NetBuild GUI</a></b>
		to graphically create one<font size=-2>
		(<a href='$TBDOCBASE/faq.php3#UTT-NETBUILD'>Additional 
		information</a>)</font>
	     </ul></p><br>";
	} else {
	echo "<p><b>Your NetBuild-generated NS file has been uploaded.</b> " .
             "To finish creating your experiment, " .
             "please fill out the following information:</p>";
        }
    }

    echo "<table align=center border=1>
          <tr>
              <td align=center colspan=2>
                 <em>(Fields marked with * are required)</em>
              </td>
          </tr>\n";

    echo "<form enctype=multipart/form-data
                action=beginexp.php3 method=post>\n";

    #
    # Select Project
    #
    echo "<tr>
              <td class='pad4'>*Select Project:</td>
              <td class='pad4'><select name=\"formfields[exp_pid]\">\n";

    # If just one project, make sure just the one option.
    if (count($projlist) != 1) {
	echo "<option value=''>Please Select &nbsp</option>\n";
    }

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
    # Select a group
    #
    echo "<tr>
              <td class='pad4'>Group:</td>
              <td class='pad4'><select name=\"formfields[exp_gid]\">
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
	<font size=-1>(Must be default or correspond to selected project)</font>
             </td>
          </tr>\n";

    #
    # Name:
    #
    echo "<tr>
              <td class='pad4'>*Name:
              <br><font size='-1'>(No blanks)</font></td>
              <td class='pad4' class=left>
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
              <td class='pad4'>*Description:<br>
                  <font size='-1'>(A concise sentence)</font></td>
              <td class='pad4' class=left>
                  <input type=text
                         name=\"formfields[exp_description]\"
                         value=\"" . $formfields[exp_description] . "\"
	                 size=60>
              </td>
          </tr>\n";

    #
    # NS file
    #
    if (isset($nsdata)) {
        echo "<tr>
                  <td class='pad4'>*Your Auto Generated NS file: &nbsp</td>
                      <input type=hidden name=nsdata value=$nsdata>
                  <td class='pad4'><a target=_blank href=spitnsdata.php3?nsdata=$nsdata>
                      View NS File</a></td>
              </tr>\n";
    } elseif (isset($nsref)) {
	if (isset($guid)) {
        echo "<tr>
                  <td class='pad4'>*Your Auto Generated NS file: &nbsp</td>
                      <input type=hidden name=nsref value=$nsref>
                      <input type=hidden name=guid value=$guid>
                  <td class='pad4'><a target=_blank href=\"spitnsdata.php3?nsref=$nsref&guid=$guid\">
                      View NS File</a></td>
              </tr>\n";
        } else {
        echo "<tr>
                  <td class='pad4'>*Your Auto Generated NS file: &nbsp</td>
                      <input type=hidden name=nsref value=$nsref>
                  <td class='pad4'><a target=_blank href=spitnsdata.php3?nsref=$nsref>
                      View NS File</a></td>
              </tr>\n";
        }
    }
    else {

	echo "<tr>
                  <td class='pad4'>*Your NS file: &nbsp</td>

                  <td><table cellspacing=0 cellpadding=0 border=0>
                    <tr>
                      <td class='pad4'>Upload<br>
			<font size='-1'>(50k&nbsp;max)</font></td>
                      <td class='pad4'>
                        <input type=hidden name=MAX_FILE_SIZE value=51200>
	                <input type=file
                               name=exp_nsfile
                               value=\"" . $exp_nsfile . "\"
	                       size=30>
                      </td>
                    </tr><tr>
                    <td>&nbsp;&nbsp;<b>or</b></td><td></td>
                    </tr><tr>
                      <td class='pad4'>On Server<br><font size='-1'>(<code>/proj</code>,
                        <code>/groups</code>, <code>/users</code>)</font></td>
                      <td class='pad4'>
	                <input type=text
                               name=\"formfields[exp_localnsfile]\"
                               value=\"" . $formfields[exp_localnsfile] . "\"
	                       size=40>
                      </td>
                    </tr></table></td></tr>\n";
    }

#
# Swapping
#

    # Add in hidden fields to send swappable and noswap_reason, since
    # they don't show on the form
    echo "<tr>
	      <td class='pad4'>*Swapping:<br>
              <font size='-1'>
	      (See <a href='$TBDOCBASE/docwrapper.php3?docname=swapping.html#swapping'>
              Swapping</a>)
              </font></td>
	      <td>
	      <input type=hidden name='formfields[exp_swappable]' value='$formfields[exp_swappable]'>
	      <input type=hidden name='formfields[exp_noswap_reason]' value='";
    echo htmlspecialchars($formfields[exp_noswap_reason], ENT_QUOTES);
    echo "'>
	      <table cellpadding=0 cellspacing=0 border=0><tr>
              <td><input type='checkbox'
	             name='formfields[exp_idleswap]'
	             value='1'";
    if ($formfields[exp_idleswap] == "1") {
	echo " checked='1'";
    }
    echo "></td>
              <td><a href='$TBDOCBASE/docwrapper.php3?docname=swapping.html#idleswap'>
	      <b>Idle-Swap:</b></a> Swap out this experiment
	      after 
              <input type='text' name='formfields[exp_idleswap_timeout]'
		     value='";
    echo htmlspecialchars($formfields[exp_idleswap_timeout], ENT_QUOTES);
    echo "' size='3'> hours idle.</td>
	      </tr><tr>
              <td> </td>
	      <td>If not, why not?<br><textarea rows=2 cols=50
                          name='formfields[exp_noidleswap_reason]'>";
			  
    echo htmlspecialchars($formfields[exp_noidleswap_reason], ENT_QUOTES);
    echo "</textarea></td>
	      </tr><tr>
	      <td><input type='checkbox'
		     name='formfields[exp_autoswap]'
		     value='1' ";
    if ($formfields[exp_autoswap] == "1") {
	echo " checked='1'";
    }
    echo "></td>
	      <td><a href='$TBDOCBASE/docwrapper.php3?docname=swapping.html#autoswap'>
	      <b>Max. Duration:</b></a> Swap out after
              <input type='text' name='formfields[exp_autoswap_timeout]'
		     value='";
    echo htmlspecialchars($formfields[exp_autoswap_timeout], ENT_QUOTES);
    echo "' size='3'> hours, even if not idle.</td>
              </tr></table>
              </td>
           </tr>";

    #
    # Batch Experiment?
    #
    echo "<tr>
  	      <td class='pad4' colspan=2>
              <input type=checkbox name='formfields[exp_batched]' value='Yep'";

    if (isset($formfields[exp_batched]) &&
	strcmp($formfields[exp_batched], "Yep") == 0) {
	    echo " checked='1'";
	}

    echo ">\n";
    echo "Batch Mode Experiment &nbsp;
          <font size='-1'>(See
          <a href='$TBDOCBASE/tutorial/tutorial.php3#BatchMode'>Tutorial</a>
          for more information)</font>
          </td>
          </tr>\n";

    #
    # Preload?
    #
    echo "<tr>
  	      <td class='pad4' colspan=2>
              <input type=checkbox name='formfields[exp_preload]' value='Yep'";

    if (isset($formfields[exp_preload]) &&
	strcmp($formfields[exp_preload], "Yep") == 0) {
	    echo " checked='1'";
	}

    echo ">\n";
    echo "Preload Experiment &nbsp;
          <font size='-1'>(Create experiment but don't swap it in now)
          </font></td>
          </tr>\n";

    echo "<tr>
              <td class='pad4' align=center colspan=2>
                 <b><input type=submit value=Submit name=submit></b></td>
         </tr>
        </form>
        </table>\n";

    echo "<p>
          <h3>Handy Links:</h3>
          <ul>
             <li> View a <a href='showosid_list.php3'>list of OSIDs</a>
                  that are available for you to use in your NS file.</li>
             <li> Create your own <a href='newimageid_ez.php3'>
                  custom disk images</a>.</li>
          </ul>\n";
}

if (isset($nsref)) {
    if (strcmp($nsref, "") == 0 || !ereg("^[0-9]+$", $nsref))
	unset($nsref);
}

if (isset($guid)) {
    if (strcmp($guid, "") == 0 || !ereg("^[0-9]+$", $guid))
	unset($guid);
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

    #
    # For users that are in one project and one subgroup, it is usually
    # the case that they should use the subgroup, and since they also tend
    # to be in the clueless portion of our users, give them some help.
    #
    if (count($projlist) == 1) {
	list($project, $grouplist) = each($projlist);

	if (count($grouplist) <= 2) {
	    $defaults[exp_pid] = $project;
	    if (count($grouplist) == 1 || strcmp($project, $grouplist[0]))
		$defaults[exp_gid] = $grouplist[0];
	    else
		$defaults[exp_gid] = $grouplist[1];
	}
	reset($projlist);
    }

    $defaults[exp_swappable] = "1";
    $defaults[exp_noswap_reason] = "";
    $defaults[exp_idleswap]  = "1";
    $defaults[exp_noidleswap_reason] = "";
    $defaults[exp_idleswap_timeout] = "$idleswaptimeout";
    $defaults[exp_autoswap]  = "0";
    $defaults[exp_autoswap_timeout] = "10";

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
# Any of these which aren't "1" become "0".
#
if (!isset($formfields[exp_swappable]) ||
    strcmp($formfields[exp_swappable], "1")) {
    $formfields[exp_swappable] = 0;

    if (!isset($formfields[exp_noswap_reason]) ||
        !strcmp($formfields[exp_noswap_reason], "")) {

        if (! ISADMIN($uid)) {
          $errors["Not Swappable"] = "No justification provided";
        } else {
          $formfields[exp_noswap_reason] = "ADMIN";
        }
    }
}

if (!isset($formfields[exp_idleswap]) ||
    strcmp($formfields[exp_idleswap], "1")) {
    $formfields[exp_idleswap] = 0;

    if (!isset($formfields[exp_noidleswap_reason]) ||
	!strcmp($formfields[exp_noidleswap_reason], "")) {
	if (! ISADMIN($uid)) {
	    $errors["Not Idle-Swappable"] = "No justification provided";
	} else {
	    $formfields[exp_noidleswap_reason] = "ADMIN";
	}
    }
}

if (!isset($formfields[exp_idleswap_timeout]) ||
    ($formfields[exp_idleswap_timeout] + 0) <= 0 ||
    ($formfields[exp_idleswap_timeout] + 0) > $idleswaptimeout) {
    $errors["Idleswap"] = "No or invalid time provided - must be non-zero and <= $idleswaptimeout";
}

if (!isset($formfields[exp_noidleswap_reason])) {
    $formfields[exp_noidleswap_reason] = "";
}


if (!isset($formfields[exp_autoswap]) ||
    strcmp($formfields[exp_autoswap], "1")) {
    $formfields[exp_autoswap] = 0;
} else {
    if (!isset($formfields[exp_autoswap_timeout]) ||
	($formfields[exp_autoswap_timeout] + 0) == 0) {
	$errors["Max. Duration"] = "No or invalid time provided";
    }
}

#
# Preload and Batch are mutually exclusive.
#
if (isset($formfields[exp_batched]) &&
    !strcmp($formfields[exp_batched], "Yep") &&
    isset($formfields[exp_preload]) &&
    !strcmp($formfields[exp_preload], "Yep")) {
    $errors["Preload"] = "Cannot use with Batch Mode";
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
    RESPIT("Group", "Group '$exp_gid' is not in project '$exp_pid'!");
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
    # Do not allow anything outside of special dirs. I do not think there
    # is a security worry, but good to enforce it anyway.
    #
    if (! ereg("^$TBPROJ_DIR/.*" ,$exp_localnsfile) &&
        ! ereg("^$TBUSER_DIR/.*" ,$exp_localnsfile) &&
        ! ereg("^$TBGROUP_DIR/.*" ,$exp_localnsfile)) {
	RESPIT("NS File",
	       "Server resident file must be in either ".
	       "$TBUSER_DIR/, $TBPROJ_DIR/, or $TBGROUP_DIR/");
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
elseif (isset($nsref) && strcmp($nsref, "")) {
    if (isset($guid)) {
      $nsfile = "/tmp/$guid-$nsref.nsfile";
    } else {
      $nsfile = "/tmp/$uid-$nsref.nsfile";
    }
    $delnsfile = 1;
    $nonsfile  = 0;
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
    chmod($nsfile, 0666);
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
# Convert Swappable, Batched params to arguments to script.
#
$exp_swappable = "";

if ($formfields[exp_swappable] == "1") {
    $exp_swappable .= " -s";
}

if ($formfields[exp_autoswap] == "1") {
    $exp_swappable .= " -a " . (60 * $formfields[exp_autoswap_timeout]);
}

if ($formfields[exp_idleswap] == "1") {
    $exp_swappable .= " -l " . (60 * $idleswaptimeout);
}

#
# All experiments are low priority for now.
#
$exp_priority  = "-n low";

if (isset($formfields[exp_batched]) &&
    strcmp($formfields[exp_batched], "Yep") == 0) {
    $exp_batched   = 1;
    $batcharg      = "";
    $exp_preload   = 0;
}
else {
    $exp_batched   = 0;
    $batcharg      = "-i";
}
if (isset($formfields[exp_preload]) &&
    strcmp($formfields[exp_preload], "Yep") == 0) {
    $exp_preload   = 1;
    $batcharg     .= " -f";
}

#
# I am not spitting out a redirection yet.
#
PAGEHEADER("Begin a Testbed Experiment");

#
# We need the email address for messages below.
#
TBUserInfo($uid, $user_name, $user_email);

# Expiration is hardwired for now.
$exp_expires = date("Y:m:d", time() + (86400 * 120));

#
# Grab the unix GID for running scripts.
#
TBGroupUnixInfo($exp_pid, $exp_gid, $unix_gid, $unix_name);

#
# Okay, time to do it.
#
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
	       "-p $exp_pid -g $exp_gid -e $exp_id ".
	       ($nonsfile ? "" : "$nsfile"),
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

# add reasons to db, since we don't want to pass these on the command line.

if ($formfields[exp_noswap_reason]) {
    DBQueryFatal("UPDATE experiments " .
		 "SET noswap_reason='" .
                 addslashes($formfields[exp_noswap_reason]).
                 "' ".
		 "WHERE eid='$exp_id' and pid='$exp_pid'");
}

if ($formfields[exp_noidleswap_reason]) {
    DBQueryFatal("UPDATE experiments " .
		 "SET noidleswap_reason='" .
                 addslashes($formfields[exp_noidleswap_reason]).
                 "' ".
		 "WHERE eid='$exp_id' and pid='$exp_pid'");
}

echo "<br><br>";
echo "<font size=+1>
        Experiment
        <a href='showexp.php3?pid=$exp_pid&eid=$exp_id'>$exp_id</a>
        in project <A href='showproject.php3?pid=$exp_pid'>$exp_pid</A>
        is configuring!<br><br>\n";

if ($nonsfile) {
    echo "Since you did not provide an NS script, no nodes have been
          allocated. You will not be able to modify or swap this experiment,
          nor do most other neat things you can do with a real experiment.
          <br><br>
          If this bothers you, <b>SUPPLY AN NS FILE!</b>\n";
}
elseif ($exp_preload) {
    echo "Since you are only pre-loading the experiment, this will typically
          take less than one minute. If you do not receive email notification
          within a reasonable amount of time, please contact $TBMAILADDR.<br>
          <br>
          While you are waiting, you can watch the log
          in <a target=_blank href=spewlogfile.php3?pid=$exp_pid&eid=$exp_id>
          realtime</a>.\n";
}
elseif ($exp_batched) {
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
          of time, please contact $TBMAILADDR.<br>
          <br>
          While you are waiting, you can watch the log of experiment creation
          in <a target=_blank href=spewlogfile.php3?pid=$exp_pid&eid=$exp_id>
          realtime</a>.\n";
}
echo "<br>
      </font>\n";

#
# Standard Testbed Footer
#
PAGEFOOTER();
?>
